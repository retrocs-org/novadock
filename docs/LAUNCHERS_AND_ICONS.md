# App and directory launchers — NovaDock 0.1

NovaDock supports separate **application/command** launchers and **Workbench
folder/drawer** launchers. Both use the optional third field for a custom icon.

## Preferences UI

Open `SYS:Prefs/NovaDockPrefs` (or `PROGDIR:NovaDockPrefs` from an unpacked
development tree). **Organize** shows the active dock in explicit row groups.
It combines plugins, applications, drawers and spacers; Remove changes active
membership, Up/Down changes order and automatically crosses a row boundary,
and Edit opens the selected plugin or launcher editor. Add items with
Organize's **Add...** button, from the dock's **Add item** window, or by
dragging a Workbench object onto the dock. Both buttons open the same editor.
The Dock rows selector advances from one to three rows before cycling back to
one; Test, Use or Save applies the resulting arrangement.

Use **Items** to create or edit reusable application and drawer
definitions:

- **Add App** adds an application/AmigaDOS command tile.
- **Add Dir** adds a Workbench directory tile.
- **Name**, **Target**, **Icon** edit the current entry; choose the application
  or directory type and press **Update** to commit it in the editor.
- **Icon...** uses the native ASL file requester; select an existing Workbench
  `.info` or IFF/ILBM `.iff` icon. The `Icon` string field also accepts any
  custom path (absolute with an Amiga assignment or relative to NovaDock).
- **Target...** opens an ASL file requester for applications, or a
  directory-only requester for drawer tiles (AmigaOS asl.library V38+).
- **Running task**, **Window pattern**, and **ARexx port** optionally identify
  the launched application for its status indicator and click-to-front. Leave
  all three empty for automatic command-basename detection.
- **Remove** deletes the selected definition. Use/Save commits
  text configuration and sends a live reload to the running dock. Closing the
  Preferences window does not persist edits.

Choosing another launcher, adding a definition, navigating to another section,
or accepting a Workbench drop automatically keeps valid edits in the shared
Preferences session. Invalid fields keep the current launcher selected and
remain on screen for correction. **Update** is still available for an explicit
commit without leaving the current row.

The dock and configuration permit up to **24 items**, including plugin tiles.
Overflow paging keeps every active item reachable on smaller screens. NovaDock
checks available Workbench screen space before accepting a Save or Use.

## Text configuration

```ini
# Application/AmigaDOS command — old short lines remain valid
LAUNCH=Shell|NewShell|Icons/Workbench.iff|ON
LAUNCH=Music|SYS:Utilities/EaglePlayer|Icons/Games.iff
LAUNCH=Preferences|SYS:Prefs/ScreenMode|SYS:Prefs/ScreenMode.info

# Extended unified Launcher Action
# name|target|icon|state|mode|arguments|start-in|hot-key|Tools-menu|free-space|task|window|port
LAUNCH=Editor|C:Ed||OFF|CLI|S:Startup-Sequence|SYS:|control alt e|YES|YES
LAUNCH=Player|rexx_EP||OFF|AREXX|PLAY 1||control alt p|NO|YES
LAUNCH=EaglePlayer|SYS:Utilities/EaglePlayer||ON|AUTO||||NO|YES|EaglePlayer|EaglePlayer*|rexx_EP

# Drawer/Workbench directory — last field is the icon (optional)
DRAWER=Games|Games:|Icons/Games.iff
DRAWER=Utilities|SYS:Utilities|Icons/Drawer.iff
DRAWER=Internet|SYS:Tools|Icons/Internet.iff
```

## Unified Launcher Action

Every application or directory definition is now one reusable action. The
same entry may be enabled in **Organize** and assigned a Commodities-compatible
global **Hot key**. An item does not have to occupy dock space for its hot key
to remain active. Older configurations may retain a Workbench Tools-menu flag;
NovaDock preserves that compatibility field, but no longer shows the ambiguous
Tools checkbox in Preferences.

The **Run as** selector supports:

- **Automatic** — preserves the original behavior: a real object is opened by
  Workbench when possible, with AmigaDOS as the fallback.
- **Workbench** — explicitly uses `OpenWorkbenchObject()` on Workbench 3.5+
  and falls back to launching the object path on earlier systems.
- **CLI** — runs Target plus Arguments asynchronously. **Start in** changes
  directory before invoking the command.
- **ARexx** — Target is the public ARexx port and Arguments is the command.
  NovaDock sends the normal request and reports an error if the port is absent
  or returns a failure. Sending to NovaDock's own `NOVADOCK` port is rejected
  to avoid a self-request deadlock.

Hot-key descriptions use the standard Commodities syntax, for example
`control alt e` or `shift f10`. Invalid descriptions stay saved so they can be
corrected, but Commodities will not activate them. Hot keys require
`commodities.library`; all other launcher behavior continues if it is absent.

### Running-application matching

The three optional identity fields are alternatives and may be combined:

- **Running task** is the exact Exec task name shown by tools such as Scout.
- **Window pattern** is matched case-insensitively against Intuition window
  titles. `*` matches any sequence and `?` matches one character; for example,
  `EaglePlayer*` matches versioned EaglePlayer titles.
