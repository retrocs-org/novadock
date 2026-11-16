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
Normal or Fast animation speed. For Demo shuffle, tick the scenes to include
and set **Shuffle seconds per scene** (10-300, default 40). Blank is excluded
by default. If every scene is unticked, shuffle uses Starfield.

The shuffle duration uses elapsed time independently of animation speed.
Switching scenes resets their animation; changing settings restarts shuffle.
Animation and the shuffle clock pause when another window takes focus.
Closing the scene or quitting NovaDock frees its window and bitmap. The dock
tile does not animate. Artwork and trigonometry are precomputed where useful;
the animation loop uses integer arithmetic and does not need an FPU.
