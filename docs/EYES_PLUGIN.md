# Eyes plugin — NovaDock 0.1

A lightweight, original Classic AmigaOS **xeyes-inspired** tile: two eyes look
at the Workbench mouse pointer. No X11, external commodity, MUI, RTG or FPU.
It works with OS 3.0+ Intuition and graphics.library drawing on classic
palette-based ECS/AGA screens, subject to the unverified 68k build status.

## Installation

The normal `build.amiga` builds `Plugins/Eyes`; `install.amiga` copies it to
`NovaDock:Plugins/Eyes`. It is included in the new sample
`novadock.cfg`. For an existing installation whose settings are preserved,
append this line to the **single** config file:

```ini
PLUGIN=Eyes|Plugins/Eyes|EYES|METER|ON
```

The bundled plugin reads and respects the standard plugin SETTINGS/QUIT
notifications. Eye painting and pointer sampling happen in the host rather
than sharing a `RastPort` with another process.

## Settings: right-click Eyes → Plugin settings...

The plugin page in `SYS:Prefs/NovaDockPrefs` exposes these settings:

```ini
OPTION.EYES.SIZE=NORMAL          # SMALL, NORMAL, LARGE
OPTION.EYES.EYE_COLOR=WHITE      # WHITE, BLUE, GREEN
OPTION.EYES.PUPIL_COLOR=DARK     # DARK, BLUE, GREEN
OPTION.EYES.SENSITIVITY=100      # 10..100 (percentage of pupil travel)
OPTION.EYES.SHOW_LABEL=NO        # YES or NO
OPTION.EYES.BLINK=YES            # YES or NO
OPTION.EYES.POLL_TICKS=20        # 20..250, 50 ticks = 1 second
```

Only `novadock.cfg` stores those values. `ENV:` holds the session copy
(Use) and `ENVARC:` holds the permanent copy (Save); no `Eyes.cfg` exists.

## Rendering and performance

- The host uses timer.device at ~5 Hz when Eyes is present (or auto-hide is on),
  so tracking works if another Workbench window has focus.
- `POLL_TICKS` limits Eye updates to 0.2..5 seconds; drawn pupil coordinates
  are compared before repaint to avoid unnecessary work.
- Integer-only raster ellipses, theme palette pens and fixed 2-eye geometry
  keep it suitable for lower-colour native screens. Eye outline/center are
  intentionally simple rather than pixel-perfect X11 anti-aliasing.
- Blinking is optional and is an occasional one-frame animation. Eyes do not
  require an off-screen bitmap or a special accelerator.
- If timer.device cannot be opened, the plugin tile can still show its static
  state, but mouse tracking will not refresh until another dock redraw occurs.

## Limits and verification

The plugin is a **source prototype**. Pupil math, option validation and config
round-trips are host-tested, but vbcc native compilation and actual
Interactive Workbench testing remain outstanding. Classic AmigaOS uses no modern
process isolation; avoid untrusted third-party plugin binaries.

The widget is inspired by [xeyes](https://www.x.org/releases/X11R7.5/doc/man/man1/xeyes.1.html),
not a port of its X11 source code.
