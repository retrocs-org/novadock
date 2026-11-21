# HASS: Home Assistant

HASS connects NovaDock 0.5 to the separate AmiHASS background service. Add
independent tiles for lights, switches, groups, scenes, scripts or sensor readings.
AmiHASS owns the Home Assistant connection and credentials. The dock uses its
C SDK through Exec message ports for discovery, cached readings and controls.

Native builds, mock-server requests and sample-service IPC checks cover the
integration. Live read-only API and paging checks have also passed. Native sample controls, room filtering and service restart recovery pass on
Workbench. Physical-device controls and physical-system resource measurements
remain release checks. WebSocket events and custom multi-entity selections remain future work for the dock.
Native sample tests cover brightness, RGB and climate target readback plus the
plugin control-panel inputs. Physical-device testing remains a separate release check.

## Connect through AmiHASS

1. Install the separate **AmiHASS 0.2** package using **Install**.
2. Configure AmiHASS's networking requirements (`C:curl`, CA certificates for HTTPS and your TCP/IP stack). The dock plugin itself makes no HTTP requests.
3. Open **SYS:Prefs/AmiHASSPrefs**, choose **Server Settings**, enter your server origin and enter a long-lived Home Assistant token or import it from a local text file. Credentials stay in AmiHASS.
4. Save Server Settings. Set the polling interval in General settings (10 to 3600 seconds, default 30), then Save the main Preferences.
5. Add **HASS** through NovaDock Preferences -> Organize -> Add -> Plugin.

Open **SYS:Prefs/AmiHASSPrefs** to change the shared connection. All tiles
share its server and token. The dock's **Edit settings...** only configures
the selected tile; the entity chooser's **Refresh** reloads its list. The NovaDock plugin only
contains the client SDK, device/entity choosers and per-tile controls. It does
not manage credentials or make authenticated HTTP requests. AmiHASS remains
running when NovaDock stops or restarts.
If it is installed but stopped, a configured tile starts the service on demand.
Launch attempts are limited to once per 30 seconds. A tile without an entity
does not start the bridge or request data. Uninstalling AmiHASS leaves a clear
missing-service status; installing/starting it allows the tile to recover.

A server origin looks like `https://homeassistant.example.org:8123`, without a
path or trailing slash. Hostnames and IPv4 are supported; IPv6 and URL subpaths
remain future work. The URL determines HTTP or HTTPS. HTTPS verifies certificates;
an HTTP URL sends the token without transport encryption.

When moving an earlier 0.4 test setup to AmiHASS, re-enter the server and import
the token in AmiHASS. Legacy `ENVARC:NovaDock/Private/HASS.connection` is not
silently migrated or deleted. **Forget token** in AmiHASS removes its saved token. Close Server Settings without Save to discard edits.

## A light-bulb toggle

The settings window offers **Tile label**, **Type**, **Room**, **Device** and
**Click Action**, plus **Read only**, **Refresh seconds**, **Display** and
**Show updated time**. Scroll to the remaining settings on a small screen.
The preview shows the automatically chosen device icon.

1. Right-click HASS and choose **Edit settings...**.
2. Leave **Type** at **light**, the default for new tiles.
3. Optionally enter its Home Assistant room name or area ID in **Room**;
   leave it empty for all rooms. Click **Select** beside **Device**. Search for the physical bulb or lamp,
   press Return, select it and click **Choose**. Then choose its light entity.
   This second list contains only entities belonging to that device and
   matching the selected type and room. **Devices...** returns to device selection.
   **Entities** on the device list opens all matching entities, including helpers
   and lights without a physical device. This is an explicit browsing choice.
4. Choose **Toggle**, **Turn on**, **Turn off** or **Show controls** for
   **Click Action**. Unsupported actions are omitted for the selected entity.
5. Set a short tile label, then click **Save**.

The Device field displays the readable entity name. Device and entity lists
have Room, search, Previous/Next and Refresh controls. Click **Rooms...** to choose a room by name from the bridge's paged room catalogue, or choose **All rooms** to include unassigned targets.
Selections retain stable area IDs. Manual names or IDs remain accepted. Unknown rooms produce an empty result. Room/type/search filters combine
on the service. Press Return to apply Room/search changes. Choosing or paging
with unapplied edits first reloads the list and clears its old selection. Each displayed page fills the
list's visible rows by combining smaller service pages; the last page shows
the remaining entries. Cancel leaves the previous selection unchanged.
Changing the type clears the old entity selection. Browsing and saving
settings never send a control action to Home Assistant.

