# Directory Opus 4 plugin

NovaDock includes an optional `DOpus4` plugin. It is installed with the other
plugins but is added through **Add item > Plugins**, so existing docks do not grow or
start Directory Opus unexpectedly.

Add it from the dock background's **Add item... → Plugins** window. The tile reports `READY` while DOpus's
main user interface is visible, `ICONIFIED` while DOpus is resident with its
main screen closed, and `OFFLINE` when no instance is present. The bridge checks
the configured names and `DOPUS.1` through `DOPUS.9`, because DOpus selects the
first free numbered port. Clicking an iconified tile sends DOpus 4's native
uniconify signal directly to the main process that owns its ARexx port, waits
for the screen to return, and then sends `DOPUSTOFRONT`. This does not depend on
the user's configured hotkey or require changes to DOpus's defaults.
Clicking a genuinely offline tile starts the configured Directory Opus
executable. While that launch is
being detected, the tile says `STARTING`; a failed launch or ARexx command is
reported briefly as `START FAILED` or `FRONT FAILED`.

This is a launcher and integration tile, not a meter. It therefore has no
Graph/Bar display mode and clicking it never changes the tile layout. Its
right-click menu matches an ordinary launcher with **Launch** (start, restore,
or bring to front), **Edit settings...**, and **Remove**.

The settings are part of the normal `novadock.cfg` file:

```text
OPTION.DOPUS4.PORT=DOPUS.1
OPTION.DOPUS4.ALT_PORT=DOPUS
OPTION.DOPUS4.RESTORE_COMMAND=UNICONIFY
OPTION.DOPUS4.FRONT_COMMAND=DOPUSTOFRONT
OPTION.DOPUS4.PROGRAM=SYS:DirOpus4/DirectoryOpus
OPTION.DOPUS4.CONFIG=
OPTION.DOPUS4.POLL_TICKS=25
```

Use **Edit...** on the plugin in Organize to adapt the port, commands or program
path to your installation. **Select** beside Program path opens the native ASL
file requester, while the field remains editable for paths such as assigns. The
standard restore and front commands need no
changes to the default DOpus configuration. Leave **Configuration file** empty
to let Directory Opus use its own normal default configuration. If a file is
selected, NovaDock only passes it to DOpus with `-c`; it never edits, replaces,
or converts the file.

NovaDock deliberately avoids retaining palette pens for launcher artwork. This
allows the default DOpus custom screen to open after NovaDock on palette-based
and RTG systems without changing DOpus's screen settings. NovaDock does not
include Directory Opus itself and does not copy its code or artwork.

The dock window uses simple refresh rather than a full-width smart-refresh
backing bitmap, and NovaDock releases its Workbench public-screen lock as soon
as the window has opened. This keeps display memory and screen ownership
available for DOpus's default Workbench-clone screen.

When `CONFIG` is empty, the plugin launches `DirectoryOpus` through Workbench,
not as a plain CLI command. This preserves the installed icon's standard
`ICONSTART`, `BUTTONSTART`, `CONFIGFILE`, and related tooltypes. Once DOpus's
ARexx port appears, the plugin brings its screen forward automatically. An
explicit `CONFIG` value still uses DOpus's documented `-c` command-line form.

The resident-task name, numbered-port search, uniconify signal, and visibility
handling mirror the GPL DOpus 4.16 implementation. They do not require changing
DOpus's default configuration.
