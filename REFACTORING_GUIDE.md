# App.cpp Refactoring Guide

## Overview
This document outlines the refactoring of App.cpp into separate component files for better code organization.

## Files Created

### 1. UIHelpers.h / UIHelpers.cpp
**Purpose**: Common UI utility functions and widgets used across all screens

**Content**:
- String utilities: `lower()`, `contains()`, `parseScore()`
- Player utilities: `playerOverall()`, `shortName()`, `playablePosStr()`, `positionTooltip()`
- Visual utilities: `shade()`, `parseColor()`
- UI widgets: `tintButton()`, `panelHeader()`, `squadPanel()`
- Tactics helpers: `bestStarterFor()`, `tacticRow()`, `kFormations[]`

**Status**: ? Created

### 2. Tactics.h / Tactics.cpp
**Purpose**: Tactics screen rendering and logic

**Content**:
- `TacticsScreen::render()` - Main tactics screen rendering
- `TacticsScreen::openTactics()` - Initialize tactics screen
- Squad list management
- Formation pitch visualization
- Player drag-and-drop
- Team instructions panel
- Player instructions panel

**Status**: ? Created

### 3. Match.h / Match.cpp
**Purpose**: Match screen rendering and playback

**Content** (TO BE CREATED):
- `MatchScreen::render()` - Main match screen rendering
- `MatchScreen::startMatch()` - Initialize match simulation
- `MatchScreen::drawPitch()` - Pitch visualization
- Match event processing
- Player statistics tracking
- Playback controls

**Lines in App.cpp**: ~1330-2000 (approximately)

### 4. PlayerDetail.h / PlayerDetail.cpp
**Purpose**: Player detail screen

**Content** (TO BE CREATED):
- `PlayerDetailScreen::render()` - Player detail screen rendering
- `PlayerDetailScreen::openPlayerDetail()` - Initialize player detail view
- Player attributes display
- Position visualization
- Player stats

**Lines in App.cpp**: ~2414-2500 (approximately)

## Integration Steps

### Step 1: Update App.cpp includes
```cpp
#include "UIHelpers.h"
#include "Tactics.h"
#include "Match.h"         // To be created
#include "PlayerDetail.h"   // To be created
```

### Step 2: Remove duplicate code from App.cpp
Remove the following sections from App.cpp (they're now in UIHelpers.cpp):
- Lines 17-127 (utility functions)
- Lines 130-297 (panelHeader, squadPanel)
- Lines 300-327 (bestStarterFor, tacticRow, kFormations)

### Step 3: Update function calls in App.cpp
- Replace `renderTactics()` implementation with delegation to `TacticsScreen::render(this)`
- Replace `openTactics()` implementation with delegation to `TacticsScreen::openTactics(this, team, returnTo)`
- Similar replacements for Match and PlayerDetail when created

### Step 4: Update App.h
- Move `Screen` enum to public section (done)
- Add friend class declarations (done)
- Ensure necessary members are accessible

### Step 5: Build system updates
Add new .cpp files to the Visual Studio project:
- UIHelpers.cpp
- Tactics.cpp
- Match.cpp (when created)
- PlayerDetail.cpp (when created)

## Next Steps (Manual Implementation Required)

### Create Match.h and Match.cpp
Extract the following functions from App.cpp:
1. `void startMatch(Team* home, Team* away)` - Lines ~1330-1525
2. `void renderMatch()` - Lines ~1620-1997
3. `void drawPitch(ImVec2 pos, ImVec2 size, const Frame* f, const Frame* nextF, float t)` - Lines ~2000-2153

### Create PlayerDetail.h and PlayerDetail.cpp
Extract the following functions from App.cpp:
1. `void openPlayerDetail(const Player* player, Screen returnTo)` - Lines ~2414-2422
2. `void renderPlayerDetail()` - Lines ~2424-2560 (approximately)

### Update App.cpp
1. Add includes for new headers at top
2. Remove now-duplicate utility functions
3. Update `render()` to delegate to component classes:
```cpp
void App::render() {
    switch (screen_) {
        case Screen::Main: renderMain(); break;
        case Screen::Friendly: renderFriendly(); break;
        case Screen::Tactics: TacticsScreen::render(this); break;  // DELEGATED
        case Screen::Match: MatchScreen::render(this); break;       // DELEGATED
        case Screen::Database: renderDatabase(); break;
        case Screen::Career: renderCareer(); break;
        case Screen::Data: renderData(); break;
        case Screen::About: renderAbout(); break;
        case Screen::PlayerDetail: PlayerDetailScreen::render(this); break;  // DELEGATED
    }
}
```

4. Update `openTactics()` and `openPlayerDetail()`:
```cpp
void App::openTactics(Team* team, Screen returnTo) {
    TacticsScreen::openTactics(this, team, returnTo);
}

void App::openPlayerDetail(const Player* player, Screen returnTo) {
    PlayerDetailScreen::openPlayerDetail(this, player, returnTo);
}
```

## Benefits

1. **Better Organization**: Each screen/component in its own file
2. **Easier Maintenance**: Smaller, focused files are easier to understand and modify
3. **Reduced Compilation Time**: Changes to one component don't require recompiling everything
4. **Better Separation of Concerns**: Clear boundaries between different UI screens
5. **Reusability**: UIHelpers can be used across all components

## File Size Reduction

- **Original App.cpp**: 2336 lines
- **After refactoring**:
  - UIHelpers.cpp: ~330 lines
  - Tactics.cpp: ~700 lines
  - Match.cpp: ~650 lines (estimated)
  - PlayerDetail.cpp: ~140 lines (estimated)
  - App.cpp (remaining): ~516 lines

**Total**: Same functionality, but split into 5 manageable files instead of one huge file.

## Testing

After refactoring:
1. Build the project
2. Test each screen:
   - Main menu
   - Friendly Match selection
   - Tactics screen (both teams)
   - Match playback
   - Player details (from various screens)
   - Database search
   - Career mode

## Notes

- The `friend class` declarations in App.h allow component classes to access private members
- This maintains encapsulation while enabling clean separation
- All original functionality is preserved
- No changes to external interfaces or behavior
