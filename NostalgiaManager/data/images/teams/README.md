# Application Background Images

This directory contains background images that will be displayed **behind all UI elements** throughout the entire Nostalgia Manager application (except the Match screen). The images auto-cycle every 3 seconds, creating an immersive atmosphere across all screens.

## Screens With Backgrounds

- ? **Friendly Match** - Team selection
- ? **Tactics** - Formation and lineup management
- ? **Database** - Player search and information
- ? **Career** - Season and league management
- ? **Data Sources** - Custom database loading
- ? **About** - Application information
- ?? **Main Menu** - Uses its own dedicated background (`menu_bg.png`)
- ? **Match** - No background (displays the pitch instead)

## What You'll See

- **Full-screen background images** behind the team selection UI
- **Auto-cycling** - images change every 3 seconds
- **Cover-scaled** - images fill the entire screen (similar to main menu background)
- **Dark overlay** - a semi-transparent overlay ensures text remains readable
- **Seamless transitions** - smooth cycling between images

## How to Add Background Images

### Step 1: Prepare Your Images
1. Collect high-quality football/soccer images:
   - Stadium views (aerial shots, panoramas, empty/full stadiums)
   - Match moments (action shots, celebrations, historical moments)
   - Atmospheric shots (floodlights, crowd, pyrotechnics)
   - Team photos or tactical boards
   - Any football-related imagery

2. Save them as **PNG** or **JPG** files
3. **Recommended size:** 1920x1080 (Full HD) or higher for best quality
4. **Aspect ratio:** 16:9 is ideal, but any ratio works (will be cropped to fit)

### Step 2: Name Your Files Sequentially
- Name files: `1.png`, `2.png`, `3.png`, `4.png`, etc.
- Or use .jpg: `1.jpg`, `2.jpg`, `3.jpg`, etc.
- **Important:** Start from 1 and number consecutively without gaps!

### Step 3: Place Files in the Directory
1. Navigate to `NostalgiaManager/data/images/teams/`
2. Copy your image files into this directory
3. Restart the application to load the images

## Example Setup

```
NostalgiaManager/data/images/teams/
??? 1.png     ? Wembley Stadium aerial view
??? 2.jpg     ? Match action: goal celebration
??? 3.png     ? Crowded stadium at sunset
??? 4.jpg     ? Stadium floodlights at night
??? 5.png     ? Historic World Cup moment
??? 6.jpg     ? Empty stadium with perfect pitch
??? 7.png     ? Crowd with tifos and banners
??? 8.jpg     ? Team walking onto pitch
??? 9.png     ? Tactical board close-up
??? 10.jpg    ? Trophy presentation
??? 11.png    ? Vintage football photograph
```

## How It Works

### Background Display
- Images are drawn behind **all UI elements**
- They **cover the entire window** (scaled to fill, maintaining aspect ratio)
- A **dark semi-transparent overlay** (opacity 47%) is added so text remains readable
- Images **auto-cycle every 3 seconds** without user interaction

### Image Scaling
- Uses "cover" scaling - fills the window completely
- Wider images: crop top and bottom
- Taller images: crop left and right
- Always maintains original aspect ratio (no stretching)

## Technical Details

### Loading
- Loads images numbered 1 through 20
- Stops at the first missing number (sequential only)
- Both PNG and JPG formats supported
- Loaded at application startup

### Performance
- All images loaded into GPU memory at startup
- Smooth, no-stutter cycling
- Memory usage depends on number and size of images

### Overlay
- Semi-transparent black overlay: `rgba(0, 0, 0, 0.47)`
- Ensures UI text remains visible on any background
- Can be adjusted in code if needed

## Image Recommendations

### Best Content Types

**Stadium Atmospheres:**
- Wide-angle stadium shots (empty or packed)
- Aerial/drone views of famous venues
- Floodlit night games
- Sunset/sunrise over pitch

**Match Moments:**
- Goal celebrations
- Intense action (tackles, saves, headers)
- Iconic historical moments
- Penalty shootouts

**Atmosphere & Crowd:**
- Tifo displays
- Pyrotechnics and flares
- Passionate fans
- Stadium choreography

**Classic/Vintage:**
- Black & white football photos
- Retro kit designs
- Historical tournament moments
- Legendary players

### Technical Recommendations

**Resolution:**
- **Minimum:** 1280x720 (HD)
- **Recommended:** 1920x1080 (Full HD)
- **Best:** 2560x1440 or 3840x2160 (4K) for ultra clarity

**Aspect Ratios:**
- **16:9** - Perfect fit for widescreen (1920x1080, 2560x1440)
- **4:3** - Classic format (1024x768) - will crop sides
- **21:9** - Ultrawide (3440x1440) - will crop top/bottom
- Any ratio works, but 16:9 is optimal

**File Formats:**
- **PNG** - Best quality, larger files, supports transparency
- **JPG** - Smaller files, good quality at 85-95% quality setting
- Compress JPEGs moderately to reduce startup time

**File Sizes:**
- Try to keep each image under 5MB for faster loading
- Total size of all images affects application startup time

## Tips for Great Backgrounds

1. **High Contrast Images:** Work best with the dark overlay
2. **Avoid Busy Patterns:** Simple compositions let UI remain clear
3. **Dark Areas:** Images with darker tones need less overlay opacity
4. **Variety:** Mix different types (stadiums, action, crowds) for visual interest
5. **Quality Over Quantity:** 5-10 great images > 20 mediocre ones

## Troubleshooting

**No background images showing?**
- Verify files are named `1.png`, `2.png`, etc. (starting from 1)
- Check files are in `NostalgiaManager/data/images/teams/`
- Ensure extensions are lowercase (.png or .jpg)
- Restart application
- Check console for loading messages

**Only first few images load?**
- Loading stops at first gap in numbering
- If you have 1, 2, 4, 5 (no 3), only 1-2 will load
- Renumber files sequentially: 1, 2, 3, 4, 5...

**Text hard to read?**
- Default overlay is 47% black opacity
- If still difficult, choose darker/simpler background images
- Or edit `IM_COL32(0, 0, 0, 120)` in code (120 = opacity 0-255)

**Images look stretched or cropped?**
- This is intentional "cover" scaling (like main menu)
- Images fill entire screen, cropping edges if needed
- Use 16:9 aspect ratio images to minimize cropping

**Cycling too fast/slow?**
- Default is 3 seconds per image
- Edit `carouselTimer >= 3.0f` in `App.cpp` to adjust

## Advanced Customization

Edit `NostalgiaManager/src/ui/App.cpp` in `renderFriendly()`:

- **Cycle speed:** Change `3.0f` to desired seconds
- **Overlay darkness:** Adjust `IM_COL32(0, 0, 0, 120)` - last number is opacity (0-255)
- **Max images:** Change loop limit `i <= 20` to load more/fewer

## Your Current Setup

You mentioned having images 1-11. Perfect! Those should now display as full-screen cycling backgrounds behind your team selection interface!
