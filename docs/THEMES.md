# Colour theme packs

NovaDock keeps each bundled theme in its own directory below
`PROGDIR:Themes`. A pack contains one small text file:

```text
Themes/Copper/theme.cfg
```

## Configuration

The text file accepts these keys:

```text
NAME=Copper
SHAPE=CHAMFER
SIDE=BOTTOM
POSITION=CENTER
FONT=TOPAZ
LABELS=ALWAYS
BACKGROUND_TRANSPARENCY=0
DOCK_BACKGROUND=ARTWORK
DOCK_STYLE=FLAT
SPACER_WIDTH=52
SPACER_LINE=YES
SPACER_LINE_COLOR=ACCENT
SPACER_LINE_WIDTH=2
BG=76,38,25
TILE=76,38,25
HOVER=111,58,34
TEXT=255,230,199
SUBTLE=205,154,112
ACCENT=225,112,56
BAR=92,51,37
ACCENT2=54,176,165
```

Colours are decimal red, green and blue values from 0 through 255. `SHAPE` is
`ROUNDED`, `SQUARE`, `CHAMFER` or `MINIMAL`. `BG` fills the dock, `TILE` and
`HOVER` distinguish items, `TEXT` and `SUBTLE` control labels, `ACCENT` and
`ACCENT2` identify meters and graphs, and `BAR` is the unfilled meter colour.
Rounded, Square and Chamfer tiles also use `HOVER` for their upper/left edge
and `BAR` for their lower/right edge, producing a lightweight theme-aware
bevel without allocating extra palette pens. Minimal deliberately omits these
edges for a flat, undecorated style.
`BEVEL=YES` draws a classic raised Workbench frame around the whole dock
(`HOVER` on the upper/left edge, `SUBTLE` on the lower/right); the bundled
Workbench theme sets it and every other theme defaults to `NO`.
Choose **Background: Artwork** in Appearance and click **Test** to see the
selected theme's artwork on the dock. The Appearance sample uses the same
artwork while you browse themes. **Solid** is the factory default and the
low-resource preset. Use or Save applies your choice.

**Colourful icons** is a user override in Appearance. It preserves artwork-bank RGB colours instead of applying a theme's icon roles. CPU, RAM and digital Clock use a fixed blue/green palette in this mode. The preference is saved as `ICON_COLOURS=COLOURFUL` or `THEME`; selecting another theme preserves it. Theme mode uses separate dark and light icon inks so black text themes still have readable displays.

**Transparency %** accepts 0–100: 0 keeps the shelf opaque and 100 reveals
the desktop. Artwork, values and captions remain opaque. RTG screens use cached
colour blending; palette screens use an ordered mask. A theme can supply
`BACKGROUND_TRANSPARENCY`, `DOCK_BACKGROUND` (`SOLID` or `ARTWORK`) and
`DOCK_STYLE` (`FLAT` or `GLASS`). Omitted keys preserve the current preferences.
`GLASS` adds a sloping horizontal shelf and reflection bands. **macOS Modern**
defaults to 65% transparency; **Mac OS X Glass** defaults to 35%. Both values
remain adjustable after selecting the theme.

**Magnify %** in Appearance accepts 0–100. Zero disables the effect; 100
uses the full magnification strength. Existing `HOVER_MAGNIFICATION=YES` settings
load as 100 and `NO` loads as 0; saved configurations use a numeric percentage.

With **Animate** off, entering a tile immediately enlarges that tile to the
selected **Magnify %**, within the available screen space. Its size stays fixed
as you move within the tile; moving to another tile switches the enlargement
directly. Other tiles keep their normal sizes.

**Animate** (`HOVER_ANIMATION=YES`) enables a smooth wave around the pointer
and makes magnified tiles grow and shrink over a few frames: each frame closes
half of the remaining distance. The animation timer waits 40 ms on fast systems,
and adapts up to 200 ms when frames take longer to render. It falls back to its
normal rate as soon as tiles settle. Housekeeping uses elapsed time, including
painting time, so changing cadence does not stretch watchdog or hide delays. The shipped configuration turns
it on; existing configurations keep it off until it is ticked. On a slow 68000
leave it off, or check `STATUS DETAIL`'s hover counters (see the ARexx guide).

