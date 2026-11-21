# Future work

## 0.5 release preflight

- [x] Add BENCH save retry for retained results without rerunning measurements.
- [x] Verify installer custom paths containing spaces on system and secondary volumes, upgrade preservation, both removal modes and startup assignments/shortcut targets.

- [x] Correct HASS setup to AmiHASS 0.2 / Install and enforce SDK/documentation consistency.
- [x] Test packaged 0.4-to-0.5 Installer lifecycle and settings/Notes/imported-component preservation at standard locations, including both removal modes.
- [x] Verify native COLOUR/QR/BENCH write-protection recovery and host-injected write/close/rename failures; use classic-filesystem-safe staging filenames.

## 0.5 Preferences performance

- [x] Defer plugin discovery until a page needs it; skip unrelated artwork for focused plugin settings.
- [x] Use a lightweight temporary preview snapshot while retaining validated Use/Save and Test/Cancel restoration.
- [x] Speed up animated-artwork validation, avoid repeated artwork-file opens and stop option lookup at its matching definition.
- [x] Cache validated package inventory with source-change invalidation and forced Refresh.
- [x] Open Plugins before discovery finishes; validate background Refresh, detail selection and cancellation on native 68000/68060 guests.
- [x] Remove idle list rebuilding, preserve result windows/edits and share room aggregations.
- [x] Add native startup/window/idle/memory regression budgets and swatch/stage/input feedback.
- [ ] Compare Preferences opening times on physical 68000/68060 systems after the native emulator checks.


## Home Assistant plugin

- [x] Separate AmiHASS application, background service, public IPC SDK and installer.
- [x] Validate the NovaDock client and shared sample controls on Workbench.
- [x] Validate the paired AmiHASS 0.2 candidate's independent Installer modes and standalone packaged SDK; rerun source-bound readiness when freezing that project.

Status: the AmiHASS-backed HASS plugin is implemented for 0.4;
mock-server tests, native compilation and live read-only API/paging checks pass.
Native Workbench sample controls, room discovery and reconnect checks pass.
Constrained native runtime checks also cover a 68000 with 8 MiB Fast RAM at
640×256 and a 640×480 screen. Physical-device checks and physical-system
resource measurements remain pending.

### Implemented first pass

- [x] Shared server/token, opt-in HTTP and verified HTTPS managed by the separate AmiHASS service.
- [x] One background REST scheduler for independent tiles and selected-entity polling.
- [x] Sensor readings with units, light/switch/fan/group on/off/toggle, and scene/script activation.
- [x] Paged room chooser and explicit service-feature negotiation.
- [x] Room/type/device filters, paged name/ID search and direct scene/script/group selection.
- [x] Selected-entity subscriptions with per-tile refresh intervals and automatic cleanup.
- [x] Read-only tiles, display/timestamp preferences and sample/server source labels.
- [x] Verified control capabilities and bounded sending/confirmation/failure feedback.
- [x] Independent labels/click actions and automatic device/state artwork owned by HASS.
- [x] Confirm state by reading it after an action; mark stale/unavailable readings.
- [x] Bounded responses, timeouts, retry backoff and credentials separate from layouts.
- [x] Setup documentation and forty-five smart-home icons.

### Follow-up

- [x] Capability-filtered smart locks and motorised doors, explicit confirmation and sample-only integration checks.

- [x] Validate authentication and paged queries against a 3,000+ entity live server.
- [x] Validate native Workbench windows and controls using sample devices.
- [ ] Compare real device control/readback on physical hardware.
- [ ] Measure idle CPU, memory and responsiveness on supported Amiga configurations.
- [x] Dimmable-light brightness slider through the C SDK, with confirmed readback.
- [x] Richer detail popups for thermostat targets, fan speed and cover position/actions.
- [x] Grouped counts for complete rooms, with bounded paged aggregation.
- [ ] Custom hand-picked multi-entity selections; existing groups come from Home Assistant.
- [ ] Evaluate a persistent WebSocket connection and selected-entity event subscriptions.
- [x] Page/filter large catalogues on the server instead of loading all state attributes.
- [ ] Add IPv6 and subpath URLs.

