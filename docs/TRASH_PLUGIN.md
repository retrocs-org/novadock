# Trash plugin

The Trash tile is the Workbench Trashcan on the dock. Add it with
**Add item > Plugins > Trash** in NovaDock Preferences.

## Using it

| Action | Result |
| --- | --- |
| Drop files or drawers on the tile | They move to the Trashcan of their own volume |
| Click the tile | Opens the Trashcan on the system disk (SYS:Trashcan) |
| Menu: Open trash | The same |
| Menu: Refresh | Recounts the known Trashcans |
| Menu: Empty trash... | Asks, then deletes everything in the trash |
| Menu: Remove | Removes the tile from the dock |

The tile shows a closed trashcan when empty and a paper-filled trashcan
when it contains items. A badge shows the count. The icons follow your theme.

## How it works

Dropped objects are moved, never copied: each goes into the `Trashcan`
drawer of the volume it is on, together with its icon, so moving is
instant and cannot leave half-copied data. A volume without a Trashcan
gets one, with the standard Workbench trashcan icon. If an object of the
same name is already in the trash, the new one gets a `.1`, `.2` suffix.
Whole volumes and objects already in a Trashcan are refused.

**Empty trash...** names the number of items and asks before deleting
anything; deleting uses `C:Delete ... ALL QUIET FORCE`, so it also removes
protected files. It empties SYS:Trashcan and every Trashcan the plugin has
used since the last reboot. Other Trashcans can be emptied from Workbench.

The icon and count refresh automatically when files are added, removed or
renamed in SYS:Trashcan or another Trashcan used by NovaDock since startup.
Filesystem notifications trigger a recount; idle disks are not repeatedly
scanned. A newly created system Trashcan is watched too.

Filesystems without notification support can be updated with **Refresh** in
the tile menu. Changes made through NovaDock also refresh the count. Other
volumes' Trashcans are included after NovaDock has used them.

With AmigaOS 3.5 or later, NovaDock trash actions also update open Workbench
windows; on 3.0 and 3.1, use **Window > Update** in Workbench.
