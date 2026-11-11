# Script tiles

A Script tile shows whatever an ARexx script returns, so a status tile needs
no C compiler. Add one with **Preferences > Add item... > Application >
Type: Script tile**. Each Script tile is its own `Plugins/Script` process with
an ID such as `SCRIPT1`; add as many as you like.

## What the tile does

Every *Refresh seconds* the tile asks RexxMast to run the script as

```text
<script> UPDATE <id>
```

and when clicked (Click: *Run script*) as `<script> CLICK <id>`. Use
`PARSE ARG action id .` to read them. The script's **RESULT** becomes the
tile:

| RESULT | Tile |
|---|---|
| `value` | caption from the settings (or `SCRIPT`), the value |
| `caption|value` | both texts |
| `caption|value|meter` | plus a meter, 0-100 (empty or `-` for none) |
| `caption|value|meter|badge` | plus a badge of up to 7 characters (empty removes it) |

Texts are cut to 31 characters and control characters become spaces. A
script that ends without a RESULT keeps the tile's previous text, so a script
may also update the dock itself: its default host is `NOVADOCK`, and commands
such as `'SETBADGE' id '"3"'` work directly.

`NovaDock:Rexx/ScriptTile.rexx` is a small example that shows
the time, with the part of the day that has passed as the meter, and the date
when clicked.

## Running and errors

The script runs in RexxMast's own process. A slow script therefore never
stalls the tile or the dock, and the tile never starts a second run of the
same script while one is still going. The tile shows:

| Text | Meaning |
|---|---|
| `NO AREXX` | RexxMast is not running (start it from WBStartup or `SYS:System/RexxMast`) |
| `NO SCRIPT` | No script set in the tile's settings |
| `TIMEOUT` | No reply within *Timeout seconds*; the result is still shown when it arrives |
| `RC n` | The script failed with return code n (an ARexx error, or `EXIT n`) |
| `BAD RESULT` | More than four fields, or a meter outside 0-100 |

## Settings

| Setting | Meaning | Default |
|---|---|---|
| ARexx script | Script path (`REXX:` and the current drawer are searched, `.rexx` may be omitted) | the example |
| Refresh seconds | 1-86400 | 30 |
| Timeout seconds | 1-600 | 30 |
| Caption | Caption when the RESULT has none | (empty: `SCRIPT`) |
| Click action | `RUN` calls the script with `CLICK`; `NONE` ignores clicks | RUN |

Refresh intervals and dock heartbeats are independent. Long intervals (including
the default 30 seconds) do not trigger the 20-second stale-plugin watchdog.
