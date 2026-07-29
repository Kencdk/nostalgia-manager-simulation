# Sample Team Colors for TeamsDB.csv

This file provides examples of team color configurations for popular football clubs.
Copy these values into your TeamsDB.csv file.

## Format
Club;Team colour main 1;Team colour main 2;Away colour 1;Away colour 2

## English Premier League

### Traditional Big Six
Man Utd;Red;White;White;Black
Arsenal;Red;White;Yellow;Navy
Chelsea;Blue;White;Yellow;Blue
Liverpool;Red;White;Yellow;Red
Man City;Sky Blue;White;Navy;Sky Blue
Tottenham;White;Navy;Navy;White

### Other Premier League Teams
Newcastle;Black;White;White;Black
Everton;Blue;White;White;Blue
Aston Villa;Maroon;Sky Blue;Sky Blue;Maroon
West Ham;Maroon;Sky Blue;White;Maroon
Leicester;Blue;White;White;Blue
Leeds Utd;White;Blue;Blue;White
Southampton;Red;White;White;Red
Crystal Palace;Blue;Red;Red;Blue
Wolves;Gold;Black;White;Gold
Brighton;Blue;White;Yellow;Blue

## Spanish La Liga
Barcelona;Red;Blue;Yellow;Red
Real Madrid;White;Gold;Navy;White
Atletico Madrid;Red;White;White;Red
Valencia;Orange;White;White;Orange
Sevilla;White;Red;Red;White
Villarreal;Yellow;Navy;Blue;Yellow
Real Betis;Green;White;White;Green

## Italian Serie A
Juventus;Black;White;White;Black
AC Milan;Red;Black;White;Red
Inter Milan;Blue;Black;White;Blue
AS Roma;Red;Orange;White;Red
Napoli;Sky Blue;White;White;Sky Blue
Lazio;Sky Blue;White;White;Sky Blue
Fiorentina;Purple;White;White;Purple

## German Bundesliga
Bayern Munich;Red;White;White;Red
Borussia Dortmund;Yellow;Black;Black;Yellow
RB Leipzig;Red;White;White;Red
Bayer Leverkusen;Red;Black;White;Red
Schalke 04;Blue;White;White;Blue
Werder Bremen;Green;White;White;Green

## French Ligue 1
Paris SG;Blue;Red;White;Blue
Marseille;White;Sky Blue;Sky Blue;White
Lyon;White;Blue;Blue;White
Monaco;Red;White;White;Red
Lille;Red;White;White;Red

## Other European Leagues
Ajax;Red;White;White;Red
PSV;Red;White;White;Red
Porto;Blue;White;White;Blue
Benfica;Red;White;White;Red
Sporting;Green;White;White;Green
Celtic;Green;White;White;Green
Rangers;Blue;White;Red;Blue

## Color Scheme Patterns

### Classic Contrasts
- Red/White (Man Utd, Arsenal, Liverpool)
- Blue/White (Chelsea, Everton, Leicester)
- White/Black (Tottenham, Juventus away)
- Black/White (Newcastle, Juventus home)

### Unique Combinations
- Sky Blue/White (Man City, Napoli, Lazio)
- Maroon/Sky Blue (Aston Villa, West Ham)
- Purple/White (Fiorentina)
- Orange/White (Valencia)
- Yellow/Black (Borussia Dortmund)

### Away Kit Inversions
Many teams invert colors for away kits:
- Home: Red/White ? Away: White/Red
- Home: Blue/White ? Away: White/Blue
- Home: Black/White ? Away: White/Black

## Tips

1. **Maintain tradition**: Use historically accurate colors for authentic feel
2. **Contrast**: Ensure Color 2 (trim/text) contrasts well with Color 1 (background)
3. **Simplicity**: Stick to 2 colors per kit for clarity
4. **Uniqueness**: Use distinctive away colors to avoid clashes

## Adding to Your TeamsDB.csv

1. Open TeamsDB.csv in a text editor or spreadsheet
2. Find the team row (first column = Club name)
3. Update columns 15-18 with the color names from above
4. Save and restart the application

Example row format:
```
Man Utd;England;Premier League;A;442;442;High;Yes;Yes;No;Ferguson;10;Old Trafford;75000;Red;White;White;Black
```

Where:
- Column 15 (Red) = Home main color
- Column 16 (White) = Home trim/number color  
- Column 17 (White) = Away main color
- Column 18 (Black) = Away trim/number color
