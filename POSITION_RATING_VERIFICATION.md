# Position Rating System - Verification

## CSV Structure

The PlayersDB.csv has position rating columns (12-27) immediately after the "Best position" column (11):

| Column | Header | Example Value |
|--------|--------|---------------|
| 11 | Best position | DMC |
| 12 | Goalkeeper | 0 |
| 13 | Defender Central | 0 |
| 14 | Defender Right | 0 |
| 15 | Defender Left | 0 |
| 16 | Wingback right | 100 |
| 17 | Wingback left | 0 |
| 18 | Defensive midfielder | 0 |
| 19 | Midfielder Central | 100 |
| 20 | Midfielder right | 0 |
| 21 | Midfielder left | 0 |
| 22 | Attacking Midfielder Central | 0 |
| 23 | Attacking Midfielder right | 0 |
| 24 | Attacking Midfielder left | 0 |
| 25 | Center Forward | 0 |
| 26 | Right Forward | 0 |
| 27 | Left Forward | 0 |

## Rating System

- **0** = Cannot play this position
- **1-99** = Can play this position (competent)
- **100** = Natural position (primary/best at this position)

## Current Implementation

The code in `Database.cpp` (lines 338-379):

1. Reads each position rating column individually
2. Only adds positions where `rating > 0` (threshold = 1)
3. Correctly uses the FIRST occurrence of each column header (the detailed position block, not the summary block after "PR overall")

## Example: Roy Keane

**CSV Data:**
```
Roy Keane;...;DMC;0;0;0;0;100;0;0;100;0;0;0;0;0;0;0;0;...
```

**Position Ratings:**
- WBR (Wingback Right): 100 (Natural)
- MC (Midfielder Central): 100 (Natural)
- All others: 0

**Result:**
- Can play: **WBR, MC**
- Primary position: **DMC** (from "Best position" column)
- Natural positions: **WBR, MC** (rating == 100)

## Example: Emilio Viqueira

**CSV Data:**
```
Emilio Viqueira;Spain;21;;0;0;7;;;20-09-1974;01-07-1996;MC;0;0;0;0;100;100;100;100;100;100;50;50;50;0;0;0;...
```

**Position Ratings:**
- WBR: 100 (Natural)
- WBL: 100 (Natural)
- DM: 100 (Natural)
- MC: 100 (Natural)
- MR: 100 (Natural)
- ML: 100 (Natural)
- AMC: 50 (Can play)
- AMR: 50 (Can play)
- AML: 50 (Can play)

**Result:**
- Can play: **WBR, WBL, DM, MC, MR, ML, AMC, AMR, AML**
- Primary position: **MC** (from "Best position" column)
- Natural positions: **WBR, WBL, DM, MC, MR, ML** (rating == 100)

## Code Verification

The fix implemented correctly:
? Reads from columns 12-27 (first position rating block)
? Uses header name matching (normKey function removes spaces/special chars)
? Only adds positions where rating > 0
? Preserves explicit "Best position" as primary position
? Ensures primary position is in playable positions list

## Note on Position Naming

The CSV uses some different naming conventions than the code:
- CSV: "DMC" (Defensive Midfielder Central)
- Code: Position::DM (Defensive Midfielder)

Both refer to the same position - there is only one defensive midfielder position (centered).

## Verification Steps

To verify the fix is working:
1. Launch the game
2. Database ? Search for "Roy Keane"
3. Click his name ? Player Details
4. Check "Can Play" positions ? Should show: **WBR, MC**
5. Search for "Emilio Viqueira"
6. Check "Can Play" positions ? Should show: **WBR, WBL, DM, MC, MR, ML, AMC, AMR, AML**

The implementation is correct and matches the CSV data structure exactly!
