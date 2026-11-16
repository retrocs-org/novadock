# PLACES plugin

Click **PLACES** for a quick picker of system drawers, mounted volumes and
five personal bookmarks. Choose a place with the mouse or arrow keys and Return.
Escape closes the picker. The list refreshes whenever you open it.

Edit settings can hide system shortcuts or mounted volumes. Each bookmark has
an optional label and a drawer selector; leave its path blank to omit it.
Save keeps the bookmarks permanently. Missing drawers fail quietly when opened.
The plugin does no volume scanning while idle, and does not track recent files.

Opening uses workbench.library version 44 or newer when available. Earlier
Workbench versions require the optional `C:WBRUN` command, like dock drawers.
The native design is inspired by the folder/bookmark/volume menu in Xfce Places:
https://docs.xfce.org/panel-plugins/xfce4-places-plugin/start
