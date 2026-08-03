// DatabaseSqlite.cpp - SQLite loader for Nostalgia Manager Simulation.
// Reads ClubsDB, "PlayersDB1997 with player ID", and Tactics tables from a
// .DB file and feeds the data through the existing row-based loaders.

#include "Database.h"
#include <string>
#include <vector>
#include "../../../third_party/sqlite/sqlite3.h"
#include "../core/Team.h"

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
    // Financial columns (spaces stripped by normCol)
    {"transferbudgetseason", "transferbudgetseason"},
    {"wagesbudgetseason",    "wagesbudgetseason"},
    {"financialtier",        "financialtier"},
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
    // Financial columns - DB has both "Wages" (current) and "Wage demand" (demand)
    // Map "Wage demand" -> "wagedemand" (already done via normCol stripping space)
    // Map "Transfer price" -> "transferprice" (already done via normCol)
    // "Wages" normCol -> "wages"; remap to "wage" so the loader's fallback finds it
    {"wages",         "wage"},
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

// ---------------------------------------------------------------------------
// Database::recalcAndPersistFinancials
// Recalculates wageDemand and transferValue for every player in memory using
// the current Ability and Age, then writes the new values back to the SQLite
// database so they are persisted for the next session.
// ---------------------------------------------------------------------------
void Database::recalcAndPersistFinancials() {
    // Recalculate in-memory values for all players.
    for (auto& team : teams) {
        for (auto& p : team.squad) {
            double ability100 = PlayerAbility(p) * 10.0;
            int age = p.age > 0 ? p.age : 25;
            p.wageDemand   = CalcWageDemand(ability100, age);
            p.transferValue = CalcTransferValue(p.wageDemand, age);
        }
    }

    // Persist to SQLite if a DB file was loaded.
    if (sqlitePath_.empty()) return;

    sqlite3* db = nullptr;
    if (sqlite3_open_v2(sqlitePath_.c_str(), &db,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX,
                        nullptr) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return;
    }

    // Try two possible table names.
    const char* tableNames[] = {
        "PlayersDB1997 with player ID",
        "PlayersDB",
        nullptr
    };

    // Check which table exists and has the columns we need.
    const char* tableName = nullptr;
    for (int i = 0; tableNames[i]; ++i) {
        std::string sql = "SELECT \"Wage demand\" FROM \"";
        sql += tableNames[i];
        sql += "\" LIMIT 1;";
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_finalize(s);
            tableName = tableNames[i];
            break;
        }
        if (s) sqlite3_finalize(s);
    }

    if (!tableName) {
        // Columns don't exist in DB yet — nothing to write.
        sqlite3_close(db);
        return;
    }

    // Build UPDATE statement using the exact column names from Data.db.
    std::string updateSql = "UPDATE \"";
    updateSql += tableName;
    updateSql += "\" SET \"Wage demand\" = ?, \"Transfer price\" = ? WHERE \"Player ID\" = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, updateSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    for (const auto& team : teams) {
        for (const auto& p : team.squad) {
            if (p.id <= 0) continue;
            sqlite3_reset(stmt);
            sqlite3_bind_int(stmt, 1, p.wageDemand);
            sqlite3_bind_int(stmt, 2, p.transferValue);
            sqlite3_bind_int(stmt, 3, p.id);
            sqlite3_step(stmt);
        }
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

} // namespace nm
