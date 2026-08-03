// Unit tests for MatchEngine.
//
// Rather than hand-building synthetic squads (risking an invalid Team that
// Team::autoSelectXI() can't fill), this loads the same bundled data files
// the shipped game uses, which doubles as a regression check that the data
// files still parse into a simulatable match.
#include "engine/MatchEngine.h"
#include "data/Database.h"

#include <iostream>
#include <string>
#include <vector>

using namespace nm;

namespace {

int g_failures = 0;

void check(bool cond, const std::string& label) {
    std::cout << (cond ? "PASS" : "FAIL") << " - " << label << "\n";
    if (!cond) ++g_failures;
}

// Returns the first two teams with a full enough squad to field a starting XI.
std::vector<Team*> findTwoPlayableTeams(Database& db) {
    std::vector<Team*> out;
    for (auto& t : db.teams) {
        if (t.squad.size() >= 11) out.push_back(&t);
        if (out.size() == 2) break;
    }
    return out;
}

void testMatchProducesFinishedResultWithSaneStats(const std::string& dataDir) {
    Database db;
    bool loaded = db.load(dataDir);
    check(loaded, "bundled data loads for match engine test");
    if (!loaded) return;

    auto teams = findTwoPlayableTeams(db);
    check(teams.size() == 2, "found two teams with full squads to play a match");
    if (teams.size() != 2) return;

    Config cfg;
    cfg.loadFile(dataDir + "/engine.cfg");  // optional; defaults apply if absent

    MatchEngine engine(cfg, /*seed=*/42u);
    MatchResult result = engine.simulate(*teams[0], *teams[1]);

    check(result.finished, "match reports finished");
    check(result.homeName == teams[0]->name, "home name recorded on result");
    check(result.awayName == teams[1]->name, "away name recorded on result");
    check(result.homeGoals >= 0 && result.awayGoals >= 0, "non-negative final score");
    check(!result.events.empty(), "commentary events were generated");
    check(result.stats.passAtt[0] + result.stats.passAtt[1] > 0,
          "at least one pass was attempted over 90 minutes");
}

void testSameSeedIsDeterministic(const std::string& dataDir) {
    Database dbA;
    dbA.load(dataDir);
    auto teamsA = findTwoPlayableTeams(dbA);
    if (teamsA.size() != 2) {
        check(false, "determinism check: could not find two playable teams");
        return;
    }
    std::string homeName = teamsA[0]->name;
    std::string awayName = teamsA[1]->name;

    Config cfg;
    cfg.loadFile(dataDir + "/engine.cfg");

    MatchEngine engineA(cfg, /*seed=*/777u);
    MatchResult a = engineA.simulate(*teamsA[0], *teamsA[1]);

    // Fresh database + fresh engine with the same seed should reproduce the
    // same match, since neither the CSV parser nor the RNG carry hidden state.
    Database dbB;
    dbB.load(dataDir);
    Team* homeB = dbB.findTeamByName(homeName);
    Team* awayB = dbB.findTeamByName(awayName);
    check(homeB != nullptr && awayB != nullptr,
          "determinism check: same two teams found in a fresh load");
    if (!homeB || !awayB) return;

    MatchEngine engineB(cfg, /*seed=*/777u);
    MatchResult b = engineB.simulate(*homeB, *awayB);

    check(a.homeGoals == b.homeGoals && a.awayGoals == b.awayGoals,
          "same seed reproduces the same final score");
    check(a.events.size() == b.events.size(),
          "same seed reproduces the same number of commentary events");
}

}  // namespace

int RunMatchEngineTests(const std::string& dataDir) {
    g_failures = 0;

    testMatchProducesFinishedResultWithSaneStats(dataDir);
    testSameSeedIsDeterministic(dataDir);

    std::cout << (g_failures == 0 ? "All MatchEngine tests passed."
                                  : std::to_string(g_failures) + " MatchEngine check(s) failed.")
              << "\n";
    return g_failures;
}
