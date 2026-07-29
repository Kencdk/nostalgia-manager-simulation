# Background Images - Quick Start

## What This Feature Does

Displays full-screen auto-cycling background images behind **all screens** in the application (except Match screen which shows the pitch).

## Screens With Backgrounds

? Friendly Match  
? Tactics  
? Database  
? Career  
? Data Sources  
? About  
? Main Menu (uses its own menu_bg.png)

## How to Use

1. **Name your image files sequentially:**
   - `1.png` or `1.jpg` - First background
   - `2.png` or `2.jpg` - Second background
   - `3.png` or `3.jpg` - Third background
   - Continue up to `11.png` (you have 1-11)

2. **Place files here:**
   ```
   NostalgiaManager/data/images/teams/
   ```

3. **Restart the application**

4. **Go to Friendly Match screen** - Your images will appear as cycling backgrounds!

## Your Current Setup

You have files named 1-11. Perfect! Make sure:
- ? Files have extensions (.png or .jpg)
- ? Files are in `NostalgiaManager/data/images/teams/`
- ? No gaps in numbering (if 3 is missing, only 1-2 will load)

## What You'll See

- **Full-screen backgrounds** behind all UI elements
- **Auto-cycles every 3 seconds**
- **Cover-scaled** to fill the entire window
- **Dark overlay** (47% opacity) so text remains readable
- **Seamless** - same behavior as main menu background

## Check Console Output

When you start the app, you should see:
```
Loaded screenshot 1 from data/images/teams/1.png
Loaded screenshot 2 from data/images/teams/2.png
...
Total screenshots loaded for Friendly screen: 11
```

## If Images Don't Show

1. File names must be exactly: `1.png`, `2.png`, etc. (not `image1.png` or `001.png`)
2. Files must be in the correct directory
3. Application must be restarted after adding files
4. Check for gaps in numbering (1,2,4,5 won't work - must be 1,2,3,4,5)

## Image Recommendations

- **Best size:** 1920x1080 (Full HD) or higher
- **Best aspect ratio:** 16:9 (for widescreen displays)
- **Content:** Stadium views, match action, crowds, atmospheric shots

## That's It!

Your 11 backgrounds should now cycle behind the Friendly Match screen, creating an immersive visual experience!
