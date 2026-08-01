#include "Database.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <unordered_map>

#include "Csv.h"

namespace nm {

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Normalise a header cell to a comparison key: lowercase, keep only [a-z0-9].
// "Off The Ball" -> "offtheball", "Strenght" -> "strenght".
std::string normKey(const std::string& s) {
    std::string out;
    for (unsigned char c : s)
        if (std::isalnum(c)) out += static_cast<char>(std::tolower(c));
    return out;
}

// Fold accented Latin characters to plain ASCII so names from Championship
// Manager / FM exports render in the bitmap GUI font (which only has ASCII).
// Handles both raw Latin-1 bytes and UTF-8 two-byte Latin-1 supplement.
std::string asciiFold(const std::string& s) {
    auto mapLatin1 = [](unsigned cp) -> const char* {
        switch (cp) {
            case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: return "A";
            case 0xC6: return "AE";
            case 0xC7: return "C";
            case 0xC8: case 0xC9: case 0xCA: case 0xCB: return "E";
            case 0xCC: case 0xCD: case 0xCE: case 0xCF: return "I";
            case 0xD0: return "D"; case 0xD1: return "N";
            case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD8: return "O";
            case 0xD9: case 0xDA: case 0xDB: case 0xDC: return "U";
            case 0xDD: return "Y"; case 0xDE: return "Th"; case 0xDF: return "ss";
            case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: return "a";
            case 0xE6: return "ae";
            case 0xE7: return "c";
            case 0xE8: case 0xE9: case 0xEA: case 0xEB: return "e";
            case 0xEC: case 0xED: case 0xEE: case 0xEF: return "i";
            case 0xF0: return "d"; case 0xF1: return "n";
            case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF8: return "o";
            case 0xF9: case 0xFA: case 0xFB: case 0xFC: return "u";
            case 0xFD: case 0xFF: return "y"; case 0xFE: return "th";
            default: return nullptr;
        }
    };
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char b = static_cast<unsigned char>(s[i]);
        if (b < 0x80) {
            out += static_cast<char>(b);
        } else if (b == 0xC3 && i + 1 < s.size()) {
            unsigned char n = static_cast<unsigned char>(s[i + 1]);
            unsigned cp = 0xC0 + (n & 0x3F);
            const char* r = mapLatin1(cp);
            if (r) out += r;
            ++i;
        } else if (b == 0xC2 && i + 1 < s.size()) {
            ++i;  // drop C2-prefixed punctuation (©, etc.)
        } else {
            const char* r = mapLatin1(b);
            if (r) out += r;  // raw Latin-1 byte
        }
    }
    return out;
}

Mentality mentalityFromString(const std::string& s) {
    std::string v = lower(s);
    if (v.rfind("def", 0) == 0) return Mentality::Defensive;
    if (v.rfind("att", 0) == 0) return Mentality::Attack;
    return Mentality::Standard;
}

int toInt(const std::string& s) {
    try {
        return s.empty() ? 0 : std::stoi(s);
    } catch (...) {
        return 0;
    }
}

int clampStat(int v) { return std::max(1, std::min(20, v)); }

constexpr size_t kNoTeam = static_cast<size_t>(-1);

// Some databases use placeholder "clubs" for unattached players. These should
// not become real teams.
bool isNonClub(const std::string& name) {
    std::string v = lower(name);
    static const char* markers[] = {"free transfer", "free agent", "retired",
                                    "non league", "non-league", "minor team",
                                    "unknown", "none", "n/a"};
    for (const char* m : markers)
        if (v == m) return true;
    return false;
}

// Maps normalised header names -> the first column index that carries them.
// First occurrence wins, which keeps the detailed CM position block (the first
// "Goalkeeper" column) rather than the later summary block.
struct Header {
    std::unordered_map<std::string, int> idx;
    void build(const std::vector<std::string>& row) {
        for (size_t i = 0; i < row.size(); ++i) {
            std::string k = normKey(row[i]);
            if (!k.empty() && idx.find(k) == idx.end()) idx[k] = static_cast<int>(i);
        }
    }
    bool has(const std::string& k) const { return idx.count(k) > 0; }
    // Returns the column index for the first key that exists, or -1.
    int col(std::initializer_list<const char*> keys) const {
        for (const char* k : keys) {
            auto it = idx.find(k);
            if (it != idx.end()) return it->second;
        }
        return -1;
    }
    std::string get(const std::vector<std::string>& r,
                    std::initializer_list<const char*> keys) const {
        int c = col(keys);
        if (c < 0 || c >= static_cast<int>(r.size())) return "";
        return r[c];
    }
    int getInt(const std::vector<std::string>& r,
               std::initializer_list<const char*> keys) const {
        return toInt(get(r, keys));
    }
};

