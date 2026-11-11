# NovaDock plugin SDK — protocol 1 with graph (v2), media (v3) and extended (v5) packets

NovaDock plugins are **ordinary, independent AmigaOS 68k executable processes**.
There are no undocumented jumps into dock memory, no shared C runtime, and no
need to ship a custom `.library`. A plugin communicates using `exec.library`
message ports; the dock draws the tiles. A faulty plugin is much less likely to
corrupt dock internals than a directly linked binary plugin, though native
AmigaOS has no modern process memory protection.

For a standalone build kit and a first-plugin walkthrough, start with
[`PLUGIN_SDK_QUICKSTART.md`](PLUGIN_SDK_QUICKSTART.md). The public download
named `NovaDock-0.1-plugin-sdk.zip` contains only the public headers, safe
client helper, Makefile and Hello example; building the complete dock is not
required.

## Running a plugin

Add a line to `novadock.cfg`, then reload via NovaDockPrefs (Use/Save) or restart NovaDock:

```ini
PLUGIN=My Widget|Plugins/MyWidget|WIDGET1
```

The fields are the initial tile label, executable path, and unique plugin ID.
Relative paths are resolved from NovaDock's executable directory by the host.
Absolute paths such as `Work:Widgets/CPU` are permitted. The host starts the
program asynchronously and passes the plugin ID as **argv[1]**. IDs have 1–19
ASCII letters, digits, `-`, `_`; no spaces. Plugins must be CLI executables;
the dock launches them, not Workbench.

**The configuration explicitly enables each plugin.** Preferences discovers
self-describing plugins from the installed Plugins drawer. Plugin IDs are
unique by default. A manifest may opt into repeated dock tiles with
`NOVADOCK_MULTIPLE=YES`; those views intentionally share one process, one
live state and one settings namespace rather than starting competing clients.

## ABI layout

See [`include/nd_protocol.h`](../include/nd_protocol.h) for the complete
`NDMessage` wire structure. The original message prefix remains protocol `ND_ABI_VERSION = 1`.
The optional graph protocol accepts `ND_GRAPH_ABI_VERSION = 2` with two appended samples.
The optional media protocol accepts `ND_MEDIA_ABI_VERSION = 3` with control
capabilities, play state, volume and a reply-side requested volume. **v1 binaries
remain valid**; graph and media functionality are opt-in and v1 progress
meters can be displayed as a single-line graph.

- The host registers the named public port `NovaDock.1`.
- A plugin allocates a private reply `MsgPort`, fills one `NDMessage`, and
  sends it to the host with `PutMsg`.
- `FindPort` and `PutMsg` occur together under `Forbid`/`Permit`: never cache
  a public port pointer outside this protected critical section.
- `mn_ReplyPort` must point to the sender's live reply port;
  `mn_Length = sizeof(struct NDMessage)`.
- On `ND_MSG_UPDATE`, the host copies `caption`, `value`, and `meter` into its
  own tile model. Each char array must be NUL terminated.
- `meter` ranges 0–100 (shown as a progress strip); 65535 hides it.
- `ND_EVENT_THROTTLE` is an additive v1-compatible reply hint indicating that
  auto-hide currently shows only the reveal tab. Headless plugins may call
  `nd_client_delay_ticks()` with their normal delay to obtain a watchdog-safe
  2..10-second backoff. Older plugins can ignore the bit. Interactive plugin
  windows should retain their normal event cadence.
- For v1 messages, the same meter can also be plotted as a one-line history
  graph when the tile is configured `GRAPH` or toggled with Shift + click.
- For v2 messages, the appended `graph_a` and `graph_b` fields provide
  **independent** 0–100 chart samples; 65535 creates a gap (no data). The
  dock must check the message version **and** `mn_Length` before accessing
  these fields. Incomplete v2 messages are rejected.
- For v3 messages, `capabilities`, `state` and `volume` let the host draw
  inline Previous, Play/Pause, Next and volume controls. Their actions are
  returned through `ND_EVENT_MEDIA_*`; an absolute slider request is returned
  in `requested_volume`. Incomplete v3 messages are rejected.