Each tile saves its device ID, readable names, entity and action independently.
Existing entity-only tiles keep working after an update; choose a device when
changing their entity. New device-based selections require the updated AmiHASS
0.2 service with device discovery support. An older service shows an update
message rather than silently browsing the whole server.

With **Toggle** selected, clicking requests a toggle. The tile shows the state reported by Home
Assistant after a fresh read; it never assumes that the action succeeded.
An on/open entity has a small dot badge. Hover for its reading or connection
status. A sad house/bridge icon marks a stopped or unreachable AmiHASS service,
missing connection settings or stale bridge data. It automatically restores
the device artwork when current readings return. **Show controls** opens a native state window instead of acting directly.

## Entities, scenes and groups

- Lights, switches, fans, input booleans, media players and Home Assistant
  `group` entities support **Toggle**, **Turn on** and **Turn off**.
- Scenes and scripts use **Activate**. A scene/script tile left on **Toggle**
  also activates it. Scenes report their activation timestamp rather than on/off.
- Sensor readings include their unit when provided by Home Assistant.
- Door/window sensors and locks are read-only. Covers and climate entities
  expose the verified controls described below; missing capabilities stay hidden.
- Group light entities are listed under **light**; classic group entities are
  under **group**. Create or manage membership in Home Assistant itself.

**Type** filters discovery. Lights and other device-backed types first show
matching devices, then only their matching entities. Scenes, scripts and groups
open a type-filtered entity list directly, without requiring a physical device.
**All** also includes domains without a specific filter entry.

Lists load when opened, searched, paged or refreshed. The chooser combines
16-entry service replies to fill its visible rows. Responses are capped at
64 KB. Entity IDs up to 95 characters are supported; longer IDs are omitted.
Selected-entity reads use AmiHASS's shared state cache. Each tile uses a
selected-entity subscription. Leases renew within 60 seconds and close when
settings/targets change or the tile exits. They expire after 120 seconds if a
client disappears. No unsolicited push messages or whole-server polling are used.
Only an explicit unsupported response enables legacy QUERY fallback.

Add another HASS tile for a different entity or scene. Each tile saves its
label, target and click action independently. Artwork follows the target
automatically. Removing an instance removes its tile settings and keeps the
AmiHASS connection for the remaining instances.

## Light brightness

Choose **Show controls** as the tile's **Click Action**, save, then click the
tile. Dimmable lights show a **Brightness** slider from 0 to 100 percent beside
the confirmed state. Release the slider to send one request through AmiHASS;
0 turns the light off. On/off/toggle buttons remain available.

The slider appears only when AmiHASS reports brightness support. Older service
builds without control metadata, on/off-only lights and other entity types keep
their existing controls. Use an updated AmiHASS 0.2 service for dimming.

The dock reads the confirmed level after a request and picks up changes made by
other clients. The slider is disabled while awaiting a reply or while data is
stale/unavailable. It never moves the knob during a drag, and offline actions
are not replayed after reconnecting.

## RGB color and climate targets

Show controls includes an RGB field when the light supports color. Enter
`#RRGGBB` and press Return. Supported climate entities show a target field with
its verified C/F unit; enter a temperature and press Return. AmiHASS validates
the range and step. Neither command changes the thermostat's operating mode.
Offline, stale or unsupported values cannot send an action.

## Automatic artwork and states

Icons are chosen automatically from the entity's domain, device class and
readable name. Supported artwork includes bulbs, light strips, groups,
seasonal lights, ceiling/floor/shelf lamps, switches, fans, TVs, climate units,
doors, garages and sensors. A generic HASS icon covers other entity types.

Confirmed **off** readings use unlit icons; **on** readings use lit or active
icons. Doors/windows and garages show closed/open artwork; locks use their
reported locked/unlocked state. Pending or unknown readings have a question
marker, and service/connection failures use the sad HASS icon. The dock changes
artwork only from reported readings, never from an unconfirmed control request.
Hover over the tile to see its state or connection status.

