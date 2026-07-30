#pragma once
#include <map>
#include <string>
#include <vector>

#include "Attributes.h"
#include "Pitch.h"

namespace nm {

// Role on the team sheet. Drives the default X column in a formation and all
// match-engine logic. The finer-grained Position (below) layers on top.
enum class Role { GK, D, DM, M, AM, F };

// Which flank a player occupies. Centre covers central/GK/DM roles.
enum class Side { Centre, Right, Left };

// Full set of pitch positions (role + flank), as found in the CM/FM database.
enum class Position {
    GK,
    DR, DC, DL,      // full-backs / centre-backs
    WBR, WBL,        // wing-backs
    DM,
    MR, MC, ML,      // wide / central midfield
    AMR, AMC, AML,   // wide / central attacking midfield
    FR, FC, FL,      // wide forwards / centre-forward
};

inline std::string RoleName(Role r) {
    switch (r) {
        case Role::GK: return "GK";
        case Role::D:  return "D";
        case Role::DM: return "DM";
        case Role::M:  return "M";
        case Role::AM: return "AM";
        case Role::F:  return "F";
    }
    return "?";
}

inline Role RoleFromString(const std::string& s) {
    if (s == "GK") return Role::GK;
    if (s == "DM") return Role::DM;
    if (s == "M")  return Role::M;
    if (s == "AM") return Role::AM;
    if (s == "F")  return Role::F;
    return Role::D;
}

// Short label for a position, e.g. "DR", "MC", "FL", "WBR".
inline std::string PosName(Position p) {
    switch (p) {
        case Position::GK:  return "GK";
        case Position::DR:  return "DR";
        case Position::DC:  return "DC";
        case Position::DL:  return "DL";
        case Position::WBR: return "WBR";
        case Position::WBL: return "WBL";
        case Position::DM:  return "DM";
        case Position::MR:  return "MR";
        case Position::MC:  return "MC";
        case Position::ML:  return "ML";
        case Position::AMR: return "AMR";
        case Position::AMC: return "AMC";
        case Position::AML: return "AML";
        case Position::FR:  return "FR";
        case Position::FC:  return "FC";
        case Position::FL:  return "FL";
    }
    return "?";
}

// Parse a position string (e.g. "DMC", "FC", "AMR") to Position enum.
// Supports both full names (e.g. "MC", "AMC") and CM-style abbreviations (e.g. "DMC" for DM).
inline Position PositionFromString(const std::string& s) {
    if (s.empty()) return Position::MC;

    // Direct matches
    if (s == "GK") return Position::GK;
    if (s == "DR") return Position::DR;
    if (s == "DC") return Position::DC;
    if (s == "DL") return Position::DL;
    if (s == "WBR") return Position::WBR;
    if (s == "WBL") return Position::WBL;
    if (s == "DM" || s == "DMC") return Position::DM;
    if (s == "MR") return Position::MR;
    if (s == "MC") return Position::MC;
    if (s == "ML") return Position::ML;
    if (s == "AMR") return Position::AMR;
    if (s == "AMC") return Position::AMC;
    if (s == "AML") return Position::AML;
    if (s == "FR") return Position::FR;
    if (s == "FC") return Position::FC;
    if (s == "FL") return Position::FL;

    // Fallback: try to infer from partial matches
    if (s.find("GK") != std::string::npos) return Position::GK;
    if (s.find("DM") != std::string::npos) return Position::DM;

    // Default fallback
    return Position::MC;
}

// The base engine Role that a Position maps to (wing-backs play as defenders).
inline Role RoleOf(Position p) {
    switch (p) {
        case Position::GK: return Role::GK;
        case Position::DR: case Position::DC: case Position::DL:
        case Position::WBR: case Position::WBL: return Role::D;
        case Position::DM: return Role::DM;
        case Position::MR: case Position::MC: case Position::ML: return Role::M;
        case Position::AMR: case Position::AMC: case Position::AML: return Role::AM;
        case Position::FR: case Position::FC: case Position::FL: return Role::F;
    }
    return Role::M;
}

// The default (centre) position for an engine role.
inline Position DefaultPosOf(Role r) {
    switch (r) {
        case Role::GK: return Position::GK;
        case Role::D:  return Position::DC;
        case Role::DM: return Position::DM;
        case Role::M:  return Position::MC;
        case Role::AM: return Position::AMC;
        case Role::F:  return Position::FC;
    }
    return Position::MC;
}

// The flank a position sits on.
inline Side SideOf(Position p) {
    switch (p) {
        case Position::DR: case Position::WBR: case Position::MR:
        case Position::AMR: case Position::FR: return Side::Right;
        case Position::DL: case Position::WBL: case Position::ML:
        case Position::AML: case Position::FL: return Side::Left;
        default: return Side::Centre;
    }
}

struct Player {
    int id = 0;
    std::string name;
    int teamId = 0;
    Role role = Role::M;
    Position primaryPos = Position::MC;       // natural (best) position
    std::vector<Position> playablePositions;  // every position he can fill
    std::map<Position, int> positionRatings;  // position -> rating (0-100)
    int shirtNumber = 0;
    Attributes attr;

    // Player bio information
    int age = 0;                    // Age in years
    std::string dateOfBirth;        // Date of birth (DD/MM/YYYY format)
    std::string nationality;        // Nationality/Country
    int internationalCaps = 0;      // Number of international appearances
    int internationalGoals = 0;     // Number of international goals

    bool canPlay(Position pos) const {
        for (Position p : playablePositions)
            if (p == pos) return true;
        return false;
    }

    // Get position rating (0 if not playable)
    int getPositionRating(Position pos) const {
        auto it = positionRatings.find(pos);
        return it != positionRatings.end() ? it->second : 0;
    }

    // Live match state (reset each match).
    Position2D position = Position2D();  // Continuous position in meters
    Position2D homePosition = Position2D();  // Formation anchor (continuous)
    Cell pos = CentreSpot();  // Grid cell (for action logic)
    Cell homePos = CentreSpot();  // Formation anchor (grid)
    bool hasBall = false;
    int dispossessionCooldown = 0;  // rounds before player can challenge after losing ball

    double norm(const std::string& stat) const { return attr.norm(stat); }

    // Maximum fields a player may move per action = 1 + pace/10 (spec section 5),
    // returned as an integer field count (1.1 -> 1, 2.0 -> 2, 3.0 -> 3).
    int maxMovePerAction() const {
        double pace = static_cast<double>(attr.get("Pace"));
        return static_cast<int>(1.0 + pace / 10.0);
    }
};

}  // namespace nm