### Wallpaper-matched background

**Background** in Appearance offers **Solid**, **Artwork** and **Wallpaper**
(`DOCK_BACKGROUND=WALLPAPER`). Wallpaper samples the Workbench screen behind
the dock when the dock opens and derives the shelf, tile, hover and text
colours from its average colour: the shelf is slightly darker than a light
desktop or lighter than a dark one, text keeps a strong contrast, and the
theme's accent colours stay as they are. It works on palette (classic
Workbench) screens of up to 256 colours; on RTG screens with more colours the
theme's own colours are used. A theme with its own artwork backdrop takes
precedence. Change the Workbench pattern and restart the dock (or choose
**Use** in Preferences) to match the new one.

A horizontal or vertical dock grows along its length while hovered. In a
multi-row layout, only the hovered row magnifies; neighbouring rows move aside
and the shelf grows to keep them separated. Icons and their labels move together, and CPU, RAM, digital/analogue
Clock and network meters enlarge with their text, meters and graphs. Neighbours
move apart using their actual visible sizes. Growth is limited to the available
screen space. Left/right docks expand inward; a top dock expands down and a
bottom dock expands up. Rounded corners retain the desktop behind them while
the shelf grows and shrinks.

Pointer movement is observed through Commodities even while another window
is active; the dock does not take focus. Queued movements are coalesced and
stationary pointers do not repaint. The low-rate timer remains a fallback.
Hover frames reuse cached artwork, live widget images and transparent masks.
The extra canvas samples the desktop when the dock opens; it is refreshed when
the window reopens. Extra bitmaps are freed on reload/close. Overflow
docks retain a smaller in-tile effect. The factory default and low-resource
preset set magnification to zero.

A theme can include `Images/Backdrop.iff` plus optional
`Images/BackdropLeft.iff` and `Images/BackdropRight.iff`. Use indexed ILBM
with up to 16 colours and dimensions up to 96 by 96 pixels. The dock repeats
the centre along its length, fits the artwork to its thickness, and turns it
for a vertical dock. Both caps must be usable and fit the available length.
The dock samples the visible Workbench pixels behind transparent end-cap
corners when it opens or changes position or size. The sample refreshes after nearby windows move or close, once window dragging ends. Use an opaque centre strip for a continuous shelf.
A missing or invalid centre falls back to Solid.

For a custom theme, copy its Images drawer alongside the installed theme.cfg.
The existing theme importer installs the configuration file. DataTypes-backed
pictures remain available for individual launcher icons.

`SIDE` (`BOTTOM`, `TOP`, `LEFT` or `RIGHT`), `POSITION` (`START`, `CENTER` or
`END`), `FONT` (`WORKBENCH`, `TOPAZ` or `BOLD`) and `LABELS` (`ALWAYS`,
`HOVER` or `HIDDEN`) are also part of a theme. All bundled themes default to **Always** labels
for normal-sized launchers. These settings appear on the Appearance
page and are applied when that theme is selected. The user can still adjust
them afterward. Older custom theme files that omit one of these keys leave
the current setting unchanged.

`SPACER_WIDTH` is the theme's default spacer size from 20 through 140 pixels.
`SPACER_LINE` is `YES` or `NO`; `SPACER_LINE_COLOR` is `SUBTLE`, `ACCENT`,
`SECONDARY` or `TEXT`; and `SPACER_LINE_WIDTH` controls divider thickness from
1 through 8 pixels. Choosing a theme applies these defaults to the current
spacers. Each spacer can still be edited afterward to override them individually.

NovaDock has compiled-in fallback values, so a missing or partially edited
file does not make the dock unusable. Selecting a theme in Appearance loads
its declared placement, typography, label, shape and spacer treatment.
Changing a control or editing an individual spacer afterward is an explicit
override.

Selecting a theme updates only the sample inside Preferences. **Test** applies
the complete preset temporarily and keeps Preferences open. **Use** applies it
for the current session and exits; **Save** applies it permanently and exits.
After a Test, closing Preferences restores the appearance that was active when
Preferences opened.

