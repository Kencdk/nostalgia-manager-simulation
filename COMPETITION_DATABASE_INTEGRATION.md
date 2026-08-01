# Competition Database Integration

## Overview
The Competitions.csv database has been successfully integrated into the Nostalgia Manager system. This database defines competition-specific settings including match scheduling, bench sizes, and substitution rules.

## Database Structure

### Competition Fields
- **League**: Competition name (e.g., "Premier League")
- **Nation**: Country/nation
- **Hierarchy**: League tier (1 = top division, 2 = second division, etc.)
- **Teams**: Number of teams in the competition
- **Rounds**: Total number of rounds/matchdays
- **Bench**: Number of substitutes allowed on the bench (default: 5)
- **Subs**: Number of substitutions allowed per match (default: 3)
- **PauseStart**: Week number when winter break starts (0 = no break)
- **PauseEnd**: Week number when winter break ends
- **TransferWindowSummerStart**: Week when summer transfer window opens
- **TransferWindowSummerEnd**: Week when summer transfer window closes
- **TransferWindowWinterStart**: Week when winter transfer window opens (0 = none)
- **TransferWindowWinterEnd**: Week when winter transfer window closes
- **Default Match Day**: Default day of the week for matches (e.g., "Saturday")
- **Round 1-38**: Week number for each specific round

## Code Changes

### New Files
None - integrated into existing Database class

### Modified Files

#### `Database.h`
- Added `Competition` struct with all competition-related fields
- Added `std::map<std::string, Competition> competitions` to Database class
- Added `bool loadCompetitions(const std::string& path)` method
- Added `Competition* findCompetition(const std::string& league)` method

#### `Database.cpp`
- Implemented `loadCompetitions()` to parse the CSV file
- Implemented `findCompetition()` to look up competitions by league name
- Updated `Database::load()` to call `loadCompetitions()`
- Added support for "competitions" key in datasources.cfg

## Usage Examples

### Getting Competition Settings
```cpp
// Get competition for a league
Competition* comp = db.findCompetition("Premier League");
if (comp) {
    int benchSize = comp->bench;  // Number of subs on bench
    int maxSubs = comp->subs;     // Number of substitutions allowed
    int totalRounds = comp->rounds;  // Total rounds in season
}
```

### Getting Match Week for a Round
```cpp
Competition* comp = db.findCompetition(leagueName);
if (comp && roundNumber < comp->roundWeeks.size()) {
    int weekNumber = comp->roundWeeks[roundNumber];
    // Use this to determine the date of the match
}
```

### Checking Transfer Windows
```cpp
Competition* comp = db.findCompetition(leagueName);
if (comp) {
    bool summerWindowOpen = (currentWeek >= comp->transferWindowSummerStart && 
                             currentWeek <= comp->transferWindowSummerEnd);
    bool winterWindowOpen = (comp->transferWindowWinterStart > 0 && 
                             currentWeek >= comp->transferWindowWinterStart && 
                             currentWeek <= comp->transferWindowWinterEnd);
}
```

## Next Steps for Integration

### 1. Update Career Mode Match Scheduling
Replace the current hardcoded match scheduling with competition-based scheduling:
```cpp
Competition* comp = app->db_.findCompetition(app->careerLeagueName_);
if (comp && roundNumber < comp->roundWeeks.size()) {
    int weekNumber = comp->roundWeeks[roundNumber];
    // Calculate actual date from week number
    // Update app->currentWeek_, app->currentMonth_, app->currentDay_
}
```

### 2. Update Tactics Screen Bench/Subs Limits
In `Tactics.cpp`, replace hardcoded values with competition settings:
```cpp
Competition* comp = app->db_.findCompetition(teamLeague);
int maxBench = comp ? comp->bench : 5;
int maxSubs = comp ? comp->subs : 3;
// Show only maxBench substitutes
// Enforce maxSubs substitution limit during match
```

### 3. Update Match Engine Substitution Rules
In match simulation, use competition-specific substitution limits:
```cpp
Competition* comp = app->db_.findCompetition(leagueName);
int subsAllowed = comp ? comp->subs : 3;
// Enforce substitution limit during match
```

### 4. Add Transfer Window Logic
Use competition transfer window data to enable/disable transfers:
```cpp
Competition* comp = app->db_.findCompetition(app->careerLeagueName_);
bool canTransfer = false;
if (comp) {
    int week = app->currentWeek_;
    canTransfer = (week >= comp->transferWindowSummerStart && week <= comp->transferWindowSummerEnd) ||
                  (comp->transferWindowWinterStart > 0 && 
                   week >= comp->transferWindowWinterStart && week <= comp->transferWindowWinterEnd);
}
```

## File Location
- **Competition Data**: `NostalgiaManager/data/Competetions.csv`
- **Database Code**: `NostalgiaManager/src/data/Database.h` and `Database.cpp`

## Notes
- The competitions file is optional - if not present, the system will use default values
- Default values: bench=5, subs=3, defaultMatchDay="Saturday"
- Empty or invalid competition entries are skipped during loading
- Week numbers in the CSV represent calendar weeks (1-52)
