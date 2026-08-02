#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <map>

#include "../core/Team.h"

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

// In-memory store of all teams (and their players / leagues) loaded from the
// CSV data files. The loader is header-driven and supports both the bundled
// sample format and Championship Manager / FM style exports (see Database.cpp).
class Database {
public:
    std::vector<Team> teams;
    std::map<std::string, Competition> competitions;  // League name -> Competition config

    // Loads using paths from data/datasources.cfg if present, else the bundled
    // TeamsDB.csv / PlayersDB.csv inside dataDir.
    bool load(const std::string& dataDir);

    bool loadTeams(const std::string& path);
    bool loadPlayers(const std::string& path);
    bool loadCompetitions(const std::string& path);

    Team* findTeam(int id);
    Team* findTeamByName(const std::string& name);
    Competition* findCompetition(const std::string& league);
    std::vector<std::string> leagues() const;
    std::vector<Team*> teamsInLeague(const std::string& league);

    // Used by the "Edit database" screen.
    void searchPlayers(const std::string& query,
                       std::vector<std::pair<const Team*, const Player*>>& out) const;
    void searchTeams(const std::string& query, std::vector<const Team*>& out) const;

private:
    int nextTeamId_ = 1;
    // jersey# -> player name, keyed by lower-cased team name; populated by loadTeams
    std::unordered_map<std::string, std::map<int, std::string>> jerseyMap_;
};

}  // namespace nm
