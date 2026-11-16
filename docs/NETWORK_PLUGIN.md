# Unified Network plugin

Network combines Roadshow control and the packet-activity sampler in one tile.
It is optional; add **Network** from the dock's Add menu. The first provider is
Roadshow. Stack selection is AUTO or ROADSHOW; the provider/control boundary is
separate from sampling so another stack can be added without another dock tile.
Other stack providers are not implemented yet.

## Tile and controls

- **Offline:** a pre-generated network icon with NETWORK at the bottom, in the
  same style as Apps and Music.
- **Online:** two separate **IN** and **OUT** packet-activity meters by default.
  Incoming traffic uses the first accent color; outgoing traffic uses the second.
  Both bars use the same configured PPS scale. The value above them is IN/OUT p/s. Click the online tile to switch between meter and graph, like CPU/RAM.
- **Right-click → Online:** reconnect the selected Roadshow interface with
  `C:ConfigureNetInterface`, renewing DHCP when its profile uses DHCP. If the
  interface has not been registered, start it using `C:AddNetInterface` and
  its exact DEVS or Storage profile.
- **Right-click → Offline:** take the selected interface and its driver offline
  with `C:ConfigureNetInterface <interface> OFFLINE QUIET`. Roadshow stays
  loaded and other interfaces are unchanged. This allows a quick reconnect.
- **Edit settings:** choose the network interface, view, stack and meter scale. Advanced contains optional sampling overrides.
  Left-clicking an offline tile does nothing; settings remain in its right-click menu.

Online means Roadshow reports the selected interface as operational; it does not promise
Internet reachability. Status polling first checks whether bsdsocket.library is
already resident, so it does not reopen the stack after a shutdown.

Control and status commands run in a separate worker. The plugin continues
servicing the dock while DHCP or interface control takes time. One command runs at a
time; a later action is queued. Polling never initiates Online or Offline.

## Setup

**Online at startup** defaults to **NO**. Enable it in Network settings and
choose **Save** to bring the selected interface online whenever the plugin
starts, including after a dock restart. The initial status check runs first:
an already-online interface is left alone. Otherwise, one background Online
attempt uses the selected profile and connection timeout. A failure is reported
normally, without an automatic retry loop. Changing settings while the plugin
is running does not trigger it again. Choosing Offline keeps it offline until
you explicitly reconnect or restart the plugin. An explicit Online or Offline
action takes precedence over a pending startup attempt.


**Interface = AUTO** prefers an active, non-loopback IPv4 interface reported
by Roadshow. When none is active, a detected A2065 board with its installed driver and
matching DEVS or Storage profile is preferred. Otherwise it picks the first
usable file, sorted by
name, from `DEVS:NetInterfaces` for connection startup. Choose a specific
interface if needed. A matching file is read for DEVICE and UNIT; it is never
changed.

The **Select** button beside Network interface opens a dedicated connection
profile list. It combines `DEVS:NetInterfaces` and
`SYS:Storage/NetInterfaces`, sorts names and removes duplicates (the DEVS
profile takes priority). Each row shows the profile and its driver, for
example **A2065 - a2065.device [Template]**. Only profiles whose driver file is
installed are offered; bare driver names are checked in `DEVS:Networks`.
Template means a profile from Storage, not a storage adapter. Installed
drivers do not prove that the physical adapter is present. AUTO remains available. Double-click
a row or choose Select to put its profile name into settings; Cancel leaves
it unchanged. Settings Use/Save then applies it. No files need to be copied
from Storage for this to work. Online rereads the saved selection in its
worker and uses the exact profile path; it cannot fall back to another
profile if the selected file is missing.

The main selector never browses raw `.device` files. `DEVS:Networks` contains
drivers, while the profile also supplies unit and DHCP/IP configuration.
That driver-only drawer is available under Advanced for sampling overrides.
Only readable, valid profile files are listed; icon sidecars and driver
filenames are excluded. The current name is highlighted when present, and
names can still be typed in the settings field.

Driver and unit are derived automatically from the selected profile or the
live interface's Device name and Device unit. No separate device choice is
needed for normal operation. Selecting or cancelling a profile never starts
the network or modifies its profile file.

**Advanced...** exposes **Override sampling driver**, **Sampling driver** and
**Sampling unit**. The override switch defaults to NO. Enable it only for an
unusual sampling setup; its driver/unit fields are disabled otherwise. The
advanced driver Select button opens `DEVS:Networks`, filtered to `.device`
files. This override affects traffic sampling only, never the connection
profile. Returning to automatic mode clears the override on the next settings
save. Older device values cannot silently override automatic selection: they
are cleared in the loaded configuration unless the advanced override is
explicitly enabled. Changing the interface or sampling source also forgets the previous resolved
source so an earlier driver selection cannot be reused accidentally.

**Meter scale PPS** is the
full-scale packets/second value, 250 by default. Meter and graph channels are RX and TX. State messages clear the meters
rather than leaving the last traffic sample visible.
These are packet rates, not bytes/second or a bandwidth percentage. Driver
counter resets and 32-bit wraps are handled by the existing rate calculator.

If Roadshow is online but the driver cannot supply statistics, the tile reports
NO STATS rather than inventing traffic. Check the interface/device settings.

## Existing installations

An enabled legacy Roadshow tile becomes Network in the same position. If both
Network and Roadshow were enabled, Network is retained and the duplicate is
removed. Legacy Roadshow settings remain available as fallback. The standalone
Roadshow executable and its alias are no longer shipped. Add Item and the
plugin catalogue offer Network.

The user's saved file is not reset. The upgraded layout is persisted with the
next ordinary settings save. The six default active tiles remain unchanged.

