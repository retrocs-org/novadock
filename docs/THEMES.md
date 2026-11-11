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
Graphic dock backdrops are intentionally unsupported for now. DataTypes-backed
artwork still applies to individual launcher icons.

`SIDE` (`BOTTOM`, `TOP`, `LEFT` or `RIGHT`), `POSITION` (`START`, `CENTER` or
`END`), `FONT` (`WORKBENCH`, `TOPAZ` or `BOLD`) and `LABELS` (`ALWAYS`,
`HOVER` or `HIDDEN`) are also part of a theme. They appear on the Appearance
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
```

`README.txt` should include the theme name, author, license, compatible
NovaDock version, theme ID, installation steps and uninstall steps. Theme
packs need no executable or install script.

To import a pack from the UI:

1. Extract the package anywhere.
2. Open NovaDock Preferences and choose **Appearance**.
3. Select **Import...** and choose the pack's `theme.cfg` file.
4. NovaDock validates the file, installs it as
   `PROGDIR:Themes/<theme-id>/theme.cfg`, and selects the new theme.
5. Choose **Test** to preview it on the dock, then **Save** when satisfied.

Importing the same theme ID again updates that theme. NovaDock copies and
validates the new file before replacing the installed copy and restores the
previous file if installation fails.

For manual installation, copy the complete theme drawer below
`NovaDock:Themes/`, then reopen the Appearance page.

To uninstall an imported theme, select it in Appearance and choose
**Import...**; NovaDock asks whether to import another theme or remove this
one. A theme the dock is using now, or that is saved, cannot be removed:
select and **Use** or **Save** another theme first. Removal deletes the
theme's `theme.cfg` and deletes its drawer only when nothing else is left in
it. Bundled themes cannot be removed.
