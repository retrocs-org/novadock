# WHDLoad Games plugin

The Games tile lists the WHDLoad games installed in a drawer and starts
them. Add it with **Add item > Plugins > WHDLoad Games** in NovaDock
Preferences, and set **Games drawer** in its settings (default `Games:`).

## Using it

Click the tile to open the games list. Then:

| Key or action | Result |
| --- | --- |
| Type the first letters | Jump to the first game with that name |
| Cursor up and down | Move through the list |
| Return, Play, or a double-click | Start the game |
| Rescan | Read the games drawer again, after installing games |
| Esc or the close gadget | Close the list |

The tile shows the number of games once the list has been read.

## Which games it finds

A game is a drawer containing a WHDLoad `.slave` file, either directly in
the games drawer (`Games:Turrican`) or one level down, as in collections
sorted by letter (`Games:T/Turrican`). Its icon is `<drawer>.info` or
`<slave name>.info`. The drawer is read when the list is first opened, never
when NovaDock starts.

## How games start

The plugin starts a game exactly as a double-click on its icon would: it
loads the icon's default tool (normally WHDLoad, looked for beside the game
and in C:, SYS:C, SYS:Utilities, SYS:System and SYS:Tools) and runs it as a
Workbench program with the icon as its argument, so WHDLoad reads the icon's
tool types (PRELOAD, Quit key and so on). This works on AmigaOS 3.0 and
later. If NovaDock is quit while a game runs, the plugin waits for the game
to end before it exits.
