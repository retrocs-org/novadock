# NovaDock 0.1 — ARexx, tooltips and file drops

## Optional ARexx host

`rexxsyslib.library` is optional. When available, NovaDock registers a standard
ARexx host named **NOVADOCK** (in addition to its existing, private Exec IPC
ports). The host does not change or replace the public plugin SDK. An existing
public port with that name is not taken over; NovaDock continues without the
ARexx interface if the name is occupied or the library is missing.

Run this example in a `.rexx` script using `C:RX` (requires ARexx):

```rexx
/* NovaDock control demo */
ADDRESS NOVADOCK
'PING'
SAY RESULT
'LIST'
SAY RESULT
```

Supported commands (case insensitive):

| Command | Behavior |
|---------|----------|
| `PING` | Returns `PONG` |
| `VERSION` | Returns `0.1` |
| `STATUS` | Returns `RUNNING` and the number of visible tiles |
| `LIST` | Returns semicolon-separated 1-based tile numbers and names for every tile |
| `LIST STEM tiles` | Fills the ARexx stem `TILES.`: `TILES.COUNT`, and `TILES.n.LABEL`, `TILES.n.KIND` (`LAUNCHER`, `DRAWER`, `GROUP`, `PLUGIN`, `SPACER`, `DISK` or `TASK`), `TILES.n.ID` and `TILES.n.VALUE` (plugin tiles) for n = 1 to COUNT; returns the count. Labels may contain any character, including `;` |
| `OPEN 2` | Launches visible tile 2 (or invokes its configured action) |
| `OPEN Shell` / `OPEN "DOpus 4"` | Launches the first tile whose label matches (case-insensitive, `*` and `?` wildcards); RC 5 and `NOT FOUND` if none |
| `MOVE 3 5` | Moves visible item 3 to position 5, saves configuration |
| `CLICK 4` | Sends a click to plugin tile 4, exactly as a mouse click would |
| `REMOVE 2` | Same as dragging tile 2 out: deletes a launcher, disables a plugin |
| `ADD "Work:Apps/Editor"` | Appends an application or drawer launcher and saves |
| `SETVALUE id caption value [meter]` | Updates a passive script tile (see below) |
| `SETBADGE id [text]` | Shows TEXT (up to 7 characters, for example `3`) as a badge on plugin tile ID; `"*"` shows a dot and no text removes it. Works on any plugin tile; a plugin that sets its own badges replaces it with its next change. RC 5 and NOT FOUND for an unknown ID |
| `SHOW` / `HIDE` / `TOGGLE` | Reveals the dock, collapses it to the reveal tab, or switches; `TOGGLE` returns `SHOWN` or `HIDDEN` |
| `LOCK` / `UNLOCK` | Leaves or enters Edit mode |
| `SUSPEND` / `RESUME` | Closes all of the dock's windows so Workbench can reset its screen (a screen mode change), and reopens them; plugins keep running. The dock reopens by itself after 30 seconds, or when screennotify.library reports the new Workbench. `RESUME` returns RC 5 and `PENDING` if Workbench is not back yet |
| `RELOAD` | Reloads text configuration from ENV:/ENVARC:/PROGDIR: |
| `PREFS` | Requests `SYS:Prefs/NovaDockPrefs` |
| `QUIT` | Gracefully requests dock shutdown |
| `HELP` | Returns the supported command names |

Arguments containing spaces are written in double quotes. `ADD` needs an
absolute path (one containing a colon); `SETVALUE` captions and values are
limited to 31 characters.

A stem avoids splitting the `LIST` string and works with any label:

```rexx
/* Print every tile */
ADDRESS NOVADOCK
OPTIONS RESULTS
'LIST STEM tiles'
DO i=1 TO tiles.count
  SAY i tiles.i.kind tiles.i.label tiles.i.value
END
```

### Script-driven tiles

A plugin entry whose command starts with `@` is a *passive tile*: NovaDock
does not start a process for it and the watchdog leaves it alone. Scripts fill
it with `SETVALUE`:

```text
PLUGIN=Mail|@rexx|MAIL|METER|ON
```

```rexx
/* Show unread mail on the MAIL tile */
ADDRESS NOVADOCK
'SETVALUE MAIL "MAIL" "3 new" 30'
```

The optional meter is 0-100 and also feeds the tile's graph view.
`SETVALUE` is refused for tiles that belong to a real plugin process.

### ARexx Launcher Actions

