# Position-Specific Power Ranking System

## Overview

The power ranking system evaluates players based on **position-specific key attributes** rather than generic overall ability. This ensures that team selection prioritizes players who excel at the specific skills needed for each position.

## How It Works

### 1. Position Power Ranking Formula

Each position has a weighted formula that emphasizes the most important attributes:

```cpp
double PositionPowerRanking(const Player& p, Position pos)
```

- Returns a power score based on:
  - **Key attribute weights** (different per position)
  - **Position rating multiplier** (0-100% based on player's ability to play that position)
- Returns `0.0` if the player cannot play the position

### 2. Team Selection Algorithm

The `autoSelectXI()` function uses a two-pass approach:

#### Pass 1: Primary Positions
- Sorts players by overall ability (for tie-breaking)
- Assigns each player to their **primary position** if it exists in the formation
- Uses power ranking to validate suitability

#### Pass 2: Remaining Positions
- For each unfilled position, selects the player with the **highest power ranking** for that specific position
- This ensures specialists are preferred over versatile players with weaker position-specific skills

#### Fallback Passes
- Role-based selection if needed
- Generic ability-based selection as last resort

### 3. Substitute Selection

The `autoOrderSubstitutes()` function also uses power rankings:
- **GK**: Selected by GK power ranking
- **DEF**: Selected by DC (center-back) power ranking
- **MID**: Selected by MC (central midfielder) power ranking  
- **ATT**: Selected by FC (forward) power ranking
- **5th sub**: Highest overall ability among remaining players

## Position-Specific Attribute Weights

### Goalkeeper (GK)
**Key attributes:**
- Goalkeeping × 3.0
- Positioning × 1.5
- Aggression × 0.5
- Determination × 0.5

### Defenders

#### Full-backs (DR, DL, WBR, WBL)
**Key attributes:**
- Tackling × 2.0
- Positioning × 2.0
- Marking × 1.5
- Pace × 1.5
- Stamina × 1.5
- Passing × 1.0

#### Center-backs (DC)
**Key attributes:**
- Tackling × 2.5
- Marking × 2.5
- Positioning × 2.0
- Heading × 2.0
- Strength × 1.5
- Jumping × 1.0

### Midfielders

#### Defensive Midfielder (DM)
**Key attributes:**
- Tackling × 2.5
- Positioning × 2.0
- Passing × 2.0
- Stamina × 1.5
- Marking × 1.5
- Determination × 1.0

#### Central Midfielder (MC)
**Key attributes:**
- Passing × 2.5
- Technique × 2.0
- Stamina × 2.0
- Creativity × 1.5
- Tackling × 1.5
- Positioning × 1.0

#### Wide Midfielders (MR, ML)
**Key attributes:**
- Pace × 2.0
- Dribbling × 2.0
- Technique × 1.5
- Passing × 1.5
- Stamina × 1.5
- Creativity × 1.0
- Flair × 1.0

#### Attacking Midfielder (AMC)
**Key attributes:**
- Creativity × 2.5
- Technique × 2.5
- Passing × 2.0
- Dribbling × 2.0
- Shooting × 1.5
- Flair × 1.0
- OffTheBall × 1.0

#### Wide Attacking Midfielders (AMR, AML)
**Key attributes:**
- Dribbling × 2.5
- Pace × 2.0
- Creativity × 2.0
- Technique × 1.5
- Shooting × 1.5
- Flair × 1.5
- OffTheBall × 1.0

### Forwards (FC, FR, FL)
**Key attributes:**
- Shooting × 3.0
- OffTheBall × 2.5
- Technique × 2.0
- Pace × 1.5
- Heading × 1.0
- Dribbling × 1.0

## Examples

### Roy Keane (Man Utd)
- **Primary Position:** DMC
- **Can also play:** MC with 100 rating
- **In a 4-4-2 (no DMC):**
  - Won't be selected in Pass 1 (primary position not in formation)
  - **Will be selected in Pass 2** for MC based on high power ranking
  - His tackling, positioning, and passing make him excellent for MC

### Paul Scholes (Man Utd)
- **Primary Position:** MC
- **Can also play:** AMC, FC
- **In a 4-4-2:**
  - Selected in Pass 1 for MC (primary position)
  - High passing, technique, and creativity give excellent MC power ranking

### Brian Laudrup (Rangers)
- **Primary Position:** AMR
- **Can also play:** MR, ML, AML, etc. with 100 rating
- **In a 4-4-2 (no AMR):**
  - Won't be selected in Pass 1
  - **Will be selected in Pass 2** for MR
  - His dribbling, pace, and creativity give him the highest MR power ranking

## Benefits

1. **Position-appropriate selection**: Players are evaluated on skills that matter for their role
2. **Best players on the pitch**: High-ability players won't be left on the bench
3. **Tactical suitability**: Roy Keane will play MC even though his primary is DMC
4. **Specialist preference**: A specialist winger beats a versatile midfielder for wing positions
5. **Realistic team sheets**: Matches how real managers pick teams

## Implementation Files

- **Team.h**: `PositionPowerRanking()` function
- **Team.cpp**: `autoSelectXI()` and `autoOrderSubstitutes()` functions
