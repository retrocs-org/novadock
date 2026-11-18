# TOOLTYPES plugin

Add TOOLTYPES from Add Item → Plugin. Drop one Workbench icon onto its tile, or
click and use Browse to select a .info file. A base application/project path is
accepted too. Loading, browsing and saving run in a background worker.

The main window edits Default tool and Stack bytes. ToolTypes opens a native,
filterable list with Add, Edit, Remove, Save, Reload and Close. Select a row and
press Edit; enter one line and Apply it to the draft. Parentheses can be used
for disabled entries, such as `(OPTION=YES)`. Save commits the current draft.
Closing a window does not save it automatically. Reload loads the disk copy.

Each save loads the existing DiskObject, changes only its metadata and writes
a sibling temporary icon. The original icon is moved to `.info.bak` before the
temporary file takes its place. Existing backups are retained as `.info.bak2`,
`.info.bak3` and so on, up to 999. An incomplete write leaves the original alone;
a failed final rename attempts to restore it from the backup. The previous
file's bytes are preserved exactly in the backup. File date, comment and
protection are restored on the saved icon when available.

The loaded object's image and drawer data are retained. Encoded NewIcons `IM1=`
and `IM2=` entries and their warning header cannot be edited or removed. A
fingerprint detects changes by another application; Reload is required before
saving over a changed disk copy. Editing is disabled while a save is in flight.
Quitting the plugin cancels an uncommitted replacement when its worker notices
that the parent has gone away; an already committed save retains its backup.

This first version accepts 64 ToolTypes of up to 159 characters each, a default
tool of up to 159 characters and icon paths below 220 characters. Icons over
2 MiB, long/large ToolTypes collections and unsupported icon data are left
unchanged. Stack is 0 (Workbench default) or 4096–1048576 bytes. A required
NewIcons/ColorIcons decoder remains the responsibility of icon.library and the
installed Workbench environment. The plugin's tile art ships in ToolTypes.assets.