Launchers in ARexx mode send their command asynchronously. NovaDock no longer
waits for the target application: a busy target cannot freeze the dock or make
plugins time out, and a target that is itself running a script addressing
`NOVADOCK` cannot deadlock it. A non-zero result is reported in the status
window when the reply arrives. On quit NovaDock waits up to about five seconds
for outstanding replies.

`MOVE` returns an error if its validated configuration change cannot be saved.
`OPEN` rejects plugin tiles; use the plugin’s own mouse controls for their
plugin-specific actions.

`MOVE` works with any visible item, including plugins and spacers.
Positions refer to the currently visible tiles, not disabled plugin entries.
Malformed commands return a nonzero ARexx result code. The `OPEN` and `MOVE`
operations are local controls, not remote APIs; they have the same privileges
as the Workbench user. `QUIT` may terminate the dock and its plugin display.

The host returns strings only when the caller sets the normal ARexx result
request flag; it does not allocate results that the caller did not request.
This is a first implementation and must be verified with a native 68k build.

## Tooltip behavior

Set `TOOLTIPS=YES` (default) or `TOOLTIPS=NO` in `novadock.cfg`, or use the
**Tips: On / Off** gadget in `SYS:Prefs/NovaDockPrefs`. A small borderless
Workbench popup appears over a tile after a short hover. The label uses the
application or plugin's configured name, with a length cap. Tooltips do not
read folders, launch programs, or allocate off-screen bitmaps. Tooltips close
on clicking, opening a launcher menu, losing focus or reloading the dock.

### Pointer tooltips

Resting the pointer on a tile shows its ordinary tooltip when enabled in
Preferences > General > Tooltips. Selected-image substitution and zoom windows
have been removed. Old `HOVER=OFF|IMAGE|ZOOM|BOTH` configuration lines are
accepted for compatibility, ignored, and omitted on the next save.

## Drag-and-drop behavior

- NovaDock starts locked. Right-click the dock and choose **Unlock dock** to
  enter Edit mode; its accent frame remains visible until it is locked again.
- Drop a Workbench object on the centre of an application tile to open it with
  that application. This action works while locked, uses the launcher's stored
  arguments and working directory, and never changes the dock layout.
- Drop files or drawers on the centre of a **drawer or volume** tile to copy
  them there with `C:Copy ... CLONE` (drawers are copied as a new
  sub-drawer, `ALL`). This works while locked. A small window opens only if
  Copy reports an error. Whole volumes are never copied, and the Workbench
  window of the destination is not refreshed automatically.
- In Edit mode, drag an application, data/project icon, or Workbench directory
  onto the background or outer edge of a tile: creates a launcher at that
  position, provided the 12-tile limit permits it. The centre of an
  application tile remains its Open With target.
- In Edit mode, drag any dock tile onto another tile to reorder launchers, drawers, and
  plugins without activating the dragged item.
- Drag a launcher or drawer genuinely outside the dock to remove it. A release
  over an empty dock gap is a no-op. Dragging a plugin outside disables its
  tile but preserves all plugin settings for re-enabling.
- A transient status window explains accepted and rejected drops. On crowded
  screens PREV/NEXT tiles page through the dock without shrinking content
  below its readable minimum.

Creating launchers respects the stored text configuration and preserves
disabled plugins. Multi-object Workbench drops are inserted in their supplied
order. For Open With, each supplied object is launched separately so one bad
path does not discard the remaining objects. A launcher dropped from Workbench
uses Workbench or Shell startup in the same way as a launcher created in the
preferences editor.

## Native testing checklist

1. Boot Workbench 3.1 or 3.2 and compile with vbcc + NDK.
2. Enable `TOOLTIPS=YES`; hover over a launcher, plugin and empty space.
3. Test `ADDRESS NOVADOCK` followed by `PING`, `LIST`, `OPEN`, `OPEN Shell`,
   `HIDE`, `SHOW`, `SETVALUE` on an `@rexx` tile and `RELOAD`.
4. Drag a WBTOOL, drawer and project icon onto the dock; check distinct paths.
5. Reorder a tile, drag a launcher out, and drag a plugin out; verify the
   item is removed from the active dock but remains available as a definition.
6. Save a rearrangement; confirm both ENV: and ENVARC: configs persist.
7. Quit, restart and run the ARexx commands again; check no stuck public ports.

The native 68k executable and locked dock menu have been exercised on Workbench.