- The dock `ReplyMsg`s the same message. The `event` field in that reply uses
  `ND_EVENT_NONE`, `ND_EVENT_CLICK`, `ND_EVENT_QUIT`, optional settings, and
  the v3 media-action bits.
- The **next update** after a user clicks a plugin tile receives `CLICK`, except
  for host-owned interactive surfaces such as MusicPlayer's inline controls.
  The plugin may use this to toggle modes (as the clock/RAM plugins do).
- On `QUIT`, or when the named host port disappears, the plugin should exit.
- A plugin can send `ND_MSG_GOODBYE` before exiting. The dock then shows it
  as offline; on a normal dock exit the public port is removed and all queued
  plugin messages are replied to with `QUIT`.

Messages are always sent **one at a time** by the bundled client helper. Never
free or mutate a sent message until its reply arrives. If implementing your
own client, ensure the message has the memory lifetime necessary for
asynchronous Exec message passing. Plugins should sleep between updates,
for example with `Delay(50)` to refresh once per second.

The ABI is currently **single instance**: only one dock publishes the name
`NovaDock.1`. It deliberately uses native 68k integer sizes and structure
alignment, and is not a network or cross-endian protocol. Third-party
programs can use another compiler, but must match the NDK struct layout and
version. The `caption` field is display text chosen by the plugin, distinct
from the configured initial label.

## Fast path: helper client

See the complete template in `plugin_src/template/hello.c`. The standalone SDK
places the same canonical source at `examples/hello.c`:

```c
struct NDClient client;
unsigned int event;
nd_client_init(&client, argv[1]);
for (;;) {
    event = nd_client_update(&client, "HELLO", "42", ND_NO_METER);
    if (event & ND_EVENT_QUIT) break;
    Delay(50);
}
nd_client_close(&client);
```

Link `plugin_src/common/nd_client.c` into your own executable and include the
headers from `include/` and `plugin_src/common/`.

```text
vc -O -Iinclude -Iplugin_src/common -o Plugins/Hello \
  plugin_src/template/hello.c plugin_src/common/nd_client.c
```

Amiga Shell commands need to be entered as one line rather than using the
Unix backslash continuation above. The standalone SDK also includes a
ready-to-use Makefile, so `make` builds `Hello` directly.

Then add `PLUGIN=Hello|Plugins/Hello|HELLO1` to the dock configuration.

## Reducing idle traffic for stable status tiles

Status-only plugins do not need to exchange an identical packet on every
poll. The client helper provides `nd_client_update_quiet`, which sends
immediately when the caption, value or meter changes and otherwise sends only
the requested heartbeat:

```c
/* The loop sleeps 10 ticks; 100 ticks gives an unchanged tile a 2 s heartbeat. */
event = nd_client_update_quiet(&client, "STATUS", value, ND_NO_METER,
                               10, 100);
if (event & ND_EVENT_QUIT) break;
Delay(10);
```

`elapsed_ticks` is the time slept since the preceding call, not the absolute
system time. A skipped exchange returns `ND_EVENT_NONE`. Keep the heartbeat
comfortably below the host's 20-second stale timeout; 100 ticks (two seconds)
is the recommended stable-tile default.

Graph plugins can sample at their normal cadence without sending unchanged
samples by using `nd_client_update_graph_quiet`. It has the same graph values
as `nd_client_update_graph`, followed by `elapsed_ticks` and
`heartbeat_ticks`. The bundled CPU and RAM plugins sample once per second but
send only changed values or a ten-second heartbeat.

Use the normal update helpers for plugins that must receive every click
immediately. Plugin actions are returned in the reply to the next exchange,
so a quiet heartbeat is also the maximum input latency while an unchanged
tile is idle. The bundled Clock, Eyes and Archive Extractor use a two-second
idle heartbeat. DOpus 4 uses one to two seconds so its start/front action
remains quick. MusicPlayer keeps its control-driven exchanges.