// Engine attribute -> candidate header keys in a CM/FM style export.
const std::vector<std::pair<const char*, std::vector<const char*>>>& attrMap() {
    static const std::vector<std::pair<const char*, std::vector<const char*>>> m = {
        {"Passing", {"passing"}},
        {"Shooting", {"shooting", "finishing"}},
        {"Technique", {"technique"}},
        {"Dribbling", {"dribbling"}},
        {"Heading", {"heading"}},
        {"Pace", {"pace", "acceleration"}},
        {"Stamina", {"stamina"}},
        {"Strength", {"strength", "strenght"}},
        {"Jumping", {"jumping"}},
        {"Positioning", {"positioning"}},
        {"OffTheBall", {"offtheball"}},
        {"Marking", {"marking"}},
        {"Tackling", {"tackling"}},
        {"Creativity", {"creativity", "vision"}},
        {"Determination", {"determination"}},
        {"Influence", {"influence", "leadership"}},
        {"Aggression", {"aggression"}},
        {"Flair", {"flair"}},
    };
    return m;
}
}  // namespace

// ---------------------------------------------------------------------------
// Teams / clubs
// ---------------------------------------------------------------------------
bool Database::loadTeams(const std::string& path) {
    std::vector<std::vector<std::string>> rows;
    if (!Csv::read(path, rows) || rows.size() < 2) return false;

    Header h;
    h.build(rows[0]);

    bool hasId = h.has("id");
    for (size_t i = 1; i < rows.size(); ++i) {
        const auto& r = rows[i];
        std::string name = asciiFold(h.get(r, {"name", "club", "clubname", "teamname", "team"}));
        if (name.empty() || isNonClub(name)) continue;
        Team t;
        t.id = hasId ? h.getInt(r, {"id"}) : nextTeamId_++;
        if (t.id >= nextTeamId_) nextTeamId_ = t.id + 1;
        t.name = name;
        t.league = asciiFold(h.get(r, {"league", "division", "div", "competition", "nation", "country"}));
        if (t.league.empty()) t.league = "League";
        std::string f = h.get(r, {"formation", "formationa", "formationb", "shape"});
        if (!f.empty()) {
            t.formation = f;
            t.preferredFormation = f;  // Store as club's preferred formation
        }
        std::string m = h.get(r, {"mentality", "mentalitiy"});
        if (!m.empty()) t.mentality = mentalityFromString(m);

        // Load team colors
        t.homeColor1 = h.get(r, {"teamcolourmain1", "teamcolormain1", "homecolor1", "homecolour1"});
        t.homeColor2 = h.get(r, {"teamcolourmain2", "teamcolormain2", "homecolor2", "homecolour2"});
        t.awayColor1 = h.get(r, {"awaycolour1", "awaycolor1"});
        t.awayColor2 = h.get(r, {"awaycolour2", "awaycolor2"});

        teams.push_back(std::move(t));
    }
    return !teams.empty();
}

