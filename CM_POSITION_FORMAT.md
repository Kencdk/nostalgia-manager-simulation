# Championship Manager Position Format

## Overview
Players' positions are now displayed in Championship Manager style, showing their versatility in a compact format that groups positions by role and side.

## Format Examples

### Simple Cases
| Playable Positions | CM Format | Description |
|-------------------|-----------|-------------|
| MC | `M C` | Central midfielder only |
| GK | `GK` | Goalkeeper |
| DC | `D C` | Centre-back |
| FC | `F C` | Centre-forward |

### Multiple Sides (Same Role)
| Playable Positions | CM Format | Description |
|-------------------|-----------|-------------|
| MC, MR | `M C/R` | Midfielder - can play centre or right |
| DC, DR, DL | `D C/R/L` | Defender - can play all positions |
| FC, FR | `F C/R` | Forward - can play centre or right |
| ML, MR | `M R/L` | Wide midfielder - both flanks |

### Multiple Roles (Same Sides)
| Playable Positions | CM Format | Description |
|-------------------|-----------|-------------|
| MC, AMC | `M/AM C` | Versatile midfielder - both central roles |
| MR, AMR | `M/AM R` | Right-sided midfielder - box to box |
| ML, AML | `M/AM L` | Left-sided midfielder - box to box |

### Complex Versatility
| Playable Positions | CM Format | Description |
|-------------------|-----------|-------------|
| MC, MR, AMC, AMR | `M/AM C/R` | Attacking/defensive midfielder, centre/right |
| MR, AMR, ML, AML | `M/AM R/L` | Wide midfielder, both flanks |
| MR, AMR, ML, AML, FC, FR, FL | `M/AM R/L, F C/R/L` | Wide attacker, can also play forward |
| DR, DC, DL, MR, ML | `D C/R/L, M R/L` | Versatile defender who can push forward |

### Real Player Examples

**David Beckham** (MC, MR, AMR)
- CM Format: `M C/R, AM R`
- Can play central or right midfield, plus right attacking midfield

**Brian Laudrup** (MR, AMR, ML, AML, FC, FR, FL)
- CM Format: `M/AM R/L, F C/R/L`
- Ultimate versatile attacker - can play anywhere in attack

**Patrick Vieira** (DM, MC)
- CM Format: `DM C, M C`
- Defensive or box-to-box midfielder

**Ryan Giggs** (ML, AML, FL)
- CM Format: `M/AM L, F L`
- Left-sided winger/wide forward

**Paolo Maldini** (DL, DC, WBL)
- CM Format: `D C/L, WB L`
- Versatile left-sided defender

## Implementation Details

### Grouping Algorithm
1. Group all playable positions by **Role** (GK, D, DM, M, AM, F)
2. For each role, collect all **Sides** (Centre, Right, Left)
3. Combine adjacent roles if they share the same sides:
   - M + AM ? `M/AM`
   - D + DM ? `D/DM`
   - AM + F ? `AM/F`
4. Format each role group with sides: `ROLE SIDES`
5. Separate multiple role groups with commas

### Side Order
Sides are always displayed in this order:
1. **C** (Centre)
2. **R** (Right)
3. **L** (Left)

Examples:
- Centre only: `M C`
- Centre and right: `M C/R`
- All three: `D C/R/L`
- Right and left (no centre): `M R/L`

### Role Order
Roles are displayed in positional order from attack to defense:
1. **F** (Forward)
2. **AM** (Attacking Midfielder)
3. **M** (Midfielder)
4. **DM** (Defensive Midfielder)
5. **D** (Defender)
6. **GK** (Goalkeeper)

Multiple role groups are separated by commas: `M/AM C/R, F C`

## Position Mapping

### Positions to Roles
- **GK** ? GK
- **DR, DC, DL, WBR, WBL** ? D
- **DM** ? DM
- **MR, MC, ML** ? M
- **AMR, AMC, AML** ? AM
- **FR, FC, FL** ? F

### Positions to Sides
- **Centre**: GK, DC, DM, MC, AMC, FC
- **Right**: DR, WBR, MR, AMR, FR
- **Left**: DL, WBL, ML, AML, FL

## Where It's Used

The CM format is now displayed in:

1. **Player Detail Screen** - Top bio section showing position
2. **Team Overview** - Squad list position column
3. **Tactics Screen** - Squad and substitutes lists
4. **Match Screens** - Squad panels showing team lineups

## Benefits

### Compact Display
- Shows full versatility in minimal space
- Groups related positions logically
- Easy to scan and understand at a glance

### Championship Manager Authenticity
- Matches the classic CM/FM format
- Familiar to long-time players
- Professional appearance

### Clear Communication
- Instantly shows if player is versatile or specialist
- Shows preferred sides (left/right/centre)
- Groups attacking/defensive capabilities

## Examples in Context

### Squad List Display
```
12  M C/R      D.Beckham
 7  M/AM R/L   R.Giggs
 8  M/AM C     P.Scholes
16  D C/R/L    R.Ferdinand
 1  GK         P.Schmeichel
```

### Player Detail Header
```
David Beckham (Manchester United)
Position: M C/R, AM R
Overall Rating: 85.3
```

## Technical Notes

### Function: `cmPositionFormat()`
Located in: `NostalgiaManager/src/ui/UIHelpers.cpp`

**Parameters:**
- `const Player& p` - Player to format positions for

**Returns:**
- `std::string` - CM-formatted position string

**Algorithm:**
1. Build map of Role ? Set of Sides from `playablePositions`
2. Iterate roles in order (GK, D, DM, M, AM, F)
3. Check if adjacent roles can be combined (same sides)
4. Format each role group with its sides
5. Join multiple groups with commas

### Integration Points
- `UIHelpers.h` - Function declaration
- `UIHelpers.cpp` - Implementation
- `PlayerDetail.cpp` - Used in player bio
- `TeamOverview.cpp` - Used in squad table
- `Tactics.cpp` - Used in squad/substitute lists
- `squadPanel()` - Used in match squad displays

## Future Enhancements

Possible improvements:
- Color coding by role (defenders blue, midfielders green, forwards red)
- Tooltips showing full position names on hover
- Position rating indicators (? for natural, ? for competent)
- Filtering/sorting by CM position format