The helper remembers the latest host pacing hint:

```c
Delay(nd_client_delay_ticks(&client, normal_poll_ticks));
```

The returned delay equals the configured value while visible. While hidden it
is four times longer, clamped to 100..500 AmigaDOS ticks (2..10 seconds), which
remains comfortably inside the 20-second watchdog interval.

## Adding graph data from a third-party plugin

In `novadock.cfg`, enable the sparkline on just the intended tiles:

```ini
PLUGIN=Example|Plugins/Example|EXAMPLE|GRAPH
PLUGIN=Clock|Plugins/Clock|CLOCK|METER
```

Hold **Shift** and left-click a plugin tile at runtime to switch between graph
and meter without affecting a plugin's own click handler. View toggles are
not currently persisted; the optional fourth field sets the startup mode.

For **one graph series**, the v1 helper is sufficient: its `meter` can be
sampled as history even if the current display is set to `METER`.
To supply **two distinct traces**, use the v2 helper:

```c
/* Both series and the existing bar are independently normalized to 0..100. */
event = nd_client_update_graph(&client, "NET", "14/7 p/s",
                               14, 14, 7);
/* Use ND_GRAPH_NO_SAMPLE for absent/invalid data or a single series. */
```

For a graph that is often unchanged, use the quiet form:

```c
event = nd_client_update_graph_quiet(&client, "NET", "14/7 p/s",
                                     14, 14, 7,
                                     elapsed_ticks, 500);
```

Here 500 ticks provides a ten-second heartbeat while changed samples are sent
immediately.

The graph API is defined in `include/nd_protocol.h`, and the implementation
is in `plugin_src/common/nd_client.c`. `struct NDGraphMessage` begins with an
unchanged `struct NDMessage` prefix; `graph_a` and `graph_b` are appended
UWORD values. Sending a graph-ABI message to a host that only implements ABI v1 is not supported;
use `nd_client_update` (v1) for widest backward compatibility.

The host stores **96 samples per tile** in a fixed-size ring buffer and draws
pixels/lines itself using graphics.library. Each update advances the graph by
one sample. These are normalized samples, **not arbitrary bitmap draws** or
raw bytes-per-second readings. At 1 update/sec a full graph spans about 96
seconds. When a plugin goes offline its history is cleared. A regular click
remains a plugin event, while Shift + click is consumed by the dock.

## Safety and stability

- Plugins are not sandboxed: treat third-party executables like any other
  Amiga program, and get them from trusted authors.
- Plugin IDs are a routing key, **not authentication**. Another local task
  with access to the public port can spoof messages.
- The host owns all drawing and doesn't use plugin-supplied bitmap pointers.
- IPC updates include a fixed-size, bounded caption and value; no arbitrary
  GUI calls are performed by the host on the plugin's behalf.
- The host validates message size, magic, protocol version, and a configured ID.
- Messages are synchronous from the helper client, with a fifteen-second
  `timer.device` deadline (`ND_PLUGIN_IPC_TIMEOUT_SECONDS`). If the public dock
  port exists but stops replying, the helper returns `ND_EVENT_QUIT` so the
  plugin can exit. The helper allocates one public wire packet at startup and
  reuses it for every normal exchange.
- After a timeout, the dock may still own the outstanding message. The helper
  therefore turns the private reply port into a signal-free `PA_IGNORE` target
  and deliberately leaves that one packet and port valid until reboot instead
  of risking a use-after-free. This exceptional leak is bounded to one timed-out
  exchange per plugin process; the normal and missing-host paths release all
  resources.
- ABI v1 supports short text, progress bars, and click callbacks. The optional
  ABI v2 extension adds two numeric graph samples, but **not arbitrary canvas
  drawing**. Custom canvases, notifications, plugin permissions and hot reload
  remain future work.

## Bundled examples

