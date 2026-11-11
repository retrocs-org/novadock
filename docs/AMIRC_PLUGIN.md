# AmIRC dock plugin

The optional AmIRC tile shows whether AmIRC is running and connected. It can
show connection status, your nickname, the current channel or the server name.
A numbered badge shows pending notification events when the supplied hook is installed.
When you are away, the caption changes to `AMIRC AWAY`. The corner dot appears
only after AmIRC confirms an IRC connection. It is cleared while offline,
disconnected, checking or unresponsive, and follows the dock running-indicator
setting. It does not indicate whether the bridge process itself is running.

Right-click the dock background, choose **Add**, and select **AmIRC** from the
plugins. It is supplied disabled; the six default tiles stay unchanged.
Right-click its tile and choose **Change settings...**:

- **AmIRC executable**: select your installed AmIRC program. This is needed
  to start AmIRC from the tile; an already running instance can be detected
  without setting the executable path.
- **ARexx port**: `AUTO` finds an `AMIRC` or numbered `AMIRC.n` public port,
  regardless of case. Set the exact port name to select a particular instance.
- **Display**: `NICK` (default), `STATUS`, `CHANNEL`, or `SERVER`.
- **Refresh ticks**: 100 by default; 50 ticks is one second. Normal NovaDock
  activity backoff also applies while idle or hidden.

Click the tile to start the selected program or restore AmIRC's window.
Workbench launch preserves AmIRC's normal icon/tooltype startup settings.
`OFFLINE` means there is no AmIRC port; `DISCONNECTED` means AmIRC is running
without a server connection. `SET PROGRAM` asks you to select its executable.
An unresponsive ARexx request displays `NO RESPONSE`; the dock continues running.

The plugin uses AmIRC's local `ISCONNECTED`, `GETAWAYSTATE`, `GETMYNICK`,
`GETCHANNEL`, `GETSERVERNAME` and `SHOW` commands. The last command runs only
when you click the tile, or after a click has launched AmIRC. Launching AmIRC
uses AmIRC's existing startup preferences.

The bridge uses non-blocking ARexx exchanges with at most one outstanding
request. If a client never replies, the request remains owned by that client;
on plugin shutdown its reply target is made non-signalling and retained rather
than freed unsafely. This exceptional case retains one small request/port and
one ARexx library reference. Normal replies and shutdown release all resources.

AmIRC is a separate application and is not bundled with NovaDock. The official
[AmIRC download](https://www.amirc.org/download) includes its AmigaGuide manual.
Commands were checked against `Documentation/AmIRC.doc` in the official
[AmigaOS 68020 package](https://www.amirc.org/sites/default/files/amirc_39_68020.lha).

## Numbered unread alerts

AmIRC 3.9 has no documented unread-counter query. NovaDock therefore counts
**Private Msg** and **Highlite** event notifications received while AmIRC is
not active. This is an unread **event** count, not an exact per-channel message
counter. If AmIRC fires both events for one message, BOTH mode counts both;
choose PRIVATE or HIGHLIGHT if you prefer one source. No message contents or
IRC traffic are forwarded.

The installer places `Rexx/NovaDockNotify.amirx` in the NovaDock drawer. To
connect AmIRC once:

1. Copy that script into your existing AmIRC `Rexx` drawer.
2. In AmIRC's **Settings → Events**, set the **Private Msg** Rexx command to
   `NovaDockNotify.amirx PRIVATE`.
3. Set the **Highlite** Rexx command to `NovaDockNotify.amirx HIGHLIGHT`.
4. Save AmIRC's settings. Highlight matching uses AmIRC's existing Hilite list.

Keep any existing event scripts by calling this script from them instead of
replacing them. The hook quietly does nothing if the NovaDock AmIRC plugin is
not running. It uses the originating AmIRC port, so another client cannot add
to a tile pinned to a different port.

The badge shows `1`, `2`, etc., capped visually at `999+`. It takes priority
over the connected dot and remains visible if you disable running-app dots.
Returning to an AmIRC window clears the count by default. A successful click
on the tile (AmIRC acknowledges SHOW) also clears it. A failed SHOW keeps it.
Switching to a different AmIRC instance or closing it resets the count.

**Notification events** selects BOTH (default), PRIVATE, HIGHLIGHT or OFF.
**Clear alerts when AmIRC is active** defaults to YES; set NO to acknowledge
only through the dock tile. Unread event counts are session-local, not saved.
