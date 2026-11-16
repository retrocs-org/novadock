# Future work

## Home Assistant plugin

Status: planned idea; implementation has not started.

### Initial scope

- [ ] Connect to an existing Home Assistant installation.
- [ ] Display sensor readings, including temperature, humidity, power usage and electricity prices where available.
- [ ] Control switches and lights, including brightness where supported.
- [ ] Activate scenes and scripts, such as an "Amiga time" scene for desk lighting and background music.
- [ ] Control devices through Home Assistant actions and reflect their reported state afterward.

### Dock and settings

- [ ] Support multiple instances with independent labels, icons, selected entities and click actions.
- [ ] Share server and connection settings between instances.
- [ ] List available entities in Edit settings, with selection controls and readable names.
- [ ] Show readings and changing state icons directly in tiles.
- [ ] Open a native popup for additional readings or device controls.
- [ ] Support count badges for grouped statuses, such as open windows.
- [ ] Show unavailable/stale states clearly, with the last update time in the tooltip.

### Connection and performance

- [ ] Evaluate REST requests through the existing downloader approach against a persistent WebSocket connection.
- [ ] Prefer updates for selected entities rather than repeatedly fetching the entire installation.
- [ ] Share one background connection across plugin instances; keep network work outside the dock UI.
- [ ] Bound response sizes, entity counts and memory usage.
- [ ] Add timeouts, reconnect backoff and cached readings without presenting old values as current.
- [ ] Measure idle CPU, memory and responsiveness on supported Amiga configurations.
- [ ] Store the access token separately from exportable layouts and exclude it from logs, packages and source control.

### Later possibilities

- [ ] Door/window status and lists of open contacts.
- [ ] Thermostat controls and room summaries.
- [ ] Energy summaries and electricity price tiles.
- [ ] Media-player track information, playback and volume controls.
- [ ] Appliance status and remaining-time displays where exposed by Home Assistant.

### Validation and documentation

- [ ] Test unavailable entities, authentication failures, connection loss and recovery.
- [ ] Verify that controls reflect confirmed device state and remain responsive during requests.
- [ ] Document setup, supported entity types, credentials and connection requirements when implemented.

### References

- [Home Assistant REST API](https://developers.home-assistant.io/docs/api/rest/)
- [Home Assistant WebSocket API](https://developers.home-assistant.io/docs/api/websocket/)

## Other future plugins

Status: planned ideas; implementation has not started.

### UPDATES

- [ ] Check for NovaDock and plugin updates and show an available-update badge.
- [ ] Show installed/available versions and release notes in a native window.
- [ ] Download selected packages and verify their checksums before offering installation.

### AGENDA

- [ ] Save tasks, dated reminders and recurring events permanently.
- [ ] Show a badge for items due today and open an agenda on click.
- [ ] Provide completion, editing and reminder controls.

### CALC

- [ ] Open a compact calculator with calculation history.
- [ ] Support decimal, hexadecimal and binary conversion.
- [ ] Copy results to the clipboard.

### SHELF

- [ ] Accept dropped files and drawers as shortcuts in a persistent collection.
- [ ] Open a native picker to launch a saved shortcut.
- [ ] Support multiple shelves with independent labels, icons and contents.
- [ ] Allow reordering and removal of shortcuts without deleting their target files.

### PET

- [ ] Add a small animated companion to a dock tile.
- [ ] React to clicks with playful animations.
- [ ] Offer appearance and animation-speed settings, with a low CPU cost when idle.

### MOON

- [ ] Display the current moon phase with a changing tile icon.
- [ ] Open a detailed view with moon-phase and sunrise/sunset information.
- [ ] Configure location and time zone, and refresh only as often as needed.

### TOOLTYPES

- [ ] Accept dropped Workbench icons and inspect their ToolTypes, default tool and stack size.
- [ ] Provide a native editor for these settings.
- [ ] Back up the original icon before saving changes and preserve its artwork.

### COLOUR

- [ ] Pick a colour from the screen and display its RGB and hexadecimal values.
- [ ] Copy colour values to the clipboard.
- [ ] Keep a small saved palette for theme development.

### ALERTS

- [ ] Collect notifications from dock plugins in one native history window.
- [ ] Show an unread-count badge and allow notifications to be marked as read.
- [ ] Offer optional notification sounds.
- [ ] Plan integration for completed downloads, timers, low disk space and Home Assistant events.

### PICTURES

- [ ] Cycle through supported images from a selected drawer in a picture-frame tile.
- [ ] Open a larger image viewer on click.
- [ ] Configure the slideshow interval and keep image decoding and memory use bounded.

### WATCH

- [ ] Watch selected drawers for new or changed files, including downloads and build output.
- [ ] Show a badge for unseen changes and open a native list of recent changes on click.
- [ ] Allow changed files or their containing drawers to be opened from the list.
- [ ] Keep monitoring lightweight and handle unavailable drawers without blocking the dock.

### LOGS

- [ ] Follow a selected log file and show new entries in a native viewer.
- [ ] Highlight errors and warnings, with a badge for unseen matches.
- [ ] Support filtering and copying entries, with bounded history and lightweight monitoring.
- [ ] Handle truncated, replaced and temporarily unavailable log files.

### BENCH

- [ ] Run short CPU, memory and graphics benchmarks on demand.
- [ ] Save dated results with configuration details for comparisons between Amiga setups.
- [ ] Keep the UI responsive and allow tests to be stopped.

### QR

- [ ] Generate QR codes locally from clipboard text or an entered URL.
- [ ] Display codes at a readable size for scanning with a phone.
- [ ] Offer image export without requiring an external service.

### TRANSIT

- [ ] Select a transport provider and favourite stops or routes.
- [ ] Show the next departure in the tile and a native departure board on click.
- [ ] Evaluate provider APIs, coverage and credential requirements before implementation.
- [ ] Cache results, display their update time and respect provider refresh limits.

## Drawer and Volume enhancements

Extend the existing Drawer/Volume items. Their space meter reports the
containing volume's usage, not the selected drawer's size.

- [ ] Show free, used and total capacity in the tooltip, with readable units.
- [ ] Add configurable low-space thresholds and warning colours.
- [ ] Add a popup summarising mounted volumes and their capacity.
- [ ] Offer remaining free space directly on the tile as an alternative display.
