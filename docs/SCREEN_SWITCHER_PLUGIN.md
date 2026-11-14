# SCREENS plugin

Add **SCREENS** in Preferences → Add item → Plugin. Its executable is
`Plugins/ScreenSwitcher` and its ID is `SCREENSWITCHER`. The plugin starts at
its own version 0.1; the dock remains version 0.2.

Click the tile to list up to 32 public screens, sorted by registered name,
with their dimensions. Choose a screen with the mouse, or use the arrow keys
and Return. Escape, clicking outside, or clicking the dock tile again closes
the picker. Reopen it to refresh the list.

Switching brings the chosen screen forward without changing its resolution
or closing applications. Private screens are excluded. If a screen closes
while the picker is open, the tile reports “Screen closed”; reopen the list.
Screen Mode remains a separate plugin for saved resolution presets.

The screen list is scanned only when opening the picker. The plugin sleeps
between dock messages while idle, and keeps no screen pointers after a scan.
There are no plugin settings to configure.
