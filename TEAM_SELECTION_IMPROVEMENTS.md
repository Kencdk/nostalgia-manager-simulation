# Auto Team Selection Power Ranking System

## Overview
The auto-selection system for starting XI now uses a sophisticated power ranking system that ensures:
1. **Players are NEVER selected for positions they cannot play** (non-natural positions)
2. **Primary position specialists are prioritized** for their natural positions
3. **Power rankings** are calculated based on position-specific attributes
4. **Team balance** is maintained through multi-pass selection

## How It Works

### Power Ranking Calculation
Each player gets a position-specific power ranking calculated by `PositionPowerRanking(player, position)`:

- **Base Score**: Weighted sum of relevant attributes for that position
- **Position Rating Multiplier**: Player's position rating (0-100) is applied as a multiplier
- **Result**: Returns 0 if player cannot play that position, otherwise returns weighted score

### Position-Specific Attribute Weights

#### Goalkeepers (GK)
- Goalkeeping: 3.0
- Positioning: 1.5
- Aggression: 0.5
- Determination: 0.5

#### Center Backs (DC)
- Tackling: 2.5
- Marking: 2.5
- Positioning: 2.0
- Heading: 2.0
- Strength: 1.5
- Jumping: 1.0
- Determination: 0.5

#### Full-backs (DR, DL, WBR, WBL)
- Tackling: 2.0
- Positioning: 2.0
- Marking: 1.5
- Pace: 1.5
- Stamina: 1.5
- Passing: 1.0
- Determination: 0.5

#### Defensive Midfielders (DM)
- Tackling: 2.5
- Positioning: 2.0
- Passing: 2.0
- Stamina: 1.5
- Marking: 1.5
- Determination: 1.0
- Strength: 0.5

#### Central Midfielders (MC)
- Passing: 2.5
- Technique: 2.0
- Stamina: 2.0
- Creativity: 1.5
- Tackling: 1.5
- Positioning: 1.0
- Determination: 0.5

#### Wide Midfielders (MR, ML)
- Pace: 2.0
- Dribbling: 2.0
- Technique: 1.5
- Passing: 1.5
- Stamina: 1.5
- Creativity: 1.0
- Flair: 1.0

#### Attacking Midfielders (AMC)
- Creativity: 2.5
- Technique: 2.5
- Passing: 2.0
- Dribbling: 2.0
- Shooting: 1.5
- Flair: 1.0
- Off The Ball: 1.0

#### Wide Attacking Midfielders (AMR, AML)
- Dribbling: 2.5
- Pace: 2.0
- Creativity: 2.0
- Technique: 1.5
- Shooting: 1.5
- Flair: 1.5
- Off The Ball: 1.0

#### Forwards (FC, FR, FL)
- Shooting: 3.0
- Off The Ball: 2.5
- Technique: 2.0
- Pace: 1.5
- Heading: 1.0
- Dribbling: 1.0
- Determination: 0.5

## Selection Algorithm

### Pass 1: Natural Position Specialists (30% bonus)
- For each formation position, find the best player whose **primary position** matches
- Only considers players who can actually play the position (`canPlay(pos) == true`)
- Gives 30% bonus to players playing in their natural position
- This ensures specialists are prioritized for their best positions

### Pass 2: Best Available by Power Ranking
- For remaining positions, select the best available player based on power ranking
- **Strict requirement**: Player MUST be able to play the position
- Still gives 30% bonus if it's their primary position
- Balances team needs with individual abilities

### Pass 3: Same Role Fallback (10% bonus)
- If positions still unfilled, look for players from the same role (e.g., any defender for DC)
- **Strict requirement**: Player MUST be able to play the position
- Gives 10% bonus if their primary role matches the target role
- Helps fill positions when squad depth is limited

### Pass 4: Any Capable Player
- Final attempt to fill positions with any player who can play there
- **Strict requirement**: Player MUST be able to play the position
- Pure power ranking without bonuses

### Pass 5: Absolute Fallback
- Only triggers if previous passes failed (rare with proper squad depth)
- Assigns best remaining players to positions they CAN play
- Searches for valid position assignments to avoid invalid selections

## Key Improvements

1. **No Non-Natural Positions**: Every selection checks `canPlay(pos)` to ensure the player can actually play that position

2. **Specialist Priority**: Players in their primary position get a 30% scoring bonus in Pass 2

3. **Balanced Team**: Multi-pass approach ensures:
   - Natural specialists fill their positions first
   - Best overall players are selected next
   - Positional requirements are always respected
   - Squad depth limitations are handled gracefully

4. **Position Rating Integration**: The position rating (0-100) multiplier ensures players are less effective in secondary positions even if they can play them

## Example

For a 4-4-2 formation:
- **GK**: Best goalkeeper (Goalkeeping attribute is key)
- **DR, DC, DC, DL**: Best defenders who can play these positions (Tackling, Marking prioritized for DC; Pace, Stamina for full-backs)
- **MR, MC, MC, ML**: Best midfielders (Passing, Technique for MC; Pace, Dribbling for wide positions)
- **FC, FC**: Best forwards (Shooting, Off The Ball are most important)

If a DC-specialist is available but a better overall player who can play DC is not a natural DC, the system will:
1. First place the DC-specialist in Pass 1 (with 30% bonus)
2. Then consider the versatile player for other positions in Pass 2
3. This ensures specialists aren't displaced unless the alternative is significantly better

## Benefits

- **Realistic team selection**: Players in natural positions perform better
- **Flexible squad management**: Can handle various squad compositions
- **No invalid assignments**: Never places players where they can't play
- **Transparent logic**: Clear priority system from specialists to versatile players
- **Balanced team**: Considers both individual ability and team needs
