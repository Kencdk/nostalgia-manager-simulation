#include "MatchEngine.h"
#include "MatchEngine.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>

#include "../core/Formation.h"

namespace nm {

std::string ActionName(Action a) {
    switch (a) {
        case Action::Passing:  return "Passing";
        case Action::Longpass: return "Longpass";
        case Action::Move:     return "Move";
        case Action::Dribble:  return "Dribble";
        case Action::Longshot: return "Longshot";
        case Action::Finish:   return "Finish";
        case Action::Header:   return "Header";
    }
    return "?";
}

namespace {
std::string shirt(const Player& p) {
    return "#" + std::to_string(p.shirtNumber) + " " + p.name;
}
}  // namespace

// ---------------------------------------------------------------------------
// Setup & match loop
// ---------------------------------------------------------------------------
void MatchEngine::setup(Team& home, Team& away) {
    dir_[0] = +1;  // home attacks towards column 13
    dir_[1] = -1;  // away attacks towards column 1

    if (home.startingXI.empty()) home.autoSelectXI();
    if (away.startingXI.empty()) away.autoSelectXI();

    // Update player roles to match their assigned tactical positions
    home.updatePlayerRoles();
    away.updatePlayerRoles();

    PlaceStartingXI(home, 1);
    PlaceStartingXI(away, 2);

    sidePlayers_[0].clear();
    sidePlayers_[1].clear();
    for (int pid : home.startingXI)
        if (Player* p = home.findPlayer(pid)) sidePlayers_[0].push_back(p);
    for (int pid : away.startingXI)
        if (Player* p = away.findPlayer(pid)) sidePlayers_[1].push_back(p);
}

void MatchEngine::kickoff(int controllingSide) {
    // Reset everyone to defensive (home) positions, ball on centre spot.
    for (int s = 0; s < 2; ++s)
        for (Player* p : sidePlayers_[s]) {
            p->pos = p->homePos;
            p->position = p->homePosition;
            p->hasBall = false;
            p->dispossessionCooldown = 0;  // Clear any cooldowns
        }
    ball_ = CentreSpot();
    ballPosition_ = cellToPosition(ball_);
    aerial_ = false;
    Player* nearest = nearestOpponent(ball_, controllingSide);  // nearest of controlling team
    if (nearest) giveBall(nearest, controllingSide);
}

void MatchEngine::swapEnds(Team& home, Team& away) {
    // Swap attacking directions
    dir_[0] = -dir_[0];
    dir_[1] = -dir_[1];

    // Reposition all players to their home positions on the opposite side
    // Mirror column positions: col -> (kCols + 1 - col)
    for (int s = 0; s < 2; ++s) {
        for (Player* p : sidePlayers_[s]) {
            // Mirror the home position column
            p->homePos.col = kCols + 1 - p->homePos.col;
            p->homePosition = cellToPosition(p->homePos);

            // Reset current position to new home position
            p->pos = p->homePos;
            p->position = p->homePosition;
            p->hasBall = false;
            p->dispossessionCooldown = 0;
        }
    }
}

MatchResult MatchEngine::simulate(Team& home, Team& away, bool verbose,
                                  const EventHook& hook) {
    MatchResult res;
    res.homeName = home.name;
    res.awayName = away.name;
    res_ = &res;
    verbose_ = verbose;
    hook_ = hook;

    setup(home, away);

    int firstBall = rng_.range(1, 2);  // Random(1-2) decides who starts
    int controlling = (firstBall == 1) ? 0 : 1;
    logEvent("Kickoff, " + (controlling == 0 ? home.name : away.name) +
                 " start with the ball",
             true);
    kickoff(controlling);

    runHalf(1);

    logEvent("Half time: " + home.name + " " + std::to_string(score_[0]) + " - " +
                 std::to_string(score_[1]) + " " + away.name,
             true);

    // Switch ends for second half: teams swap directions
    swapEnds(home, away);

    // Second half: other side kicks off.
    int secondControl = 1 - controlling;
    logEvent("Second half kickoff", true);
    kickoff(secondControl);
    runHalf(2);

    res.homeGoals = score_[0];
    res.awayGoals = score_[1];
    res.homeShots = shots_[0];
    res.awayShots = shots_[1];
    res.stats = stats_;
    logEvent("Full time: " + home.name + " " + std::to_string(score_[0]) + " - " +
                 std::to_string(score_[1]) + " " + away.name,
             true);
    res.finished = true;
    return res;
}

void MatchEngine::runHalf(int half) {
    half_ = half;
    // 45 minutes, 6 action rounds per minute.
    for (int minute = 0; minute < 45; ++minute) {
        for (int round = 0; round < 6; ++round) {
            // First half: minutes 1-45, Second half: minutes 46-90
            clock_ = (half - 1) * 45.0 + minute + 1.0 + round / 6.0;
            resolveBallAction();
            moveOffBallPlayers();

            // Decrement dispossession cooldowns
            for (int s = 0; s < 2; ++s)
                for (Player* p : sidePlayers_[s])
                    if (p->dispossessionCooldown > 0)
                        --p->dispossessionCooldown;

            if (verbose_) logPlayerRound();
        }
    }
}

// ---------------------------------------------------------------------------
// Zones & allowed actions
// ---------------------------------------------------------------------------
int MatchEngine::zoneProgress(const Cell& c, int side) const {
    // Progress towards the opponent goal, expressed as 1..13.
    return (side == 0) ? c.col : (kCols + 1 - c.col);
}

std::string MatchEngine::zoneName(int progress) const {
    if (progress <= 4) return "Defensive";
    if (progress <= 9) return "Midfield";
    return "Attack";
}

std::vector<Action> MatchEngine::allowedActions(int progress) const {
    std::string z = zoneName(progress);

    // Get pressure level
    int defenders = opponentsNear(ball_, 1 - carrierSide_, 1);
    bool underPressure = defenders >= 1;
    bool heavyPressure = defenders >= 2;

    if (z == "Defensive") {
        // Own third: prioritize safety
        if (heavyPressure) {
            // Under heavy pressure: clear or quick pass only
            return {Action::Passing, Action::Longpass};
        } else if (underPressure) {
            // Under pressure: pass or try to move
            return {Action::Passing, Action::Longpass, Action::Move};
        } else {
            // Space available: can build up
            return {Action::Passing, Action::Longpass, Action::Move, Action::Dribble};
        }
    }

    if (z == "Midfield") {
        // Midfield: balanced options
        if (heavyPressure) {
            // Heavy pressure: pass or try to dribble out
            return {Action::Passing, Action::Dribble};
        } else {
            // Standard midfield options
            std::vector<Action> actions = {Action::Passing, Action::Longpass, Action::Move, Action::Dribble};
            // Only allow long shots if in good position (central, not too deep)
            if (progress >= 8 && std::abs(ball_.row - 4) <= 2) {
                actions.push_back(Action::Longshot);
            }
            return actions;
        }
    }

    // Attack zone
    if (progress >= 12) {
        // Very close to goal - shooting is priority
        if (heavyPressure) {
            return {Action::Passing, Action::Finish};  // Quick pass or shoot
        } else {
            return {Action::Passing, Action::Dribble, Action::Finish};
        }
    } else if (progress >= 10) {
        // In box area - shooting and passing
        std::vector<Action> actions = {Action::Passing, Action::Dribble};
        // Allow shooting if in good position
        if (std::abs(ball_.row - 4) <= 3) {  // Central or semi-central
            actions.push_back(Action::Finish);
            actions.push_back(Action::Longshot);
        }
        return actions;
    } else {
        // Attacking third but not in box
        return {Action::Passing, Action::Move, Action::Dribble, Action::Longshot};
    }
}

// ---------------------------------------------------------------------------
// Phase A: desire
// ---------------------------------------------------------------------------
double MatchEngine::desire(const Player& p, Action a, const std::string& zone,
                           bool opponentNearby) const {
    const std::string an = ActionName(a);
    double score = 0.0;
    for (const auto& stat : AttributeNames()) {
        double w = cfg_.get("desire." + an + "." + stat, 0.0);
        if (w != 0.0) score += w * p.norm(stat);
    }
    double zmod = cfg_.get("zonemod." + an + "." + zone, 1.0);
    score *= zmod;

    // Per-role appetite for each action (e.g. defenders rarely dribble).
    score *= cfg_.get("rolemod." + RoleName(p.role) + "." + an, 1.0);

    // Context-specific bonuses
    if (a == Action::Dribble && opponentNearby)
        score += cfg_.get("bonus.Dribble.opponentNearby", 0.0);

    // Shooting context: prefer finish over longshot when close and central
    if (a == Action::Finish) {
        int progress = zoneProgress(ball_, carrierSide_);
        int centrality = std::abs(ball_.row - 4);  // 0=center, 4=edge
        if (progress >= 12) score *= 1.5;  // Very close to goal
        if (centrality == 0) score *= 1.3;  // Perfect central position
        else if (centrality >= 3) score *= 0.7;  // Wide angle
    } else if (a == Action::Longshot) {
        int centrality = std::abs(ball_.row - 4);
        if (centrality <= 1) score *= 1.2;  // Prefer from central positions
        else if (centrality >= 3) score *= 0.5;  // Discourage from wide

        // Players with high long shots rating are more willing
        double longShotSkill = p.norm("Longshots");
        if (longShotSkill > 0.7) score *= 1.3;
        else if (longShotSkill < 0.4) score *= 0.6;
    }

    // Tunable per-action multiplier (balancing knob, spec section 12).
    score *= cfg_.get("desire.scale." + an, 1.0);
    return score;
}

// ---------------------------------------------------------------------------
// Phase B: execution
// ---------------------------------------------------------------------------
double MatchEngine::execution(const Player& p, Action a) {
    const std::string an = ActionName(a);
    auto cat = [&](const char* c) {
        double v = 0.0;
        for (const auto& stat : AttributeNames()) {
            double w = cfg_.get("exec." + an + "." + std::string(c) + "." + stat, 0.0);
            if (w != 0.0) v += w * p.norm(stat);
        }
        return v;
    };
    double skill = cat("skill");
    double mental = cat("mental");
    double physical = cat("physical");
    double r = cfg_.get("exec.random", 0.15);
    double base = cfg_.get("exec.coef.skill", 0.75) * skill +
                  cfg_.get("exec.coef.mental", 0.15) * mental +
                  cfg_.get("exec.coef.physical", 0.10) * physical;
    // Tunable additive lift so realistic skill levels clear the spec thresholds
    // (balancing knob, spec section 12).
    base += cfg_.get("exec.base", 0.0);
    return base + rng_.real(-r, r);
}

double MatchEngine::pressure(const Cell& at, int defendingSide, int& defenderCount,
                             Player** best) {
    defenderCount = 0;
    double bestVal = 0.0;
    *best = nullptr;
    for (Player* d : sidePlayers_[defendingSide]) {
        // Skip players on dispossession cooldown
        if (d->dispossessionCooldown > 0) continue;

        if (CellDistance(d->pos, at) <= 1) {
            ++defenderCount;
            double v = 0.0;
            for (const auto& stat : AttributeNames()) {
                double w = cfg_.get("pressure." + stat, 0.0);
                if (w != 0.0) v += w * d->norm(stat);
            }
            if (v > bestVal) {
                bestVal = v;
                *best = d;
            }
        }
    }
    return bestVal;
}

std::pair<std::string, double> MatchEngine::threshold(Action a, bool pressured,
                                                      bool crowded) const {
    const std::string an = ActionName(a);
    switch (a) {
        case Action::Passing:
            return pressured ? std::make_pair(std::string("Hard"),
                                              cfg_.get("threshold.Passing.Hard"))
                             : std::make_pair(std::string("Medium"),
                                              cfg_.get("threshold.Passing.Medium"));
        case Action::Longpass:
            return {"Medium", cfg_.get("threshold.Longpass.Medium")};
        case Action::Move:
            return pressured ? std::make_pair(std::string("Pressured"),
                                              cfg_.get("threshold.Move.Pressured"))
                             : std::make_pair(std::string("Open"),
                                              cfg_.get("threshold.Move.Open"));
        case Action::Dribble:
            return crowded ? std::make_pair(std::string("Crowded"),
                                            cfg_.get("threshold.Dribble.Crowded"))
                           : std::make_pair(std::string("Normal"),
                                            cfg_.get("threshold.Dribble.Normal"));
        case Action::Longshot:
            return {"Normal", cfg_.get("threshold.Longshot.Normal")};
        case Action::Finish:
            return crowded ? std::make_pair(std::string("Tight"),
                                            cfg_.get("threshold.Finish.Tight"))
                           : std::make_pair(std::string("Good"),
                                            cfg_.get("threshold.Finish.Good"));
        case Action::Header:
            return {"Normal", cfg_.get("threshold.Header.Normal")};
    }
    return {"Medium", 0.6};
}

// ---------------------------------------------------------------------------
// The core decision: resolve the action of the player on the ball.
// ---------------------------------------------------------------------------
void MatchEngine::resolveBallAction() {
    if (!carrier_) return;
    Player& p = *carrier_;
    int side = carrierSide_;
    int defendingSide = 1 - side;

    ++stats_.possTicks[side];  // one ball-action tick of possession

    int progress = zoneProgress(ball_, side);
    std::string zone = zoneName(progress);

    int challenge = opponentsNear(ball_, defendingSide, 1);  // immediate pressure
    bool pressured = challenge > 0;
    bool crowded = challenge >= 2;
    bool opponentNearby = opponentsNear(ball_, defendingSide, 2) > 0;  // for dribble

    // Goalkeeper-specific restrictions
    bool isGoalkeeper = (p.role == Role::GK);

    // Allowed actions (context rules: dribble needs an opponent; header only on
    // an aerial ball in the attacking zone).
    std::vector<Action> actions;

    if (isGoalkeeper) {
        // Goalkeepers should never dribble, shoot, or move with ball if opponents nearby
        // They should only pass or clear (long pass)
        if (opponentNearby || pressured) {
            // Under any pressure: only passing allowed
            actions.push_back(Action::Passing);
            actions.push_back(Action::Longpass);
        } else {
            // No pressure: can move within box, but prefer passing
            for (Action a : allowedActions(progress)) {
                // Skip shooting, dribble, and header entirely for GK
                if (a == Action::Dribble || a == Action::Finish || 
                    a == Action::Longshot || a == Action::Header) continue;
                // Allow move only if no opponents within 3 cells
                if (a == Action::Move) {
                    if (opponentsNear(ball_, defendingSide, 3) > 0) continue;
                }
                actions.push_back(a);
            }
        }
    } else {
        // Normal player action selection
        for (Action a : allowedActions(progress)) {
            if (a == Action::Dribble && !opponentNearby) continue;
            actions.push_back(a);
        }
        if (aerial_ && zone == "Attack") actions.push_back(Action::Header);
    }

    if (actions.empty()) actions.push_back(Action::Passing);

    // Phase A: weighted-random selection on desire (section 6).
    std::vector<double> weights;
    weights.reserve(actions.size());
    double total = 0.0;
    for (Action a : actions) {
        double w = std::max(0.01, desire(p, a, zone, opponentNearby));
        weights.push_back(w);
        total += w;
    }
    double pick = rng_.real(0.0, total);
    Action chosen = actions.back();
    double acc = 0.0;
    for (size_t i = 0; i < actions.size(); ++i) {
        acc += weights[i];
        if (pick <= acc) {
            chosen = actions[i];
            break;
        }
    }

    // Phase B: execution vs defensive pressure (sections 7-9).
    double exec = execution(p, chosen);
    int defCount = 0;
    Player* bestDef = nullptr;
    double pr = pressure(ball_, defendingSide, defCount, &bestDef);
    double mult = cfg_.get("pressure.multiplier", 0.35);
    double extraBonus = 0.0;
    if (defCount > 1)
        extraBonus = std::min(cfg_.get("pressure.extra.max", 0.15),
                              (defCount - 1) * cfg_.get("pressure.extra.bonus", 0.05));
    double finalScore = exec - mult * pr - extraBonus;

    auto thr = threshold(chosen, pressured, crowded);
    bool success = finalScore >= thr.second;

    // ---- Apply outcome (section 10) ----
    switch (chosen) {
        case Action::Passing:
        case Action::Longpass: {
            bool isLong = (chosen == Action::Longpass);
            ++stats_.passAtt[side];
            if (success) ++stats_.passOk[side];
            if (success) {
                Player* target = choosePassTarget(isLong);
                if (!target && isLong) target = choosePassTarget(false);
                if (!target) {  // no outlet: clear the ball upfield
                    // Clear distance varies based on pressure and player ability
                    double passingSkill = p.norm("Passing");
                    double visionSkill = p.norm("Vision");

                    // Base clear: 2-4 grid cells depending on skill
                    float baseClear = 2.0f + (passingSkill + visionSkill);  // 2-4 cells

                    // Under pressure: desperate clear is longer but less controlled
                    if (pressured) {
                        baseClear += rng_.real(1.0f, 2.0f);  // +1-2 cells
                    }

                    float clearDist = baseClear * (kPitchLength / kCols);
                    ballPosition_.x += dir_[side] * clearDist;

                    // Sideways variance (poor technique = less accurate)
                    float lateralVariance = (1.0f - passingSkill * 0.7f) * (kPitchWidth / kRows);
                    if (rng_.chance(0.6)) {
                        ballPosition_.y += rng_.real(-lateralVariance, lateralVariance);
                    }

                    aerial_ = true;
                    logEvent(shirt(p) + (pressured ? " clears under pressure" : " launches it forward"));

                    // Check if clearance went out of bounds
                    if (checkBallOutOfBounds()) {
                        break;  // Ball went out - restart handled
                    }

                    // Ball stayed in - update position
                    ballPosition_ = clampPosition(ballPosition_);
                    ball_ = positionToCell(ballPosition_);
                    p.pos = ball_;
                    p.position = ballPosition_;
                    break;
                }
                logEvent(shirt(p) + (isLong ? " plays a long ball to " : " passes to ") +
                         shirt(*target));
                aerial_ = isLong;  // long balls arrive in the air

                // Check if defenders can deflect the aerial ball
                if (isLong && aerial_) {
                    int defendingSide = 1 - side;
                    // Count defenders near the target
                    int nearbyDefenders = 0;
                    for (Player* d : sidePlayers_[defendingSide]) {
                        if (d->position.distanceTo(target->position) < 8.0f) {
                            nearbyDefenders++;
                        }
                    }

                    // Chance of deflection increases with nearby defenders
                    double deflectionChance = nearbyDefenders * 0.08;  // 8% per defender
                    if (rng_.chance(deflectionChance)) {
                        // Ball is deflected - add random deflection
                        float deflectX = rng_.real(-10.0f, 10.0f);
                        float deflectY = rng_.real(-8.0f, 8.0f);

                        Position2D deflectedPos = target->position;
                        deflectedPos.x += deflectX;
                        deflectedPos.y += deflectY;
                        ballPosition_ = deflectedPos;

                        if (checkBallOutOfBounds()) {
                            logEvent("Cross deflected out of play");
                            break;
                        }

                        // Deflection stayed in - ball is loose
                        ballPosition_ = clampPosition(ballPosition_);
                        ball_ = positionToCell(ballPosition_);
                        aerial_ = false;
                        logEvent("Ball is deflected - loose ball!");

                        // Nearest player to deflection wins it
                        Player* winner = nearestOpponent(ball_, side);
                        Player* defWinner = nearestOpponent(ball_, defendingSide);
                        if (defWinner && (!winner || 
                            defWinner->position.distanceTo(ballPosition_) < 
                            winner->position.distanceTo(ballPosition_))) {
                            winner = defWinner;
                            side = defendingSide;
                        }
                        if (winner) {
                            winner->pos = ball_;
                            winner->position = ballPosition_;
                            giveBall(winner, side);
                        }
                        break;
                    }
                }

                giveBall(target, side);
            } else {
                // Failed pass: ball intercepted partway to target or goes out
                Player* target = choosePassTarget(isLong);
                if (target) {
                    // Ball goes roughly halfway toward intended target
                    Position2D targetPos = target->position;
                    float interpFactor = rng_.real(0.3f, 0.6f);  // 30-60% of the way

                    // Chance of over-hitting and sending out of bounds
                    if (isLong && rng_.chance(0.25)) {
                        // Long pass over-hit - ball travels further and may go out
                        interpFactor = rng_.real(0.8f, 1.3f);
                    }

                    ballPosition_.x += (targetPos.x - ballPosition_.x) * interpFactor;
                    ballPosition_.y += (targetPos.y - ballPosition_.y) * interpFactor;

                    // Check if over-hit pass went out
                    if (checkBallOutOfBounds()) {
                        logEvent(shirt(p) + (isLong ? " over-hits a long ball" : " misplaces a pass") +
                                 " out of play");
                        break;
                    }

                    ballPosition_ = clampPosition(ballPosition_);
                    ball_ = positionToCell(ballPosition_);
                }
                logEvent(shirt(p) + (isLong ? " over-hits a long ball" : " misplaces a pass") +
                         " - intercepted");
                turnover("interception");
            }
            break;
        }
        case Action::Move: {
            if (success) {
                // Move ball forward in continuous space
                int step = std::max(1, std::min(p.maxMovePerAction(), 2));
                float moveDist = step * (kPitchLength / kCols);  // Convert grid steps to meters

                ballPosition_.x += dir_[side] * moveDist;

                // Sideways drift based on technique (skilled players run straighter)
                double technique = p.norm("Technique");
                double driftChance = 0.3 * (1.0 - technique * 0.5);  // 15-30% based on skill
                if (rng_.chance(driftChance)) {
                    float maxDrift = (1.0f - technique * 0.3f) * (kPitchWidth / kRows);
                    float sideDrift = rng_.real(-maxDrift, maxDrift);
                    ballPosition_.y += sideDrift;
                }

                ballPosition_ = clampPosition(ballPosition_);

                // Goalkeeper restriction: keep ball within penalty box
                if (p.role == Role::GK) {
                    // Box is approximately first/last 2 columns (16.5m box in 105m pitch)
                    float boxLimit = (dir_[side] > 0) ? 
                        (2.0f * kPitchLength / kCols) :  // ~16m for team attacking right
                        kPitchLength - (2.0f * kPitchLength / kCols);  // ~89m for team attacking left

                    if (dir_[side] > 0) {
                        ballPosition_.x = std::min(ballPosition_.x, boxLimit);
                    } else {
                        ballPosition_.x = std::max(ballPosition_.x, boxLimit);
                    }
                }

                ball_ = positionToCell(ballPosition_);

                p.pos = ball_;
                p.position = ballPosition_;
                aerial_ = false;
                logEvent(shirt(p) + " carries the ball forward");
            } else {
                logEvent(shirt(p) + " is tackled by " +
                         (bestDef ? shirt(*bestDef) : std::string("a defender")));
                turnover("loss");
            }
            break;
        }
        case Action::Dribble: {
            if (success) {
                // Dribble forward in continuous space
                int step = std::max(1, std::min(p.maxMovePerAction(), 2));
                float dribbleDist = step * (kPitchLength / kCols);  // Convert grid steps to meters

                ballPosition_.x += dir_[side] * dribbleDist;

                // Skilled dribblers can beat the ball sideways slightly
                double dribbling = p.norm("Dribbling");
                if (dribbling > 0.7 && rng_.chance(0.3)) {
                    float sideDrift = rng_.real(-0.5f, 0.5f) * (kPitchWidth / kRows);
                    ballPosition_.y += sideDrift;
                }

                ballPosition_ = clampPosition(ballPosition_);
                ball_ = positionToCell(ballPosition_);

                p.pos = ball_;
                p.position = ballPosition_;
                aerial_ = false;

                // Apply dispossession cooldown to beaten defender(s)
                if (bestDef) {
                    bestDef->dispossessionCooldown = rng_.range(2, 3);
                    logEvent(shirt(p) + " dribbles past " + shirt(*bestDef));
                } else {
                    logEvent(shirt(p) + " dribbles forward");
                }

                // Exceptional dribblers can occasionally beat multiple defenders
                if (dribbling > 0.8 && defCount > 1 && rng_.chance(0.25)) {
                    for (Player* d : sidePlayers_[defendingSide]) {
                        if (d != bestDef && CellDistance(d->pos, ball_) <= 1) {
                            d->dispossessionCooldown = rng_.range(1, 2);
                        }
                    }
                }
            } else {
                // A beaten defender concedes a foul some of the time, more often
                // the more aggressive they are.
                double aggr = bestDef ? bestDef->norm("Aggression") : 0.5;
                double foulChance = cfg_.get("foul.base", 0.05) + 
                                   cfg_.get("foul.aggression", 0.10) * aggr;

                if (rng_.chance(foulChance)) {
                    ++stats_.fouls[defendingSide];

                    // Award free kick to attacking side
                    Position2D foulPosition = ballPosition_;
                    logEvent(shirt(p) + " is fouled by " + 
                            (bestDef ? shirt(*bestDef) : std::string("the defender")), true);
                    freeKick(side, foulPosition);
                } else {
                    logEvent(shirt(p) + " tries to dribble past " +
                            (bestDef ? shirt(*bestDef) : std::string("the defender")) +
                            ", but loses possession");
                    turnover("turnover");
                }
            }
            break;
        }
        case Action::Longshot:
        case Action::Finish:
        case Action::Header: {
            ++shots_[side];
            ++stats_.shots[side];  // "Attempts" counter on the match screen
            onShot(chosen, finalScore, thr.second);
            break;
        }
    }
}

void MatchEngine::onShot(Action a, double finalScore, double thr) {
    int side = carrierSide_;
    int defendingSide = 1 - side;
    Player* shooter = carrier_;
    std::string kind = (a == Action::Longshot)
                           ? "lets fly from distance"
                           : (a == Action::Header ? "rises for a header" : "shoots");
    if (finalScore < thr) {
        // Shot off target or blocked
        // Determine if it's deflected for a corner or goes out for goal kick
        // ~40% chance of corner if blocked/deflected, otherwise goal kick
        bool isCorner = rng_.chance(0.40);

        if (isCorner) {
            logEvent(shirt(*shooter) + " " + kind + " but it's deflected for a corner", true);
            cornerKick(side);
        } else {
            logEvent(shirt(*shooter) + " " + kind + " but it is off target / blocked", true);
            goalKick(defendingSide);
        }
        return;
    }
    ++stats_.onTarget[side];
    // On target -> goalkeeper save model.
    Player* gk = goalkeeper(defendingSide);
    double gkNorm = gk ? gk->norm("Goalkeeping") : 0.2;
    double margin = finalScore - thr;
    double saveChance = cfg_.get("gk.save.base", 0.30) +
                        cfg_.get("gk.save.skill", 0.45) * gkNorm - 0.20 * margin;
    saveChance = std::max(0.05, std::min(0.95, saveChance));
    if (gk && rng_.chance(saveChance)) {
        // Saved shots result in corners (keeper parries/deflects the ball)
        logEvent(shirt(*shooter) + " " + kind + " - SAVED by " + shirt(*gk) + 
                 ", corner kick", true);
        cornerKick(side);
    } else {
        ++score_[side];
        logEvent("GOAL! " + shirt(*shooter) + " scores! (" +
                     std::to_string(score_[0]) + "-" + std::to_string(score_[1]) + ")",
                 true);
        // Reset to center and give ball to conceding side for kickoff
        kickoff(defendingSide);
        // Note: kickoff() resets ball to center, repositions all players to home positions,
        // and gives possession to the conceding team for restart
    }
}

// ---------------------------------------------------------------------------
// Off-ball movement (section 5): players drift between their formation anchor
// and the ball depending on attacking/defending.
// ---------------------------------------------------------------------------
void MatchEngine::moveOffBallPlayers() {
    for (int s = 0; s < 2; ++s) {
        bool attacking = (s == carrierSide_);
        // How far the ball has advanced into this team's attacking half,
        // expressed in this side's own frame so the shape slides the right way
        // (towards the opponent goal when attacking, back when defending).
        int progress = zoneProgress(ball_, s) - 7;  // -6..+6
        int ballProgress = zoneProgress(ball_, s);  // 1..13 absolute progress

        // The two closest defenders also actively close down the ball.
        std::vector<std::pair<int, Player*>> byDist;
        if (!attacking)
            for (Player* p : sidePlayers_[s])
                byDist.emplace_back(CellDistance(p->pos, ball_), p);
        std::sort(byDist.begin(), byDist.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        for (Player* p : sidePlayers_[s]) {
            if (p == carrier_) continue;
            if (p->role == Role::GK) {
                // Keeper stays in the penalty box near its own goal.
                // Box is approximately columns 1-2 (defending left) or 12-13 (defending right)
                int boxLimit = (dir_[s] > 0) ? 2 : 12;  // Max col GK can reach

                // Keep GK at home position, but adjust slightly based on ball position
                p->pos.row = clampRow(p->homePos.row);

                // If ball is very close, GK can move slightly toward it within box
                if (ballProgress >= 12 && CellDistance(ball_, p->homePos) <= 3) {
                    // Move toward ball but stay in box
                    int targetCol = (dir_[s] > 0) ? 
                        std::min(boxLimit, ball_.col) : 
                        std::max(boxLimit, ball_.col);
                    p->pos.col = targetCol;
                } else {
                    p->pos.col = clampCol(p->homePos.col);
                }

                p->position = cellToPosition(p->pos);
                continue;
            }

            // Team-shape target: anchor column slid by the ball advance, in the
            // team's attacking direction.
            // Possession-based movement: push forward when attacking, drop back when defending
            int targetCol, targetRow = p->homePos.row;

            if (attacking) {
                // ATTACKING: Push forward based on ball position
                int pushForward = 0;
                if (ballProgress >= 11) pushForward = 6;      // Ball very deep - aggressive
                else if (ballProgress >= 9) pushForward = 4;  // Ball in opp half - good push
                else if (ballProgress >= 7) pushForward = 2;  // Midfield - moderate
                else pushForward = 1;                          // Own half - minimal

                // Role adjustments
                if (p->role == Role::F && ballProgress >= 9) pushForward++;
                else if (p->role == Role::D) pushForward = std::min(pushForward, 3);

                targetCol = clampCol(p->homePos.col + dir_[s] * pushForward);
            } else {
                // DEFENDING: Drop back based on ball position
                int dropBack = 0;
                if (ballProgress >= 11) dropBack = -5;      // Ball very close to goal - deep block
                else if (ballProgress >= 9) dropBack = -3;  // Own third - solid drop
                else if (ballProgress >= 7) dropBack = -2;  // Midfield - moderate drop
                else dropBack = -1;                          // Opp half - stay compact

                // Role adjustments
                if (p->role == Role::F) dropBack = std::max(dropBack, -2);
                else if (p->role == Role::M && ballProgress >= 10) dropBack--;

                targetCol = clampCol(p->homePos.col + dir_[s] * dropBack);
            }

            // Determine player's lateral constraint based on their home position
            // This keeps wide players wide and central players central
            int minRow = p->homePos.row;
            int maxRow = p->homePos.row;

            // Calculate how far from center (row 4) this position is
            int lateralDistance = std::abs(p->homePos.row - 4);

            if (lateralDistance >= 3) {
                // Very wide positions (ML, MR, WBL, WBR, FL, FR, etc.)
                // Allow only 1 row of movement to maintain width
                minRow = std::max(0, p->homePos.row - 1);
                maxRow = std::min(8, p->homePos.row + 1);
            } else if (lateralDistance >= 2) {
                // Wide-ish positions (AML, AMR, DL, DR)
                // Allow 2 rows of movement
                minRow = std::max(0, p->homePos.row - 2);
                maxRow = std::min(8, p->homePos.row + 2);
            } else {
                // Central positions (GK, DC, DM, MC, AMC, FC)
                // Allow 3 rows of movement for more fluidity
                minRow = std::max(0, p->homePos.row - 3);
                maxRow = std::min(8, p->homePos.row + 3);
            }


            // The nearest two defenders press the ball directly (but only if close enough)
            bool presser = false;
            if (!attacking) {
                for (int i = 0; i < 2 && i < static_cast<int>(byDist.size()); ++i) {
                    if (byDist[i].second == p && byDist[i].first <= 4) {  // Only press if within 4 cells
                        presser = true;
                        break;
                    }
                }
            }
            if (presser) {
                // Move towards ball but don't commit fully - maintain some shape
                targetCol = clampCol(targetCol + (ball_.col - targetCol) / 2);
                int rowAdjust = (ball_.row - targetRow) / 2;
                targetRow = clampRow(targetRow + rowAdjust);
            }

            // Intelligent attacking movement during build-up play
            if (attacking && !presser) {
                int playerProgress = zoneProgress(p->homePos, s);

                // If ball is in defensive/midfield zone (progress < 10)
                if (ballProgress < 10) {
                    // Forwards and attacking mids ahead of the ball drop back to offer support
                    if ((p->role == Role::F || p->role == Role::AM) && playerProgress > ballProgress) {
                        // Drop back towards the ball, but stay ahead slightly
                        int dropBackTarget = ballProgress + 1;  // Less aggressive drop
                        targetCol = clampCol(p->homePos.col + dir_[s] * (dropBackTarget - 7));

                        // Wide players MUST stay in their lane
                        targetRow = p->homePos.row;
                    }
                    // Central midfielders also provide passing options
                    else if (p->role == Role::M || p->role == Role::DM) {
                        // Stay between ball and forwards to link play
                        if (playerProgress < ballProgress - 1) {
                            // Push up slightly if too far back
                            targetCol = clampCol(targetCol + dir_[s] * 1);
                        }
                        // Wide mids stay wide, central mids stay central
                        targetRow = p->homePos.row;
                    }
                }
                // Ball is in attacking zone (progress >= 10) - make forward runs
                else if (ballProgress >= 10) {
                    // Forwards make runs towards goal
                    if (p->role == Role::F) {
                        // Sprint towards goal if behind the ball or level
                        int runTarget = std::min(13, ballProgress + 1);  // Less aggressive
                        targetCol = clampCol(p->homePos.col + dir_[s] * (runTarget - 7));

                        // Maintain positional discipline - wide players stay wide
                        if (lateralDistance >= 2) {
                            targetRow = p->homePos.row;  // Strict width
                        } else {
                            // Central forwards can make limited diagonal runs
                            if (CellDistance(p->pos, ball_) > 4 && rng_.chance(0.4)) {
                                int rowDiff = p->pos.row - ball_.row;
                                if (std::abs(rowDiff) > 2) {
                                    targetRow = clampRow(p->pos.row + (rowDiff > 0 ? -1 : 1));
                                }
                            }
                        }
                    }
                    // Attacking mids make supporting runs
                    else if (p->role == Role::AM) {
                        // Push up behind the forwards
                        int supportTarget = std::min(11, ballProgress);  // Stay behind forwards
                        targetCol = clampCol(p->homePos.col + dir_[s] * (supportTarget - 7));

                        // Maintain width
                        targetRow = p->homePos.row;
                    }
                }
                // Transitional zone (midfield to attack)
                else {
                    // All players maintain their width - no drifting
                    targetRow = p->homePos.row;
                    // Slight push forward for wide players
                    if (lateralDistance >= 2) {
                        targetCol = clampCol(targetCol + dir_[s] * 1);
                    }
                }
            }

            // Intelligent roaming to find space when tightly marked (simplified)
            if (attacking) {
                int defendingSide = 1 - s;
                Player* closestMarker = nullptr;
                int closestDist = 100;

                for (Player* d : sidePlayers_[defendingSide]) {
                    int dist = CellDistance(p->pos, d->pos);
                    if (dist < closestDist) {
                        closestDist = dist;
                        closestMarker = d;
                    }
                }

                // Only roam if directly marked (same cell)
                if (closestMarker && closestDist == 0) {
                    // Make a small movement to create separation
                    // Wide players can only move forward/back
                    if (lateralDistance >= 3) {
                        targetCol = clampCol(targetCol + dir_[s] * 1);
                    }
                    // Central players can move slightly laterally
                    else if (lateralDistance <= 1) {
                        if (p->pos.row > minRow && p->pos.row < 4) {
                            targetRow = clampRow(targetRow - 1);
                        } else if (p->pos.row < maxRow && p->pos.row > 4) {
                            targetRow = clampRow(targetRow + 1);
                        }
                    }
                }
            }

            // Clamp target row to respect positional constraints
            targetRow = std::max(minRow, std::min(maxRow, targetRow));

            // Proactive collision avoidance: check if target is too close to other players
            // and adjust to maintain spacing before movement
            Position2D candidateTarget = cellToPosition(Cell{targetRow, targetCol});
            constexpr float avoidanceRadius = 3.5f;  // Same as collision resolution

            for (int otherSide = 0; otherSide < 2; ++otherSide) {
                for (Player* other : sidePlayers_[otherSide]) {
                    if (other == p) continue;

                    float distToOther = candidateTarget.distanceTo(other->position);
                    if (distToOther < avoidanceRadius) {
                        // Target is too close to another player - adjust laterally
                        // Try to move away from the crowded area
                        float dx = candidateTarget.x - other->position.x;
                        float dy = candidateTarget.y - other->position.y;

                        if (std::abs(dx) > 0.01f || std::abs(dy) > 0.01f) {
                            float dist = std::sqrt(dx * dx + dy * dy);
                            float adjustX = (dx / dist) * avoidanceRadius;
                            float adjustY = (dy / dist) * avoidanceRadius;

                            candidateTarget.x = other->position.x + adjustX;
                            candidateTarget.y = other->position.y + adjustY;
                            candidateTarget = clampPosition(candidateTarget);
                        }
                    }
                }
            }

            // Calculate target position in continuous space
            Position2D targetPosition = candidateTarget;

            // Add slight positional variance to prevent exact overlap at formation spots
            // Players within same cell should occupy slightly different positions
            int nearbyCount = 0;
            for (int otherSide = 0; otherSide < 2; ++otherSide) {
                for (Player* other : sidePlayers_[otherSide]) {
                    if (other == p) continue;
                    if (other->pos == p->pos) {
                        nearbyCount++;
                        // Add small offset based on player number to create consistent spacing
                        float offsetAngle = (p->shirtNumber * 2.0f + nearbyCount * 1.5f) * 0.5f;
                        targetPosition.x += std::cos(offsetAngle) * 1.5f;
                        targetPosition.y += std::sin(offsetAngle) * 1.5f;
                        targetPosition = clampPosition(targetPosition);
                    }
                }
            }

            // Smooth continuous movement towards target
            float maxSpeed = p->maxMovePerAction() * (kPitchLength / kCols);  // Convert to meters
            maxSpeed *= 0.5f;  // Scale for smoother movement per action round

            float dx = targetPosition.x - p->position.x;
            float dy = targetPosition.y - p->position.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance > 0.1f) {  // Only move if not already at target
                float moveDistance = std::min(maxSpeed, distance);
                float ratio = moveDistance / distance;

                p->position.x += dx * ratio;
                p->position.y += dy * ratio;
                p->position = clampPosition(p->position);
            }

            // Update grid position for action logic
            p->pos = positionToCell(p->position);
        }
    }

    // Collision resolution: ensure no two players occupy the same cell
    resolveCollisions();
}

// ---------------------------------------------------------------------------
// Collision resolution: ensure no two players are too close
// ---------------------------------------------------------------------------
void MatchEngine::resolveCollisions() {
    constexpr float minDistance = 3.5f;  // Minimum distance between players (meters)

    for (int s1 = 0; s1 < 2; ++s1) {
        for (size_t i = 0; i < sidePlayers_[s1].size(); ++i) {
            Player* p1 = sidePlayers_[s1][i];

            // Check against all other players
            for (int s2 = 0; s2 < 2; ++s2) {
                size_t startJ = (s1 == s2) ? i + 1 : 0;
                for (size_t j = startJ; j < sidePlayers_[s2].size(); ++j) {
                    Player* p2 = sidePlayers_[s2][j];

                    float dist = p1->position.distanceTo(p2->position);
                    if (dist < minDistance && dist > 0.01f) {
                        // Players too close - push them apart
                        // Ball carrier doesn't move
                        Player* toMove = p1->hasBall ? p2 : (p2->hasBall ? p1 : p1);
                        Player* fixed = (toMove == p1) ? p2 : p1;

                        // Calculate push direction
                        float dx = toMove->position.x - fixed->position.x;
                        float dy = toMove->position.y - fixed->position.y;

                        // Normalize and push
                        float pushDist = minDistance - dist;
                        if (dist > 0.01f) {
                            dx /= dist;
                            dy /= dist;
                        } else {
                            // If exactly on top, push in random direction
                            float angle = rng_.real(0.0, 6.28318f);
                            dx = std::cos(angle);
                            dy = std::sin(angle);
                        }

                        toMove->position.x += dx * pushDist;
                        toMove->position.y += dy * pushDist;
                        toMove->position = clampPosition(toMove->position);
                        toMove->pos = positionToCell(toMove->position);
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
int MatchEngine::opponentsNear(const Cell& at, int defendingSide, int radius) const {
    int n = 0;
    for (Player* d : sidePlayers_[defendingSide]) {
        // Skip players on dispossession cooldown
        if (d->dispossessionCooldown > 0) continue;
        if (CellDistance(d->pos, at) <= radius) ++n;
    }
    return n;
}

Player* MatchEngine::nearestOpponent(const Cell& at, int defendingSide) const {
    Player* best = nullptr;
    int bestDist = 1 << 30;
    for (Player* d : sidePlayers_[defendingSide]) {
        int dist = CellDistance(d->pos, at);
        if (dist < bestDist) {
            bestDist = dist;
            best = d;
        }
    }
    return best;
}

Player* MatchEngine::choosePassTarget(bool longPass) {
    int side = carrierSide_;
    int defendingSide = 1 - side;
    std::vector<Player*> options;
    std::vector<double> weights;
    double total = 0.0;

    int ballProgress = zoneProgress(ball_, side);

    for (Player* t : sidePlayers_[side]) {
        if (t == carrier_) continue;

        int dist = CellDistance(t->pos, ball_);

        // Distance filtering
        if (longPass) {
            if (dist < 4 || dist > 10) continue;  // Long passes: 4-10 cells
        } else {
            if (dist < 1 || dist > 5) continue;   // Short passes: 1-5 cells (tighter)
        }

        // Calculate various factors
        int forward = (zoneProgress(t->pos, side) - ballProgress);
        int markers = opponentsNear(t->pos, defendingSide, 1);
        int lateralDiff = std::abs(t->pos.row - ball_.row);

        // Check for defenders blocking the passing lane
        int defendersInLane = 0;
        for (Player* d : sidePlayers_[defendingSide]) {
            if (d->dispossessionCooldown > 0) continue;

            // Simple lane check: is defender between passer and receiver?
            int minCol = std::min(ball_.col, t->pos.col);
            int maxCol = std::max(ball_.col, t->pos.col);
            int minRow = std::min(ball_.row, t->pos.row);
            int maxRow = std::max(ball_.row, t->pos.row);

            if (d->pos.col >= minCol && d->pos.col <= maxCol &&
                d->pos.row >= minRow && d->pos.row <= maxRow) {
                defendersInLane++;
            }
        }

        // Base weight
        double w = 1.0;

        // Passing lane penalty (defenders blocking the path)
        if (defendersInLane > 0) {
            w *= std::pow(0.6, defendersInLane);  // Each blocker reduces by 40%
        }

        // Forward progress bonus (prefer progressive passes)
        if (forward > 0) {
            w += forward * 0.5;  // Reward forward movement
        } else if (forward < -2) {
            w *= 0.4;  // Penalize backward passes heavily (but allow safety)
        }

        // Pressure penalty (avoid marked targets)
        if (markers > 0) {
            w *= (1.0 - markers * 0.35);  // Each marker reduces appeal by 35%
        }

        // Positional preferences based on zone
        if (ballProgress <= 5) {
            // Defensive zone: prefer safe passes, avoid risky forward balls
            if (forward > 3) w *= 0.6;  // Long forward passes are risky from own half
            if (markers == 0) w *= 1.4;  // Strongly prefer unmarked players
        } else if (ballProgress <= 8) {
            // Midfield: balanced approach
            if (lateralDiff > 3) w *= 1.2;  // Slight bonus for wide switches
        } else {
            // Attacking zone: prefer penetrating passes
            if (forward > 0) w *= 1.5;  // Strong bonus for through-balls
            if (t->role == Role::F || t->role == Role::AM) {
                w *= 1.3;  // Prefer forwards and attacking mids in final third
            }
        }

        // Distance penalty (prefer closer passes for control)
        if (!longPass) {
            if (dist <= 2) w *= 1.3;  // Bonus for very close, safe passes
            else if (dist >= 4) w *= 0.8;  // Penalty for longer passes
        }

        // Wide player bonus in attacking situations
        if (ballProgress >= 7) {
            int lateralPos = std::abs(t->pos.row - 4);
            if (lateralPos >= 3) {  // Wide player
                w *= 1.2;  // Use width in attack
            }
        }

        // Ensure minimum weight
        w = std::max(0.05, w);

        options.push_back(t);
        weights.push_back(w);
        total += w;
    }

    if (options.empty()) return nullptr;

    // Weighted random selection
    double pick = rng_.real(0.0, total);
    double acc = 0.0;
    for (size_t i = 0; i < options.size(); ++i) {
        acc += weights[i];
        if (pick <= acc) return options[i];
    }
    return options.back();
}

Player* MatchEngine::goalkeeper(int side) const {
    for (Player* p : sidePlayers_[side])
        if (p->role == Role::GK) return p;
    return sidePlayers_[side].empty() ? nullptr : sidePlayers_[side].front();
}

void MatchEngine::giveBall(Player* p, int side) {
    if (carrier_) carrier_->hasBall = false;
    carrier_ = p;
    carrierSide_ = side;
    p->hasBall = true;
    ball_ = p->pos;
    ballPosition_ = p->position;
}

void MatchEngine::turnover(const std::string& reason) {
    (void)reason;  // reason is reflected in the preceding narrated event
    int oldSide = carrierSide_;
    int newSide = 1 - carrierSide_;
    Player* loser = carrier_;
    Player* winner = nearestOpponent(ball_, newSide);
    aerial_ = false;

    if (winner) {
        // Check if winner commits a foul during the challenge
        // More likely in dangerous areas (attacking third) and with aggressive players
        double winnerAggr = winner->norm("Aggression");
        int progress = zoneProgress(ball_, oldSide);  // How far attacking team had advanced
        bool dangerousArea = progress >= 10;  // In attacking third

        double foulChance = 0.03;  // Base 3% chance during challenges
        if (dangerousArea) foulChance += 0.04;  // +4% in dangerous area
        foulChance += winnerAggr * 0.06;  // Up to +6% for aggressive defenders

        if (rng_.chance(foulChance)) {
            ++stats_.fouls[newSide];
            Position2D foulPosition = ballPosition_;
            logEvent(shirt(*loser) + " is fouled by " + shirt(*winner), true);
            freeKick(oldSide, foulPosition);
            return;  // Free kick awarded - no turnover
        }

        // Normal turnover
        winner->pos = ball_;
        winner->position = ballPosition_;
        giveBall(winner, newSide);
        // The dispossessed player has lost momentum: nudge them back towards
        // their own goal so the new carrier isn't instantly re-challenged at
        // point-blank range (which would cause endless ping-pong).
        if (loser) {
            loser->pos.col = clampCol(loser->pos.col - dir_[oldSide] * 2);
            loser->position = cellToPosition(loser->pos);
            loser->hasBall = false;
            // Set cooldown: player cannot challenge for possession for 2-3 rounds
            loser->dispossessionCooldown = rng_.range(2, 3);
        }
    }
}

void MatchEngine::goalKick(int side) {
    // A goal kick / clearance launches the ball out to the midfield band so the
    // ball actually transitions away from the box (instead of being won back by
    // a camped attacker for another shot).
    aerial_ = true;
    Cell mid{4, 7};  // neutral zone, centre
    // Nudge it slightly towards the kicking team's own half so their player
    // collects it.
    mid.col = clampCol(7 - dir_[side] * 1);
    ballPosition_ = cellToPosition(mid);
    ball_ = mid;
    Player* receiver = nearestOpponent(mid, side);  // nearest of 'side'
    if (receiver) {
        receiver->pos = mid;
        receiver->position = ballPosition_;
        giveBall(receiver, side);
    } else if (Player* gk = goalkeeper(side)) {
        giveBall(gk, side);
    }
}

void MatchEngine::cornerKick(int attackingSide) {
    // Corner kick: realistic set piece with proper positioning
    ++stats_.corners[attackingSide];

    int defendingSide = 1 - attackingSide;

    logEvent("Corner kick for " + (attackingSide == 0 ? res_->homeName : res_->awayName));

    // Determine which corner based on which side the ball went out
    // For now, randomly choose top or bottom corner at attacking end
    bool topCorner = rng_.chance(0.5);
    int cornerRow = topCorner ? 0 : 8;  // Row 0 (top) or 8 (bottom)
    int cornerCol = (dir_[attackingSide] > 0) ? 13 : 1;  // Attacking end

    Cell cornerFlag{cornerRow, cornerCol};
    Position2D cornerPosition = cellToPosition(cornerFlag);

    // Adjust corner position to be at the actual corner flag (edge of pitch)
    if (dir_[attackingSide] > 0) {
        cornerPosition.x = kPitchLength - 1.0f;  // Right corner
    } else {
        cornerPosition.x = 1.0f;  // Left corner
    }
    cornerPosition.y = topCorner ? 1.0f : kPitchWidth - 1.0f;

    // --- STEP 1: Find corner taker (good crossing/passing ability) ---
    Player* cornerTaker = nullptr;
    double bestSkill = 0.0;
    for (Player* p : sidePlayers_[attackingSide]) {
        // Prefer wide players or players with good crossing ability
        double skill = p->norm("Passing") * 0.5 + p->norm("Crossing") * 0.3 + 
                      p->norm("Technique") * 0.2;
        if (skill > bestSkill) {
            bestSkill = skill;
            cornerTaker = p;
        }
    }

    if (!cornerTaker) {
        cornerTaker = sidePlayers_[attackingSide].front();
    }

    // Position corner taker at corner flag
    cornerTaker->position = cornerPosition;
    cornerTaker->pos = positionToCell(cornerPosition);
    ballPosition_ = cornerPosition;
    ball_ = cornerTaker->pos;
    giveBall(cornerTaker, attackingSide);

    logEvent(shirt(*cornerTaker) + " prepares to take the corner");

    // --- STEP 2: Position players in and around the box (fluid formation) ---

    // Define the penalty box area
    int boxColDeep = (dir_[attackingSide] > 0) ? 12 : 2;    // Deep in box (6-yard)
    int boxColCenter = (dir_[attackingSide] > 0) ? 11 : 3;  // Penalty spot area
    int boxColEdge = (dir_[attackingSide] > 0) ? 10 : 4;    // Edge of box
    int boxColOutside = (dir_[attackingSide] > 0) ? 9 : 5;  // Just outside box
    int midfield = 7;  // Halfway line

    // Position attacking players (fluid, varied positioning)
    std::vector<Player*> attackers;
    for (Player* p : sidePlayers_[attackingSide]) {
        if (p != cornerTaker) {
            attackers.push_back(p);
        }
    }

    // Position defenders
    std::vector<Player*> defenders = sidePlayers_[defendingSide];
    Player* gk = goalkeeper(defendingSide);

    // ATTACKING TEAM POSITIONING (total 10 outfield):
    // - 5 in the box (varied positions)
    // - 1 short option near corner
    // - 2 just outside box for clearances/rebounds
    // - 2 center backs staying deep to defend counter-attacks

    int attackerIdx = 0;
    for (Player* p : attackers) {
        // Identify player type by role for smart positioning
        bool isDefender = (p->role == Role::D || p->role == Role::DM);
        bool isTallStriker = (p->role == Role::F && p->norm("Heading") > 0.65);

        if (attackerIdx == 0) {
            // Player 1: Short option near corner (for short corner variation)
            float shortDist = topCorner ? 12.0f : 10.0f;  // 10-12m from corner
            p->position.x = cornerPosition.x + (dir_[attackingSide] > 0 ? -shortDist : shortDist);
            p->position.y = cornerPosition.y + (topCorner ? shortDist : -shortDist);
            p->position = clampPosition(p->position);

        } else if (attackerIdx >= 1 && attackerIdx <= 5) {
            // Players 2-6: IN THE BOX (5 players)
            // Vary positions - near post, far post, penalty spot, 6-yard line

            if (attackerIdx == 1) {
                // Near post runner (where corner is coming from)
                p->pos.col = boxColDeep;
                p->pos.row = topCorner ? 2 : 6;
                p->position = cellToPosition(p->pos);
                p->position.y += rng_.real(-2.0f, 2.0f);  // Add variation

            } else if (attackerIdx == 2 && isTallStriker) {
                // Big striker at penalty spot
                p->pos.col = boxColCenter;
                p->pos.row = 4;
                p->position = cellToPosition(p->pos);
                p->position.x += rng_.real(-1.5f, 1.5f);
                p->position.y += rng_.real(-1.5f, 1.5f);

            } else if (attackerIdx == 3) {
                // Far post attacker
                p->pos.col = boxColDeep;
                p->pos.row = topCorner ? 6 : 2;
                p->position = cellToPosition(p->pos);
                p->position.y += rng_.real(-2.0f, 2.0f);

            } else if (attackerIdx == 4) {
                // Central box runner
                p->pos.col = boxColCenter;
                p->pos.row = 3 + rng_.range(0, 2);
                p->position = cellToPosition(p->pos);
                p->position.x += rng_.real(-2.0f, 2.0f);
                p->position.y += rng_.real(-2.0f, 2.0f);

            } else {  // attackerIdx == 5
                // Lurking at edge of 6-yard box
                p->pos.col = boxColDeep;
                p->pos.row = 4;
                p->position = cellToPosition(p->pos);
                p->position.y += rng_.real(-3.0f, 3.0f);
            }

        } else if (attackerIdx >= 6 && attackerIdx <= 7) {
            // Players 7-8: Just OUTSIDE the box for rebounds/clearances
            p->pos.col = boxColEdge;
            p->pos.row = (attackerIdx == 6) ? 3 : 5;
            p->position = cellToPosition(p->pos);
            // Add some variation
            p->position.x += rng_.real(-2.0f, 2.0f);
            p->position.y += rng_.real(-3.0f, 3.0f);

        } else {
            // Players 9-10: Center backs staying DEEP (ready for counter-attack)
            p->pos.col = midfield - dir_[attackingSide] * 2;  // On own half
            p->pos.row = (attackerIdx == 8) ? 3 : 5;
            p->position = cellToPosition(p->pos);
            p->position.x += rng_.real(-3.0f, 3.0f);
        }

        p->position = clampPosition(p->position);
        p->pos = positionToCell(p->position);
        attackerIdx++;
    }

    // DEFENDING TEAM POSITIONING:
    // - Goalkeeper on line
    // - 6-7 defenders in box marking zones/players
    // - 2-3 at edge of box
    // - 1 midfielder deep for clearances

    int defenderIdx = 0;
    for (Player* d : defenders) {
        if (d == gk) {
            // Goalkeeper on goal line, slightly off-center
            d->pos.col = (dir_[attackingSide] > 0) ? 13 : 1;
            d->pos.row = 4;
            d->position = cellToPosition(d->pos);
            d->position.y += rng_.real(-1.0f, 1.0f);  // Slight positioning variance

        } else if (defenderIdx < 3) {
            // Defenders 1-3: Marking in 6-yard box (near/far post, central)
            if (defenderIdx == 0) {
                // Near post marker
                d->pos.col = boxColDeep;
                d->pos.row = topCorner ? 2 : 6;
            } else if (defenderIdx == 1) {
                // Central 6-yard box
                d->pos.col = boxColDeep;
                d->pos.row = 4;
            } else {
                // Far post marker
                d->pos.col = boxColDeep;
                d->pos.row = topCorner ? 6 : 2;
            }
            d->position = cellToPosition(d->pos);
            d->position.x += rng_.real(-1.0f, 1.0f);
            d->position.y += rng_.real(-2.0f, 2.0f);

        } else if (defenderIdx < 7) {
            // Defenders 4-7: Zonal marking in penalty area
            d->pos.col = boxColCenter;
            d->pos.row = 2 + (defenderIdx - 3);
            d->position = cellToPosition(d->pos);
            d->position.x += rng_.real(-2.0f, 2.0f);
            d->position.y += rng_.real(-2.5f, 2.5f);

        } else if (defenderIdx < 9) {
            // Defenders 8-9: Edge of box (for clearances)
            d->pos.col = boxColEdge;
            d->pos.row = (defenderIdx == 7) ? 3 : 5;
            d->position = cellToPosition(d->pos);
            d->position.x += rng_.real(-2.0f, 2.0f);
            d->position.y += rng_.real(-3.0f, 3.0f);

        } else {
            // Defender 10: Deep midfielder position (counter-attack outlet)
            d->pos.col = boxColOutside;
            d->pos.row = 4;
            d->position = cellToPosition(d->pos);
            d->position.x += rng_.real(-4.0f, 4.0f);
        }

        d->position = clampPosition(d->position);
        d->pos = positionToCell(d->position);
        defenderIdx++;
    }

    // --- STEP 3: Deliver the corner (cross into box) ---

    aerial_ = true;

    // Determine crossing quality
    double crossQuality = cornerTaker->norm("Passing") * 0.4 + 
                         cornerTaker->norm("Crossing") * 0.4 + 
                         cornerTaker->norm("Technique") * 0.2;

    // Choose target area in box (near post, far post, or penalty spot)
    double targetChoice = rng_.real(0.0, 1.0);
    int targetRow;
    int targetCol = boxColCenter;

    if (targetChoice < 0.35) {
        // Near post
        targetRow = topCorner ? 2 : 6;
        logEvent(shirt(*cornerTaker) + " swings the corner towards the near post");
    } else if (targetChoice < 0.70) {
        // Penalty spot / central
        targetRow = 4;
        logEvent(shirt(*cornerTaker) + " delivers the corner to the penalty spot");
    } else {
        // Far post
        targetRow = topCorner ? 6 : 2;
        logEvent(shirt(*cornerTaker) + " aims for the far post");
    }

    Cell targetArea{targetRow, targetCol};

    // Check crossing quality - poor crosses may be cleared
    bool goodCross = rng_.chance(crossQuality * 0.8 + 0.2);  // 20-100% based on skill

    if (!goodCross) {
        // Poor delivery - cleared by defense
        logEvent("Poor corner delivery - cleared by the defense");

        // Ball cleared to midfield
        Cell clearTarget{4, 7};
        ballPosition_ = cellToPosition(clearTarget);
        ball_ = clearTarget;
        aerial_ = false;

        Player* clearer = nearestOpponent(clearTarget, defendingSide);
        if (clearer) {
            clearer->pos = clearTarget;
            clearer->position = ballPosition_;
            giveBall(clearer, defendingSide);
        }
        return;
    }

    // Good cross - find players near target area
    std::vector<Player*> nearbyAttackers;
    std::vector<Player*> nearbyDefenders;

    for (Player* p : attackers) {
        if (CellDistance(p->pos, targetArea) <= 2) {
            nearbyAttackers.push_back(p);
        }
    }

    for (Player* d : defenders) {
        if (CellDistance(d->pos, targetArea) <= 2) {
            nearbyDefenders.push_back(d);
        }
    }

    if (nearbyAttackers.empty()) {
        // No attackers near - defenders clear easily
        logEvent("Corner cleared by the defense");

        if (rng_.chance(0.40)) {
            // Cleared out for another corner
            logEvent("Ball deflected out - another corner!");
            cornerKick(attackingSide);
        } else {
            // Cleared away
            Cell clearTarget{4, 7};
            ballPosition_ = cellToPosition(clearTarget);
            ball_ = clearTarget;
            aerial_ = false;

            Player* clearer = nearestOpponent(clearTarget, defendingSide);
            if (clearer) {
                clearer->pos = clearTarget;
                clearer->position = ballPosition_;
                giveBall(clearer, defendingSide);
            }
        }
        return;
    }

    // --- STEP 4: Contested aerial duel ---

    ballPosition_ = cellToPosition(targetArea);
    ball_ = targetArea;

    // Select the best attacker and best defender in area
    Player* bestAttacker = nearbyAttackers[0];
    double bestAttackScore = 0.0;
    for (Player* a : nearbyAttackers) {
        double score = a->norm("Heading") * 0.6 + a->norm("Jumping") * 0.3 + 
                      a->norm("Positioning") * 0.1;
        if (score > bestAttackScore) {
            bestAttackScore = score;
            bestAttacker = a;
        }
    }

    Player* bestDefender = nearbyDefenders.empty() ? nullptr : nearbyDefenders[0];
    double bestDefendScore = 0.0;
    if (bestDefender) {
        for (Player* d : nearbyDefenders) {
            double score = d->norm("Heading") * 0.5 + d->norm("Jumping") * 0.3 + 
                          d->norm("Marking") * 0.2;
            if (score > bestDefendScore) {
                bestDefendScore = score;
                bestDefender = d;
            }
        }
    }

    logEvent(shirt(*bestAttacker) + " rises for the header!", true);

    // Aerial contest
    double attackerQuality = bestAttacker->norm("Heading") * 0.7 + 
                            bestAttacker->norm("Jumping") * 0.3;
    double defenderQuality = bestDefender ? 
                            (bestDefender->norm("Heading") * 0.6 + 
                             bestDefender->norm("Marking") * 0.4) : 0.3;

    // Include goalkeeper if close
    if (gk && CellDistance(gk->pos, targetArea) <= 2) {
        double gkClaim = gk->norm("Goalkeeping") * 0.5 + gk->norm("Positioning") * 0.3;
        if (gkClaim > defenderQuality) {
            defenderQuality = gkClaim;
            if (rng_.chance(0.40)) {
                logEvent(shirt(*gk) + " comes out to punch clear!");

                // Goalkeeper punches clear
                Cell clearTarget{4, 6 + rng_.range(-1, 1)};
                ballPosition_ = cellToPosition(clearTarget);
                ball_ = clearTarget;
                aerial_ = false;

                Player* receiver = nearestOpponent(clearTarget, defendingSide);
                if (receiver) {
                    receiver->pos = clearTarget;
                    receiver->position = ballPosition_;
                    giveBall(receiver, defendingSide);
                }
                return;
            }
        }
    }

    double contestResult = attackerQuality - defenderQuality + rng_.real(-0.25, 0.25);

    if (contestResult > 0.05) {
        // Attacker wins the header - attempt on goal
        bestAttacker->pos = targetArea;
        bestAttacker->position = ballPosition_;
        giveBall(bestAttacker, attackingSide);

        logEvent(shirt(*bestAttacker) + " heads towards goal!", true);

        // Header attempt on goal
        double headerScore = attackerQuality + rng_.real(-0.2, 0.2);
        Action headerAction = Action::Header;
        ++shots_[attackingSide];
        ++stats_.shots[attackingSide];
        onShot(headerAction, headerScore, 0.45);
    } else {
        // Defender wins - clear the ball
        if (bestDefender) {
            logEvent(shirt(*bestDefender) + " heads clear!");
        } else {
            logEvent("Headed clear by the defense");
        }

        // Ball cleared - could go out for another corner or to midfield
        if (rng_.chance(0.25)) {
            logEvent("Cleared out for another corner");
            cornerKick(attackingSide);
        } else {
            Cell clearTarget{4, 6 + rng_.range(-2, 2)};
            ballPosition_ = cellToPosition(clearTarget);
            ball_ = clearTarget;
            aerial_ = false;

            Player* clearer = nearestOpponent(clearTarget, defendingSide);
            if (clearer) {
                clearer->pos = clearTarget;
                clearer->position = ballPosition_;
                giveBall(clearer, defendingSide);
            }
        }
    }
}

void MatchEngine::throwIn(int side, const Position2D& outPos) {
    // Throw-in: ball went out over the sideline
    ++stats_.throwIns[side];
    aerial_ = false;

    // Clamp to sideline - keep X position, set Y to boundary
    Position2D throwPos = outPos;
    if (outPos.y < 0.0f) {
        throwPos.y = 2.0f;  // Just inside top sideline
    } else {
        throwPos.y = kPitchWidth - 2.0f;  // Just inside bottom sideline
    }
    throwPos.x = std::max(5.0f, std::min(kPitchLength - 5.0f, outPos.x));

    ballPosition_ = throwPos;
    ball_ = positionToCell(ballPosition_);

    logEvent("Throw-in for " + (side == 0 ? res_->homeName : res_->awayName));

    // Find nearest player from throwing team to take the throw
    Player* thrower = nearestOpponent(ball_, side);
    if (thrower) {
        thrower->pos = ball_;
        thrower->position = ballPosition_;
        giveBall(thrower, side);
    }
}

void MatchEngine::freeKick(int attackingSide, const Position2D& foulPos) {
    // Free kick awarded to attacking side at foul position
    ++stats_.freeKicks[attackingSide];

    // Position ball at foul location (slightly adjusted to be in bounds)
    ballPosition_ = clampPosition(foulPos);
    ball_ = positionToCell(ballPosition_);

    int defendingSide = 1 - attackingSide;

    // Calculate distance to goal and position
    float goalX = (dir_[attackingSide] > 0) ? kPitchLength : 0.0f;
    float distanceToGoal = std::abs(ballPosition_.x - goalX);
    int progress = zoneProgress(ball_, attackingSide);
    bool inShootingRange = distanceToGoal < 30.0f && progress >= 10;  // Within ~30m and in attacking zone

    // Central position check (good for shooting)
    float centerY = kPitchWidth * 0.5f;
    float lateralDist = std::abs(ballPosition_.y - centerY);
    bool centralPosition = lateralDist < 15.0f;  // Within 15m of center

    logEvent("Free kick for " + (attackingSide == 0 ? res_->homeName : res_->awayName));

    // Find the best free kick taker (highest combination of passing, shooting, technique)
    Player* taker = nullptr;
    double bestScore = 0.0;
    for (Player* p : sidePlayers_[attackingSide]) {
        double score = p->norm("Passing") * 0.3 + 
                      p->norm("Shooting") * 0.3 + 
                      p->norm("Technique") * 0.3 +
                      p->norm("Freekicks") * 0.1;
        if (score > bestScore) {
            bestScore = score;
            taker = p;
        }
    }

    if (!taker) {
        // Fallback - just give to nearest player
        taker = nearestOpponent(ball_, attackingSide);
    }

    if (!taker) return;

    // Decide on free kick type based on position and player attributes
    double shootingSkill = taker->norm("Shooting");
    double freeKickSkill = taker->norm("Freekicks");
    double passingSkill = taker->norm("Passing");

    // Weighted decision based on position and skills
    double shootChance = 0.0;
    double crossChance = 0.0;
    double passChance = 0.0;

    if (inShootingRange && centralPosition) {
        // Good shooting position
        shootChance = 0.40 * (1.0 + shootingSkill + freeKickSkill);
        crossChance = 0.30 * (1.0 + passingSkill);
        passChance = 0.30 * (1.0 + passingSkill);
    } else if (inShootingRange) {
        // Wide free kick in attacking third
        shootChance = 0.15 * (1.0 + shootingSkill);
        crossChance = 0.50 * (1.0 + passingSkill);
        passChance = 0.35 * (1.0 + passingSkill);
    } else {
        // Deep position - passing or crossing
        shootChance = 0.05;
        crossChance = 0.45 * (1.0 + passingSkill);
        passChance = 0.50 * (1.0 + passingSkill);
    }

    double total = shootChance + crossChance + passChance;
    double pick = rng_.real(0.0, total);

    // Position taker at ball
    taker->pos = ball_;
    taker->position = ballPosition_;
    giveBall(taker, attackingSide);

    aerial_ = true;  // Free kicks are typically lofted

    if (pick < shootChance) {
        // OPTION 1: Direct shot on goal
        logEvent(shirt(*taker) + " lines up a direct free kick shot", true);

        // Calculate shot quality
        double shotQuality = (shootingSkill * 0.4 + freeKickSkill * 0.4 + 
                             taker->norm("Technique") * 0.2);

        // Distance penalty
        double distanceFactor = 1.0 - (distanceToGoal / 40.0);  // Decreases with distance
        distanceFactor = std::max(0.3, distanceFactor);

        // Angle penalty
        double angleFactor = 1.0 - (lateralDist / 30.0);
        angleFactor = std::max(0.4, angleFactor);

        double finalScore = shotQuality * distanceFactor * angleFactor + rng_.real(-0.15, 0.15);

        // Thresholds
        double onTargetThreshold = 0.45;
        double scoreThreshold = 0.65;

        if (finalScore < onTargetThreshold) {
            // Off target
            bool isCorner = rng_.chance(0.30);
            if (isCorner) {
                logEvent(shirt(*taker) + "'s free kick is deflected for a corner", true);
                cornerKick(attackingSide);
            } else {
                logEvent(shirt(*taker) + "'s free kick goes wide", true);
                goalKick(defendingSide);
            }
        } else {
            // On target
            ++stats_.onTarget[attackingSide];
            ++shots_[attackingSide];
            ++stats_.shots[attackingSide];

            Player* gk = goalkeeper(defendingSide);
            double gkNorm = gk ? gk->norm("Goalkeeping") : 0.2;
            double saveChance = 0.55 + 0.35 * gkNorm - 0.25 * (finalScore - onTargetThreshold);
            saveChance = std::max(0.15, std::min(0.85, saveChance));

            if (gk && rng_.chance(saveChance)) {
                logEvent(shirt(*taker) + "'s free kick saved by " + shirt(*gk), true);
                // Saved free kick -> corner or collected
                if (rng_.chance(0.60)) {
                    cornerKick(attackingSide);
                } else {
                    logEvent(shirt(*gk) + " collects the ball");
                    giveBall(gk, defendingSide);
                }
            } else {
                // GOAL from free kick!
                ++score_[attackingSide];
                logEvent("GOAL! " + shirt(*taker) + " scores from the free kick! (" +
                        std::to_string(score_[0]) + "-" + std::to_string(score_[1]) + ")", true);
                kickoff(defendingSide);
            }
        }
    } else if (pick < shootChance + crossChance) {
        // OPTION 2: Cross into the box
        logEvent(shirt(*taker) + " floats the free kick into the box");

        // Target dangerous area in box
        int targetCol = (dir_[attackingSide] > 0) ? 11 : 3;
        int targetRow = 4 + rng_.range(-2, 2);
        Cell target{targetRow, targetCol};

        // Check for receivers and defenders
        std::vector<Player*> attackers;
        std::vector<Player*> defenders;

        for (Player* p : sidePlayers_[attackingSide]) {
            if (CellDistance(p->pos, target) <= 3) {
                attackers.push_back(p);
            }
        }
        for (Player* p : sidePlayers_[defendingSide]) {
            if (CellDistance(p->pos, target) <= 3) {
                defenders.push_back(p);
            }
        }

        // Quality of delivery
        double crossQuality = passingSkill * 0.7 + freeKickSkill * 0.3;
        bool goodDelivery = rng_.chance(crossQuality);

        if (!goodDelivery || attackers.empty()) {
            // Poor delivery or no attackers
            if (rng_.chance(0.5)) {
                logEvent("Free kick delivery is poor - cleared by defense");
                goalKick(defendingSide);
            } else {
                logEvent("Free kick is cleared by the defense");
                // Ball cleared to midfield
                Cell clearTarget{4, 7};
                ballPosition_ = cellToPosition(clearTarget);
                ball_ = clearTarget;
                Player* clearer = nearestOpponent(ball_, defendingSide);
                if (clearer) {
                    clearer->pos = ball_;
                    clearer->position = ballPosition_;
                    giveBall(clearer, defendingSide);
                }
            }
        } else {
            // Good delivery - contested header
            ballPosition_ = cellToPosition(target);
            ball_ = target;

            Player* attacker = attackers[rng_.range(0, static_cast<int>(attackers.size()) - 1)];
            double attackerHeading = attacker->norm("Heading");

            double defenseQuality = 0.5;
            if (!defenders.empty()) {
                Player* defender = defenders[rng_.range(0, static_cast<int>(defenders.size()) - 1)];
                defenseQuality = defender->norm("Heading") * 0.7 + defender->norm("Jumping") * 0.3;
            }

            double contestResult = attackerHeading - defenseQuality + rng_.real(-0.2, 0.2);

            if (contestResult > 0.15) {
                // Attacker wins header - attempt on goal
                logEvent(shirt(*attacker) + " rises to meet the free kick!", true);
                attacker->pos = target;
                attacker->position = ballPosition_;
                giveBall(attacker, attackingSide);

                // Header attempt
                double headerScore = attackerHeading + rng_.real(-0.2, 0.2);
                Action headerAction = Action::Header;
                onShot(headerAction, headerScore, 0.50);
            } else {
                // Defender clears or ball deflected
                if (rng_.chance(0.50)) {
                    logEvent("Headed clear by the defense");
                    cornerKick(attackingSide);
                } else {
                    logEvent("Defense clears the free kick");
                    Cell clearTarget{4, 7};
                    ballPosition_ = cellToPosition(clearTarget);
                    ball_ = clearTarget;
                    Player* clearer = nearestOpponent(ball_, defendingSide);
                    if (clearer) {
                        clearer->pos = ball_;
                        clearer->position = ballPosition_;
                        giveBall(clearer, defendingSide);
                    }
                }
            }
        }
    } else {
        // OPTION 3: Short pass to teammate
        logEvent(shirt(*taker) + " takes a short free kick");

        // Find nearby teammate
        Player* target = nullptr;
        int bestDist = 1000;
        for (Player* p : sidePlayers_[attackingSide]) {
            if (p == taker) continue;
            int dist = CellDistance(p->pos, ball_);
            if (dist >= 2 && dist <= 5 && dist < bestDist) {
                bestDist = dist;
                target = p;
            }
        }

        if (target) {
            logEvent(shirt(*taker) + " passes to " + shirt(*target));
            aerial_ = false;
            giveBall(target, attackingSide);
        } else {
            // No good target - just play it safe
            logEvent(shirt(*taker) + " plays it simple");
            aerial_ = false;
            // Ball stays with taker
        }
    }
}

bool MatchEngine::checkBallOutOfBounds() {
    // Check if ball has left the pitch
    if (!isOutOfBounds(ballPosition_)) {
        return false;  // Ball is still in play
    }

    OutType outType = getOutType(ballPosition_);
    Position2D outPos = ballPosition_;  // Save position before clamping

    // Determine which team gets possession based on who last touched it
    int lastTouch = carrierSide_;
    int oppositeSide = 1 - lastTouch;

    switch (outType) {
        case OutType::SidelineTop:
        case OutType::SidelineBottom:
            // Ball over sideline -> throw-in for opposite team
            logEvent("Ball out of play over the sideline");
            throwIn(oppositeSide, outPos);
            return true;

        case OutType::GoallineLeft:
        case OutType::GoallineRight: {
            // Ball over goal line - determine if corner or goal kick
            // Check which goal line: left (0) or right (105)
            bool isLeftLine = (outType == OutType::GoallineLeft);

            // Team 0 (home) attacks right (towards x=105), defends left (x=0)
            // Team 1 (away) attacks left (towards x=0), defends right (x=105)
            bool isHomeDefendingEnd = isLeftLine;  // Left end is home's goal

            // If ball went out at defending team's end and attacking team touched last -> corner
            // If ball went out at defending team's end and defending team touched last -> goal kick
            if (isHomeDefendingEnd) {
                // Ball went out at home's (left) end
                if (lastTouch == 1) {
                    // Away team touched last -> corner for away
                    logEvent("Ball goes behind for a corner");
                    cornerKick(1);
                } else {
                    // Home team touched last -> goal kick for home
                    logEvent("Ball out for a goal kick");
                    goalKick(0);
                }
            } else {
                // Ball went out at away's (right) end
                if (lastTouch == 0) {
                    // Home team touched last -> corner for home
                    logEvent("Ball goes behind for a corner");
                    cornerKick(0);
                } else {
                    // Away team touched last -> goal kick for away
                    logEvent("Ball out for a goal kick");
                    goalKick(1);
                }
            }
            return true;
        }

        case OutType::None:
        default:
            return false;
    }
}

std::string MatchEngine::renderPitch() const {
    // Build a grid; each cell holds a short token. Home players use their shirt
    // number, away players are shown in (parentheses). The ball carrier is *.
    std::string grid[kRows][kCols + 1];
    for (int r = 0; r < kRows; ++r)
        for (int c = 1; c <= kCols; ++c) grid[r][c] = " . ";

    for (int s = 0; s < 2; ++s) {
        for (Player* p : sidePlayers_[s]) {
            int r = clampRow(p->pos.row);
            int c = clampCol(p->pos.col);
            std::string num = std::to_string(p->shirtNumber);
            if (num.size() < 2) num = " " + num;
            std::string tok = (s == 0) ? (num + " ") : ("(" + num);
            if (p == carrier_) tok = (s == 0) ? (num + "*") : ("*" + num);
            grid[r][c] = tok;
        }
    }
    // Overlay ball position if empty.
    {
        int r = clampRow(ball_.row), c = clampCol(ball_.col);
        if (grid[r][c] == " . ") grid[r][c] = " o ";
    }

    std::ostringstream os;
    os << "     ";
    for (int c = 1; c <= kCols; ++c) {
        std::string h = std::to_string(c);
        if (h.size() < 2) h = " " + h;
        os << h << " ";
    }
    os << "\n";
    for (int r = 0; r < kRows; ++r) {
        os << "  " << RowLetter(r) << "  ";
        for (int c = 1; c <= kCols; ++c) os << grid[r][c];
        os << "\n";
    }
    // --- Match Status Controls ---
    os << "  [STATUS] Possession Time: ";
    if (carrier_) {
        os << "Active";
    } else {
        os << "N/A";
    }
    os << "\n  [STATUS] Current Action: ";
    if (carrier_) {
        os << "In progress"; 
    } else {
        os << "Waiting for action";
    }
    os << "\n  [STATUS] Goal Chance: ";
    if (carrier_) os << "Calculating";
    else os << "N/A";
    // --- End Status Controls ---
    os << "\n";
    return os.str();
}

void MatchEngine::logEvent(const std::string& text, bool key) {
    MatchEvent e;
    e.minute = clock_;
    e.half = half_;
    e.text = text;
    e.key = key;
    res_->events.push_back(e);
    if (hook_) hook_(e);
}

void MatchEngine::logPlayerRound() {
    std::ostringstream os;
    os << "[" << std::fixed;
    os.precision(1);
    os << clock_ << "] ";
    for (int s = 0; s < 2; ++s)
        for (Player* p : sidePlayers_[s]) {
            os << (p == carrier_ ? "*" : "") << "#" << p->shirtNumber << "@"
               << CellName(p->pos) << " ";
        }
    res_->fullLog.push_back(os.str());
}

}  // namespace nm
