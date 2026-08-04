#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <map>

#include "../core/Team.h"
#include "XmlRules.h"

namespace nm {

// Competition configuration for leagues and tournaments
struct Competition {
    std::string league;
    std::string nation;
    int hierarchy = 1;
    int teams = 0;
    int rounds = 0;
    int bench = 5;              // Number of substitutes on bench
    int subs = 3;               // Number of substitutions allowed
    int pauseStart = 0;         // Week when winter break starts (0 = no break)
    int pauseEnd = 0;           // Week when winter break ends
    int transferWindowSummerStart = 1;
    int transferWindowSummerEnd = 52;
    int transferWindowWinterStart = 0;
    int transferWindowWinterEnd = 0;
    std::string defaultMatchDay = "Saturday";
    std::vector<int> roundWeeks;  // Week number for each round
};

// Positional layout for a named formation loaded from Tactics.csv.
// positionCounts maps position abbreviation ("DC", "MC", etc.) to the number
// of players required in that position.
struct TacticTemplate {
    std::string name;                          // e.g. "442", "4231"
    std::map<std::string, int> positionCounts; // position -> count (only entries > 0)
    std::string style;                         // optional style tag
    std::string mentality;                     // optional mentality tag

    // Expand positionCounts into an ordered list of Position enums suitable
    // for use in autoSelectXI / getFormationPositions.
    // Order: GK, D (DC/DR/DL/WBR/WBL), DMC, M (MC/MR/ML), AM (AMC/AMR/AML), F (FC/FR/FL)
    std::vector<Position> toPositionList() const {
        // Canonical order to emit positions
        const struct { const char* key; Position pos; } order[] = {
            {"GK",  Position::GK},
            {"DC",  Position::DC}, {"DR",  Position::DR}, {"DL",  Position::DL},
            {"WBR", Position::WBR},{"WBL", Position::WBL},
            {"DMC", Position::DM},
            {"MC",  Position::MC}, {"MR",  Position::MR}, {"ML",  Position::ML},
            {"AMC", Position::AMC},{"AMR", Position::AMR},{"AML", Position::AML},
            {"FC",  Position::FC}, {"FR",  Position::FR}, {"FL",  Position::FL},
        };
        std::vector<Position> out;
        for (const auto& o : order) {
            auto it = positionCounts.find(o.key);
            if (it == positionCounts.end()) continue;
            for (int i = 0; i < it->second; ++i) out.push_back(o.pos);
        }
        return out;
    }

    // Returns a human-readable summary, e.g. "GK x1  DC x2  MC x2  FC x2"
    std::string summary() const {
        std::string s;
        for (const auto& kv : positionCounts) {
            if (!s.empty()) s += "  ";
            s += kv.first + " x" + std::to_string(kv.second);
        }
        return s;
    }
};

// In-memory store of all teams (and their players / leagues) loaded from the
// CSV data files. The loader is header-driven and supports both the bundled
// sample format and Championship Manager / FM style exports (see Database.cpp).
class Database {
public:
    std::vector<Team> teams;
    std::map<std::string, Competition> competitions;  // League name -> Competition config
    std::map<std::string, TacticTemplate> tactics;    // Formation name -> TacticTemplate
    XmlRulesLoader xmlRules;                          // Country competition rules from XML files

    // Loads using paths from data/datasources.cfg if present, else the bundled
    // TeamsDB.csv / PlayersDB.csv inside dataDir.
    bool load(const std::string& dataDir);

    bool loadTeams(const std::string& path);
    bool loadPlayers(const std::string& path);
    bool loadCompetitions(const std::string& path);
    bool loadTactics(const std::string& path);
    // Patch formation fields on already-loaded teams from a secondary CSV
    // (matched by club name). Used when ClubsDB provides IDs/colours and
    // TeamsDB provides the actual formations.
    void patchFormations(const std::string& path);

    // Load teams and players directly from a SQLite database file.
    // The DB must contain tables "ClubsDB" and "PlayersDB1997 with player ID".
    bool loadFromSqlite(const std::string& dbPath);

    Team* findTeam(int id);
    Team* findTeamByName(const std::string& name);
    Competition* findCompetition(const std::string& league);
    const TacticTemplate* findTactic(const std::string& formation) const;
    std::vector<std::string> leagues() const;
    std::vector<Team*> teamsInLeague(const std::string& league);
    std::vector<Team*> teamsInNationLeague(const std::string& nation, const std::string& league);

    // Used by the "Edit database" screen.
    void searchPlayers(const std::string& query,
                       std::vector<std::pair<const Team*, const Player*>>& out) const;
    void searchTeams(const std::string& query, std::vector<const Team*>& out) const;

private:
    int nextTeamId_ = 1;
    // jersey# -> player ID, keyed by team ID; populated by loadTeams
    std::unordered_map<int, std::map<int, int>> jerseyMap_;

    // Row-based helpers called by both the CSV and SQLite loaders.
    // rows[0] must be the header row; subsequent rows are data.
    bool loadTeamsFromRows(const std::vector<std::vector<std::string>>& rows);
    bool loadPlayersFromRows(const std::vector<std::vector<std::string>>& rows);
    bool loadTacticsFromRows(const std::vector<std::vector<std::string>>& rows);
};

}  // namespace nm
