# Half-Time Team Switching Implementation

## ? Teams Now Switch Ends at Half-Time

The match engine now properly swaps team ends at half-time, just like in real football.

### What Changed:

#### 1. **New `swapEnds()` Function** (`MatchEngine.cpp`)

Added a new function that executes at half-time to:

**a) Reverse attacking directions:**
```cpp
dir_[0] = -dir_[0];  // Home team reverses direction
dir_[1] = -dir_[1];  // Away team reverses direction
```

**b) Mirror all player positions:**
- Players' home positions are mirrored across the pitch center
- Column positions are inverted: `col ? (kCols + 1 - col)` (e.g., col 3 ? col 11)
- Players are reset to their new home positions on their own half
- All cooldowns and ball possession are cleared

#### 2. **Half-Time Sequence** (`MatchEngine.cpp`)

The match flow now includes the end swap:

```
First Half (minutes 1-45)
    ?
Half-Time
    ?
swapEnds() ? NEW
    ?
Second Half Kickoff (minutes 46-90)
```

### How It Works:

#### Before Second Half:
1. **Home team** was attacking towards column 13 (right) ? Now attacks towards column 1 (left)
2. **Away team** was attacking towards column 1 (left) ? Now attacks towards column 13 (right)

#### Player Repositioning:
- **Goalkeeper** at column 2 ? Moves to column 12 (mirrored)
- **Defenders** at columns 3-4 ? Move to columns 10-11
- **Midfielders** at columns 5-7 ? Move to columns 7-9
- **Forwards** at columns 9-11 ? Move to columns 3-5

All players start the second half **on their own defensive half**, properly positioned for the kickoff.

### Example:

**First Half:**
```
Home Team (Blue):
GK at col 1  ?  Attacks towards col 13 ?
Defender at col 3 ? Midfielder at col 6 ? Forward at col 10

Away Team (Red):
? Attacks towards col 1  Forward at col 4 ? Midfielder at col 8 ? Defender at col 11 ? GK at col 13
```

**Second Half (after swapEnds):**
```
Home Team (Blue):
? Attacks towards col 1  Forward at col 4 ? Midfielder at col 8 ? Defender at col 11 ? GK at col 13

Away Team (Red):
GK at col 1  ?  Attacks towards col 13 ?
Defender at col 3 ? Midfielder at col 6 ? Forward at col 10
```

### Key Features:

? **Proper Direction Reversal**: Both teams reverse their attacking direction  
? **Position Mirroring**: All players are repositioned symmetrically on the opposite end  
? **Own Half Start**: Players begin second half on their own defensive half  
? **Clean State**: Ball possession, cooldowns, and positions are reset  
? **Maintains Formation**: Players keep their tactical roles, just mirrored  

### Technical Details:

#### Direction Array:
```cpp
// First half:
dir_[0] = +1;  // Home attacks right (towards col 13)
dir_[1] = -1;  // Away attacks left (towards col 1)

// Second half (after swap):
dir_[0] = -1;  // Home attacks left (towards col 1)
dir_[1] = +1;  // Away attacks right (towards col 13)
```

#### Position Mirroring Formula:
```cpp
newColumn = kCols + 1 - oldColumn
// where kCols = 13 (columns 1-13)
```

Examples:
- Column 1 ? Column 13 (GK positions swap)
- Column 3 ? Column 11 (Defender positions)
- Column 7 ? Column 7 (Center stays center)

#### Row Positions:
Row positions (lateral, 0-8) remain unchanged - players maintain their width positioning (left/center/right).

### Impact on Gameplay:

1. **More Realistic**: Matches now follow real football rules with teams switching ends
2. **Fair Balance**: Both teams get equal opportunity attacking from each end
3. **Strategic**: Wind, sun, or pitch conditions would affect both teams equally
4. **Visual Authenticity**: Match rendering shows proper team positioning
5. **Tactical Positioning**: Players correctly positioned on their defensive half to start each half

### Files Modified:

- **`MatchEngine.cpp`**: Added `swapEnds()` function, integrated into half-time sequence
- **`MatchEngine.h`**: Added `swapEnds()` declaration

### Code Location:

**Function**: `MatchEngine::swapEnds()`  
**Called From**: `MatchEngine::simulate()` between first and second half  
**Timing**: After "Half time" log event, before second half kickoff

---

**Result**: Teams now properly switch ends at half-time, with all players repositioned to their base positions on the opposite side of the pitch! ???
