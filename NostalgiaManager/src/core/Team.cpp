#include "Team.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <vector>

namespace nm {

// Normalise a formation string so that both "4231" and "4-2-3-1" (and
// "4231 defensive" etc.) all resolve to the same canonical dash-separated
// form used by the position table below.
// "4231"          -> "4-2-3-1"
// "433 defensive" -> "4-3-3"   (variant words are stripped)
// "4-4-2"         -> "4-4-2"   (already canonical, left unchanged)
static std::string normaliseFormation(const std::string& raw) {
    // Strip any trailing non-digit/non-dash word (e.g. " defensive", " Custom")
    std::string s;
    for (char c : raw) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') s += c;
        else if (c == ' ') break;  // stop at first space
    }
    if (s.empty()) return raw;

    // If it already contains dashes it is already canonical
    if (s.find('-') != std::string::npos) return s;

    // Insert a dash between every adjacent digit pair
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        out += s[i];
        if (i + 1 < s.size()) out += '-';
    }
    return out;
}

// Returns the specific positions for a formation layout.
// Each formation defines exactly which positions to fill.
std::vector<Position> getFormationPositions(const std::string& formation) {
    // Strip " Custom" suffix if present, then normalise to dash-separated digits
    std::string baseFormation = normaliseFormation(formation);
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
    if (baseFormation == "4-3-3") {
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

    std::vector<Position> formationPos = getFormationPositions(formation);
    autoSelectXIFromPositions(formationPos);
}

void Team::autoSelectXI(const void* tacticTemplatePtr) {
    startingXI.clear();
    assignedPositions.clear();

    // tacticTemplatePtr is a TacticTemplate* passed as void* to avoid a
    // circular include between Team.cpp and Database.h.
    // We reconstruct the position list via a small inline helper that mirrors
    // TacticTemplate::toPositionList() — kept in sync manually.
    struct PosEntry { const char* key; Position pos; };
    static const PosEntry order[] = {
        {"GK",  Position::GK},
        {"DC",  Position::DC}, {"DR",  Position::DR}, {"DL",  Position::DL},
        {"WBR", Position::WBR},{"WBL", Position::WBL},
        {"DMC", Position::DM},
        {"MC",  Position::MC}, {"MR",  Position::MR}, {"ML",  Position::ML},
        {"AMC", Position::AMC},{"AMR", Position::AMR},{"AML", Position::AML},
        {"FC",  Position::FC}, {"FR",  Position::FR}, {"FL",  Position::FL},
    };

    // The caller guarantees the pointer is a const std::map<std::string,int>*
    // (the positionCounts member) or null.
    const std::map<std::string, int>* counts =
        static_cast<const std::map<std::string, int>*>(tacticTemplatePtr);

    std::vector<Position> formationPos;
    if (counts && !counts->empty()) {
        for (const auto& o : order) {
            auto it = counts->find(o.key);
            if (it == counts->end()) continue;
            for (int i = 0; i < it->second; ++i) formationPos.push_back(o.pos);
        }
    }
    if (formationPos.empty())
        formationPos = getFormationPositions(formation);

    autoSelectXIFromPositions(formationPos);
}

void Team::autoSelectXIFromPositions(const std::vector<Position>& formationPos) {

    // Build all valid (player, slot) candidates.
    // A candidate is only valid when positionRating >= 50 (hard minimum).
    // Score = PlayerAbility * positionRating / 100 so a fully natural player
    // (rating 100) always outranks the same player in an awkward position (< 100).
    struct Candidate {
        double score;
        int playerIdx;
        int slotIdx;
    };

    std::vector<Candidate> candidates;
    candidates.reserve(squad.size() * formationPos.size());

    for (int pi = 0; pi < (int)squad.size(); ++pi) {
        const Player& p = squad[pi];
        double ability = (double)p.ability;
        for (int si = 0; si < (int)formationPos.size(); ++si) {
            int rating = p.getPositionRating(formationPos[si]);
            if (rating < 50) continue;
            candidates.push_back({ ability * rating / 100.0, pi, si });
        }
    }

    // Sort best score first.
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.score > b.score;
    });

    std::unordered_set<int> usedPlayers;
    std::vector<bool> filledSlots(formationPos.size(), false);

    // Greedy assignment: repeatedly pick the highest-scoring valid pair.
    for (const Candidate& c : candidates) {
        if (filledSlots[c.slotIdx]) continue;
        if (usedPlayers.count(squad[c.playerIdx].id)) continue;

        startingXI.push_back(squad[c.playerIdx].id);
        assignedPositions.push_back(formationPos[c.slotIdx]);
        usedPlayers.insert(squad[c.playerIdx].id);
        filledSlots[c.slotIdx] = true;

        if (startingXI.size() == formationPos.size()) break;
    }

    // Thin-squad fallback: if some slots are still empty, relax the 50-rating
    // floor and accept any player with rating > 0 for that slot.
    for (int si = 0; si < (int)formationPos.size(); ++si) {
        if (filledSlots[si]) continue;

        const Player* best = nullptr;
        double bestScore = -1.0;
        for (const auto& p : squad) {
            if (usedPlayers.count(p.id)) continue;
            int rating = p.getPositionRating(formationPos[si]);
            if (rating <= 0) continue;
            double score = (double)p.ability * rating / 100.0;
            if (score > bestScore) { bestScore = score; best = &p; }
        }
        if (best) {
            startingXI.push_back(best->id);
            assignedPositions.push_back(formationPos[si]);
            usedPlayers.insert(best->id);
            filledSlots[si] = true;
        }
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
    for (const auto& p : squad) sum += (double)p.ability;
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
