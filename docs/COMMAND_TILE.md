# NovaDock command-output tile

A command tile runs any AmigaDOS command at a set interval and shows one
line of its output, optionally as a meter. It is a bundled plugin
(`Plugins/Command`); every tile is a separate instance with its own settings.

## Adding one

- Right-click the dock background, choose **Add item...**, select
  **Application**, then choose **Command tile** from its Type selector. Enter its command, refresh interval,
  output line, meter and click behaviour before choosing **Add item**.
- Or add another Command Tile and use `Version` as its command to make a
  simple version-status tile.
- In `novadock.cfg` a tile is an ordinary plugin entry whose program is
  `Plugins/Command`, followed by its settings:

```text
PLUGIN=Free RAM|Plugins/Command|FREERAM|METER|ON
OPTION.FREERAM.COMMAND=Avail TOTAL
OPTION.FREERAM.INTERVAL=10
```

Rename or configure a tile with **Edit...** in Organize or **Edit settings...**
from the tile's context menu.

## Settings

| Setting | Meaning | Default |
|---|---|---|
| Command | Shell command line (max 95 characters) | `Version` |
| Refresh seconds | Time between runs, 1-86400 | 60 |
| Output line | Which non-empty output line to show, 1-20 | 1 |
| Meter from output | `PERCENT` uses the first number in that line (clamped to 0-100) as the tile's meter and graph | `NONE` |
| Caption | Small caption text; empty keeps the tile label | empty |
| Click action | `RERUN` runs the command now, `COMMAND` starts the click command, `NONE` | `RERUN` |
| Click command | Command for `COMMAND` clicks, e.g. an application to open | empty |
| Timeout seconds | When the tile reports `TIMEOUT` | 30 |

The shown text is cleaned up: console colour/escape codes and control
characters are removed, runs of spaces collapse, and the text is cut to the
tile's 31 characters. If a command prints nothing the tile shows `NO OUTPUT`,
or `RC <n>` when it failed.

Examples:

```text
Version                         Kickstart 47.115, Workbench 47.3
Avail TOTAL       (line 1)      free memory in bytes
Info DH0:  + Meter PERCENT      uses the first number on the line
Date                            the current date and time
C:Ping -c 1 192.168.1.1 (line 2) a round-trip line
```

## How it runs commands

- Each run starts an asynchronous shell with `FailAt 2147483647` and
  `Stack 16384`, runs the command with its output in
  `T:NovaDockCmd-<ID>.out`, then writes the return code to
  `T:NovaDockCmd-<ID>.done`. The plugin only polls for that file, so a slow
  command never blocks the plugin, the dock or other plugins, and the watchdog
  keeps seeing heartbeats.
- A command that exceeds the timeout shows `TIMEOUT`. AmigaDOS has no safe way
  to kill another shell, so the next run starts only after it ends.
- Clicks are delivered at once (the plugin sleeps with `nd_client_wait`).
- Commands run with your own rights, exactly as typed in a Shell. Only
  configure commands you would run yourself; the tile never executes text
  from command output.
