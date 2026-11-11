# NovaDock 0.1 — Auto-hide, dock menu, and tile highlighting

This is a **source prototype**. Portable tests and native 68k compilation pass;
the dock has also been exercised on Workbench.

## Auto-hide

In `SYS:Prefs/NovaDockPrefs`, set **Hide** to **On** and select **Use** or
**Save**. Or edit the same text file manually:

```ini
AUTO_HIDE=YES
HIDE_DELAY=3
```

`AUTO_HIDE=NO` is the backwards-compatible default. `HIDE_DELAY` is an
integer from **1 to 60 seconds**, default **3**. The dock checks the pointer
five times per second using one asynchronous timer.device request. The timer
is not started when auto-hide is off; selecting Hide=On while running starts
it on live reload.

When the pointer stays outside the dock for the configured time, the dock
contracts to a **42×12 px horizontal tab**, or **12×42 px vertical tab**, near
the configured screen edge. Pointing at the tab restores the dock. Clicking
the tab also restores it. Popup menus, drawer flyouts, the clock calendar,
tooltips, and active drag operations suspend hiding. Plugins continue running
while hidden but their tiles are not repainted until the dock is visible.

The event code waits for `IDCMP_CHANGEWINDOW` before painting the changed
window size because `ChangeWindowBox()` is asynchronous. It does not rely on
`IDCMP_INTUITICKS` for background polling; Intuition only sends those to the
active window. If timer.device cannot be opened, NovaDock remains visible.

## Dock-wide menu

Right-click the background/gap of the dock to open the compact dock menu:

- Unlock dock / Lock dock
- Add item... (Application, Drawer / disk, installed Plugin, Spacer, or Command tile)
- Preferences…
- About NovaDock…
- Quit NovaDock

Click outside any dock popup to dismiss it; a separate Cancel row is not
needed. Row count and row assignment are managed in Preferences, where the
result can be reviewed before Use or Save.

**About NovaDock…** opens a small non-modal window with NovaDock's name,
version, short description, MIT license and public website. The same shared
information is available from the dedicated About section in Preferences.

NovaDock starts locked. Unlocking enters Edit mode, shown by an accent frame,
and enables direct Workbench drops, reordering and drag-out removal/disable.
Locking returns the dock to normal launch-only use. Brief adjacent status
windows confirm the mode and the outcome of every direct edit.
The Add Item window uses fixed-width type buttons and a separate field area,
so changing category never resizes or overlaps the controls. It shows only the
paths, arguments, appearance, or command options relevant to that type. The
**Plugins** category lists installed manifest-backed plugins which are not
already enabled on the dock; selecting one shows its name and executable and
Use or Save activates it. Vertical
docks remain one column. Adding a row creates an empty row. In Edit mode,
release a tile over another row's tile or empty background to move it there.
Row membership is saved, rows may contain only one tile, and the frame width
tracks the widest populated row. Removing a row merges its tiles into the
preceding row.

Right-clicking a tile immediately opens its own menu without selecting it
first. Apps retain Launch/Edit/Remove. Drawers add **Popup contents**. The
Plugins use the same concise actions as launchers: **Edit settings...** and
**Remove**. A **Launch** entry is shown only when the plugin has a genuine
application-launch action (currently DOpus, and Music when a default player is
configured). Clock calendars and meter display changes remain direct tile
actions. Spacers
offer Edit spacer and Remove spacer. The spacer editor controls the item's
width, divider visibility, line thickness and theme-aware colour. All configuration entry points open
the same NovaDockPrefs application.

Locking protects drag-reordering, drag-removal and structural background
actions. Explicit item-menu commands such as **Remove**, **Remove spacer** and
**Remove** remains available while locked because choosing the command is
itself confirmation of the edit.

The **Appearance...** page in NovaDockPrefs shows a live sample of launcher,
clock, CPU-meter and volume-meter tiles before applying a theme. Each theme
pack defines its own palette and default shape; the shape control can still
override that default. Themes use native solid colour roles only; graphic
dock backdrops are intentionally disabled. Optional
picture datatypes remain available for individual launcher artwork.

