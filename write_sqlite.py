
src = r"""// DatabaseSqlite.cpp - SQLite loader for Nostalgia Manager Simulation.
// Reads ClubsDB, "PlayersDB1997 with player ID", and Tactics tables from a
// .DB file and feeds the data through the existing row-based loaders.

#include "Database.h"
#include <string>
#include <vector>
#include "../../../third_party/sqlite/sqlite3.h"

namespace nm {
namespace {

// Normalise a column name: lowercase, keep only [a-z0-9].
// Matches normKey() in Database.cpp so the Header struct recognises the keys.
static std::string normCol(const std::string& raw) {
    std::string out;
    for (unsigned char c : raw)
        if (std::isalnum(c)) out += static_cast<char>(std::tolower(c));
    return out;
}

struct ColRemap { const char* from; const char* to; };

// ---------------------------------------------------------------------------
// ClubsDB: map real DB column names -> loader-expected header keys.
// Jersey 1-20 normalise to "jersey1"-"jersey20" already and need no entry.
// Jerseys 21-45 are stored as Field44-Field68 in the DB.
// ---------------------------------------------------------------------------
static const ColRemap kClubsRemap[] = {
    {"teamid",             "clubid"},
    {"teamname",           "club"},
    {"country",            "nation"},
    {"primaryformation",   "formationa"},
    {"secondaryformation", "formationb"},
    {"shirtcolourhome",    "homeshirtcolour"},
    {"numbercolourhome",   "homeshortscolour"},
    {"shirtcolouraway",    "awayshirtcolour"},
    {"numbercolouraway",   "awayshortscolour"},
    {"field44","jersey21"},{"field45","jersey22"},{"field46","jersey23"},
    {"field47","jersey24"},{"field48","jersey25"},{"field49","jersey26"},
    {"field50","jersey27"},{"field51","jersey28"},{"field52","jersey29"},
    {"field53","jersey30"},{"field54","jersey31"},{"field55","jersey32"},
    {"field56","jersey33"},{"field57","jersey34"},{"field58","jersey35"},
    {"field59","jersey36"},{"field60","jersey37"},{"field61","jersey38"},
    {"field62","jersey39"},{"field63","jersey40"},{"field64","jersey41"},
    {"field65","jersey42"},{"field66","jersey43"},{"field67","jersey44"},
    {"field68","jersey45"},
    {nullptr, nullptr}
};

// ---------------------------------------------------------------------------
// PlayersDB: the DB now has proper column names (not field15..field62).
// Only map entries where the normalised DB name differs from what the loader
// expects. Everything else (nationality, age, club, caps, ability, passing,
// heading, shooting, stamina, strength, pace, flair, dribbling, tackling,
// jumping, marking, creativity, determination, positioning, character) already
// matches after normCol and needs no entry.
// ---------------------------------------------------------------------------
static const ColRemap kPlayersRemap[] = {
    // Identity / bio
    {"playerid",       "id"},
    {"playername",     "fullname"},
    {"goalsfornation", "internationalgoals"},
    {"birhsday",       "dateofbirth"},  // typo in DB column name
    {"bestposition",   "position"},
    // Position ratings: DB uses short codes; loader expects full English names
    {"gk",  "goalkeeper"},
    {"dc",  "defendercentral"},
    {"dr",  "defenderright"},
    {"dl",  "defenderleft"},
    {"wbr", "wingbackright"},
    {"wbl", "wingbackleft"},
    {"dmc", "defensivemidfielder"},
    {"mc",  "midfieldercentral"},
    {"mr",  "midfielderright"},
    {"ml",  "midfielderleft"},
    {"amc", "attackingmidfieldercentral"},
    {"amr", "attackingmidfielderright"},
    {"aml", "attackingmidfielderleft"},
    {"fc",  "centerforward"},
    {"fr",  "rightforward"},
    {"fl",  "leftforward"},
    // Skills where DB name differs from attrMap keys
    {"technical",  "technique"},    // DB: "Technical"  loader: "Technique"
    {"injprone",   "injuryproneness"},
    {"agression",  "aggression"},   // typo in DB
    {"indfluence", "influence"},    // typo in DB
    {nullptr, nullptr}
};

static const char* applyRemap(const std::string& norm, const ColRemap* remap) {
    if (!remap) return nullptr;
    for (const ColRemap* r = remap; r->from; ++r)
        if (norm == r->from) return r->to;
    return nullptr;
}

// Run SELECT * on a table and return rows[0]=header, rows[1..n]=data rows.
// Pass remap=nullptr to keep raw normalised column names (used for Tactics).
static bool queryTable(sqlite3* db, const char* table, const ColRemap* remap,
                       std::vector<std::vector<std::string>>& out) {
    std::string sql = "SELECT * FROM \"";
    sql += table;
    sql += "\";";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    bool headerBuilt = false;
    int colCount = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!headerBuilt) {
            colCount = sqlite3_column_count(stmt);
            std::vector<std::string> header;
            header.reserve(colCount);
            for (int c = 0; c < colCount; ++c) {
                const char* raw = sqlite3_column_name(stmt, c);
                std::string norm = normCol(raw ? raw : "");
                const char* mapped = applyRemap(norm, remap);
                header.push_back(mapped ? mapped : norm);
            }
            out.push_back(std::move(header));
            headerBuilt = true;
        }
        std::vector<std::string> row;
        row.reserve(colCount);
        for (int c = 0; c < colCount; ++c) {
            const unsigned char* val = sqlite3_column_text(stmt, c);
            row.push_back(val ? reinterpret_cast<const char*>(val) : "");
        }
        out.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return out.size() >= 2;
}

} // namespace

// ---------------------------------------------------------------------------
// Database::loadFromSqlite
// ---------------------------------------------------------------------------
bool Database::loadFromSqlite(const std::string& dbPath) {
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(dbPath.c_str(), &db,
                        SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX,
                        nullptr) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    // 1. Clubs / teams
    {
        std::vector<std::vector<std::string>> rows;
        if (queryTable(db, "ClubsDB", kClubsRemap, rows))
            loadTeamsFromRows(rows);
    }

    // 2. Players
    bool ok = false;
    {
        std::vector<std::vector<std::string>> rows;
        if (!queryTable(db, "PlayersDB1997 with player ID", kPlayersRemap, rows))
            queryTable(db, "PlayersDB", kPlayersRemap, rows);
        if (rows.size() >= 2)
            ok = loadPlayersFromRows(rows);
    }

    // 3. Tactics - Formation/GK/DC/MC/etc. already match loader expectations
    {
        std::vector<std::vector<std::string>> rows;
        if (queryTable(db, "Tactics", nullptr, rows))
            loadTacticsFromRows(rows);
    }

    sqlite3_close(db);
    return ok;
}

} // namespace nm
"""

with open(r"D:\DEV\Nostalgia\NostalgiaManager\src\data\DatabaseSqlite.cpp", "w", encoding="utf-8") as f:
    f.write(src.lstrip())

print("written", len(src), "chars")