### Later possibilities

- [x] Door/window status and open/closed artwork.
- [x] Lists of open contacts in read-only room summaries.
- [x] Capability-gated thermostat target controls.
- [x] Read-only room summaries, light/open-contact counts and unavailable totals through the AmiHASS SDK.
- [ ] Energy summaries and electricity price tiles.
- [x] Media-player track information, playback and volume controls through the AmiHASS SDK.
- [ ] Appliance status and remaining-time displays where exposed by Home Assistant.

### Validation and documentation

- [x] Mock-test unavailable entities, authentication failures and connection errors.
- [x] Mock-test repeated outages, target changes and late replies without action replay.
- [x] Native sample-service brightness/readback, external actions and three stop/start recovery cycles.
- [ ] Repeat connection loss/recovery testing with the native UI and real server.
- [x] Verify confirmed sample state, pending controls and bounded confirmation while the native client remains responsive.
- [x] Document setup, supported types, brightness, credentials and connection requirements.

### References

- [Home Assistant REST API](https://developers.home-assistant.io/docs/api/rest/)
- [Home Assistant WebSocket API](https://developers.home-assistant.io/docs/api/websocket/)

## Other future plugins

Status: completed first passes are checked below; unchecked items remain planned.

### UPDATES

Planned tile/package management; NovaDock already has app update checks in Preferences.

- [ ] Check for NovaDock and plugin updates and show an available-update badge.
- [ ] Show installed/available versions and release notes in a native window.
- [ ] Download selected packages and verify their checksums before offering installation.

### AGENDA

- [x] Save tasks, dated reminders and recurring events permanently.
- [x] Show a badge for items due today and open an agenda on click.
- [x] Provide completion, editing and reminder controls.

### CALC

- [x] Open a compact calculator with calculation history.
- [x] Support decimal, hexadecimal and binary conversion.
- [x] Copy results to the clipboard.

### SHELF

- [x] Accept dropped files and drawers as shortcuts in a persistent collection.
- [x] Open a native picker to launch a saved shortcut.
- [x] Support multiple shelves with independent labels, icons and contents.
- [x] Allow reordering and removal of shortcuts without deleting their target files.

### PET

- [ ] Add a small animated companion to a dock tile.
- [ ] React to clicks with playful animations.
- [ ] Offer appearance and animation-speed settings, with a low CPU cost when idle.

### MOON

- [x] Display the current moon phase with a changing tile icon.
- [x] Open a detailed view with moon-phase and sunrise/sunset information.
- [x] Configure location and time zone, and refresh only as often as needed.

### TOOLTYPES

- [x] Accept dropped Workbench icons and inspect their ToolTypes, default tool and stack size.
- [x] Provide a native editor for these settings.
- [x] Back up the original icon before saving changes and preserve its artwork.

### COLOUR

- [x] Pick a colour from the screen and display its RGB and hexadecimal values.
- [x] Copy colour values to the clipboard.
- [x] Keep a small saved palette for theme development.

### ALERTS

- [x] Collect notifications from dock plugins in one native history window.
- [x] Show an unread-count badge and allow notifications to be marked as read.
- [x] Offer optional notification sounds.
- [x] Integrate completed downloads, timers, low disk space and AmiHASS connection events.

### PICTURES

- [x] Cycle through supported images from a selected drawer in a picture-frame tile.
- [x] Open a larger image viewer on click.
- [x] Configure the slideshow interval and keep image decoding and memory use bounded.

### WATCH

- [x] Watch selected drawers for new or changed files, including downloads and build output.
- [x] Show a badge for unseen changes and open a native list of recent changes on click.
- [x] Allow changed files or their containing drawers to be opened from the list.
- [x] Keep monitoring lightweight and handle unavailable drawers without blocking the dock.

### LOGS

- [x] Follow a selected log file and show new entries in a native viewer.
- [x] Highlight errors and warnings, with a badge for unseen matches.
- [x] Support filtering and copying entries, with bounded history and lightweight monitoring.
- [x] Handle truncated, replaced and temporarily unavailable log files.

### BENCH

- [x] Run short CPU, memory and graphics benchmarks on demand.
- [x] Save dated results with configuration details for comparisons between Amiga setups.
- [x] Keep the UI responsive and allow tests to be stopped.

### QR

- [x] Generate QR codes locally from clipboard text or an entered URL.
- [x] Display codes at a readable size for scanning with a phone.
- [x] Offer image export without requiring an external service.

### TRANSIT

- [ ] Select a transport provider and favourite stops or routes.
- [ ] Show the next departure in the tile and a native departure board on click.
- [ ] Evaluate provider APIs, coverage and credential requirements before implementation.
- [ ] Cache results, display their update time and respect provider refresh limits.

## Drawer and Volume enhancements

Extend the existing Drawer/Volume items. Their space meter reports the
containing volume's usage, not the selected drawer's size.

- [x] Show free, used and total capacity in the tooltip, with readable units.
- [x] Add configurable low-space thresholds and warning colours.
- [x] Add a popup summarising mounted volumes and their capacity.
- [x] Offer remaining free space directly on the tile as an alternative display.

## SENSOR: Sensei / i2csensors.library integration

Status: optional background provider implemented for NovaDock 0.4; real-hardware validation remains pending.

The [Sensei 1.6 archive](https://m68k.aminet.net/package/driver/other/Sensei)
includes i2csensors.library 2.8 and sensor definitions. The useful interface is
[the sensor library and SDK](https://gitlab.com/HenrykRichter/i2csensors), not
Sensei's display window. Sensei does not need to run for library clients.
Reviewed upstream HEAD: `49e6d00894423a53c727d1bd406cb05453481f5e` (16 November 2026).

### Candidate hardware and readings

- [x] Add an optional i2csensors.library v2+ provider alongside existing Warp,
  ZZ9000 and AK_Sensors providers; missing libraries must leave those working.
- [ ] Cover configured DS3231, LTC2990, MCP9808, BMP/BME and AM2320 sensors,
  plus MAX31760 fan controllers, through their installed library definitions.
- [ ] Validate TK060/BFG9060 temperature and voltage definitions, including the
  board-specific calibration. These definitions do not establish TF1260 support.
- [ ] Evaluate the upstream Z3660 provider and avoid duplicate ZZ9000 readings
  when both the native and library providers can read the same board.
- [x] Retain WarpTool support. No Warp or direct TF1260 provider was found in
  the archive/SDK material reviewed; physical sensors still need matching hardware.

### Discovery, preferences and persistence

- [x] Extend the legacy 14 channels with a bounded dynamic I2C inventory
  carrying stable sensor ID, name, unit and availability.
- [x] Enumerate with i2c_SensorNum, read with i2c_ReadSensor, and retain v2 IDs
  from i2c_SensorID instead of saving a changing enumeration index.
- [x] Only list readings confirmed by successful reads: configured definitions
  can describe hardware that is absent. Never interpret an error as zero.
- [x] Preserve independent instance labels and selections, including legacy
  SENSOR options, when definitions are added, removed or reordered.
- [x] Refine per-channel missing-provider and stale-state explanations in Preferences.

### Units, performance and validation

- [x] Support Celsius, volts, amps, RPM, pressure, watts, humidity and labelled
  miscellaneous readings using the units supplied by the library.
- [x] Convert signed 16.16 values without floating-point work in NovaDock,
  handling negative temperatures, zero, rounding and overflow explicitly.
- [x] Respect returned name/unit pointers and the documented read-error contract;
  copy bounded strings while the library remains open.
- [x] Share cached sampling across instances in a background worker; do not run
  potentially slow bus reads from the dock or Preferences input loops.
- [x] Keep DEVS:Sensors setup, bus addresses and calibration under the user's
  sensor-driver setup; do not invent or install board definitions automatically.
- [x] Review upstream licensing before reusing or redistributing its code,
  headers or libraries; the helper licence is bundled separately; the vendor library is not included.
- [ ] Test absent libraries/devices, valid zero readings, negative values,
  changed IDs, stale results and duplicate sources, then compare on real boards
  against Sensei/simplesensors and measure idle CPU and memory use.


## SENSOR: native Warp library interface

Status: standalone experimental 30.74 probe implemented; dock integration and
physical-board validation are pending. See [probe interface and usage](../tools/warp-sensor-probe.md).

Reviewed firmware pack `warp_fmw_3074.zip` (SHA-256
`7cccba3ddc7edcd0c41645f0b9cda91fc3715be7c84d8712e0312b9cfd17098e`).
Its WarpTool, WarpDiag, WarpDashboard and cswarp.library identify as 30.74,
dated 8 November 2026. The archive includes runtime programs, a user guide and
firmware images, but no C headers, FD/SFD files or source code. The library's
Amiga executable has no symbol table. Static inspection confirms the seven
WarpTool diagnostic labels already handled by SENSOR: CPU internal/external,
ARM and FPGA temperatures, 5 V supply, backup battery and fan output. This is
format verification, not a successful read on physical hardware.

Also reviewed `warpedvision_install_2.0.6.lha` and the
[WarpedVision manual revision 2.0](https://amigawarp.eu/download-files/WarpedVision-User-Manual-rev20.pdf)
(30 pages, firmware 2.0.6). These cover the scandoubler/flicker-fixer, video,
genlock and firmware uploading. No temperature/voltage/fan API was found;
the manual's IR sensor is the remote-control receiver.

- [x] Recover the 30.74 diagnostic vector (-42), register convention, 428-byte
  output and seven sensor fields from WarpTool and the library independently.
- [x] Add an opt-in version-gated probe and tested integer-only decoder, excluded
  from normal builds/packages. Print only sensors and clear the full result.
- [ ] Locate vendor-approved cswarp.library headers, calling conventions,
  diagnostic structure definitions and minimum-version requirements.
- [ ] Use the Linux Warp MFD diagnostic-frame patches as a protocol reference,
  not as an AmigaOS driver to call or install. The reviewed older frame omits
  FPGA temperature, so verify the 30.74 layout before replacing any reading.
- [ ] Prefer access through cswarp.library; direct mailbox access must coordinate
  with the existing driver and other clients of the shared buffer/interrupts.
- [ ] Keep WarpTool Diag as a fallback; preserve independent SENSOR selections
  and share bounded, cached background reads across instances.
- [ ] Resolve the observed unbounded library wait before automatic sampling;
  do not assume Ctrl-C cancels a stalled call or start overlapping retries.
- [ ] Validate native values and errors against WarpTool on actual boards,
  including absent hardware, timeouts, unsupported library versions and FPGA
  temperature. Obtain a real 30.74 Diag transcript for a versioned fixture.
- [ ] Check reuse terms before incorporating vendor or kernel implementation
  code. No firmware, library or vendor program is bundled by this review.


## 0.4 Wi-Fi and Warp CPU validation

- [x] Trace WarpTool/cswarp.library 30.74 SSID, password and clock setters.
- [x] Add independent workers, masked password entry and conditional menus.
- [x] Inspect zenPrismWifi and the Prism2 driver; use the documented SSID tag.
- [x] Add native Prism scanning and tested WPA key/profile serialization.
- [ ] Verify menu detection and masked editing on Workbench with real hardware.
- [ ] Compare Warp1260 clock readback and saved Wi-Fi settings with WarpTool.
- [ ] Check Prism scans and reconnect through WirelessManager on physical cards.
- [ ] Confirm busy-call recovery and profile rollback on target filesystems.
- [ ] Add other Warp library versions only after checking their individual ABI.
