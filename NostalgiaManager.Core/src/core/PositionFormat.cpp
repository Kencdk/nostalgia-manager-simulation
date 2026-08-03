#include "PositionFormat.h"

#include <map>
#include <set>
#include <vector>

namespace nm {

std::string cmPositionFormat(const Player& p) {
    // Championship Manager style position format
    // Groups positions by Role and Side
    // Examples:
    //   MC, MR -> "M C/R"
    //   MR, AMR, ML, AML, FC, FR, FL -> "M/AM R/L, F C/R/L"
    //   GK -> "GK"
    //   DC, DR, DL -> "D C/R/L"

    if (p.playablePositions.empty()) {
        return PosName(p.primaryPos);
    }

    // Group positions by role
    std::map<Role, std::set<Side>> roleToSides;

    for (Position pos : p.playablePositions) {
        Role role = RoleOf(pos);
        Side side = SideOf(pos);
        roleToSides[role].insert(side);
    }

    // Build the output string
    std::string result;

    // Process roles in order: GK, D, DM, M, AM, F
    std::vector<Role> roleOrder = {Role::GK, Role::D, Role::DM, Role::M, Role::AM, Role::F};

    for (Role role : roleOrder) {
        auto it = roleToSides.find(role);
        if (it == roleToSides.end()) continue;

        const std::set<Side>& sides = it->second;

        // Add separator for multiple role groups
        if (!result.empty()) {
            result += ", ";
        }

        // Add role(s) part
        // Check if this role group should be combined with adjacent roles
        bool combinedWithNext = false;

        // For M and AM, check if they share the same sides
        if (role == Role::M) {
            auto amIt = roleToSides.find(Role::AM);
            if (amIt != roleToSides.end() && amIt->second == sides) {
                result += "M/AM";
                combinedWithNext = true;
                roleToSides.erase(Role::AM); // Skip AM in next iteration
            } else {
                result += RoleName(role);
            }
        }
        // For D and DM (rare but possible)
        else if (role == Role::D) {
            auto dmIt = roleToSides.find(Role::DM);
            if (dmIt != roleToSides.end() && dmIt->second == sides) {
                result += "D/DM";
                combinedWithNext = true;
                roleToSides.erase(Role::DM);
            } else {
                result += RoleName(role);
            }
        }
        // For AM and F
        else if (role == Role::AM) {
            auto fIt = roleToSides.find(Role::F);
            if (fIt != roleToSides.end() && fIt->second == sides) {
                result += "AM/F";
                combinedWithNext = true;
                roleToSides.erase(Role::F);
            } else {
                result += RoleName(role);
            }
        }
        else {
            result += RoleName(role);
        }

        // Add sides part
        if (role == Role::GK) {
            // GK never shows side (always centre)
            // Don't add anything
        }
        else if (sides.size() == 1 && *sides.begin() == Side::Centre) {
            // Only centre position, add " C"
            result += " C";
        }
        else if (sides.size() > 1 || *sides.begin() != Side::Centre) {
            // Multiple sides or non-centre
            result += " ";
            bool firstSide = true;

            // Order: Centre, Right, Left
            if (sides.count(Side::Centre)) {
                result += "C";
                firstSide = false;
            }
            if (sides.count(Side::Right)) {
                if (!firstSide) result += "/";
                result += "R";
                firstSide = false;
            }
            if (sides.count(Side::Left)) {
                if (!firstSide) result += "/";
                result += "L";
            }
        }
    }

    return result.empty() ? PosName(p.primaryPos) : result;
}

}  // namespace nm
