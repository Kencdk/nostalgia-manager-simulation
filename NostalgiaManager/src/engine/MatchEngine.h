#pragma once
#include <functional>
#include <string>
#include <vector>

#include "../core/Pitch.h"
#include "../core/Team.h"
#include "Config.h"
#include "Rng.h"

namespace nm {

enum class Action { Passing, Longpass, Move, Dribble, Longshot, Finish, Header };

std::string ActionName(Action a);

struct MatchEvent {
    double minute = 0.0;
    int half = 1;
    std::string text;
    bool key = false;  // goals, shots, big chances -> shown prominently
};

// Live, cumulative match statistics per side (0 = home, 1 = away). Counters
// only — they never feed back into the simulation, so balance is unaffected.
struct MatchStats {
    int shots[2] = {0, 0};       // total attempts
    int onTarget[2] = {0, 0};    // attempts on goal
    int passAtt[2] = {0, 0};     // passes attempted
    int passOk[2] = {0, 0};      // passes completed
    int fouls[2] = {0, 0};
    long possTicks[2] = {0, 0};  // ball-action ticks spent in possession
};

struct MatchResult {
    std::string homeName, awayName;
    int homeGoals = 0;
    int awayGoals = 0;
    int homeShots = 0, awayShots = 0;
    MatchStats stats;
    std::vector<MatchEvent> events;        // narrated ball actions
    std::vector<std::string> fullLog;      // per-player-per-round verbose log
    bool finished = false;
};

// Implements the GBNFM "Developer-Ready Action System": two phases (Desire then
// Execution), zone-restricted actions, defensive pressure and thresholds.
class MatchEngine {
public:
    MatchEngine(const Config& cfg, unsigned seed) : cfg_(cfg), rng_(seed) {}

    // Optional callback fired for every narrated event (used for the live match
    // screen). If it returns false the simulation keeps running silently.
    using EventHook = std::function<void(const MatchEvent&)>;

    MatchResult simulate(Team& home, Team& away, bool verbose = false,
                         const EventHook& hook = nullptr);

    // ASCII rendering of the live pitch matrix (A1-I13) with player shirt
    // numbers and the ball. Valid during simulation (e.g. from an EventHook).
    std::string renderPitch() const;
    const std::string& homeName() const { return res_->homeName; }
    const std::string& awayName() const { return res_->awayName; }

    // Live state accessors for the graphical match screen (valid mid-sim).
    const MatchStats& stats() const { return stats_; }
    float ballX() const { return ballPosition_.x; }  // 0-105m, continuous
    float ballY() const { return ballPosition_.y; }  // 0-68m, continuous
    int ballCol() const { return ball_.col; }   // 1..13, goal-to-goal (grid)
    int ballRow() const { return ball_.row; }    // 0..8, across the pitch (grid)
    int carrierSide() const { return carrierSide_; }  // 0 home, 1 away, -1 none

    // Player positions for graphical pitch rendering (valid mid-sim).
    struct PlayerSnapshot {
        int shirtNumber = 0;
        float x = 52.5f;  // Continuous X position (0-105m)
        float y = 34.0f;  // Continuous Y position (0-68m)
        int col = 7;      // 1..13 (grid cell for compatibility)
        int row = 5;      // 1..9 (grid cell for compatibility)
        int side = 0;     // 0 home, 1 away
        bool isCarrier = false;
    };
    std::vector<PlayerSnapshot> getPlayerPositions() const {
        std::vector<PlayerSnapshot> result;
        for (int s = 0; s < 2; ++s) {
            for (const Player* p : sidePlayers_[s]) {
                PlayerSnapshot ps;
                ps.shirtNumber = p->shirtNumber;
                ps.x = p->position.x;
                ps.y = p->position.y;
                ps.col = p->pos.col;
                ps.row = p->pos.row;
                ps.side = s;
                ps.isCarrier = (p == carrier_);
                result.push_back(ps);
            }
        }
        return result;
    }

private:
    const Config& cfg_;
    Rng rng_;

    // --- live state ---
    std::vector<Player*> sidePlayers_[2];  // 0 = home, 1 = away
    int dir_[2] = {+1, -1};                 // attacking column direction per side
    Position2D ballPosition_ = Position2D();  // Continuous ball position
    Cell ball_ = CentreSpot();  // Grid cell (for action logic)
    int carrierSide_ = 0;
    Player* carrier_ = nullptr;
    bool aerial_ = false;
    int score_[2] = {0, 0};
    int shots_[2] = {0, 0};
    MatchStats stats_;

    MatchResult* res_ = nullptr;
    bool verbose_ = false;
    EventHook hook_;
    int half_ = 1;
    double clock_ = 0.0;

    // --- helpers ---
    void setup(Team& home, Team& away);
    void kickoff(int controllingSide);
    void runHalf(int half);
    void resolveBallAction();
    void moveOffBallPlayers();
    void resolveCollisions();  // Prevent players from occupying same cell

    int zoneProgress(const Cell& c, int side) const;  // 1..13 toward goal
    std::string zoneName(int progress) const;         // Defensive/Midfield/Attack
    std::vector<Action> allowedActions(int progress) const;

    double desire(const Player& p, Action a, const std::string& zone,
                  bool opponentNearby) const;
    double execution(const Player& p, Action a);
    double pressure(const Cell& at, int defendingSide, int& defenderCount,
                    Player** best);
    std::pair<std::string, double> threshold(Action a, bool pressured,
                                             bool crowded) const;

    int opponentsNear(const Cell& at, int defendingSide, int radius) const;
    Player* nearestOpponent(const Cell& at, int defendingSide) const;
    Player* choosePassTarget(bool longPass);
    Player* goalkeeper(int side) const;

    void giveBall(Player* p, int side);
    void turnover(const std::string& reason);
    void goalKick(int side);
    void onShot(Action a, double finalScore, double thr);

    void logEvent(const std::string& text, bool key = false);
    void logPlayerRound();
};

}  // namespace nm