- **ARexx port** is the exact public port name, such as `rexx_EP`. Its owning
  task is also used to locate and bring a window forward.

The launcher is considered running when any configured identity is present.
When all are blank, NovaDock derives the task name from Target as before. If a
running identity has no window that can be activated, NovaDock reports that
fact instead of launching a duplicate copy.

Workbench Tools entries and hot keys use the full Items catalogue, including
definitions currently removed from Organize. The extended fields are
written after the legacy icon/state fields, so older short configuration
lines continue to load unchanged.

`LAUNCH=` continues to launch AmigaDOS commands as in previous versions.
`DRAWER=` is a distinct type and does **not** send its directory path to the
AmigaDOS Shell as an executable. On Workbench 3.5+ it uses
`OpenWorkbenchObject()` through `workbench.library` V44+. On older
Workbench systems it attempts `C:WBRUN` if that utility is installed.
Classic 3.0/3.1 systems **without** a compatible Workbench opener need a
supported opener installed before drawer activation can work; no filesystem
changes are made by an unsuccessful click.

## Bundled icons

| Icon | Path | Suggested use |
|---|---|---|
| Workbench | `Icons/Workbench.iff` | OS/tools/application shortcuts |
| Drawer | `Icons/Drawer.iff` | Directory launchers |
| Games | `Icons/Games.iff` | Game drawers or launchers |
| Internet | `Icons/Internet.iff` | Network/browsers |
| Tools | `Icons/Tools.iff` | Configuration/utilities |

- 32 x 32, **IFF ILBM** 4 planes (16 indexed colors), transparent color 0.
- Read directly on AmigaOS 3.0+ by the included portable IFF parser. Colors
  are translated to the nearest existing Workbench palette entries with
  `FindColor()`. NovaDock deliberately retains no palette pens, so applications
  such as Directory Opus 4 can open their default shared-pen custom screens
  after the dock has started.
- The built-in IFF loader accepts raw or ByteRun1 compressed ILBM, up to
  64 x 64 and 4 planes, with transparency by index.
- Other picture files are decoded through `datatypes.library` and the
  installed picture datatypes. For example, PNG artwork works when a PNG
  datatype is installed; an alpha/mask plane is retained when the datatype
  provides one. Artwork is aspect-fitted and translated to existing Workbench
  palette entries without asking picture.datatype to reserve shared pens.
- Legacy Workbench `.info` is also supported using `icon.library`; omit
  `.info` from the path if preferred (`GetDiskObject` expects the base path).
  On icon.library V44+, NovaDock caches ColorIcons, NewIcons and classic
  planar icons as screen-mapped bitmaps. It honors generated masks and removes
  only dithered background pixels connected to the border of older icons;
  identical colours enclosed inside the artwork are preserved. The result is
  aspect-fitted to the dock tile. Older systems keep the planar renderer, with a
  textual fallback for unsupported or oversized icons.
- NovaDock treats DataTypes as optional. Missing formats fall back to the
  appropriate Workbench application or drawer icon, so configurations remain
  usable on minimal installations.

## Installation or upgrades

`Execute build.amiga` compiles the unified `NovaDockPrefs` as well as the dock.
`Execute install.amiga` copies that single Preferences tool and the five `.iff`
icons into `NovaDock:Icons/`. Existing user configuration
is preserved, including older two-field and three-field `LAUNCH=` entries.
Add new launcher entries via Preferences or edit the configuration file.
On a fresh install, two stock launcher tiles show bundled example icons.

## Dock context menu and direct manipulation (NovaDock 0.1)

The runtime dock includes a small native Intuition popup for each item type.
Apps offer **Launch**, **Edit settings...**, and **Remove**. Drawers
also offer **Popup contents**, a browsable icon grid or list with sorting,
nested drawers and type-ahead (see `DRAWER_POPUPS.md`), which opens files or
subdrawers without first opening the Workbench drawer window. Changes are
edited in the unified NovaDockPrefs application.

A new setting determines normal left-click behavior:

```ini
LEFT_CLICK=LAUNCH
```

- `LAUNCH`: normal left click opens an app/drawer (selected in the bundled
  sample); **right-click** opens its context popup with **Launch** first.
- `MENU`: normal left click opens the same popup instead.
- `NovaDockPrefs` offers the same option as **Click: Run / Menu**.
- Plugins keep their old click behavior: one click triggers the plugin's click
  action; Shift+click changes graph vs meter.
- An inactive dock accepts the first right-click: NovaDock observes the
  right-button-down event without consuming it, activates the dock, waits for
  the relative pointer position to settle, then opens the menu for that
  tile. The live input-device qualifier remains as a compatibility fallback.
  No left-click selection is needed.
- Popup height follows the number of actions. Click outside a popup to close
  it; no dedicated Cancel row is shown.

