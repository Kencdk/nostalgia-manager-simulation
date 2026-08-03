#pragma once
#include <string>
#include <cmath>

namespace nm {

// Continuous pitch coordinates (in meters)
constexpr float kPitchLength = 105.0f;  // Length in meters
constexpr float kPitchWidth = 68.0f;    // Width in meters

// Continuous position on the pitch (x, y in meters)
struct Position2D {
    float x = kPitchLength * 0.5f;  // 0 to 105 (left to right)
    float y = kPitchWidth * 0.5f;   // 0 to 68 (top to bottom)

    Position2D() = default;
    Position2D(float x_, float y_) : x(x_), y(y_) {}

    // Distance between two positions
    float distanceTo(const Position2D& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

// Pitch matrix per the design doc: rows A-I (9 rows, the Y axis) and columns
// 1-13 (the X axis). E7 is the centre spot. Team 1 defends columns 1-6 and
// attacks towards column 13; Team 2 is mirrored.
struct Cell {
    int row = 4;  // 0..8  -> A..I
    int col = 7;  // 1..13

    bool operator==(const Cell& o) const { return row == o.row && col == o.col; }
};

constexpr int kRows = 9;
constexpr int kCols = 13;

inline char RowLetter(int row) { return static_cast<char>('A' + row); }

inline std::string CellName(const Cell& c) {
    return std::string(1, RowLetter(c.row)) + std::to_string(c.col);
}

inline Cell CentreSpot() { return Cell{4, 7}; }  // E7

inline int clampRow(int r) { return r < 0 ? 0 : (r >= kRows ? kRows - 1 : r); }
inline int clampCol(int c) { return c < 1 ? 1 : (c > kCols ? kCols : c); }

// Manhattan distance between two cells (used for pressure / pass range checks).
inline int CellDistance(const Cell& a, const Cell& b) {
    int dr = a.row - b.row;
    int dc = a.col - b.col;
    if (dr < 0) dr = -dr;
    if (dc < 0) dc = -dc;
    return dr + dc;
}

// Convert continuous position to grid cell for action logic
inline Cell positionToCell(const Position2D& pos) {
    // Map x (0-105m) to columns (1-13)
    int col = static_cast<int>((pos.x / kPitchLength) * kCols + 1.0f);
    col = clampCol(col);

    // Map y (0-68m) to rows (0-8)
    int row = static_cast<int>((pos.y / kPitchWidth) * kRows);
    row = clampRow(row);

    return Cell{row, col};
}

// Convert grid cell to continuous position (center of cell)
inline Position2D cellToPosition(const Cell& cell) {
    // Map columns (1-13) to x (0-105m)
    float x = ((cell.col - 0.5f) / kCols) * kPitchLength;

    // Map rows (0-8) to y (0-68m)
    float y = ((cell.row + 0.5f) / kRows) * kPitchWidth;

    return Position2D(x, y);
}

// Clamp position to pitch boundaries
inline Position2D clampPosition(const Position2D& pos) {
    float x = pos.x < 0.0f ? 0.0f : (pos.x > kPitchLength ? kPitchLength : pos.x);
    float y = pos.y < 0.0f ? 0.0f : (pos.y > kPitchWidth ? kPitchWidth : pos.y);
    return Position2D(x, y);
}

// Check if position is out of bounds (beyond pitch boundaries)
inline bool isOutOfBounds(const Position2D& pos) {
    return pos.x < 0.0f || pos.x > kPitchLength || pos.y < 0.0f || pos.y > kPitchWidth;
}

// Determine which boundary the ball crossed (for throw-ins, corners, goal kicks)
enum class OutType { None, SidelineTop, SidelineBottom, GoallineLeft, GoallineRight };

inline OutType getOutType(const Position2D& pos) {
    if (pos.y < 0.0f) return OutType::SidelineTop;
    if (pos.y > kPitchWidth) return OutType::SidelineBottom;
    if (pos.x < 0.0f) return OutType::GoallineLeft;
    if (pos.x > kPitchLength) return OutType::GoallineRight;
    return OutType::None;
}

}  // namespace nm
