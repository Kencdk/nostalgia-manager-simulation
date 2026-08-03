#pragma once
#include <string>
#include <vector>

#include "Player.h"
#include "TacticalSettings.h"

namespace nm {

enum class Mentality { Defensive, Standard, Attack };

inline std::string MentalityName(Mentality m) {
    switch (m) {
        case Mentality::Defensive: return "Defensive";
        case Mentality::Attack:    return "Attack";
        default:                   return "Standard";
    }
}

struct Team {
    int id = 0;
    std::string name;
    std::string nation;
    std::string league;
    std::string formation = "4-4-2";
    std::string preferredFormation = "4-4-2";  // Club's default formation (from database)
    Mentality mentality = Mentality::Standard;

    // Team colors from TeamsDB
    std::string homeColor1;      // Main jersey color (fill)
    std::string homeColor2;      // Trim/number color
    std::string awayColor1;      // Away main color
    std::string awayColor2;      // Away trim/number color

    TeamTactics tactics;

    std::vector<Player> squad;
    std::vector<int> startingXI;  // player ids
    std::vector<Position> assignedPositions;  // formation positions for startingXI (parallel array)
    std::vector<int> subsUsed;    // player ids brought on

    Player* findPlayer(int playerId) {
        for (auto& p : squad)
            if (p.id == playerId) return &p;
        return nullptr;
    }

    // Best 11 by aggregate ability, always including a goalkeeper, used when no
    // explicit XI is configured. If tacticTemplate is non-null its position list
    // is used instead of the hardcoded getFormationPositions table.
    void autoSelectXI();
    void autoSelectXI(const void* tacticTemplate);  // accepts TacticTemplate* (void* avoids circular include)
    void autoSelectXIFromPositions(const std::vector<Position>& positions);

    // Order substitutes to ensure positional balance (1 GK, min 1 DEF, min 1 MID, min 1 ATT)
    // Reorders the squad so the first 5 non-starters are balanced substitutes
    void autoOrderSubstitutes();

    // Update assigned positions to match current formation (without changing players)
    void updateFormationPositions();

    // Update player roles to match their assigned tactical positions
    void updatePlayerRoles();

    double averageAbility() const;
};

inline double PlayerAbility(const Player& p) {
    double sum = 0.0;
    int n = 0;
    for (const auto& name : AttributeNames()) {
        if (name == "Goalkeeping" && p.role != Role::GK) continue;
        sum += p.attr.get(name);
        ++n;
    }
    return n ? sum / n : 0.0;
}

// Calculate position-specific power ranking based on key attributes for that position
inline double PositionPowerRanking(const Player& p, Position pos) {
    const auto& attr = p.attr;

    // Position rating (0-100) affects base score
    int posRating = p.getPositionRating(pos);
    if (posRating == 0) return 0.0; // Cannot play this position

    double posMultiplier = posRating / 100.0; // 0.0 to 1.0
    double score = 0.0;

    switch (pos) {
        case Position::GK:
            // Goalkeepers: Goalkeeping is king
            score = attr.get("Goalkeeping") * 3.0 +
                    attr.get("Positioning") * 1.5 +
                    attr.get("Aggression") * 0.5 +
                    attr.get("Determination") * 0.5;
            break;

        case Position::DR:
        case Position::DL:
        case Position::WBR:
        case Position::WBL:
            // Full-backs: Tackling, Pace, Stamina, Positioning
            score = attr.get("Tackling") * 2.0 +
                    attr.get("Positioning") * 2.0 +
                    attr.get("Marking") * 1.5 +
                    attr.get("Pace") * 1.5 +
                    attr.get("Stamina") * 1.5 +
                    attr.get("Passing") * 1.0 +
                    attr.get("Determination") * 0.5;
            break;

        case Position::DC:
            // Center-backs: Tackling, Marking, Heading, Positioning, Strength
            score = attr.get("Tackling") * 2.5 +
                    attr.get("Marking") * 2.5 +
                    attr.get("Positioning") * 2.0 +
                    attr.get("Heading") * 2.0 +
                    attr.get("Strength") * 1.5 +
                    attr.get("Jumping") * 1.0 +
                    attr.get("Determination") * 0.5;
            break;

        case Position::DM:
            // Defensive midfielders: Tackling, Passing, Positioning, Stamina
            score = attr.get("Tackling") * 2.5 +
                    attr.get("Positioning") * 2.0 +
                    attr.get("Passing") * 2.0 +
                    attr.get("Stamina") * 1.5 +
                    attr.get("Marking") * 1.5 +
                    attr.get("Determination") * 1.0 +
                    attr.get("Strength") * 0.5;
            break;

        case Position::MC:
            // Central midfielders: Passing, Technique, Stamina, Vision (Creativity)
            score = attr.get("Passing") * 2.5 +
                    attr.get("Technique") * 2.0 +
                    attr.get("Stamina") * 2.0 +
                    attr.get("Creativity") * 1.5 +
                    attr.get("Tackling") * 1.5 +
                    attr.get("Positioning") * 1.0 +
                    attr.get("Determination") * 0.5;
            break;

        case Position::MR:
        case Position::ML:
            // Wide midfielders: Pace, Dribbling, Passing, Stamina, Crossing (Technique)
            score = attr.get("Pace") * 2.0 +
                    attr.get("Dribbling") * 2.0 +
                    attr.get("Technique") * 1.5 +
                    attr.get("Passing") * 1.5 +
                    attr.get("Stamina") * 1.5 +
                    attr.get("Creativity") * 1.0 +
                    attr.get("Flair") * 1.0;
            break;

        case Position::AMC:
            // Attacking midfielders: Creativity, Technique, Passing, Shooting, Dribbling
            score = attr.get("Creativity") * 2.5 +
                    attr.get("Technique") * 2.5 +
                    attr.get("Passing") * 2.0 +
                    attr.get("Dribbling") * 2.0 +
                    attr.get("Shooting") * 1.5 +
                    attr.get("Flair") * 1.0 +
                    attr.get("OffTheBall") * 1.0;
            break;

        case Position::AMR:
        case Position::AML:
            // Wide attacking midfielders: Dribbling, Pace, Creativity, Technique
            score = attr.get("Dribbling") * 2.5 +
                    attr.get("Pace") * 2.0 +
                    attr.get("Creativity") * 2.0 +
                    attr.get("Technique") * 1.5 +
                    attr.get("Shooting") * 1.5 +
                    attr.get("Flair") * 1.5 +
                    attr.get("OffTheBall") * 1.0;
            break;

        case Position::FC:
        case Position::FR:
        case Position::FL:
            // Forwards: Shooting, OffTheBall, Pace, Finishing (Technique)
            score = attr.get("Shooting") * 3.0 +
                    attr.get("OffTheBall") * 2.5 +
                    attr.get("Technique") * 2.0 +
                    attr.get("Pace") * 1.5 +
                    attr.get("Heading") * 1.0 +
                    attr.get("Dribbling") * 1.0 +
                    attr.get("Determination") * 0.5;
            break;
    }

    // Apply position rating multiplier
    return score * posMultiplier;
}

}  // namespace nm
