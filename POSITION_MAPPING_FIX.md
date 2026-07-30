# Player Position Mapping Fix

## Issue
Players were showing incorrect playable positions. For example, Roy Keane (who should only play DMC and MC according to the CSV data) was showing as able to play DC, DR, DMC, MC, MR and other positions.

## Root Cause
The previous implementation used a flawed approach:
1. It calculated **role ratings** (D, DM, M, AM, F) by taking the MAX of all sided positions in that role
2. It calculated **side ratings** (Right, Centre, Left) by taking the MAX across ALL roles
3. It then generated playable positions by combining every role where rating > 0 with every side where rating > 0

This caused cross-contamination. For example:
- If a player had high ratings for DMC (100) and MC (100)
- The code would see: Role M rating = 100, Role DM rating = 100
- Side Centre rating = max(DC, DMC, MC, AMC, FC) = 100
- Side Right rating = max(DR, MR, AMR, FR) = some value if any exists
- Then it would add: DM+Centre=DMC, DM+Right=DMR (wrong!), M+Centre=MC, M+Right=MR (wrong!)

## Solution
Changed to directly use individual position ratings instead of aggregating them:

### Before:
```cpp
int rD = std::max({DR, DC, DL, WBR, WBL});  // Aggregate all defender positions
int rM = std::max({MR, MC, ML});             // Aggregate all midfielder positions
// Then combine with generic side ratings to generate positions
```

### After:
```cpp
int rDR = pos({"defenderright"});            // Read each position individually
int rDC = pos({"defendercentral"});
int rDM = pos({"defensivemidfielder"});
int rMC = pos({"midfieldercentral"});
int rMR = pos({"midfielderright"});
// Add only positions with actual ratings > 0
if (rDM > 0) playablePositions.push_back(DM);
if (rMC > 0) playablePositions.push_back(MC);
```

## Changes Made

### Database.cpp (lines 338-420)

**Removed:**
- Role aggregation logic (calculating rD, rM, rAM, rF as max of sided positions)
- Side rating derivation (calculating sR, sL, sC across all roles)
- Complex nested loop combining roles with sides
- `sidedPos()` helper function
- `RoleRow` struct and arrays

**Added:**
- Individual position rating reads for all 16 positions (GK, DR, DC, DL, WBR, WBL, DM, MR, MC, ML, AMR, AMC, AML, FR, FC, FL)
- Simple threshold-based position addition (if rating > 0, add position)
- Updated primary position calculation to find the highest-rated individual position

## Expected Results

For Roy Keane (CSV data shows DMC:100, MC:100):
- **Before fix:** GK, DC, DR, DM, DMC, MC, MR (and possibly others)
- **After fix:** DMC, MC (only positions with actual ratings)

## Benefits

1. **Accurate position mapping** - Players can only play positions they have ratings for
2. **No cross-contamination** - A player's defensive ratings won't make them playable in midfield positions
3. **Simpler logic** - Direct mapping instead of complex role+side combinations
4. **Matches CSV data** - Playable positions now directly reflect the CSV position ratings

## Testing

To verify the fix:
1. Load the game
2. Go to Database ? Search for "Roy Keane"
3. Click on Roy Keane to view player details
4. Check "Can Play" positions - should only show DMC, MC (or positions with ratings > 0 in CSV)

## Compatibility

This fix maintains compatibility with:
- ? FM/CM style exports with individual position columns
- ? Explicit best position (column 11) reading
- ? Legacy database format (unchanged)
- ? Missing position data handling (positions with rating 0 are excluded)

## Related Files
- `NostalgiaManager/src/data/Database.cpp` - Position loading logic
- `NostalgiaManager/data/PlayersDB.csv` - Player database
- Test player: Roy Keane (row 11437)
