# Position Proficiency Categorization System

## Overview
Enhanced the player position system to categorize positions by proficiency level and display them in a clear, color-coded format.

## Position Proficiency Categories

Based on the position rating (0-100) from the CSV:

| Category | Rating Range | Color | Description |
|----------|-------------|-------|-------------|
| **Preferred** | Primary Position | Green | The player's main/preferred position |
| **Natural** | 100 | Light Green | Positions where the player is naturally gifted |
| **Accomplished** | 70-99 | Yellow-Green | Highly competent positions |
| **Competent** | 40-69 | Orange | Adequate performance in these positions |
| **Unconvincing** | 1-39 | Red-Orange | Can play but not recommended |

## Changes Made

### 1. Player.h
**Added:**
- `std::map<Position, int> positionRatings` - Stores rating (0-100) for each position
- `int getPositionRating(Position pos)` - Returns the rating for a position (0 if not playable)
- `#include <map>` - Required for the position ratings map

### 2. Database.cpp (lines 359-435)
**Updated:** Position loading logic to store ratings alongside positions:
```cpp
if (rDL > threshold) {
    p.playablePositions.push_back(Position::DL);
    p.positionRatings[Position::DL] = rDL;  // Store the rating
}
```

### 3. UIHelpers.h / UIHelpers.cpp
**Added:**
- `std::string playablePosByProficiency(const Player& p)` - Returns categorized position string
- **Updated** `positionTooltip()` - Now shows positions categorized by proficiency when hovering

### 4. App.cpp - Player Detail Screen
**Completely redesigned** position display section to show:
- Primary position
- Positions categorized by proficiency level
- Color-coded display:
  - Preferred (Green)
  - Natural (Light Green)  
  - Accomplished (Yellow-Green)
  - Competent (Orange)
  - Unconvincing (Red-Orange)

## Example: Phil Neville

### CSV Data:
```
Phil Neville;England;19;Man Utd;6;0;;;;21-01-1977;01-07-1996;DL;0;0;100;100;50;50;0;0;50;50;0;0;0;0;0;0
```

### Position Ratings:
- DR: 100
- DL: 100
- WBR: 50
- WBL: 50
- MR: 50
- ML: 50

### Display:
**Preferred:** DL (primary position from CSV)

**Natural:** DR (rating = 100)

**Competent:** WBR, WBL, MR, ML (rating = 50)

## Example: Roy Keane

### CSV Data:
```
Roy Keane;...;DMC;0;0;0;0;100;0;0;100;0;0;0;0;0;0;0;0
```

### Position Ratings:
- WBR: 100
- MC: 100

### Display:
**Preferred:** DM (primary position - "DMC" maps to DM)

**Natural:** WBR, MC (rating = 100)

## Visual Layout

### Player Detail Screen:

```
???????????????????????????????????????????????????????????
?  [< Back]  Player Name                                  ?
???????????????????????????????????????????????????????????
? ??????????????????????????? ?????????????????????????? ?
? ? Player Information      ? ? Position Map           ? ?
? ?                         ? ?                        ? ?
? ? Primary Position: DL    ? ?    [Pitch Visual]      ? ?
? ? Shirt Number: 18        ? ?                        ? ?
? ? Overall: 75             ? ?    Showing all         ? ?
? ?                         ? ?    playable positions  ? ?
? ? Positions by Proficiency? ?                        ? ?
? ?                         ? ?                        ? ?
? ? Preferred              ? ?                        ? ?
? ?   DL                   ? ?                        ? ?
? ?                         ? ?                        ? ?
? ? Natural                ? ?                        ? ?
? ?   DR                   ? ?                        ? ?
? ?                         ? ?                        ? ?
? ? Competent              ? ?                        ? ?
? ?   WBR, WBL, MR, ML     ? ?                        ? ?
? ?                         ? ?                        ? ?
? ? Attributes             ? ?                        ? ?
? ? Pace: 15  Shooting: 8  ? ?                        ? ?
? ? ...                     ? ?                        ? ?
? ??????????????????????????? ?????????????????????????? ?
???????????????????????????????????????????????????????????
```

## Hover Tooltips

Hovering over a player's position in any screen now shows:
```
Positions by proficiency:
?????????????????????????
Preferred: DL
Natural: DR
Competent: WBR, WBL, MR, ML
```

## Benefits

1. **Clear Position Understanding** - Immediately see which positions are natural vs. competent
2. **Better Team Management** - Make informed decisions about player positioning
3. **Visual Feedback** - Color coding makes it easy to assess at a glance
4. **Accurate Data** - Directly reflects CSV position ratings (0-100 scale)
5. **Consistent Display** - Same categorization used in tooltips and detail screens

## Testing

To verify:
1. Launch game
2. Database ? Search "Phil Neville"
3. Click name ? Player Detail
4. Check positions display:
   - **Preferred:** DL (green)
   - **Natural:** DR (light green)
   - **Competent:** WBR, WBL, MR, ML (orange)

5. Search "Roy Keane"
6. Check positions:
   - **Preferred:** DM (from "DMC" in CSV)
   - **Natural:** WBR, MC

## Build Status
? **Build Successful** - All changes compile without errors