The Roadshow commands and their arguments were checked against its installed
ShowNetStatus, AddNetInterface and ConfigureNetInterface reference manuals. No Roadshow
binaries are bundled with NovaDock.

## Network Details

Right-click Network and choose **Details...** for a centered, scrollable
read-only window. It shows the selected stack, interface, SANA-II device/unit
and connection timeout. When Roadshow is running, it also shows live interface
IP configuration, routes/gateway and DNS servers using C:ShowNetStatus.

Use **Refresh** to reread the configuration and live values. Opening Details
while offline does not start the stack; unavailable live values are explained
in the window. The window runs independently of the dock and traffic sampler.
Opening Details again brings the existing window forward.

ShowNetStatus arguments were checked against its installed Roadshow reference
manual. No NAMES option is used, so the query does not wait for reverse DNS.

The context menu offers Online while offline, and Offline while online.
Failed explicit state changes open an independent error requester; the dock
continues running. On larger Workbench screens all eight normal settings are shown
together; Advanced adds three sampling controls, with scrolling reserved for screens too small to fit them.

If ShowNetStatus reports an unsupported status-query method, Details explains
that the active socket library cannot be queried through Roadshow. Networking
provided by another TCP/IP stack may not support Roadshow status or control
commands. Use the controls supplied with that stack.

AUTO first selects an active, non-loopback IPv4 interface reported by
ShowNetStatus, including wrapped status-table rows. It uses a matching
DEVS:NetInterfaces file for device/unit metadata. If the active interface
has no matching file, it reads the live interface's reported Device name and
Device unit for traffic sampling. If these cannot be read, it asks for an
explicit SANA-II device rather than borrowing an unrelated interface's device. With no active interface, AUTO falls back
to a configured interface file for connection startup. Explicit selections
and explicitly enabled advanced sampling overrides are preserved. Active-interface discovery is cached
for 30 seconds during passive polling and refreshed for control actions,
settings reloads and Details.

## Disconnecting and reconnecting

Offline drains and closes NovaDock's sampling request before taking the
selected interface offline. It does not unload Roadshow or shut down other
interfaces. Use Roadshow's own NetShutdown command separately if a complete
TCP/IP stack shutdown is needed.

An Online click during STOPPING queues one reconnect. It starts only after
the Offline worker replies; repeated clicks cannot create overlapping control
commands. The temporary operation guard is cleared before the reconnect starts.
An existing interface is brought online with ConfigureNetInterface. After
that command succeeds, a DHCP profile requests a fresh DHCP configuration in
a separate command; a static profile retains its configuration. AddNetInterface is reserved for interfaces not yet registered.

Periodic polling reports the selected interface, so another active connection
cannot make an explicitly disconnected interface appear online. Polling does
not load an absent stack or initiate a connection. The dock remains responsive
while a worker runs. Offline has a 15-second wait allowance; Online uses the
configured timeout plus 15 seconds. A timeout opens an explanation but does not
start a competing command or forcibly terminate a running Roadshow operation.
A driver request that cannot be drained prevents Offline from starting and
produces a separate explanation instead of releasing live I/O memory.

For an AUTO connection, NovaDock remembers the last active interface within
the running plugin so reconnecting targets that interface. If its file is
not installed in DEVS:NetInterfaces, a matching SYS:Storage/NetInterfaces
file can be used, as with the standard A2065 test template. Configuration
files are not created or changed automatically.

A2065 detection reads its expansion-board identity without initializing
unrelated network drivers. This lets AUTO start the A2065 Storage template
when its interface is inactive instead of trying an unrelated network profile.
The runtime metadata parser accepts both Device unit and Device unit number.

After a manual binary update, quit and restart NovaDock. The private worker
protocol is version-checked, and parent/worker binaries must be from the
same build; files copied over an older resident process do not replace it.


## Quiet operation and debug logging

**Debug log** defaults to **NO**. Before an explicit Online or Offline action,
NovaDock starts Roadshow's `C:NetLogViewer CX_POPUP=NO` if it is not already
running. Roadshow's separate log messages are captured in the hidden viewer,
so routine connection notifications do not open its automatic
console. Command output remains quiet too. A viewer already running is reused
without relaunching it (relaunching would bring its window forward).

Choose **Debug log = YES** to show the viewer for the next Online/Offline
action. Roadshow's own Shift+Alt+F8 hotkey or Commodities Exchange can also
show the retained messages. NovaDock does not change interface debug flags or
Roadshow startup files. An already-open log window is left available; turning
Debug log off prevents subsequent operations from opening it automatically.
The viewer remains available between interface disconnects. If Roadshow was
fully shut down separately, Online starts a new hidden viewer when needed.

`NetLogViewer` must be installed with Roadshow. If capture cannot be started,
NovaDock reports that error and leaves the requested operation unstarted,
rather than silently producing an unwanted console. Explicit connection or
disconnect failures still use NovaDock's normal error requester. Passive polling
and opening Settings never start the viewer or the network stack.


## Saved display view

**Default view** in Network preferences offers METER and GRAPH, defaulting to
METER. Using or saving the preference applies the view to the tile. Clicking
an online tile switches view and saves the same preference in the background,
so preferences and the tile stay in sync. View-only changes redraw the dock;
they do not reload the Network sampler or change the network connection.
Existing configurations retain their saved PLUGIN view until a VIEW preference
is explicitly written. Offline left-clicks (including keyboard activation)
perform no action; right-click still exposes Online, Details and settings.

## Tile layout

Online tiles align the NETWORK heading with CPU/RAM and show IN/OUT meters or
a two-channel graph. Packet rates and control/sampling status are available
in the hover tooltip. Offline tiles keep the network icon and bottom title.