General also offers **Refresh: Low mem / Smooth**. Low mem is the default
and uses Intuition's simple-refresh path, which is safest on classic planar
screens and leaves more display memory available to applications such as
Directory Opus 4. Smooth opts into a smart-refresh backing bitmap to reduce
expose redraws on systems with sufficient graphics memory. Use **Use** for the
current session or **Save** to make the choice permanent.

## Highlighting

- A launcher pressed with the left button gets a high-contrast accent border.
- Edit mode draws an accent frame around the dock.
- A dragged item draws an insertion line at its prospective drop position.
- The hover background changes, and existing tooltips remain optional.

When the configured tiles do not fit at their minimum readable size, the dock
uses full-sized PREV/NEXT tiles to page through them. The controls follow the
dock orientation and do not replace or discard configuration entries.

## Keyboard control

Set a **Dock hot key** on the General page of NovaDock Preferences, for
example `ctrl alt d` (any Commodities hot key description; leave it empty
for none). Pressing it reveals the dock, makes it the active window and
highlights a tile. A cursor key does the same while the dock window is
already active.

| Key | Action |
| --- | --- |
| Cursor keys | Move to the previous or next visible tile |
| Return or Space | Act on the tile, exactly like a click |
| M | Open the tile's menu |
| Esc | Leave keyboard control |

In the tile's menu, cursor up and down choose an entry, Return or Enter
selects it, and Esc closes it. A drawer opened with Return continues with
the drawer popup's own keys (cursor keys, Return, Backspace, type-ahead).

Moving the mouse, or activating another window, hands control back to the
mouse. Auto-hide waits while keyboard control is active, and the selected
tile shows its tooltip after the usual delay. Only the tiles on the current
page are reached; use the PREV/NEXT tiles to change page. When keyboard
control is not active, Esc or Q in the active dock window still quits
NovaDock, as before.

## Workbench smoke checklist

1. Build on OS3.0–3.2 with vbcc + NDK and start NovaDock. Confirm normal
   launchers and all nine plugins function with `AUTO_HIDE=NO`.
2. Turn Hide on through NovaDockPrefs; verify live reload and that the setting
   is saved into the correct ENV/ENVARC file.
3. On 640×256 Workbench, verify bottom/top tabs; on a tall-enough screen,
   verify left/right docks (nine vertical 68px tiles will not fit 256 lines).
4. Move the mouse to another **active** Workbench window; wait the configured
   time and confirm the tab appears even though NovaDock is inactive.
5. Move the pointer over the tab (without clicking), then click it to confirm
   both methods restore the dock.
6. Right-click a blank dock area and several tile kinds; test the compact dock
   menu and contextual item menus without preselecting a tile.
7. Drag an application tile over another tile: ensure the border indicates
   placement and that disabled plugin entries survive reordering/saving.
8. Drag a launcher outside to delete it and a plugin outside to disable it;
   drop a Workbench icon into the dock to add it at the pointed position.
9. Run MusicPlayer/Roadshow/Network updates while hidden and confirm they
   resume updating the correct tile on reveal.
10. Disable auto-hide while hidden using Preferences/ARexx reload and confirm
   the dock reopens visibly rather than remaining a tab.
11. Exit and restart repeatedly; watch for stuck Exec/timer.device/ARexx ports,
    memory leaks, broken refreshes, and orphan plugin processes.

**Limitations:** no sliding animation yet; auto-hide is deliberately a small
static reveal tab to avoid CPU-heavy full-frame animation on 68000 systems.
The timer, refresh-mode and window-resize integrations remain untested on
physical hardware.

In this release right-clicking a plugin tile opens its own context menu,
including **Plugin settings...**, rather than the dock-wide menu.
Right-click blank dock space for dock-wide actions.


## Showing and hiding on demand

Even with auto-hide off, the dock can be collapsed to its reveal tab and
brought back:

- Commodities Exchange lists **NovaDock** with working **Show** and **Hide**
  buttons (and Enable/Disable for its hot keys).
- ARexx: `SHOW`, `HIDE` and `TOGGLE`.
- Starting NovaDock a second time reveals and raises the running dock instead
  of failing.

A manually hidden dock reopens when the pointer touches its tab, as with
auto-hide.

**Keep front** now skips the layer operation when nothing overlaps the dock,
so it no longer redraws needlessly five times a second.
