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

    // Group players by their best-fit for each position
    auto byAbility = [](const Player* a, const Player* b) {
        return PlayerAbility(*a) > PlayerAbility(*b);
    };

    std::unordered_set<int> chosen;

    // For each position in the formation, find the best available player
    for (Position pos : formationPos) {
        const Player* best = nullptr;
        double bestScore = -1.0;

        for (const auto& p : squad) {
            // Skip already chosen players
            if (chosen.count(p.id)) continue;

            // Calculate suitability score for this position
            double score = PlayerAbility(p);

            // Bonus if it's their primary position
            if (p.primaryPos == pos) {
                score *= 1.3;
            }
            // Bonus if they can play this position
            else if (p.canPlay(pos)) {
                score *= 1.1;
            }
            // Penalty if wrong role (but still allow as fallback)
            else if (RoleOf(p.primaryPos) != RoleOf(pos)) {
                score *= 0.5;
            }

            if (score > bestScore) {
                bestScore = score;
                best = &p;
            }
        }

        if (best) {
            startingXI.push_back(best->id);
            assignedPositions.push_back(pos);
            chosen.insert(best->id);
        }
    }

    // Fill any missing slots with best available (shouldn't happen with valid formations)
    while (startingXI.size() < 11) {
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

        startingXI.push_back(best->id);
        assignedPositions.push_back(best->primaryPos);
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

}  // namespace nm
