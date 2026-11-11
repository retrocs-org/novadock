# Event hooks

NovaDock can run a command when something happens: a disk is inserted, the
network goes online, a plugin stops responding. Hooks are set in
`novadock.cfg` (Preferences keeps them when it saves), one per line:

```text
HOOK=EVENT|command
```

The dock runs the command in the background, with the event name and one
quoted detail appended, so one script can handle several events:

```text
HOOK=DISK_IN|Rexx:NovaDockEvents.rexx       runs  Rexx:NovaDockEvents.rexx DISK_IN "Work Disk"
HOOK=NET_ONLINE|Run >NIL: SYS:C/SetDate     runs  Run >NIL: SYS:C/SetDate NET_ONLINE "NET"
```

| Event | When | Detail |
|---|---|---|
| `STARTUP` | The dock has started and its plugins are running | NovaDock version |
| `QUIT` | The dock is quitting | (empty) |
| `DISK_IN` | A removable medium appeared (needs **Media: On** in Preferences) | volume name |
| `DISK_OUT` | A removable medium was removed (needs **Media: On**) | volume name |
| `NET_ONLINE` | The Network or Roadshow tile changed from offline to online | tile ID |
| `NET_OFFLINE` | The Network or Roadshow tile went offline | tile ID |
| `PLUGIN_STALE` | The watchdog restarts a plugin that stopped answering | plugin ID |
| `SAVE_FAILED` | Settings could not be saved | (empty) |

Up to eight hooks; an event may have several. The command is at most 95
characters. Network events report changes only: the first status after the
dock starts does not run a hook. A hook command starts like a launcher with
CLI start mode: use `Run >NIL:` or an ARexx script (`RX`, or a script file
with the `s` protection bit and a `/*` first line) for anything that should
not open a window.

An ARexx handler could look like this:

```rexx
/* Rexx:NovaDockEvents.rexx */
PARSE ARG event detail
detail = STRIP(detail,'B','"')
SELECT
  WHEN event = 'DISK_IN' THEN ADDRESS COMMAND 'Say Inserted' detail
  WHEN event = 'NET_ONLINE' THEN ADDRESS COMMAND 'SetDate >NIL: NTP'
  OTHERWISE NOP
END
```