| Plugin | Display | Click behavior | Notes |
|---|---|---|---|
| `Clock` | Current system time | Toggle 24h/12h | Reads AmigaDOS DateStamp |
| `Activity` | CPU used percentage with graph | Shift-click graphs | EClock time outside a lowest-priority Exec task |
| `Memory` | Separate Chip (`G`) and Fast (`F`) used-total meters and graphs | Per-tile settings | Uses AvailMem; Chip is Workbench's `graphics mem`; legacy `RAM` IDs still support all/chip selection |
| `Network` | RX/TX packets/sec, dual graph | Cycle RX/TX, RX and TX | Reads a configured SANA-II interface; `OPTION.NET.*` in the unified `novadock.cfg` |
| `ArchiveExtractor` | Installed unpackers and latest result | Drop an archive | Host-routed ZIP, LHA/LZH, RAR and ARJ extraction with a live Shell progress window; see `ARCHIVE_PLUGIN.md` |
| `Hello` | Text from example plugin | Toggle text | SDK template, not enabled by default |

Archive drops are a bundled host integration rather than a new wire-protocol
feature: NovaDock validates the Workbench drop, then starts the same
`ArchiveExtractor` executable in short-lived helper mode. Third-party plugin
ABI compatibility is unchanged.

**Network meter terminology:** `S2_GETGLOBALSTATS` supplies aggregate packet
counters, not global byte totals. The network plugin reads a configured
SANA-II device without an exclusive open and calculates RX/TX packets/sec.
`SCALE_PPS` controls the bar scale, not bandwidth usage. See the README for
configuration and driver limitations. Its parser/rate implementation lives in
`plugin_src/network/nd_network.c` and `include/nd_network.h`.

**CPU meter terminology:** `Activity` briefly wakes a lowest-priority Exec task
and uses its task-switch callbacks to accumulate EClock time spent idle during
a 0.1-second sample. Each sample displays `100 - idle_percent`, clamped to
0–100. The task blocks for the rest of the update interval instead of continuously
sampling idle time. A task monitor may attribute the free part
of that short window to `NovaDock CPU sample (free)`; it is not NovaDock load.

## NovaDock 0.1 preferences and control port

The independent preferences executable does **not** send plugin messages to
`NovaDock.1`. It writes text to `ENV:NovaDock/novadock.cfg` for **Use** or
both `ENV:` and `ENVARC:` for **Save**, then sends a synchronous versioned
control message to the separate `NovaDock.Control.1` port. See
`include/nd_control.h` and `src/control_client.c`. The host replies with a
status code; a missing dock is not an error to the preferences editor. Ordinary
commands and Diagnostics requests use a five-second `timer.device` deadline.
Reload uses its own bounded 20-second deadline because it completes the window,
artwork, colour, font and plugin rebuild before acknowledging Preferences.
`ND_CONTROL_TIMEOUT` is distinct from an invalid configuration, and the
client's heap-backed packet/reply target remains valid if a wedged dock still
owns it after either deadline.

`ND_CONTROL_DIAGNOSTICS_RESET` starts a new measurement baseline. It clears
telemetry totals and per-plugin diagnostic counts, but deliberately retains
plugin liveness timestamps, graph history, online state, watchdog retry/backoff
state and configuration. The Preferences Diagnostics page can also save its
current human-readable rows to `RAM:NovaDock-Diagnostics.txt`; this export is
local to Preferences and does not add another control-port message format.

The optional fifth `PLUGIN` config field is `ON` (default) or `OFF`:

```ini
PLUGIN=Example|Plugins/Example|EXAMPLE|GRAPH|ON
PLUGIN=Disabled|Plugins/Disabled|DISABLED|METER|OFF
```

IDs remain unique across enabled and disabled entries. The editor can reorder
entries, flip ON/OFF and GRAPH/METER without altering the executable path or
plugin IDs; paths and IDs may still be edited manually. Existing v1/v2 plugin
binaries do not need to link any preferences client code.

