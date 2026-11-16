# WHDLoad plugin

Each WHDLoad tile has its own label and one folder to index. Add it using
**Add item > Plugins > WHDLoad**. You can add it more than once, for example
**Games** and **Demos**, without sharing their settings or game lists.

Right-click a tile and choose **Edit settings**:

- **Tile label**: the name displayed on this tile, up to 31 characters.
- **Scan folder (recursive)**: the root folder for this instance. Choose
  **Select** to browse drawers, or type an Amiga path such as `Work:Games`.
  New tiles have no folder selected. All real subdirectories are searched.
- **Tile icon**: new instances select the dedicated WHDLoad icon by default.
  The settings preview shows the selected artwork. Choose **Select** to open a gallery of illustrated icons. Click a
  thumbnail then **Choose**, or double-click it. Arrow keys move the selection;
  Return chooses and Esc cancels. Small screens have Previous/Next pages; paging buttons are hidden when all icons fit.
  The library includes WHDLoad, Games, Demos, Apps, Arcade, Music, Graphics, Code, Movies,
  Books, Disks, Network, Utilities, Favourites, Drawer, Workbench, Tools and Internet.
- **Custom icon or picture**: choose **Select** and browse to a game's `.info`
  file or another icon/picture. An existing custom file takes priority over the
  library selection automatically; clear the field to use the library icon.
  A missing custom file falls back to the selected library icon.
  The original file and its ToolTypes are not changed.

Choose **Use** for this session or **Save** to keep the settings after restart.
Changing the folder invalidates that instance's previous index and starts a new
background scan. Other WHDLoad tiles keep their own lists and settings.
Old `OPTION.WHDLOAD.DRAWER` settings and existing custom labels are retained.

## Using it

Click a tile to open its list. Its corner badge shows the indexed count, including
**0**, or **...** while indexing. Saved counts load after the tile connects;
a missing cache starts a scan in small batches, keeping the dock usable.
A zero-count tile does not open an empty browser. Clicking it rescans and opens
the browser only if installs are found. Progress appears in its tooltip.

The **Filter** field narrows the list as you type. Words match anywhere in a
title, ignoring case; all entered words must match. **Clear** restores the full
list. The window title shows the filtered and total counts.

An unconfigured or invalid folder shows a sad
WHDLoad icon. Hover over it to see whether a drawer is unset, missing, a file, unreadable or
on an unavailable volume. Choose a valid folder in Edit settings to restore
the selected artwork.

| Key or action | Result |
| --- | --- |
| Type the first letters | Jump to the first matching name |
| Cursor up and down | Move through the list |
| Return, Play, or a double-click | Start the selected install |
| Rescan | Rebuild this tile's index |
| Esc or the close gadget | Close the list |

An invalid root shows the reason in its tooltip. An incomplete scan is
identified in the window title rather than presented as a complete list.

## Persistent index cache

Each instance saves a separate, checksum-validated index under
`ENVARC:NovaDock/Cache/WHDLoad/<instance-id>.idx`. It is reused across plugin,
dock and Workbench restarts. Opening a cached list does not walk the folders
again. The window identifies it as a cached index.

Use **Rescan** after adding, moving or removing games/demos. Changing only the
tile label or artwork keeps its index. Changing the scan folder invalidates the old index.
A missing, corrupt or incompatible cache automatically triggers a new scan.
Incomplete scans never replace a successful cached index. Cache write failures
leave the current list usable and are reported in its window title.

## Which installs it finds

An install is a drawer containing a WHDLoad `.slave` file and a usable icon.
The scanner searches the root itself and every normal subdirectory, including
collections nested by category, letter and game/demo name. Directory links and
soft links are skipped to prevent cycles or indexing outside the selected root.

The icon can be named after the drawer or slave inside the install, or be the
`.info` icon beside the drawer. Installs without an icon are not launchable
and are omitted. Each drawer contributes one entry. Paths are limited to 299
characters and the list to 10,000 entries; allocation/read errors or those limits
mark the index incomplete. No files in the collection are changed.

## How installs start

**WHDLoad command** defaults to `WHDLoad`. Enter an executable name or full
Amiga path, or use **Select** to choose a particular WHDLoad executable.
Each instance saves its own command, so games and demos can use different
versions. An empty value falls back to `WHDLoad`. Changing it reuses the index.

The plugin looks for a bare executable name beside the install and in C:,
SYS:C, SYS:Utilities, SYS:System and SYS:Tools. A full path selects that file.
It runs the configured executable as a Workbench program with the original
icon as its argument, retaining the
icon's ToolTypes, including PRELOAD and the quit key. This works on AmigaOS 3.0
and later. If NovaDock quits while an install runs, its plugin waits for it to
finish before exiting.

Additional tiles receive unique IDs such as `WHDLOAD_2`. Their labels, roots and commands
are stored separately in the normal `novadock.cfg`; they do not need duplicate
executables or a separate plugin configuration file.

## Removing a tile

Removing a WHDLoad tile clears that instance's label, folder, command and artwork
settings, along with its saved index and any temporary/backup index files.
Other WHDLoad instances and the actual games/demos are untouched. The bundled
WHDLoad entry remains available in Add item with fresh default settings.
