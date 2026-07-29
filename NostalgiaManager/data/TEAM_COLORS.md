# Team Colors Feature Guide

## Overview

Teams now display their official colors during matches. Colors are loaded from the `TeamsDB.csv` file and are used for:
- **Player jerseys** on the pitch (fill and trim)
- **Team names** in the scoreboard (background and text)

## How Team Colors Work

### Home vs Away Colors

- **Home Team**: Uses `Team colour main 1` and `Team colour main 2`
- **Away Team**: Uses `Away colour 1` and `Away colour 2`

### Color Roles

1. **Color 1** (Main Color):
   - Player jersey fill (the main body of the player circle)
   - Team name background in scoreboard

2. **Color 2** (Secondary/Trim Color):
   - Player jersey trim (outline of player circle)
   - Shirt number color
   - Team name text in scoreboard

## TeamsDB.csv Column Format

The relevant columns in `TeamsDB.csv` are:

```
Club;...;Team colour main 1;Team colour main 2;Away colour 1;Away colour 2
```

### Column Positions
- **Team colour main 1**: Column 15 - Home jersey main color
- **Team colour main 2**: Column 16 - Home jersey trim/number color
- **Away colour 1**: Column 17 - Away jersey main color
- **Away colour 2**: Column 18 - Away jersey trim/number color

### Example Entries

```csv
Club;...;Team colour main 1;Team colour main 2;Away colour 1;Away colour 2
Man Utd;...;Red;White;White;Black
Arsenal;...;Red;White;Yellow;Navy
Chelsea;...;Blue;White;White;Blue
Liverpool;...;Red;White;Yellow;Red
```

## Supported Color Names

The following color names are recognized (case-insensitive):

### Basic Colors
- **Red** - Traditional red (220, 50, 50)
- **Blue** - Standard blue (50, 100, 220)
- **Green** - Bright green (50, 180, 50)
- **Yellow** - Bright yellow (255, 220, 50)
- **Orange** - Vibrant orange (255, 140, 50)
- **Purple** / **Violet** - Purple (150, 50, 200)
- **White** - Off-white (240, 240, 240)
- **Black** - Dark black (40, 40, 40)
- **Grey** / **Gray** - Medium grey (150, 150, 150)
- **Pink** - Light pink (255, 150, 180)
- **Brown** - Brown (140, 90, 50)

### Special Colors
- **Navy** - Dark navy blue (30, 40, 100)
- **Sky Blue** / **Light Blue** - Light blue (135, 206, 250)
- **Dark Blue** - Deep blue (20, 50, 150)
- **Dark Green** - Deep green (30, 100, 30)
- **Light Green** - Pale green (144, 238, 144)
- **Maroon** - Deep red (128, 0, 0)
- **Teal** - Blue-green (0, 128, 128)
- **Gold** - Golden yellow (255, 215, 0)
- **Silver** - Metallic grey (192, 192, 192)

### Unrecognized Colors
If a color name is not recognized, it defaults to **White** (240, 240, 240).

## Visual Examples

### Manchester United
```
Home: Red jersey with White trim/numbers
Away: White jersey with Black trim/numbers
```

### Arsenal
```
Home: Red jersey with White trim/numbers
Away: Yellow jersey with Navy trim/numbers
```

### Chelsea
```
Home: Blue jersey with White trim/numbers
Away: White jersey with Blue trim/numbers
```

## How to Update Team Colors

1. **Open TeamsDB.csv** in a text editor or spreadsheet program
2. **Find your team** in the Club column
3. **Update the color columns**:
   - Column 15: `Team colour main 1` (home main)
   - Column 16: `Team colour main 2` (home trim)
   - Column 17: `Away colour 1` (away main)
   - Column 18: `Away colour 2` (away trim)
4. **Use supported color names** from the list above
5. **Save the file**
6. **Restart the application** to load the new colors

### Example Edit

Before:
```csv
Liverpool;...;;;;
```

After:
```csv
Liverpool;...;Red;White;Yellow;Red
```

## Tips for Good Color Combinations

### High Contrast
Choose color combinations with good contrast for readability:
- ? **Red** jersey with **White** numbers
- ? **Blue** jersey with **White** numbers
- ? **Yellow** jersey with **Navy** numbers
- ? **Red** jersey with **Orange** numbers (poor contrast)
- ? **Blue** jersey with **Purple** numbers (hard to read)

### Traditional Football Kits
Many teams have classic color schemes:
- **Man Utd**: Red/White (home), White/Black (away)
- **Barcelona**: Red/Blue (home), Yellow/Red (away)
- **Real Madrid**: White/Gold (home), Navy/White (away)
- **Juventus**: Black/White (home), White/Black (away)
- **AC Milan**: Red/Black (home), White/Red (away)

### Default Colors
If you leave color fields empty:
- **Home team** defaults to Red jersey with White trim
- **Away team** defaults to Blue jersey with White trim

## Match Display

### On the Pitch
- **Players**: Circles filled with Color 1, outlined with Color 2
- **Numbers**: Shirt numbers displayed in Color 2
- **Ball carrier**: Yellow highlight ring around the player

### In the Scoreboard
- **Team names**: Colored boxes with Color 1 background and Color 2 text
- **Score**: Displayed in neutral yellow/gold between team names

## Troubleshooting

**Colors not showing?**
1. Check the CSV columns are correct (15-18)
2. Verify the color names match the supported list
3. Ensure there are no extra spaces in the color names
4. Restart the application after making changes

**Colors look wrong?**
1. Verify you're using home colors for home team, away colors for away team
2. Check you haven't swapped Color 1 and Color 2
3. Try using basic color names (Red, Blue, White, etc.)

**Default colors appearing?**
- This happens when color fields are empty or unrecognized
- Check spelling of color names (case-insensitive but must match exactly)

## Future Enhancements

Potential improvements:
- RGB hex code support (e.g., `#FF5733`)
- Gradient/striped jerseys
- Custom team badges on jerseys
- More color presets (e.g., "Claret", "Amber")
- Kit patterns (stripes, hoops, etc.)
