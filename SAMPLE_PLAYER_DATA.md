# Sample Player Data with Bio Information

## Example CSV Format

Below is an example of how player data with the new bio fields should be structured:

```csv
firstname,secondname,age,dateofbirth,nationality,internationalcaps,internationalgoals,position,currentability,pace,stamina,strength,jumping,passing,shooting,technique,dribbling,heading,positioning,offtheball,marking,tackling,creativity,determination,influence,aggression,flair,goalkeeping
David,Beckham,28,02/05/1975,England,87,17,MR,180,14,15,11,12,19,16,18,17,13,16,15,8,9,17,18,17,11,16,1
Zinedine,Zidane,31,23/06/1972,France,108,31,AMC,195,12,14,13,13,19,15,20,18,14,17,16,10,11,20,17,19,12,19,1
Ronaldo,de Lima,27,22/09/1976,Brazil,97,62,FC,190,17,15,15,14,14,19,19,19,13,18,19,6,8,18,19,17,13,20,1
Paolo,Maldini,35,26/06/1968,Italy,126,7,DL,185,13,16,14,13,16,10,17,13,17,18,18,19,18,15,18,18,13,11,1
Peter,Schmeichel,36,18/11/1963,Denmark,129,1,GK,190,9,15,16,14,11,8,12,8,9,16,9,10,11,14,19,17,12,10,20
```

## Field Descriptions

### Bio Fields
- **firstname** / **secondname**: Player's name components
- **age**: Current age (calculated or fixed reference date)
- **dateofbirth**: Birth date in DD/MM/YYYY format
- **nationality**: Player's country/nationality
- **internationalcaps**: Number of international appearances
- **internationalgoals**: Goals scored for national team

### Position & Ability
- **position**: Best position (e.g., MC, FC, DR, GK)
- **currentability**: Overall ability rating (0-200 scale)

### Attribute Fields
All attributes on 1-20 scale:

**Technical**
- passing, shooting, technique, dribbling, heading

**Physical**
- pace, stamina, strength, jumping

**Tactical**
- positioning, offtheball, marking, tackling

**Mental**
- creativity, determination, influence, aggression, flair

**Specialized**
- goalkeeping (only relevant for GK)

## Alternative Column Names

The system recognizes multiple column name variants:

### Date of Birth
- `dateofbirth`, `dob`, `birthday`

### Nationality
- `nationality`, `nation`, `nat`

### International Caps
- `internationalcaps`, `intcaps`, `caps`

### International Goals
- `internationalgoals`, `intgoals`, `intlgoals`

## Example Without Bio Data

If bio data is not available, the system gracefully handles it:

```csv
firstname,secondname,position,currentability,pace,shooting,passing,...
John,Smith,MC,150,13,12,15,...
```

Bio fields will be empty/zero and won't be displayed in the UI.

## Generating Test Data

To test the enhanced player detail screen, you can:

1. Create a CSV with the sample data above
2. Load it via the Data screen in the application
3. View any player to see the enhanced bio section
4. Compare players with full bio data vs. minimal data

## Notes

- **Age calculation**: If age is not provided, it could be calculated from date of birth and a reference date (e.g., start of 1997-98 season)
- **International data**: Only relevant for top-tier players; can be 0 for youth/lower league players
- **Date format**: DD/MM/YYYY is recommended but system should handle various formats
- **Missing data**: Any missing fields default to 0 or empty string and are hidden in UI
