#include "Team.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <vector>

namespace nm {

// Returns the specific positions for a formation layout.
// Each formation defines exactly which positions to fill.
std::vector<Position> getFormationPositions(const std::string& formation) {
    // Strip " Custom" suffix if present
    std::string baseFormation = formation;
    const std::string customSuffix = " Custom";
    if (baseFormation.size() >= customSuffix.size() && 
        baseFormation.substr(baseFormation.size() - customSuffix.size()) == customSuffix) {
        baseFormation = baseFormation.substr(0, baseFormation.size() - customSuffix.size());
    }

    if (baseFormation == "4-4-2") {
        return {Position::GK, 
                Position::DR, Position::DC, Position::DC, Position::DL,
                Position::MR, Position::MC, Position::MC, Position::ML,
                Position::FC, Position::FC};
    }
    if (formation == "4-3-3") {
        return {Position::GK,
                Position::DR, Position::DC, Position::DC, Position::DL,
                Position::DM, Position::MC, Position::MC,
                Position::FL, Position::FC, Position::FR};
    }
    if (formation == "4-4-1-1") {
        return {Position::GK,
                Position::DR, Position::DC, Position::DC, Position::DL,
                Position::MR, Position::MC, Position::MC, Position::ML,
                Position::AMC, Position::FC};
    }
    if (formation == "3-5-2") {
        return {Position::GK,
                Position::DC, Position::DC, Position::DC,
                Position::MR, Position::DM, Position::MC, Position::MC, Position::ML,
                Position::FC, Position::FC};
    }
    if (formation == "4-5-1") {
        return {Position::GK,
                Position::DR, Position::DC, Position::DC, Position::DL,
                Position::MR, Position::DM, Position::MC, Position::MC, Position::ML,
                Position::FC};
    }
    if (formation == "5-3-2") {
        return {Position::GK,
                Position::DR, Position::DC, Position::DC, Position::DC, Position::DL,
                Position::MC, Position::MC, Position::MC,
                Position::FC, Position::FC};
    }
    if (formation == "3-4-3") {
        return {Position::GK,
                Position::DC, Position::DC, Position::DC,
                Position::MR, Position::MC, Position::MC, Position::ML,
                Position::FL, Position::FC, Position::FR};
    }
    if (formation == "4-2-3-1") {
        return {Position::GK,
                Position::DR, Position::DC, Position::DC, Position::DL,
                Position::DM, Position::DM,
                Position::AMR, Position::AMC, Position::AML,
                Position::FC};
    }
    if (formation == "5-4-1") {
        return {Position::GK,
                Position::WBR, Position::DC, Position::DC, Position::DC, Position::WBL,
                Position::MR, Position::MC, Position::MC, Position::ML,
                Position::FC};
    }
    if (formation == "4-1-4-1") {
        return {Position::GK,
                Position::DR, Position::DC, Position::DC, Position::DL,
                Position::DM,
                Position::MR, Position::MC, Position::MC, Position::ML,
                Position::FC};
    }

    // Default fallback (4-4-2)
    return {Position::GK,
            Position::DR, Position::DC, Position::DC, Position::DL,
            Position::MR, Position::MC, Position::MC, Position::ML,
            Position::FC, Position::FC};
}

// "4-4-2" -> {defenders, midfielders, forwards}; the first number is the
// defensive line, the last the forward line, everything between is midfield.
void parseFormation(const std::string& f, int& nd, int& nm, int& nf) {
    std::vector<int> parts;
    std::string num;
    for (char c : f) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            num += c;
        } else if (!num.empty()) {
            parts.push_back(std::stoi(num));
            num.clear();
        }
    }
    if (!num.empty()) parts.push_back(std::stoi(num));

    nd = 4;
    nm = 4;
    nf = 2;
    if (parts.size() >= 2) {
        nd = parts.front();
        nf = parts.back();
        nm = 0;
        for (size_t i = 1; i + 1 < parts.size(); ++i) nm += parts[i];
    }
    if (nd + nm + nf != 10) nm = std::max(0, 10 - nd - nf);
}

