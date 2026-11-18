# SHELF plugin

SHELF keeps shortcuts to files and drawers. Add it through Preferences >
Organize > Add > Plugin. Each instance has its own label, artwork, contents
and stable instance ID. The count badge includes unavailable targets.

With the dock locked, drop one or several files/drawers on SHELF. They are
added to the same collection. A drop never executes, copies or moves the
original files. Duplicate paths are ignored without changing their order.
Unlock the dock to rearrange its tiles as usual.

Click SHELF for a native list. Type in **Filter** to narrow the visible
shortcuts. Select a row and click **Open**, press Return or double-click.
**Up**, **Down** and **Remove shortcut** change the saved collection;
they never delete or rename the target. **Add path** accepts a full AmigaDOS
path such as `Work:Documents/Readme`. Missing or unmounted targets stay saved
and can be opened after the volume returns.

**Edit settings** sets the label, built-in tile icon, optional custom picture
or .info icon, and storage drawer. An existing custom image takes priority.
The default drawer is `ENVARC:NovaDock/Shelves`. Each file is named after the
instance, for example `SHELF_1.shelf`. Up to 64 shortcuts of 259 characters are
supported. Saves use a temporary file and retain a `.bak` previous copy.
Unreadable/corrupt data is not overwritten silently; a valid backup can be
loaded instead. The browser title reports the last save/open result.

Removing a dock tile preserves its saved collection. Removing NovaDock with
**Keep settings** also retains it. **Remove settings** removes shelves in the
default settings drawer; separately configured storage stays untouched.
Opening objects on older Workbench versions requires `C:WBRUN`; Workbench
44 or newer uses its native object-opening API.
