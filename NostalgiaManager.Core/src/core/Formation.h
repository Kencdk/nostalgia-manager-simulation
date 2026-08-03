#pragma once
#include <algorithm>
#include <map>
#include <vector>

#include "Pitch.h"
#include "Player.h"
#include "Team.h"

namespace nm {

// Maps a role to its X column based on team side and mentality, exactly as in
// the "X Positions" table of the design doc.
inline int RoleColumn(Role role, int side /*1 or 2*/, Mentality m) {
    // [side][mentality][role index GK,D,DM,M,AM,F]
    static const int table[2][3][6] = {
        // Team 1
        {
            {1, 2, 3, 4, 5, 6},     // Defensive
            {2, 4, 5, 6, 8, 10},    // Standard
            {3, 7, 8, 9, 11, 12},   // Attack
        },
        // Team 2 (mirrored)
        {
            {13, 12, 11, 10, 9, 8}, // Defensive
            {12, 10, 9, 8, 6, 4},   // Standard
            {11, 6, 5, 4, 3, 2},    // Attack
        },
    };
    int s = (side == 2) ? 1 : 0;
    int mi = (m == Mentality::Defensive) ? 0 : (m == Mentality::Attack ? 2 : 1);
    int ri = static_cast<int>(role);
    return table[s][mi][ri];
}

// Convert a tactical Position to pitch Cell coordinates based on side and mentality
inline Cell PositionToCell(Position pos, int side, Mentality mentality) {
    // Base normalized coordinates (0-1) from tactics screen visualization
    // xNorm: lateral position (0 = left touchline, 0.5 = center, 1 = right touchline)
    // yNorm: depth (0 = attacking end, 1 = defensive end)
    float yNorm = 0.5f, xNorm = 0.5f;

    // IMPORTANT: Wide players should be at 0.5 in their channel (middle of the flank)
    // Central players should be at 0.5 (middle of the pitch)

    switch (pos) {
        // Forwards
        case Position::FL:  yNorm = 0.08f; xNorm = 0.25f; break;  // Left channel middle
        case Position::FC:  yNorm = 0.08f; xNorm = 0.50f; break;  // Center
        case Position::FR:  yNorm = 0.08f; xNorm = 0.75f; break;  // Right channel middle

        // Attacking Midfielders
        case Position::AML: yNorm = 0.28f; xNorm = 0.20f; break;  // Left channel
        case Position::AMC: yNorm = 0.28f; xNorm = 0.50f; break;  // Center
        case Position::AMR: yNorm = 0.28f; xNorm = 0.80f; break;  // Right channel

        // Midfielders
        case Position::ML:  yNorm = 0.48f; xNorm = 0.15f; break;  // Left flank middle
        case Position::MC:  yNorm = 0.48f; xNorm = 0.50f; break;  // Center
        case Position::MR:  yNorm = 0.48f; xNorm = 0.85f; break;  // Right flank middle

        // Defensive Midfielders
        case Position::DM:  yNorm = 0.65f; xNorm = 0.50f; break;  // Center

        // Defenders
        case Position::WBL: yNorm = 0.74f; xNorm = 0.12f; break;  // Left wing-back
        case Position::DL:  yNorm = 0.80f; xNorm = 0.22f; break;  // Left back
        case Position::DC:  yNorm = 0.83f; xNorm = 0.50f; break;  // Center back
        case Position::DR:  yNorm = 0.80f; xNorm = 0.78f; break;  // Right back
        case Position::WBR: yNorm = 0.74f; xNorm = 0.88f; break;  // Right wing-back

        // Goalkeeper
        case Position::GK:  yNorm = 0.95f; xNorm = 0.50f; break;  // Center
    }

    // Convert to row (0-8, where 0 is top/attacking for team 1)
    int row = static_cast<int>(xNorm * (kRows - 1) + 0.5f);
    row = clampRow(row);

    // Get base column from role and mentality
    Role role = RoleOf(pos);
    int col = RoleColumn(role, side, mentality);

    return Cell{row, col};
}

// Place starting XI according to their assigned tactical positions
inline void PlaceStartingXI(Team& team, int side) {
    // Count how many players are assigned to each position
    std::map<Position, std::vector<size_t>> positionGroups;

    for (size_t i = 0; i < team.startingXI.size() && i < team.assignedPositions.size(); ++i) {
        Position pos = team.assignedPositions[i];
        positionGroups[pos].push_back(i);
    }

    // Place each player, spreading out multiples in the same position
    for (const auto& group : positionGroups) {
        Position tacticalPos = group.first;
        const std::vector<size_t>& playerIndices = group.second;
        int count = static_cast<int>(playerIndices.size());

        // Get base position for this tactical role
        Cell baseCell = PositionToCell(tacticalPos, side, team.mentality);

        if (count == 1) {
            // Single player - use exact position
            size_t idx = playerIndices[0];
            Player* p = team.findPlayer(team.startingXI[idx]);
            if (p) {
                p->homePos = baseCell;
                p->pos = p->homePos;
                p->homePosition = cellToPosition(p->homePos);
                p->position = p->homePosition;
                p->hasBall = false;
            }
        } else {
            // Multiple players in same position - spread them intelligently
            // Central players (row near 4) spread symmetrically around center
            // Wide players maintain their flank but spread along it

            bool isCentral = (baseCell.row >= 3 && baseCell.row <= 5);  // Rows 3-5 are central

            for (int i = 0; i < count; ++i) {
                size_t idx = playerIndices[i];
                Player* p = team.findPlayer(team.startingXI[idx]);
                if (!p) continue;

                Cell cell = baseCell;

                if (isCentral) {
                    // CENTRAL PLAYERS: Spread symmetrically around center (row 4)
                    if (count == 2) {
                        // Two central players: one left of center, one right
                        cell.row = (i == 0) ? 3 : 5;  // Rows 3 and 5 (equidistant from 4)
                    } else if (count == 3) {
                        // Three central players: left, center, right
                        int positions[] = {3, 4, 5};  // Rows 3, 4, 5
                        cell.row = positions[i];
                    } else if (count == 4) {
                        // Four central players: spread wider
                        int positions[] = {2, 3, 5, 6};  // Skip center, use 2,3,5,6
                        cell.row = positions[i];
                    } else if (count == 5) {
                        // Five central players: full spread
                        int positions[] = {2, 3, 4, 5, 6};
                        cell.row = positions[i];
                    } else {
                        // More than 5: distribute across available rows
                        float spacing = 5.0f / (count - 1);  // Spread from row 2 to row 6
                        cell.row = clampRow(2 + static_cast<int>(i * spacing));
                    }
                } else {
                    // WIDE PLAYERS: Stay on their flank, spread slightly
                    // Apply small lateral offset but keep them wide
                    if (count == 2) {
                        int offset = (i == 0) ? -1 : 1;
                        cell.row = clampRow(baseCell.row + offset);
                    } else if (count == 3) {
                        int offset = i - 1;  // -1, 0, +1
                        cell.row = clampRow(baseCell.row + offset);
                    } else {
                        // Wider spread for more players
                        float spacing = 3.0f / (count - 1);
                        int offset = static_cast<int>(i * spacing) - 1;
                        cell.row = clampRow(baseCell.row + offset);
                    }
                }

                p->homePos = cell;
                p->pos = p->homePos;
                p->homePosition = cellToPosition(p->homePos);
                p->position = p->homePosition;
                p->hasBall = false;
            }
        }
    }
}

}  // namespace nm