A removed/disabled plugin receives `ND_EVENT_QUIT` at its next ordinary
status update. Existing IDs retain their plugin instance when reloading;
changing the executable path for an existing ID requires restarting the dock.
The current ABI lacks authenticated registration and instance generation
identifiers, so immediate OFF→ON or recovery of a genuinely hung task can
briefly duplicate a process. Plugins should poll and honor `ND_EVENT_QUIT`
promptly rather than relying on host shutdown or direct task killing.

After a blocking reload, online plugins receive a fresh 20-second watchdog
window; diagnostics still show their actual last update time.

The host checks heartbeats on its existing low-rate housekeeping timer; it
does not add a second polling task. An enabled plugin that produces no update
for 20 seconds is marked stale and relaunched. `ND_MSG_GOODBYE` schedules a
quicker five-second retry. Repeated failures wait 20, 40, 80, 160 and then
300 seconds between attempts, preventing a bad path or broken executable from
creating a shell-launch storm. Plugins with unusually slow work should still
send a lightweight status heartbeat inside the 20-second window.

## Network connection example

`plugin_src/network/` combines traffic sampling with Roadshow connection
controls. Its shared `nd_roadshow.c` backend validates command arguments;
its independent worker owns connection and Details windows. See
[NETWORK_PLUGIN.md](NETWORK_PLUGIN.md). No standalone Roadshow plugin is shipped.

## MusicPlayer ARexx bridge example (0.1)

`plugin_src/musicplayer/` uses the optional v3 media extension while retaining
the unchanged v1 message prefix. It detects EaglePlayer, AmigaAMP or HippoPlayer
ARexx public message ports. Its fixed-width tile is the control surface:
transport buttons and a capability-gated volume slider are available directly
in the dock, while clicks on non-control space are intentionally inert.
Player selection and ARexx actions stay inside the separate plugin process.
The `nd_musicplayer.c` config/selection component builds with portable C89.
See [`MUSICPLAYER_PLUGIN.md`](MUSICPLAYER_PLUGIN.md) for setup, selection,
ARexx limitations, and upgrade instructions.

## One-file settings API (0.1)

All options are stored in `novadock.cfg`. Right-click a plugin ->
**Plugin settings...** to open the plugin page in `NovaDockPrefs`. Custom modules may declare
`OPTIONDEF=ID|KEY|Label|BOOL/CHOICE/UINT/TEXT|choices-or-range|default`
inside that file and receive `ND_EVENT_SETTINGS` on changed preferences.
Existing ABI 1/2 message structs have not changed; v3 is additive.
See `docs/PLUGIN_SETTINGS.md` and `include/nd_option_runtime.h`.

## Bundled xeyes-style Eyes tile

`plugin_src/eyes/eyes.c` is a regular v1-ABI plugin process. The host has a
special-case native widget for plugin ID `EYES`, using
`src/eyes.c` for bounded integer pupil calculation and Intuition/graphics
raster drawing. This is not a general plugin-provided custom drawing API:
third-party plugins continue to submit text, meter or graph samples to the
host. The separate Eyes executable is still needed for the dock to report
it online and show the tracking display.

Seven settings are declared in `src/nd_options.c` and kept in the same
`novadock.cfg`; native Preferences provides editing automatically.


## Badges (ABI 4, optional)

A plugin can show a small label on its tile's top-right corner, for
example an unread count:

```c
event |= nd_client_set_badge(&client, "3");   /* show "3" */
event |= nd_client_set_badge(&client, "*");   /* a plain dot */
event |= nd_client_set_badge(&client, "");    /* remove it */
```

Badges are up to seven characters, drawn in the theme's secondary accent
colour, and shown as a dot on tiny tiles or when the text does not fit.
They take the place of the running dot in the same corner, appear on every
tile of a multi-tile plugin, and disappear when the plugin says goodbye.
Call `nd_client_set_badge()` only when the badge changes; it sends nothing
when the text is unchanged.