// ---------------------------------------------------------------------------
// Players
// ---------------------------------------------------------------------------
bool Database::loadPlayers(const std::string& path) {
    std::vector<std::vector<std::string>> rows;
    if (!Csv::read(path, rows) || rows.size() < 2) return false;

    Header h;
    h.build(rows[0]);

    // Legacy bundled format has an explicit teamId + role + number column.
    bool legacy = h.has("teamid") && h.has("role");

    // For CM/FM exports: resolve each engine skill to a column once and detect
    // the value scale. Some databases store skills 0-100, others 1-20; a few
    // rows contain corrupt outliers, so detection ignores values above 100
    // (otherwise a single bad cell would crush every player's ratings).
    constexpr int kSkillMax = 100;  // largest plausible raw skill value
    std::vector<std::pair<std::string, int>> skillCols;
    double skillScale = 1.0;
    if (!legacy) {
        for (const auto& kv : attrMap()) {
            int c = -1;
            for (const char* k : kv.second)
                if (h.has(k)) { c = h.idx.at(k); break; }
            skillCols.emplace_back(kv.first, c);
        }
        long total = 0, over20 = 0;
        for (size_t i = 1; i < rows.size(); ++i)
            for (const auto& sc : skillCols)
                if (sc.second >= 0 && sc.second < static_cast<int>(rows[i].size())) {
                    int v = toInt(rows[i][sc.second]);
                    if (v > 0 && v <= kSkillMax) { ++total; if (v > 20) ++over20; }
                }
        // If a meaningful share of values exceed 20, the file uses a 0-100 scale.
        if (total > 0 && over20 > total / 20) skillScale = 20.0 / kSkillMax;
    }

    std::unordered_map<std::string, size_t> nameToIdx;
    for (size_t i = 0; i < teams.size(); ++i) nameToIdx[lower(teams[i].name)] = i;

    // When a clubs file has already been loaded, only attach players to those
    // known clubs; otherwise synthesise teams from the players' Club column.
    const bool haveClubs = !teams.empty();
    auto ensureIdx = [&](const std::string& clubName) -> size_t {
        std::string key = lower(clubName);
        auto it = nameToIdx.find(key);
        if (it != nameToIdx.end()) return it->second;
        if (haveClubs) return kNoTeam;
        Team t;
        t.id = nextTeamId_++;
        t.name = clubName.empty() ? ("Club " + std::to_string(t.id)) : clubName;
        t.league = "League";
        size_t idx = teams.size();
        teams.push_back(std::move(t));
        nameToIdx[key] = idx;
        return idx;
    };

    int autoId = 1;
    for (size_t i = 1; i < rows.size(); ++i) {
        const auto& r = rows[i];
        Player p;

        if (legacy) {
            p.id = h.getInt(r, {"id"});
            p.name = asciiFold(h.get(r, {"name"}));
            p.role = RoleFromString(h.get(r, {"role"}));
            p.primaryPos = DefaultPosOf(p.role);
            p.playablePositions = {p.primaryPos};
            p.shirtNumber = h.getInt(r, {"number"});

            // Load bio information if available
            p.age = h.getInt(r, {"age"});
            p.dateOfBirth = h.get(r, {"dateofbirth", "dob", "birthday"});
            p.nationality = h.get(r, {"nationality", "nation", "nat"});
            p.internationalCaps = h.getInt(r, {"internationalcaps", "intcaps", "caps"});
            p.internationalGoals = h.getInt(r, {"internationalgoals", "intgoals", "intlgoals"});

            for (const auto& an : AttributeNames()) {
                std::string v = h.get(r, {normKey(an).c_str()});
                p.attr.set(an, v.empty() ? 10 : clampStat(toInt(v)));
            }
            int teamId = h.getInt(r, {"teamid"});
            Team* t = findTeam(teamId);
            if (!t) continue;
            t->squad.push_back(std::move(p));
            continue;
        }

        // --- Championship Manager / FM style export ---
        // Prefer First + Second name: some exports append the club to "Fullname"
        // (e.g. "Soren Andersen AaB"), so build the display name from the parts.
        std::string fn = h.get(r, {"firstname", "firstnam"});
        std::string sn = h.get(r, {"secondname", "surname", "lastname"});
        std::string name = Csv::trim(fn + " " + sn);
        if (name.empty()) name = h.get(r, {"fullname", "name", "playername"});
        if (name.empty()) continue;
        p.id = autoId++;
        p.name = asciiFold(name);

        // Load bio information if available
        p.age = h.getInt(r, {"age"});
        p.dateOfBirth = h.get(r, {"dateofbirth", "dob", "birthday"});
        p.nationality = h.get(r, {"nationality", "nation", "nat"});
        p.internationalCaps = h.getInt(r, {"internationalcaps", "intcaps", "caps"});
        p.internationalGoals = h.getInt(r, {"internationalgoals", "intgoals", "intlgoals"});

        // Overall ability (0-200) drives the GK rating and acts as a baseline
        // for skills the export left blank (these databases are often sparse).
        int ability = h.getInt(r, {"ability", "currentability", "ca"});
        int abil20 = static_cast<int>(std::lround(ability / 10.0));
        int baseline = ability > 0 ? clampStat(abil20) : 8;

        // Try to read the best position directly from a position column or from column 11
        std::string bestPosStr = h.get(r, {"position", "bestposition", "primaryposition", "pos"});
        // If no header match, try column 11 directly (0-indexed) for CSV files with position there
        if (bestPosStr.empty() && r.size() > 11) {
            bestPosStr = Csv::trim(r[11]);
        }
        Position explicitBestPos = Position::MC;  // Default fallback
        bool hasExplicitPos = false;
        if (!bestPosStr.empty() && bestPosStr.size() >= 2 && bestPosStr.size() <= 4) {
            // Only parse if it looks like a position string (2-4 chars, e.g., "MC", "AMC", "DR")
            explicitBestPos = PositionFromString(bestPosStr);
            hasExplicitPos = true;
        }

        // Skills, rescaled to the engine's 1-20 range. Values above the plausible
        // ceiling are corrupt outliers and are treated as missing (filled from
        // the ability baseline) rather than dragging the rating up or down.
        for (const auto& sc : skillCols) {
            int raw = (sc.second >= 0 && sc.second < static_cast<int>(r.size()))
                          ? toInt(r[sc.second])
                          : 0;
            int v = (raw > 0 && raw <= kSkillMax)
                        ? static_cast<int>(std::lround(raw * skillScale))
                        : baseline;
            p.attr.set(sc.first, clampStat(v));
        }

        // Some exports leave Jumping blank; derive it from Heading/Strength.
        if (toInt(h.get(r, {"jumping"})) <= 0)
            p.attr.set("Jumping",
                       clampStat((p.attr.get("Heading") + p.attr.get("Strength")) / 2));

        // Positions from the CM/FM ratings. Read individual position ratings
        // and only add positions that have actual ratings (not derived from role+side combos).
        auto pos = [&](std::initializer_list<const char*> keys) { return h.getInt(r, keys); };

        // Read all specific position ratings
        int rGK = pos({"goalkeeper", "gk"});
        int rDR = pos({"defenderright"});
        int rDC = pos({"defendercentral", "sweeper"});
        int rDL = pos({"defenderleft"});
        int rWBR = pos({"wingbackright"});
        int rWBL = pos({"wingbackleft"});
        int rDM = pos({"defensivemidfielder", "defensivemidfield", "anchor"});
        int rMR = pos({"midfielderright"});
        int rMC = pos({"midfieldercentral", "midfieldcenter", "midfield"});
        int rML = pos({"midfielderleft"});
        int rAMR = pos({"attackingmidfielderright"});
        int rAMC = pos({"attackingmidfieldercentral"});
        int rAML = pos({"attackingmidfielderleft"});
        int rFR = pos({"rightforward"});
        int rFC = pos({"centerforward", "centreforward", "attack"});
        int rFL = pos({"leftforward"});

        // Build playable positions only from positions with actual ratings
        // Also store the ratings for each position
        const int threshold = 1;  // Only add if rating > 0

        p.playablePositions.clear();
        p.positionRatings.clear();

        if (rGK > threshold) {
            p.playablePositions.push_back(Position::GK);
            p.positionRatings[Position::GK] = rGK;
        }
        if (rDR > threshold) {
            p.playablePositions.push_back(Position::DR);
            p.positionRatings[Position::DR] = rDR;
        }
        if (rDC > threshold) {
            p.playablePositions.push_back(Position::DC);
            p.positionRatings[Position::DC] = rDC;
        }
        if (rDL > threshold) {
            p.playablePositions.push_back(Position::DL);
            p.positionRatings[Position::DL] = rDL;
        }
        if (rWBR > threshold) {
            p.playablePositions.push_back(Position::WBR);
            p.positionRatings[Position::WBR] = rWBR;
        }
        if (rWBL > threshold) {
            p.playablePositions.push_back(Position::WBL);
            p.positionRatings[Position::WBL] = rWBL;
        }
        if (rDM > threshold) {
            p.playablePositions.push_back(Position::DM);
            p.positionRatings[Position::DM] = rDM;
        }
        if (rMR > threshold) {
            p.playablePositions.push_back(Position::MR);
            p.positionRatings[Position::MR] = rMR;
        }
        if (rMC > threshold) {
            p.playablePositions.push_back(Position::MC);
            p.positionRatings[Position::MC] = rMC;
        }
        if (rML > threshold) {
            p.playablePositions.push_back(Position::ML);
            p.positionRatings[Position::ML] = rML;
        }
        if (rAMR > threshold) {
            p.playablePositions.push_back(Position::AMR);
            p.positionRatings[Position::AMR] = rAMR;
        }
        if (rAMC > threshold) {
            p.playablePositions.push_back(Position::AMC);
            p.positionRatings[Position::AMC] = rAMC;
        }
        if (rAML > threshold) {
            p.playablePositions.push_back(Position::AML);
            p.positionRatings[Position::AML] = rAML;
        }
        if (rFR > threshold) {
            p.playablePositions.push_back(Position::FR);
            p.positionRatings[Position::FR] = rFR;
        }
        if (rFC > threshold) {
            p.playablePositions.push_back(Position::FC);
            p.positionRatings[Position::FC] = rFC;
        }
        if (rFL > threshold) {
            p.playablePositions.push_back(Position::FL);
            p.positionRatings[Position::FL] = rFL;
        }

        // Primary = best-rated position, OR use explicit position if available.
        if (hasExplicitPos) {
            // Use the explicit best position from the CSV
            p.primaryPos = explicitBestPos;
            p.role = RoleOf(p.primaryPos);
            // Ensure the explicit position is in playable positions
            if (!p.canPlay(p.primaryPos)) p.playablePositions.push_back(p.primaryPos);
        } else {
            // Calculate primary position from the highest rated position
            struct PosRating { Position pos; int rating; };
            std::vector<PosRating> posRatings = {
                {Position::GK, rGK}, {Position::DR, rDR}, {Position::DC, rDC}, {Position::DL, rDL},
                {Position::WBR, rWBR}, {Position::WBL, rWBL}, {Position::DM, rDM},
                {Position::MR, rMR}, {Position::MC, rMC}, {Position::ML, rML},
                {Position::AMR, rAMR}, {Position::AMC, rAMC}, {Position::AML, rAML},
                {Position::FR, rFR}, {Position::FC, rFC}, {Position::FL, rFL}
            };

            // Find the position with the highest rating
            auto best = std::max_element(posRatings.begin(), posRatings.end(),
                [](const PosRating& a, const PosRating& b) { return a.rating < b.rating; });

            if (best != posRatings.end() && best->rating > 0) {
                p.primaryPos = best->pos;
            } else {
                // Fallback to MC if no positions have ratings
                p.primaryPos = Position::MC;
            }

            p.role = RoleOf(p.primaryPos);
            if (!p.canPlay(p.primaryPos)) p.playablePositions.push_back(p.primaryPos);
        }

        // Goalkeeping: no dedicated GK skill in the export, so derive it from a
        // keeper's overall Ability (0-200 -> ~1-20); outfielders get a low value.
        if (p.role == Role::GK)
            p.attr.set("Goalkeeping", clampStat(ability > 0 ? abil20 : 12));
        else
            p.attr.set("Goalkeeping", clampStat(std::max(1, abil20 / 4)));

        std::string clubName = asciiFold(h.get(r, {"club", "team", "clubname"}));
        // Skip free agents / unattached players so they don't form junk teams.
        if (clubName.empty() || isNonClub(clubName)) continue;
        size_t idx = ensureIdx(clubName);
        if (idx == kNoTeam) continue;  // unknown club and clubs file present
        teams[idx].squad.push_back(std::move(p));
    }

    // Assign shirt numbers per club (1..N) where missing, and pick a starting XI.
    for (auto& t : teams) {
        bool anyNumbers = false;
        for (const auto& pl : t.squad)
            if (pl.shirtNumber > 0) { anyNumbers = true; break; }
        if (!anyNumbers) {
            int n = 1;
            for (auto& pl : t.squad) pl.shirtNumber = n++;
        }
        if (t.startingXI.empty()) t.autoSelectXI();
        t.autoOrderSubstitutes();  // Ensure substitutes have proper positional balance
    }
    return true;
}