void Team::autoSelectXI() {
    startingXI.clear();
    assignedPositions.clear();

    // Get the specific positions for this formation
    std::vector<Position> formationPos = getFormationPositions(formation);

    std::unordered_set<int> chosen;

    // Track which positions are still unfilled
    std::vector<bool> positionFilled(formationPos.size(), false);

    // Sort all squad players by overall ability for reference
    std::vector<const Player*> sortedSquad;
    for (const auto& p : squad) {
        sortedSquad.push_back(&p);
    }
    std::sort(sortedSquad.begin(), sortedSquad.end(), [](const Player* a, const Player* b) {
        return PlayerAbility(*a) > PlayerAbility(*b);
    });

    // Pass 1: For each position in formation, find the BEST player whose primary position matches
    // This ensures specialists get priority, but only if they're actually good at the position
    for (size_t i = 0; i < formationPos.size(); ++i) {
        Position pos = formationPos[i];
        const Player* best = nullptr;
        double bestPower = -1.0;

        // Find the best player whose primary position matches this formation position
        for (const Player* p : sortedSquad) {
            if (chosen.count(p->id)) continue;
            if (p->primaryPos != pos) continue; // Only consider players whose primary is this position

            double powerRank = PositionPowerRanking(*p, pos);
            if (powerRank > bestPower) {
                bestPower = powerRank;
                best = p;
            }
        }

        if (best && bestPower > 0) {
            startingXI.push_back(best->id);
            assignedPositions.push_back(pos);
            chosen.insert(best->id);
            positionFilled[i] = true;
        }
    }

    // Pass 2: Fill remaining positions using power rankings
    // For each unfilled position, pick the best available player based on position-specific power ranking
    for (size_t i = 0; i < formationPos.size(); ++i) {
        if (positionFilled[i]) continue;

        Position pos = formationPos[i];
        const Player* best = nullptr;
        double bestPower = -1.0;

        for (const Player* p : sortedSquad) {
            if (chosen.count(p->id)) continue;

            double powerRank = PositionPowerRanking(*p, pos);
            if (powerRank > bestPower) {
                bestPower = powerRank;
                best = p;
            }
        }

        if (best) {
            startingXI.push_back(best->id);
            assignedPositions.push_back(pos);
            chosen.insert(best->id);
            positionFilled[i] = true;
        }
    }

    // Fallback: fill any remaining positions with best available from same role
    for (size_t i = 0; i < formationPos.size(); ++i) {
        if (positionFilled[i]) continue;

        Position pos = formationPos[i];
        Role targetRole = RoleOf(pos);
        const Player* best = nullptr;
        double bestScore = -1.0;

        for (const Player* p : sortedSquad) {
            if (chosen.count(p->id)) continue;

            // Prefer players from the same role
            double score = PlayerAbility(*p);
            if (RoleOf(p->primaryPos) == targetRole) {
                score *= 1.5;
            }

            if (score > bestScore) {
                bestScore = score;
                best = p;
            }
        }

        if (best) {
            startingXI.push_back(best->id);
            assignedPositions.push_back(pos);
            chosen.insert(best->id);
            positionFilled[i] = true;
        }
    }

    // Final fallback: fill any missing slots with best available
    while (startingXI.size() < 11 && startingXI.size() < squad.size()) {
        const Player* best = nullptr;
        double bestScore = -1.0;
        for (const auto& p : squad) {
            if (!chosen.count(p.id)) {
                double score = PlayerAbility(p);
                if (score > bestScore) {
                    bestScore = score;
                    best = &p;
                }
            }
        }
        if (!best) break;

        // Assign to first unfilled position or their primary position
        Position assignPos = best->primaryPos;
        for (size_t i = 0; i < formationPos.size(); ++i) {
            if (!positionFilled[i]) {
                assignPos = formationPos[i];
                positionFilled[i] = true;
                break;
            }
        }

        startingXI.push_back(best->id);
        assignedPositions.push_back(assignPos);
        chosen.insert(best->id);
    }
}

void Team::updateFormationPositions() {
    std::vector<Position> newPositions = getFormationPositions(formation);
    if (newPositions.size() == assignedPositions.size()) {
        assignedPositions = newPositions;
    }
}

void Team::updatePlayerRoles() {
    // Update each starting XI player's role to match their assigned tactical position
    for (size_t i = 0; i < startingXI.size() && i < assignedPositions.size(); ++i) {
        Player* p = findPlayer(startingXI[i]);
        if (p) {
            p->role = RoleOf(assignedPositions[i]);
        }
    }
}