Older docks answer unknown commands with QUIT, so badges use a
handshake: a dock with badge support sets `ND_EVENT_HOST_BADGES` on every
reply, and the helper sends `ND_MSG_BADGE` only after seeing it. A badge
set earlier is held and sent automatically. On an older dock the badge is
simply never shown and the plugin keeps working, so using badges does not
require `NOVADOCK_ABI=4`.

Without the helper, send a v1 header with `command=ND_MSG_BADGE` and the
badge text in `value` (caption and meter are ignored), and only to a dock
whose replies carry `ND_EVENT_HOST_BADGES`.

Always test reply events with `&`: new docks may set bits your plugin does
not know about, such as `ND_EVENT_HOST_BADGES`.

## Waking on clicks (optional)

Plugins traditionally sleep with `Delay()`, so a click waited up to one poll
interval. The client library now provides

```c
unsigned int nd_client_wait(struct NDClient *c, unsigned int ticks);
```

It sleeps for up to `ticks` (1/50 s) but returns as soon as NovaDock signals
`ND_WAKE_SIGNAL` (`SIGBREAKF_CTRL_F`), which the dock sends to the task owning
the plugin's reply port when a click, media action or settings change is
queued. The next exchange then delivers the event. The return value is the
number of ticks that actually elapsed and can be passed straight to
`nd_client_update_quiet()`. The message ABI is unchanged; plugins that keep
using `Delay()` simply ignore the signal. Do not use `SIGBREAKF_CTRL_F` for
anything else in a plugin's main task.

A plugin built with the current client library also publishes a signal-less
public port named `NovaDock.Wake.<ID>` (`ND_WAKE_PORT_PREFIX`) whose
`mp_SigTask` is the plugin's process. NovaDock then signals the plugin only
while that port still names the task from the latest exchange, so an exited
plugin's task pointer is never signalled, even if Exec has reused its memory.
`nd_client_init()` and `nd_client_close()` manage the port; plugins built with
an older client keep the earlier task-pointer wake-up.

## Second event word (ABI 5, optional)

Every bit of the 16-bit `event` field is in use. ABI 5 adds a superset packet,
`struct NDExtMessage`, whose reply carries a 32-bit `event2`. A dock that
understands it sets the priority (`ln_Pri`) of its public `NovaDock` port to
`ND_HOST_ABI` (5); older docks leave it at 0. `nd_client` reads that value when
it finds the port and only then sends v5 packets, so a plugin built with the
current client works unchanged with older docks. Existing calls
(`nd_client_update`, `_graph`, `_media` and their quiet variants) need no
change. Collect the new events with

```c
unsigned long nd_client_events2(struct NDClient *c); /* ND_EVENT2_*, clears */
```

which is always 0 with an older dock. `ND_EVENT2_REFRESH` asks for a fresh
value now. Writing the packet by hand: set `version` to `ND_EXT_ABI_VERSION`,
`mn_Length` to `sizeof(struct NDExtMessage)`, and `flags` to
`ND_EXT_HAS_GRAPH` and/or `ND_EXT_HAS_MEDIA` for the fields you filled in, and
send it only when the dock port's `ln_Pri` is at least 5.

## Settings and long configuration lines

`nd_plugin_options_effective()` first reads `ENV:NovaDock/novadock.source`,
which the running dock writes with the path of the file it actually loaded
(possibly a recovered `.bak` generation). Lines longer than the reader's
buffer are skipped rather than aborting the read.

### Timeout ownership and graph timing

Current clients allocate explicitly owned reply ports. On timeout they return
only the signal bit and leave an inert port while the dock owns the packet.
The updated dock reclaims that port and packet when it finishes the request,
including during shutdown. A reply that won the timeout race is reclaimed by
the sender instead. This keeps delayed replies safe even after the sender
exits. Older hosts cannot perform this reclamation; ownership must remain
retained if a legacy or crashed receiver never returns it.

Graph history is indexed by received samples. Adaptive polling and quiet
updates vary the wall-clock distance between columns; it is not a fixed-seconds
time axis. Diagnostics labels this explicitly.
