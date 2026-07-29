# Screen Background Images Feature Guide

## Overview

All screens in Nostalgia Manager now display cycling background images that auto-cycle every 3 seconds. This creates an immersive, atmospheric visual experience throughout the entire application (except the Match screen which displays the pitch).

## Screens With Backgrounds

- ? **Main Menu** - Classic background (always had this)
- ? **Friendly Match** - Team selection with backgrounds
- ? **Tactics** - Formation and squad selection
- ? **Database** - Player search and browse
- ? **Career** - Season management
- ? **Data Sources** - Custom CSV loading
- ? **About** - Application information
- ? **Match** - No background (shows pitch instead)

## What You'll See

### Background Display
- **Full-screen images** behind all UI elements
- **Auto-cycling** - images change every 3 seconds
- **Cover-scaled** - fills the entire window (like main menu)
- **Dark overlay** - semi-transparent (47% opacity) ensures text remains readable
- **Seamless transitions** - smooth cycling between images
- **Consistent experience** - same backgrounds across all screens

## How to Add Screenshots

### Step 1: Prepare Your Images
1. Collect football/soccer-related images:
   - Stadium views (aerial shots, empty/full stadiums)
   - Match action shots (goals, celebrations, tackles)
   - Team photos (starting lineups, team groups)
   - Tactical diagrams or formation boards
   - Crowd scenes
   - Trophy presentations
   - Historic moments

2. Save them as **PNG** or **JPG** files
3. Recommended size: **1280x720 pixels** (16:9 aspect ratio) or **1024x768** (4:3)

### Step 2: Name Your Files Sequentially
- Name files: `1.png`, `2.png`, `3.png`, `4.png`, etc.
- Or use .jpg: `1.jpg`, `2.jpg`, `3.jpg`, etc.
- **Important:** Start from 1 and number consecutively without gaps!

### Step 3: Place Files in the Directory
1. Navigate to `NostalgiaManager/data/images/teams/`
2. Copy your screenshot files into this directory
3. Restart the application to load the images

## Example Setup

```
NostalgiaManager/data/images/teams/
??? 1.png     ? Wembley Stadium aerial view
??? 2.jpg     ? Iconic goal celebration
??? 3.png     ? Tactical formation diagram
??? 4.jpg     ? Stadium floodlights at night
??? 5.png     ? Team walking out to pitch
??? 6.jpg     ? Crowd with flares
??? 7.png     ? Historic moment (World Cup final)
??? 8.jpg     ? Modern stadium interior
??? 9.png     ? Team huddle before match
??? 10.jpg    ? Trophy ceremony
??? 11.png    ? Vintage football photo
```

## Technical Details

### Image Loading
- The application loads images numbered 1 through 20
- Loading stops at the first missing number (so 1,2,3,5 would only load 1,2,3)
- Images are loaded at startup
- Both PNG and JPG formats are supported

### Display Behavior
- **Auto-cycling:** Images change every 3 seconds automatically
- **Centered:** Images are horizontally centered on screen
- **Scaled:** Images scale to 80% of available width, maintaining aspect ratio
- **Looping:** After the last image, the carousel returns to the first

### Performance
- All images are loaded into GPU memory at startup
- Smooth transitions with no disk I/O during cycling
- Memory usage depends on number and size of images

## Image Recommendations

### Content Ideas

**Stadium Views:**
- Aerial shots of famous stadiums
- Empty stadium at sunset
- Stadium packed with fans
- Floodlit stadium at night

**Match Moments:**
- Goal celebrations
- Last-minute winners
- Penalty shootouts
- Tactical plays

**Atmosphere:**
- Crowd with tifos and banners
- Pyrotechnics and flares
- Referee controversies
- Player emotions

**Historical:**
- Vintage football photos (1960s-1990s)
- Classic kit designs
- Legendary moments
- Trophy wins

### Aspect Ratios
- **16:9 (1920x1080, 1280x720):** Best for modern widescreen displays
- **4:3 (1024x768, 800x600):** Good for classic/vintage aesthetic
- **Any aspect ratio works** - images will scale appropriately

### Image Quality
- Use high-resolution images for better display quality
- Images scale down well, so larger is usually better
- Compress JPEGs moderately (quality 80-90) to reduce file size
- PNG is better for diagrams, text overlays, or images with transparency

## Troubleshooting

**No images showing?**
- Verify files are named `1.png`, `2.png`, etc. (starting from 1)
- Check files are in `NostalgiaManager/data/images/teams/`
- Ensure file extensions are lowercase (.png or .jpg)
- Restart the application
- Check console output for loading messages

**Only first few images showing?**
- The loader stops at the first gap in numbering
- If you have 1.png, 2.png, 4.png (missing 3), only 1-2 will load
- Renumber your files to be sequential: 1, 2, 3, 4, ...

**Images cycling too fast/slow?**
- Default is 3 seconds per image
- This can be adjusted in the code if needed (search for `carouselTimer >= 3.0f`)

**Images look stretched?**
- This shouldn't happen as aspect ratio is maintained
- Check if your source images are already distorted

## Advanced: Customization

If you want to modify the carousel behavior, edit `NostalgiaManager/src/ui/App.cpp` in the `renderFriendly()` function:

- **Cycle speed:** Change `3.0f` to your desired seconds
- **Image size:** Adjust `availWidth * 0.8f` (0.8 = 80% width)
- **Max images:** Change the loop limit `i <= 20` to load more/fewer images

## Future Enhancements

Potential improvements:
- Manual navigation (previous/next buttons)
- Pause/play controls
- Random order instead of sequential
- Fade transitions between images
- Multiple carousel speeds
- Image captions or descriptions