double Team::averageAbility() const {
    if (squad.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& p : squad) sum += PlayerAbility(p);
    return sum / squad.size();
}

void Team::autoOrderSubstitutes() {
    // Get all non-starting players
    std::vector<Player*> bench;
    for (auto& p : squad) {
        bool isStarter = std::find(startingXI.begin(), startingXI.end(), p.id) != startingXI.end();
        if (!isStarter) {
            bench.push_back(&p);
        }
    }

    if (bench.size() < 5) return;  // Not enough for proper substitutes

    // Ensure balanced substitutes: 1 GK, min 1 DEF, min 1 MID, min 1 ATT
    std::vector<Player*> orderedSubs;
    orderedSubs.reserve(5);

    // Helper to check if a player's role matches categories
    auto isGK = [](const Player* p) { return p->role == Role::GK; };
    auto isDEF = [](const Player* p) { return p->role == Role::D; };
    auto isMID = [](const Player* p) { return p->role == Role::DM || p->role == Role::M || p->role == Role::AM; };
    auto isATT = [](const Player* p) { return p->role == Role::F; };

    // Step 1: Pick best GK by GK power ranking
    Player* bestGK = nullptr;
    double bestGKPower = -1.0;
    for (Player* p : bench) {
        if (isGK(p)) {
            double power = PositionPowerRanking(*p, Position::GK);
            if (power > bestGKPower) {
                bestGKPower = power;
                bestGK = p;
            }
        }
    }
    if (bestGK) {
        orderedSubs.push_back(bestGK);
        bench.erase(std::remove(bench.begin(), bench.end(), bestGK), bench.end());
    }

    // Step 2: Pick best DEF by DC power ranking (most versatile defensive position)
    Player* bestDEF = nullptr;
    double bestDEFPower = -1.0;
    for (Player* p : bench) {
        if (isDEF(p)) {
            double power = PositionPowerRanking(*p, Position::DC);
            if (power > bestDEFPower) {
                bestDEFPower = power;
                bestDEF = p;
            }
        }
    }
    if (bestDEF) {
        orderedSubs.push_back(bestDEF);
        bench.erase(std::remove(bench.begin(), bench.end(), bestDEF), bench.end());
    }

    // Step 3: Pick best MID by MC power ranking (most versatile midfield position)
    Player* bestMID = nullptr;
    double bestMIDPower = -1.0;
    for (Player* p : bench) {
        if (isMID(p)) {
            double power = PositionPowerRanking(*p, Position::MC);
            if (power > bestMIDPower) {
                bestMIDPower = power;
                bestMID = p;
            }
        }
    }
    if (bestMID) {
        orderedSubs.push_back(bestMID);
        bench.erase(std::remove(bench.begin(), bench.end(), bestMID), bench.end());
    }

    // Step 4: Pick best ATT by FC power ranking
    Player* bestATT = nullptr;
    double bestATTPower = -1.0;
    for (Player* p : bench) {
        if (isATT(p)) {
            double power = PositionPowerRanking(*p, Position::FC);
            if (power > bestATTPower) {
                bestATTPower = power;
                bestATT = p;
            }
        }
    }
    if (bestATT) {
        orderedSubs.push_back(bestATT);
        bench.erase(std::remove(bench.begin(), bench.end(), bestATT), bench.end());
    }

    // Step 5: Fill remaining slot with best available by overall ability
    if (orderedSubs.size() < 5 && !bench.empty()) {
        // Sort remaining bench by ability
        std::sort(bench.begin(), bench.end(), [](const Player* a, const Player* b) {
            return PlayerAbility(*a) > PlayerAbility(*b);
        });
        orderedSubs.push_back(bench.front());
        bench.erase(bench.begin());
    }

    // Reorder squad: startingXI players first, then ordered subs, then rest
    std::vector<Player> reorderedSquad;

    // Add starting XI in their current order
    for (int id : startingXI) {
        Player* p = findPlayer(id);
        if (p) reorderedSquad.push_back(*p);
    }

    // Add ordered substitutes
    for (Player* p : orderedSubs) {
        reorderedSquad.push_back(*p);
    }

    // Add remaining players
    for (Player* p : bench) {
        reorderedSquad.push_back(*p);
    }

    // Replace squad with reordered version
    squad = std::move(reorderedSquad);
}

}  // namespace nm
