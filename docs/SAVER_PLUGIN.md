# SAVER plugin

Click **SAVER** to open a fullscreen scene. Any key or mouse click returns to
Workbench. The release of the opening click is ignored. There are no input
hooks or password locks, and scenes start only when clicked.

## Scenes

- **Boing ball**: a rotating red-and-white sphere bouncing over the classic grid.
- **Copper waves**: colourful raster bars, sine waves and scrolling stars.
- **Vector show**: rotating wireframe cubes, pyramids and an Amiga-style checkmark.
- **Fireworks**: colourful bursts above a night skyline.
- **Maze**: a Workbench-style maze builds its passages, then a character walks
  the route to the exit. It repeats after the character arrives.
- **Starfield**: stars fly towards the viewer.
- **Bouncing ball**: a simple bouncing ball.
- **Blank**: a black screen with no animation updates.
- **Demo shuffle**: cycles through your selected scenes automatically.

## Settings

Right-click the tile and choose **Edit settings**. Select a scene and a Slow,
Normal or Fast animation speed. New tiles default to **Boing ball** and **Fast**.
For Demo shuffle, tick the scenes to include
and set **Shuffle seconds per scene** (10-300, default 40). Blank is excluded
by default. If every scene is unticked, shuffle uses Starfield.

**Render size** defaults to **Full**, using the largest-fitting scene. Select
**Auto** for a smaller scene: 320 x 160 on screens deeper than 8 bits, and up to
640 x 320 on indexed screens when space permits. Fixed sizes are also available.
The window covers the screen, with the scene centred and black borders where
needed. SAVER does not switch Workbench's display mode or change its palette.

Slow, Normal and Fast target 5, 10 and 25 frames per second. Actual speed depends
on the CPU, screen mode and graphics driver. Drawing time counts towards the
animation clock, so slow frames skip ahead rather than slowing the whole scene.
The shuffle duration uses elapsed time independently of animation speed.
Switching scenes resets their animation; changing settings restarts shuffle.
Animation and the shuffle clock pause when another window takes focus.
Closing the scene or quitting NovaDock frees its window and bitmap. The dock
tile does not animate. Artwork and trigonometry are precomputed where useful;
the animation loop uses integer arithmetic and does not need an FPU.

Boing Ball caches its rotation frames as they are first used, restores the
background only around the moving ball/shadow, and copies only that changed
area to the screen. Cache memory is capped at 768 KiB and released on exit;
allocation failure or noninteger Full scaling uses the ordinary renderer.
Copper waves combines matching raster rows before drawing them. Coordinates
are scaled through lookup tables and unchanged screen borders are not repainted.
These reduce rendering work; real-hardware frame rates still need measurement.

## Measuring performance

Enable **Performance diagnostics** in SAVER settings, then open a scene. The
small overlay shows actual presented FPS and average/peak frame-rendering time
(in milliseconds), followed by render dimensions, screen depth and cache use.
It refreshes the measurements about every two seconds. Closing the scene writes
`RAM:NovaDock-SAVER.txt`, which you can retain for comparisons between scenes,
render sizes and graphics modes. Paused time is excluded from the FPS figures.
Timing resolution is 20 ms; short frames can read zero. Rendering time includes
waiting for bitmap operations to finish, and excludes drawing the overlay.
Diagnostics are off by default and add measurement/overlay work when enabled.
The defaults remain **Full**, **Fast** and **Boing ball**.
