# Team Screenshots

This directory contains team screenshots that will be displayed in the Friendly Match team selection screen.

## How to Add Screenshots

1. Take a screenshot of your team (stadium view, team photo, or any relevant image)
2. Save the image as either PNG or JPG format
3. Name the file with the team's ID number (e.g., `1.png`, `42.jpg`, `100.png`)
4. Place the file in this directory (`NostalgiaManager/data/images/teams/`)

## Finding Team IDs

To find a team's ID:
1. Open the `TeamsDB.csv` file in the data directory
2. The first row after the header contains the team data
3. Look at the team names to find your team
4. The team ID is internally assigned during database loading (check the Database screen in the app to see team IDs)

Alternatively, you can check the console output or Database screen in the application which shows team IDs.

## Image Guidelines

- **Supported formats:** PNG (.png) or JPEG (.jpg)
- **Recommended size:** 640x480 pixels or similar 4:3 aspect ratio
- **File naming:** Must match the team ID exactly (case-insensitive extension)
- **Examples:**
  - `1.png` - Screenshot for team with ID 1
  - `42.jpg` - Screenshot for team with ID 42
  - `100.png` - Screenshot for team with ID 100

## What Screenshots Will Show

- **Small thumbnails** (24x24px) appear next to team names in the team list
- **Large preview** (360px width, proportional height) appears below the team list when a team with a screenshot is selected
- Teams without screenshots will display normally without any image

## Notes

- Screenshots are loaded when the application starts
- If you add new screenshots, restart the application to see them
- Invalid or missing images are silently ignored - the team will still be selectable