HASS carries its own artwork in `Plugins/HASS.assets/Artwork.ndart`, beside
its executable and manifest. It uses the public SDK artwork API, independently
of the shared `Icons` library. NovaDock caches the bank and maps its palette
to Workbench colours. Changing state needs no disk access and does not add
Home Assistant polling. Update the bank together with the plugin, then reload
or restart NovaDock. Tile and custom-icon controls are replaced by automatic
selection. See [Plugin-owned artwork](PLUGIN_SDK_ARTWORK.md) for the SDK format.

## Offline behavior and credentials

One shared background service serializes requests for every tile. Routine
polls fetch selected entities, not the entire Home Assistant installation.
Duplicate entity selections share a cached reading. Network calls run in a
separate downloader process owned by AmiHASS, with connection/request timeouts
and retry backoff. The dock reads cached state every five seconds; this does not
change the service's configured HTTP polling interval.
Old readings are marked stale; unavailable entities cannot be controlled.
Controls are not queued for replay after reconnecting.

AmiHASS saves the server and token locally in plain text in
`ENVARC:AmiHASS/AmiHASS.cfg`. Temporary authenticated curl configuration
files are removed after requests; no token goes into command arguments, IPC
packets, logs, NovaDock configuration or exported layouts. The service does
not follow redirects to another origin.

Other local Amiga programs can access local files; the OS provides no isolation.
Uninstalling NovaDock does not stop AmiHASS or remove its connection settings.
Use **Install -> Uninstall** to stop/remove the separate service; choose
whether to keep or remove its token. NovaDock's removal only affects tile settings.

Open **NovaDock Preferences -> Plugins**, select the affected instance and
click **Refresh** for its current runtime status and repair guidance. This
distinguishes a stopped plugin from a stopped bridge, missing setup,
unavailable entity or connection failure. A light reporting **off** is healthy.

For network, server, token, curl or certificate failures, inspect status and
logs in **AmiHASSPrefs**. Confirm the entity exists in Home Assistant and allow
time for a fresh poll. Changing a tile's target discards pending replies and
capabilities from its previous target. Failed controls are never replayed.

