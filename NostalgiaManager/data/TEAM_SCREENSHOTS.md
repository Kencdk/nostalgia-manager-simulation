# Team Screenshots Feature Guide

## Overview

The Friendly Match team selection screen now supports displaying team screenshots. When you select teams for a friendly match, you can see visual previews of the teams if screenshots are available.

## What You'll See

### In the Team Selection List
- **Small thumbnails** (24x24px) appear next to team names that have screenshots
- Teams without screenshots display normally (text-only)

### Preview Panel
- When you select a team that has a screenshot, a **larger preview** (360px width) appears below the team list
- The preview maintains the original aspect ratio of your screenshot

## How to Add Screenshots

### Step 1: Prepare Your Images
1. Take screenshots of your teams (stadium views, team photos, tactical screens, etc.)
2. Save them as **PNG** or **JPG** files
3. Recommended size: **640x480 pixels** (or any 4:3 aspect ratio)

### Step 2: Find Team IDs
Team IDs are assigned automatically when the database loads. To find them:

**Method 1: Database Screen in the Application**
1. Launch Nostalgia Manager
2. Go to the **Database** screen
3. Browse or search for your team - the ID will be shown

**Method 2: Check During Team Selection**
1. Go to **Friendly Match**
2. Select a league and team
3. The application internally assigns sequential IDs based on the order teams are loaded from `TeamsDB.csv`

**Note:** Team IDs typically match the order teams appear in `TeamsDB.csv` (starting from 0 or 1), but the safest way is to check the Database screen.

### Step 3: Name Your Files
- Name each screenshot with the team ID followed by `.png` or `.jpg`
- Examples:
  - `1.png`
  - `42.jpg`
  - `100.png`

### Step 4: Place Files in the Teams Folder
1. Navigate to `NostalgiaManager/data/images/teams/`
2. Copy your screenshot files into this directory
3. Restart the application to load the new screenshots

## Example Setup

```
NostalgiaManager/
??? data/
    ??? images/
        ??? teams/
            ??? README.md
            ??? 1.png         ? Manchester United screenshot
            ??? 2.png         ? Arsenal screenshot
            ??? 3.jpg         ? Liverpool screenshot
            ??? 10.png        ? Real Madrid screenshot
            ??? 42.jpg        ? Barcelona screenshot
```

## Technical Details

### Supported Formats
- **PNG** (.png) - recommended for quality
- **JPEG** (.jpg) - good for smaller file sizes

### Image Loading
- Screenshots are loaded when the application starts
- Files are checked in this order: `{teamId}.png`, then `{teamId}.jpg`
- The first valid image found is used
- Invalid or missing images are silently skipped (no error messages)

### Performance
- All screenshots are loaded into GPU memory at startup
- This provides smooth rendering during team selection
- Memory usage depends on the number and size of screenshots

## Tips

1. **Consistent Sizing**: Use the same resolution for all screenshots for a uniform look
2. **File Size**: Optimize your images to reduce loading time (PNG compression or JPEG quality settings)
3. **Naming**: Double-check team IDs before naming files to avoid mismatches
4. **Organization**: Keep a spreadsheet mapping team names to IDs for easy reference

## Troubleshooting

**Screenshot not showing?**
- Verify the file name exactly matches the team ID
- Check the file extension is `.png` or `.jpg` (lowercase recommended)
- Ensure the file is in the correct directory: `data/images/teams/`
- Restart the application after adding new screenshots
- Check the Database screen to confirm the correct team ID

**Image looks stretched or distorted?**
- The preview maintains aspect ratio, so this shouldn't happen
- If it does, check your source image isn't already distorted

**Application slow to start?**
- Large number of high-resolution screenshots can increase startup time
- Consider reducing image file sizes or resolutions

## Future Enhancements

Potential improvements for this feature:
- Team badges/logos alongside screenshots
- Multiple images per team (carousel/gallery)
- Screenshots in other screens (Career mode, Match setup, etc.)
- Dynamic screenshot capture from within the application