// ---------------------------------------------------------------------------
// Top-level load (honours data/datasources.cfg)
// ---------------------------------------------------------------------------
bool Database::load(const std::string& dataDir) {
    teams.clear();
    competitions.clear();
    nextTeamId_ = 1;

    std::string teamsPath = dataDir + "/TeamsDB.csv";
    std::string playersPath = dataDir + "/PlayersDB.csv";
    std::string competitionsPath = dataDir + "/Competetions.csv";

    // Optional override file lets the user point at their own databases
    // (e.g. players = D:\DEV\Docs\Players db1 csv.csv).
    std::ifstream cfg(dataDir + "/datasources.cfg");
    if (cfg.is_open()) {
        std::string line;
        while (std::getline(cfg, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = lower(Csv::trim(line.substr(0, eq)));
            std::string val = Csv::trim(line.substr(eq + 1));
            if (val.empty()) continue;
            auto resolve = [&](const std::string& v) -> std::string {
                bool absolute = v.size() > 1 &&
                                (v[0] == '/' || v[0] == '\\' || v[1] == ':');
                return absolute ? v : (dataDir + "/" + v);
            };
            if (key == "players" || key == "playersdb") playersPath = resolve(val);
            else if (key == "clubs" || key == "teams" || key == "teamsdb")
                teamsPath = resolve(val);
            else if (key == "competitions" || key == "competitionsdb")
                competitionsPath = resolve(val);
        }
    }

    // Clubs are optional: if absent or unreadable, they are created on demand
    // from the players' Club column.
    loadTeams(teamsPath);
    if (!loadPlayers(playersPath)) return false;

    // Load competitions (optional)
    loadCompetitions(competitionsPath);

    return !teams.empty();
}

// ---------------------------------------------------------------------------
// Lookups
// ---------------------------------------------------------------------------
Team* Database::findTeam(int id) {
    for (auto& t : teams)
        if (t.id == id) return &t;
    return nullptr;
}

Team* Database::findTeamByName(const std::string& name) {
    for (auto& t : teams)
        if (lower(t.name) == lower(name)) return &t;
    return nullptr;
}

Competition* Database::findCompetition(const std::string& league) {
    auto it = competitions.find(league);
    if (it != competitions.end()) return &it->second;
    return nullptr;
}

std::vector<std::string> Database::leagues() const {
    std::vector<std::string> out;
    for (const auto& t : teams)
        if (std::find(out.begin(), out.end(), t.league) == out.end()) out.push_back(t.league);
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<Team*> Database::teamsInLeague(const std::string& league) {
    std::vector<Team*> out;
    for (auto& t : teams)
        if (t.league == league) out.push_back(&t);
    return out;
}

void Database::searchPlayers(const std::string& query,
                             std::vector<std::pair<const Team*, const Player*>>& out) const {
    std::string q = lower(query);
    for (const auto& t : teams)
        for (const auto& p : t.squad)
            if (q.empty() || lower(p.name).find(q) != std::string::npos)
                out.emplace_back(&t, &p);
}

void Database::searchTeams(const std::string& query, std::vector<const Team*>& out) const {
    std::string q = lower(query);
    for (const auto& t : teams)
        if (q.empty() || lower(t.name).find(q) != std::string::npos) out.push_back(&t);
}

// ---------------------------------------------------------------------------
// Load Competitions
// ---------------------------------------------------------------------------
bool Database::loadCompetitions(const std::string& path) {
    std::vector<std::vector<std::string>> rows;
    if (!Csv::read(path, rows)) {
        // Competitions file is optional
        return false;
    }

    if (rows.empty()) return false;

    // Parse header
    Header hdr;
    hdr.build(rows[0]);

    // Parse each competition
    for (size_t i = 1; i < rows.size(); ++i) {
        const auto& row = rows[i];
        if (row.empty()) continue;

        std::string league = Csv::trim(row.size() > 0 ? row[0] : "");
        if (league.empty()) continue;

        Competition comp;
        comp.league = league;
        comp.nation = Csv::trim(hdr.get(row, {"nation"}));
        comp.hierarchy = hdr.getInt(row, {"hiracy", "hierarchy"});
        comp.teams = hdr.getInt(row, {"teams"});
        comp.rounds = hdr.getInt(row, {"rounds"});
        comp.bench = hdr.getInt(row, {"bench"});
        comp.subs = hdr.getInt(row, {"subs", "substitutions"});
        comp.pauseStart = hdr.getInt(row, {"pausestart"});
        comp.pauseEnd = hdr.getInt(row, {"pauseend"});
        comp.transferWindowSummerStart = hdr.getInt(row, {"transferwindowsummerstart"});
        comp.transferWindowSummerEnd = hdr.getInt(row, {"transferwindowsummerend"});
        comp.transferWindowWinterStart = hdr.getInt(row, {"transferwindowwinterstart"});
        comp.transferWindowWinterEnd = hdr.getInt(row, {"transferwindowwinterend"});
        comp.defaultMatchDay = Csv::trim(hdr.get(row, {"defaultmatchday", "default match day"}));

        // Set defaults if not specified
        if (comp.bench == 0) comp.bench = 5;
        if (comp.subs == 0) comp.subs = 3;
        if (comp.defaultMatchDay.empty()) comp.defaultMatchDay = "Saturday";

        // Parse round weeks (Round 1, Round 2, etc.)
        comp.roundWeeks.clear();
        for (int r = 1; r <= comp.rounds; ++r) {
            std::string roundKey = "round" + std::to_string(r);
            int week = hdr.getInt(row, {roundKey.c_str()});
            comp.roundWeeks.push_back(week);
        }

        competitions[league] = comp;
    }

    return true;
}

}  // namespace nm