API references: [REST API](https://developers.home-assistant.io/docs/api/rest/)
and [authentication](https://developers.home-assistant.io/docs/auth_api/).

## AmiHASS framework for developers

AmiHASS is a separate application/service project. Its public C SDK is also
vendored under `plugin_src/hass/service` for the NovaDock adapter. The named
port is **AmiHASS.Service**, with magic, ABI version and exact message length.
Apps can query selected entities, browse devices and device-scoped catalogue pages, request supported
actions, read service status or reload settings. No NovaDock executable is
needed by other clients. AmiHASS owns authentication, background HTTPS requests,
caching, rate/backoff policy and connection generation handling.

`hass.c` implements dock tiles. `nd_hass_ui.c` provides device and entity choosers.
Shared connection setup belongs to the separate AmiHASSPrefs application. `nd_hass_client.c` is the vendored SDK client;
`nd_hass_native.h` adapts its names inside NovaDock. Backend and credential code
are maintained and tested in the independent AmiHASS project.

Each client has one outstanding message at a time, an owned reply port and a
ten-second local timeout. Late replies are reclaimed safely. Selected entities
are deduplicated, with a 48-entity bound, eight queued actions and 16-entry
catalogue pages. Clients renew a selected entity by querying it within two
minutes. Routine server polls read selected entities only. Fixed read-only
server templates provide filtered pages without loading every entity locally.

This version polls REST rather than using a persistent WebSocket. Unavailable
or stale readings cannot be controlled. Failed controls are cleared rather
than replayed after reconnecting. The AmiHASS SDK guide documents the full ABI,
status/shutdown operations, ownership and a read-only C example.

## Integration checks

The public AmiHASS 0.2/ABI 1 snapshot is recorded in
`plugin_src/hass/service/sdk.json`, including hashes for the protocol, client
and portable request/metadata decoder. Optional `QUERY.items[]` control
metadata extends ABI 1 without changing its message size. Older replies remain
valid and simply omit brightness controls.

`tests/test_hass_bridge.py` covers startup limits, confirmed action readback,
external state changes, changed targets, stale data, timeouts, capability checks
and thirty outage/recovery cycles without command replay.
`tests/test_hass_transport.py` exercises authenticated toggle and brightness
requests against a local mock, plus unavailable entities and HTTP failures.
`tests/test_hass_picker.py` checks type/device filtering, paging, cancellation
and older-service errors. `qa/HASS.md` describes isolated native tests using
AmiHASSService's sample provider, including repeated service restarts and
brightness readback. These checks require no real access token or device.

The dock plugin uses structured C SDK requests. ARexx remains available through
AmiHASS for scripts and testing.

The device picker keeps the selected **Type**: choosing `light` lists only
devices that expose light entities. Its text filter, paging, Refresh and
**Devices...** retain that type. **All** lists every device; scenes, scripts
and groups use their own entity lists.

A green corner dot means the selected entity reports **on** or **open**. The
automatic device artwork follows the same confirmed state; an action request
alone does not light the icon before AmiHASS confirms the change.

Brightness changes are sent when you release the slider. Status sampling does
not move the knob during a drag. A released value can wait briefly for an
in-flight query; disconnecting, closing controls or changing the tile cancels
that pending edit. The confirmed server value appears after the action.

## Fan, cover and temperature controls

Show controls builds the popup from the selected entity's capability metadata
returned by the AmiHASS C SDK. Supported light controls show brightness/RGB;
thermostats show a target temperature and the reported range/step. Fans can
show a speed percentage. Covers can show position and independently supported
Open cover, Close cover and Stop cover buttons. Unsupported controls are omitted.

All actions still go to the Amiga AmiHASS service through structured Exec
messages. The plugin does not connect to Home Assistant. Fan and cover controls
require the matching updated AmiHASS 0.2 service; optional metadata extends the
existing ABI 1 message without changing its layout. Older bridges leave these
controls unavailable. Locks remain read-only.

Percentage changes submit on release. A released value or numeric/color edit
can wait briefly for an in-flight query, with a bounded lifetime. Closing,
changing target, losing the connection or reaching the timeout discards drafts;
commands are not replayed after recovery. Values come back from service queries.
RGB/temperature readbacks update untouched fields without overwriting active
edits. Home Assistant integrations may quantize fan speed or cover position;
the popup shows the state reported by the integration after the request.

Feature flags and service names follow Home Assistant's primary
[fan definitions](https://contributors.home-assistant.io/core/entities/fan/)
and [cover definitions](https://contributors.home-assistant.io/core/entities/cover/).

## Per-tile refresh, display and read-only mode

**Refresh seconds** is Default, 5, 10, 30, 60, 300, 900 or 3600. Default uses
the service's polling preference. Clients selecting the same entity share its
cache and use the shortest active refresh preference; a slow tile does not
create another downloader. Catalogue browsing is separate from state polling.
The detail window's Refresh requests one fresh read and then waits for it,
without repeatedly invalidating the cache.

**Read only** disables Click Action and shows a state/capability window when
clicked. That window has Refresh and Close; no sliders, numeric control fields
or device-action buttons are created. The adapter also rejects actions below
the UI layer, even if an older saved configuration retains Toggle.

**Display** chooses State, Name and state, or Name for the tile's hover reading.
**Show updated time** controls the detail window's Updated field. Short dock
readings may be truncated; the detail window keeps the state separate from
its display preference. Settings apply independently to duplicate HASS tiles.

The detail window identifies **Sample data** versus **Home Assistant** server
data and shows verified extra controls. Connection failures can include the
bridge's suggested fix. Changing provider discards old readings and unsent
controls. Connection/server preferences and credentials remain in AmiHASSPrefs.

Device actions show Sending, Waiting for confirmation, Confirmed or failure.
Acceptance alone never changes the icon or reading. Confirmation requires a
subsequent matching state/parameter (or a new activation timestamp for scenes).
After nine seconds without confirmation the UI reports failure; it does not
resend the action. Another action waits until the previous one resolves.

AmiHASS Dashboard pages, starter layouts, undo and backup remain features of
that application. Each dock tile stores its own target and appearance settings.
Push transport remains future work; the new subscriptions use shared polling.

## Bridge compatibility

The tested release pair is **NovaDock 0.5 with AmiHASS 0.2**. Install and
configure the bridge separately; no credentials belong in NovaDock. A missing
service shows **Start AmiHASS 0.2+ service** and connection failures must not
retain a successful Sample data label. Newer bridges are accepted by their
advertised capabilities, rather than rejected for an unfamiliar version.

Discovery checks the service's advertised room/filter features before sending
catalogue requests. An older AmiHASS 0.1 build that does not advertise these
features shows **Update to AmiHASS 0.2+**, rather than ignoring a room/type
filter. Refresh after updating the bridge. ABI 1 and packet sizes are unchanged.
Configured entity tiles can retain legacy cached QUERY support, while their
detail window identifies the need to update the bridge. Optional controls still
require their own capability metadata; missing support never becomes a zero.

## Read-only room summaries (0.5)

Set Type to room and use Select beside Room. The chooser lists rooms supplied
by AmiHASS; a specific room is required. HASS obtains all room entities through
the same public Exec-port SDK, without connecting directly to Home Assistant.

The tile reports light entities switched on and open contacts/covers. Click for
on/off light counts, open/closed door and window contacts/covers, unavailable
entities and a room total. Lights and Open contacts show matching entries.
Motion sensors are not counted as open doors. Opening/closing covers count as
open until fully closed. These summaries send no control commands.

Refresh defaults to sixty seconds. A complete paged snapshot is required before
counts become current. On failure the previous counts remain explicitly stale
and the badge clears. Summaries support at most 1,024 distinct room entities,
with up to sixty list details visible; counts include the full supported room.
Oversized, inconsistent or duplicate pages produce a refresh error. AmiHASS
must advertise room and filter support. Custom hand-picked entity sets remain
future work.

Tiles for the same room share one paged aggregation and one duplicate-check
workspace. The fastest requested interval applies to that room; repeated Refresh
requests are combined. Different rooms start their reads at staggered times.
Closing the tile performing the read lets a remaining tile take over. A stalled
owner is reported as stale rather than displaying its counts as current.

## Media players

Choose Type media_player and select a device/entity through AmiHASS. Show
controls opens only the actions advertised by that entity: Previous, Play,
Pause, Next and a confirmed Volume slider. Track and artist appear when the
bridge reports them. The default State tile display shows track information.
Read-only tiles show metadata and Refresh but expose no mutating controls.

Playback and volume commands use the public Exec-port C SDK. The updated
AmiHASS 0.2 service owns all network calls, validates advertised capabilities
and supplies sample media playback, volume and track changes for offline tests.
Older service builds without media metadata safely leave these controls absent.
Commands are not replayed after outages; volume and playback use confirmed
readback, and track navigation waits for an updated state timestamp.

Feature flags and service names follow the official Home Assistant
[media-player entity API](https://contributors.home-assistant.io/core/entities/media-player/).

## Locks and motorised doors

Choose Type lock and select a device/entity. The click-action picker and control
popup offer Lock, Unlock and Open lock only when AmiHASS reports those controls.
Open lock releases the latch; it is distinct from Unlock. A lock that requires a
keypad code stays read-only because NovaDock does not collect or send PINs.
Door contact sensors also remain read-only and display Open/Closed. Lock states
display Locked, Unlocked or Latch open without changing the raw SDK state.

Cover entities retain their advertised Open cover, Close cover, Stop cover and
position controls. Stop is confirmed only by an Open or Closed state, not by an
unknown, jammed or moving state. The tile artwork follows confirmed raw states.

Unlocking, releasing a latch, opening a cover, or increasing its position opens
a separate confirmation window with the device name, entity ID and action.
Confirmations also apply to sample devices. Cancel, Escape and closing the
confirmation send nothing. The dialog expires after 30 seconds; a settings,
source, capability or availability change invalidates it. Confirm checks the
current state again, and the plugin does not replay an accepted action after
a connection loss. Read-only mode blocks actions below the UI as well.

The settings window opens immediately, then queries the selected lock, cover or
media player's capabilities asynchronously through the Exec SDK. Until the
capabilities are verified, its action picker offers Show controls. Unsupported
actions are removed; failed queries leave the picker read-only. No Home
Assistant credentials or HTTP calls are handled by the dock.

These controls require the current AmiHASS 0.2 build with lock metadata. Older
ABI 1 services remain usable for supported existing entities; missing lock
metadata never enables an action. Integration QA uses only the bridge's shared
sample front-door lock and garage door, with no real lock or door commands.