**Choose `Unlock dock` to rearrange:** NovaDock starts locked so an ordinary
drop cannot accidentally alter the dock. Dropping a Workbench object onto the
centre of an application tile is still available while locked: it opens that
object with the application and does not change the layout. Right-click a dock
gap and choose **Unlock dock** for drag-based layout editing. Explicit
right-click **Edit settings...** and **Remove** commands work consistently for
launchers and plugins; launch-capable plugins also offer **Launch**. These commands work
while locked because selecting one is already an unambiguous edit request.
The accent outline means Edit mode is active. Choose **Add item...**, then
**Spacer**, to append a persistent divider; it can then be dragged into place, removed from its own menu, or
dragged out just like another dock item.

**Drag an app or drawer from Workbench onto the dock:** The dock registers its
window with Workbench using `AddAppWindowA()` (available on AmigaOS 2.0+).
The drop message contains file/drawer locks. NovaDock copies and validates the
paths and type, then creates the matching application or directory entry.
It uses the object's own Workbench `.info` icon by default. For volume roots it
correctly resolves the special `Disk.info` name; if no private icon exists, it
uses Workbench's current system-default disk, drawer, or tool icon.
Volume-root tiles such as `System:` and `Scratch:` show a live used-space
meter by default. Every directory tile can use the same meter for the volume
that contains it: right-click the tile, choose **Edit**, then switch **Show
space usage** on or off. Ordinary subdirectories such as `SYS:Utilities`
default to off, while roots ending in `:` default to on. A disabled meter is
not polled, drawn, or added to the tooltip. Older configuration lines without
an explicit choice use those same root-on/subdirectory-off defaults; the final
extended field stores the per-drawer choice.
Multiple dropped objects are handled in drop-message order. The centre three
fifths of an application tile is its **Open With** target. NovaDock launches
one application instance per dropped object, placing the action's configured
arguments before the quoted object path and respecting its working directory.
ARexx actions, drawers, plugins and spacers are not Open With targets. In Edit
mode, the outer fifths of a tile remain insertion zones; dropping there inserts
before that tile, while dropping on the surrounding dock background appends
the objects.

NovaDock may be started from `User-Startup` before `LoadWB` has fully brought
the Workbench process online. If Workbench initially rejects the AppWindow,
NovaDock retries registration from its normal low-rate timer (or Intuition
ticks when `timer.device` is unavailable). No manual reload is required.

**Rearrange existing dock items:** In Edit mode, hold the left button on any
tile and drag to another tile position, then release. The source item moves to
the destination position; it is never activated during a drag. An insertion
line shows the destination. Releasing over an empty dock gap is a safe no-op;
only releasing genuinely outside removes an item from the active dock. Its
definition remains available in Preferences. This works for launchers,
drawers, plugins, and spacers.

**Persistence:** Direct rearranging, removal and Workbench drops update the
editable `ENV:NovaDock/novadock.cfg`, live-reload the dock and write the same
full configuration into `ENVARC:NovaDock/novadock.cfg`. Disabled plugins are
preserved because interactive changes operate on the *full* configuration,
not the running dock's compacted list. If every tile cannot fit at the minimum
readable size, full-sized **PREV** and **NEXT** tiles page through the items.
The application supports at most 12 tiles in this release. A short status
window confirms accepted/rejected drops, moves, removals and lock changes.

**Compatibility:** The native Workbench v36 AppWindow interface is used for
drops; no MUI/RTG is required. Classic `.info` images and 32x32 palette-based
ILBM images remain supported. Workbench v44+ can use `OpenWorkbenchObject`
when available, with Shell execution as a fallback for eligible programs.
A drawer opener (`C:WBRUN`) is still needed on some earlier OS3 configurations.

**Testing:** Portable unit tests cover config movement, overflow geometry,
disabled plugin preservation, and serialization. Native 68k compilation and
the locked dock menu have also been exercised on Workbench.

## Later 0.1: tooltips, Workbench drops, ARexx

See `ARexx_AND_DROPS.md` for drag-to-add/remove behavior, tooltips and control commands.


## Stack, quoting and running applications

- Every launcher, plugin and helper is started in a shell that first runs
  `Stack`, because a shell created by a Workbench-started dock would otherwise
  give commands only 4 KB. The stack is 16 KB, or the stack in the program's
  tool icon when that is larger.
- Paths are quoted with AmigaDOS escaping (`*` becomes `**`, a quote `*"`), so
  names containing `*` or `"` work in working directories, drawer popups and
  plugin paths.
- Running-application detection now also recognises programs started from a
  shell (including everything NovaDock launches) by their CLI command name,
  not only Workbench-started tasks. Click-to-front re-checks that the window
  still exists and brings its screen forward when the application uses its
  own screen.

Applications without configured launchers never create dock tiles. To browse
them without changing the dock layout, add the optional **Running Apps**
plugin; see `docs/TASKBAR.md`.

### Consistent artwork placement

Normal-size launcher and launcher-style plugin artwork fits within 48x40
pixels (or the available tile area when smaller), preserving its aspect ratio
and never enlarging small sources. Normal and double-width tiles use the same
artwork limit. Icons share one vertical center above their bottom label. This
also applies to the real configured DOpus application icon. Tiny tiles retain
their compact artwork area and central placement.