Refresh mode is deliberately not theme-bound. It is a machine resource choice,
not a visual style: Low mem remains safe when changing themes, while Smooth is
enabled only when the user explicitly selects it in General.

## Bundled packs

- macOS Modern — a rounded translucent shelf.
- Mac OS X Glass — a sloping reflective shelf.
- Mocha
- Light
- Workbench
- Graphite
- High Contrast
- Copper
- Ocean
- Emerald
- NeXTSTEP — the intentional placement exception: a square, vertically
  centred right-edge dock inspired by the classic NeXT shelf, with labels
  always visible by default.
- BeOS — warm classic greys, yellow focus accents and chamfered tiles.
- Nord — a cool slate, ice-blue and muted green rounded preset.
- Vapor — a deep violet, magenta and cyan minimal preset with bold text.

All other bundled presets select a bottom-centred dock. NeXTSTEP selects
`SIDE=RIGHT` and `POSITION=CENTER`; placement can still be overridden after
selecting the preset.

The Appearance chooser is built dynamically. It shows the bundled presets
first, followed by every valid third-party theme found below
`PROGDIR:Themes`, sorted by its `NAME`. Adding a theme does not replace a
bundled preset and there is no fixed catalogue size; the practical limit is
available Amiga memory.

## Third-party theme packages

Each third-party theme lives in its own drawer. The drawer name is its stable
theme ID and may contain letters, digits, `_` and `-` (up to 31 characters).
The optional `NAME=` entry is the friendly label shown in Preferences.

A recommended ZIP or LHA layout is:

```text
BlueCopper-1.0/
  README.txt
  Themes/
    BlueCopper/
      theme.cfg
      Images/
        Backdrop.iff        (optional shelf artwork)
        BackdropLeft.iff    (optional start cap)
        BackdropRight.iff   (optional end cap)
```

Artwork images are indexed ILBM files of up to 96 x 96 pixels and 16 colours,
the same limits as tile icons.

`README.txt` should include the theme name, author, license, compatible
NovaDock version, theme ID, installation steps and uninstall steps. Theme
packs need no executable or install script.

To import a pack from the UI:

1. Extract the package anywhere.
2. Open NovaDock Preferences and choose **Appearance**.
3. Select **Import...** and choose the pack's `theme.cfg` file.
4. NovaDock validates the file, installs it as
   `PROGDIR:Themes/<theme-id>/theme.cfg`, and selects the new theme. Any
   `Images/Backdrop.iff`, `BackdropLeft.iff` and `BackdropRight.iff` beside the
   pack's `theme.cfg` are checked and copied too; the window title says when
   an image was not usable and was left out.
5. Choose **Test** to preview it on the dock, then **Save** when satisfied.

Importing the same theme ID again updates that theme. NovaDock copies and
validates the new file before replacing the installed copy and restores the
previous file if installation fails.

For manual installation, copy the complete theme drawer below
`NovaDock:Themes/`, then reopen the Appearance page.

### Picking a backdrop image

**Import...** also offers **Backdrop...**, which picks any ILBM image (up to
96 x 96 pixels, 16 colours) as the dock's shelf artwork. For an imported theme
the image replaces that theme's backdrop. For a bundled theme NovaDock creates
an imported theme called **Custom** from the current theme's colours, shape and
placement, so bundled themes are never changed. The picked image becomes the
centre piece and any end caps are removed, and **Artwork** is selected as the
background. Choose **Test** to try it and **Save** to keep it; **Custom** can be
removed like any imported theme.

To uninstall an imported theme, select it in Appearance and choose
**Import...**; NovaDock asks whether to import another theme or remove this
one. A theme the dock is using now, or that is saved, cannot be removed:
select and **Use** or **Save** another theme first. Removal deletes the
theme's `theme.cfg` and its imported `Images` artwork, and deletes its drawer only when nothing else is left in
it. Bundled themes cannot be removed.

When Always on top and animated magnification are enabled, newly covered
headroom is sampled just before the dock grows. This keeps a window opened
behind the dock visible between enlarged icons instead of restoring an old
desktop image. The overlapping dock area is never sampled, avoiding trails
from the dock itself. RTG tint rows reuse their existing bitmap and buffer.
