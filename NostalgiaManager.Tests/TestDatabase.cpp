// Unit tests for Database CSV loading.
#include "data/Database.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace nm;

namespace {

int g_failures = 0;

void check(bool cond, const std::string& label) {
    std::cout << (cond ? "PASS" : "FAIL") << " - " << label << "\n";
    if (!cond) ++g_failures;
}

void writeFile(const fs::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
}

// Minimal valid "legacy" bundled-format fixture (see README: TeamsDB.csv is
// id,name,league,formation,mentality; PlayersDB.csv is
// id,name,teamId,role,number,<attributes>).
void writeValidFixture(const fs::path& dir) {
    writeFile(dir / "TeamsDB.csv",
              "id,name,league,formation,mentality\n"
              "1,Test FC,Test League,4-4-2,Standard\n");
    writeFile(dir / "PlayersDB.csv",
              "id,name,teamId,role,number,age,Passing,Shooting\n"
              "101,Alice Striker,1,F,9,24,15,18\n"
              "102,Bob Keeper,1,GK,1,29,8,5\n");
}

void testSuccessfulDataLoadingAndParsing() {
    fs::path dir = fs::temp_directory_path() / "nm_test_db_valid";
    fs::create_directories(dir);
    writeValidFixture(dir);

    Database db;
    bool loaded = db.load(dir.string());
    check(loaded, "valid fixture loads successfully");
    check(db.teams.size() == 1, "exactly one team loaded");

    if (!db.teams.empty()) {
        const Team& t = db.teams[0];
        check(t.name == "Test FC", "team name parsed");
        check(t.league == "Test League", "team league parsed");
        check(t.formation == "4-4-2", "team formation parsed");
        check(t.squad.size() == 2, "both players attached to the team");

        const Player* alice = nullptr;
        const Player* bob = nullptr;
        for (const auto& p : t.squad) {
            if (p.name == "Alice Striker") alice = &p;
            if (p.name == "Bob Keeper") bob = &p;
        }
        check(alice != nullptr, "Alice Striker was loaded");
        check(bob != nullptr, "Bob Keeper was loaded");
        if (alice) {
            check(alice->role == Role::F, "Alice's role parsed as Forward");
            check(alice->shirtNumber == 9, "Alice's shirt number parsed");
            check(alice->attr.get("Passing") == 15, "Alice's Passing attribute parsed");
            check(alice->attr.get("Shooting") == 18, "Alice's Shooting attribute parsed");
        }
        if (bob) {
            check(bob->role == Role::GK, "Bob's role parsed as Goalkeeper");
        }
    }

    fs::remove_all(dir);
}

void testPlayerWithUnknownTeamIdIsSkipped() {
    fs::path dir = fs::temp_directory_path() / "nm_test_db_unknown_team";
    fs::create_directories(dir);
    writeFile(dir / "TeamsDB.csv",
              "id,name,league,formation,mentality\n"
              "1,Test FC,Test League,4-4-2,Standard\n");
    writeFile(dir / "PlayersDB.csv",
              "id,name,teamId,role,number\n"
              "101,Valid Player,1,F,9\n"
              "102,Orphan Player,999,F,10\n");

    Database db;
    bool loaded = db.load(dir.string());
    check(loaded, "fixture with one bad teamId still loads");
    check(db.teams.size() == 1, "no phantom team is created for the bad id");
    if (!db.teams.empty()) {
        check(db.teams[0].squad.size() == 1,
              "player referencing an unknown teamId is silently dropped");
    }

    fs::remove_all(dir);
}

void testEmptyTeamsFileYieldsNoTeams() {
    fs::path dir = fs::temp_directory_path() / "nm_test_db_empty";
    fs::create_directories(dir);
    // Header only, no data rows -> loadTeams fails and no teams are created.
    writeFile(dir / "TeamsDB.csv", "id,name,league,formation,mentality\n");
    writeFile(dir / "PlayersDB.csv",
              "id,name,teamId,role,number\n"
              "101,Nobody,1,F,9\n");

    Database db;
    bool loaded = db.load(dir.string());
    check(!loaded, "load() reports failure when no teams exist");
    check(db.teams.empty(), "teams collection stays empty");

    fs::remove_all(dir);
}

void testLoadsBundledProductionData(const std::string& dataDir) {
    Database db;
    bool loaded = db.load(dataDir);
    check(loaded, "bundled game data (" + dataDir + ") loads without error");
    check(!db.teams.empty(), "bundled data produces at least one team");

    bool anySquad = false;
    for (const auto& t : db.teams)
        if (!t.squad.empty()) { anySquad = true; break; }
    check(anySquad, "at least one bundled team has players");
    check(!db.leagues().empty(), "bundled data exposes at least one league");

    if (!db.teams.empty()) {
        Team* found = db.findTeamByName(db.teams[0].name);
        check(found != nullptr && found->id == db.teams[0].id,
              "findTeamByName is consistent with the loaded list");
    }
}

}  // namespace

int RunDatabaseTests(const std::string& dataDir) {
    g_failures = 0;

    testSuccessfulDataLoadingAndParsing();
    testPlayerWithUnknownTeamIdIsSkipped();
    testEmptyTeamsFileYieldsNoTeams();
    testLoadsBundledProductionData(dataDir);

    std::cout << (g_failures == 0 ? "All Database tests passed."
                                  : std::to_string(g_failures) + " Database check(s) failed.")
              << "\n";
    return g_failures;
}
