# Database Configuration

## Your Custom Databases Are Now Active! ?

The game is configured to use your custom CSV databases:

### Current Configuration
- **Teams Database**: `NostalgiaManager/data/TeamsDB.csv`
- **Players Database**: `NostalgiaManager/data/PlayersDB.csv`
- **Configuration File**: `NostalgiaManager/data/datasources.cfg`

### What's Been Set Up

1. **TeamsDB.csv** - Contains your team/club information:
   - Team name
   - League/division
   - Formation (e.g., 442, 433)
   - Mentality
   - Stadium information

2. **PlayersDB.csv** - Contains your player information:
   - Player names and nationalities
   - Current team/club
   - Position and role
   - All player attributes (Passing, Shooting, Technique, etc.)
   - Age and dates

### How The Loading Works

The game automatically:
1. Reads `datasources.cfg` to find the database file paths
2. Loads teams from `TeamsDB.csv`
3. Loads players from `PlayersDB.csv` and assigns them to their teams
4. Generates shirt numbers if not provided
5. Auto-selects starting XI for each team based on player ratings and positions

### CSV Format Support

The system supports both:
- **Championship Manager / FM style exports** (your current format)
- **Custom legacy format** with explicit teamId and role columns

The loader is intelligent and detects:
- Whether skills are on a 1-20 or 0-100 scale (auto-converts to 1-20)
- Various column name variations (e.g., "club" vs "team" vs "clubname")
- Multiple attribute name formats
- Different position rating systems

### Customization Options

#### Option 1: Keep Using Your Current Files (Recommended)
Your files are already set up correctly in:
- `NostalgiaManager/data/TeamsDB.csv`
- `NostalgiaManager/data/PlayersDB.csv`

Just edit these files as needed!

#### Option 2: Point to Different Files
Edit `NostalgiaManager/data/datasources.cfg`:

```
# Relative paths (from data/ folder):
players = custom/MyPlayers.csv
teams = custom/MyTeams.csv

# Or absolute paths:
players = D:\MyDatabase\Players.csv
teams = D:\MyDatabase\Teams.csv
```

### Verification

To verify your databases loaded correctly:
1. Run the game
2. Check the console/log for any loading errors
3. Navigate to team selection - you should see your teams
4. Select a team - you should see the players from your CSV

### Database Updates

To update your database:
1. Edit the CSV files directly
2. Restart the game
3. Changes will be loaded automatically

### Supported Team Columns
- Name/Club/TeamName
- League/Division/Competition/Nation
- Formation/Shape (e.g., "442", "433", "4-4-2")
- Mentality (Defensive/Standard/Attack)

### Supported Player Columns
- Name/FirstName+SecondName/FullName
- Club/Team/ClubName
- All CM/FM attribute columns (Passing, Shooting, Technique, etc.)
- Position ratings (Goalkeeper, Defender, Midfielder, etc.)
- Side ratings (Right, Left, Centre)
- Ability/CurrentAbility/CA (overall rating)
- Age/DateOfBirth
- Nationality/Nation

### Notes

- Player shirt numbers are auto-generated if not in the CSV
- Starting XI is auto-selected based on positions and ratings
- Teams without players are skipped
- Free agents and non-club entries are filtered out
- Character encoding is handled (accented names are converted to ASCII for display)

---

**Your databases are ready to use!** The game will load them automatically on startup.
