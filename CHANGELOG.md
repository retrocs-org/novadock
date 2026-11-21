# Changelog

## NovaDock 0.5

### Features

- HASS supports advertised smart-lock and motorised-door actions through AmiHASS.

- AGENDA keeps persistent tasks and recurring reminders with a due badge.
- PICTURES displays local slideshows with larger previews and bounded background image decoding.
- HASS exposes supported media playback, volume and track information through AmiHASS.

- BENCH runs cancellable CPU, memory-copy and off-screen drawing comparisons with dated saved reports.
- QR generates local UTF-8 codes from text or clipboard contents, with readable previews and ILBM export.
- MOON shows offline moon phases, approximate illumination and location-based sunrise/sunset estimates.
- COLOUR samples Workbench pixels, copies hex/RGB values and keeps a protected 32-colour named palette.
- HASS adds read-only room summaries for lights, open contacts/covers and unavailable entities through AmiHASS.

### Enhancements

- Confirm HASS unlock, latch and door-opening actions; show clear door states and load supported actions in the background.

- Include Trash between Music and CPU in the default dock layout.

- Show percentage changes beside BENCH saved-result comparisons.
- Add optional bounded plugin thumbnails without host-side image decoding or palette changes.

- Retry saving a completed BENCH report after a destination error without rerunning its measurements.

- Open Plugins immediately while inventory, cache writes and third-party details load in a cancellable background worker.
- Derive installer QA versions from package executables and pair the HASS client explicitly with AmiHASS 0.2.

- Cache validated plugin inventory with source fingerprints and a forced Refresh; corrupt caches fall back to normal scans.
- Remove idle palette rebuilding and unchanged list copies; wait on signals in idle tool windows.
- Share HASS room aggregations, coalesce refreshes and transfer ownership when a room tile closes.
- Keep tool result windows and user edits intact during background updates; add COLOUR swatches, BENCH stages, QR export gating and MOON input validation.
- Add paired native startup/window-latency, idle-wakeup and memory budgets with a regression checker.

- Speed up built-in option lookup and animated artwork validation; read each artwork bank only once during package health checks.

- Open General and bundled plugin settings without scanning unrelated plugins; use a lightweight, checksummed temporary preview backup.

- Rename the installer and its Workbench icon to Install and Install.info.
- New package drawer and installer icon.
- Use the current packaging time for LHA and ZIP file dates and verify extracted LHA timestamps.

### Bugs

- Use factory defaults for fresh installs from a runnable development drawer, preserving existing installed settings on updates.

- Synchronize the AmiHASS SDK and require confirmed open/closed readback after stopping a cover.

- Keep QR/BENCH staging filenames within classic filesystem limits; report write-protected, full and invalid destinations without losing results or replacing existing files.
- Preserve COLOUR edits and saved palettes on failed writes, with actionable destination and recovery errors.
- Correct HASS setup instructions to AmiHASS 0.2 and Install, and check the documentation against the paired SDK.

- Avoid false plugin repair warnings for absent optional artwork on AmigaOS, restoring affected plugins to Add Item.
- Reject release builds with missing, invalid or incompatible plugin packages after archive extraction.

## NovaDock 0.4

### Features

- CALC with expression history, decimal/hex/binary conversion and clipboard copy.
- TOOLTYPES edits Workbench metadata with retained artwork and numbered backups.

- WATCH drawer monitoring and LOGS file following, with independent instances, native filtered histories, unread badges, notifications and bundled artwork.

- ALERTS collects session notifications with an unread badge and optional system beep.
- SHELF stores independent shortcut collections with filtering, drops and safe reordering.
- SENSOR can discover readings through an optional i2csensors.library v2 provider.
- AmiRocket chat integration with unread badges, connection states and click to start or restore the client.
- Optional SAVER performance overlay and saved report with actual FPS and average/peak rendering time.
- HASS tiles for entities, groups, scenes and scripts through the independent AmiHASS service.
- Device-first HASS setup with room/type/search filters and light on/off/toggle actions.
- Dimmable-light brightness controls through AmiHASS, with confirmed readback.
- Forty-five smart-home icons in the shared icon library.
- Plugin SDK artwork banks with named states, theme colours, value layouts, validated imports and an ArtHello example.
- Bundled static and animated plugin icons ship with their plugins through a shared renderer.
- HASS artwork and sad WHDLoad, MUSIC and SENSOR icons ship with their respective plugins.
- Automatic HASS device icons show off/on, closed/open and unknown states, with a sad bridge for connection failures.
- Separate startup and 2/4/6-hour update checks, Check now, last-check status and verified release downloads.
- Warp1260 Wi-Fi settings and CPU turbo presets through native library calls; physical-board validation pending.
- Prism Wi-Fi network chooser, scanning and WPA/Open WirelessManager profiles that preserve existing settings; physical-card validation pending.

### Enhancements

- HASS adds a paged room chooser and explicit AmiHASS feature negotiation.

- WATCH adds name filters, pause/resume and batched copy-burst notifications.
- LOGS adds live-tail, frozen views, severity filtering and safe text export.
- HASS popups expose supported temperature, fan-speed and cover controls through AmiHASS.
- HASS tiles add selected-entity subscriptions, refresh intervals, read-only mode and display preferences.
- HASS shows sample/server sources, verified control capabilities and bounded action confirmation.

- Drawer/Volume tiles show capacity details, configurable low-space warnings, free-space text and a mounted-volume overview.

### Bug fixes

- Website release labels, plugin counts and SDK downloads agree with published packages.

- Shared panels retain their libraries while other utility windows remain open.
- Background workers retain explicit ownership through cancellation and late replies.

- SENSOR distinguishes disabled channels, stale readings, unavailable devices and a missing optional I2C library in its details and Preferences.

- HASS brightness dragging survives status refreshes and sends released values after pending queries.
- Plugin artwork keeps its palette pens reserved so other windows cannot turn its colours grey.

- Badge and notification replies preserve artwork capabilities so HASS on/off icons stay synchronized.

- HASS device selection retains the chosen type when opening, paging and refreshing the picker.

- Compact Appearance, Add Item, launcher and Wi-Fi dialogs keep controls reachable on 640x256 screens.
- APPS pages long application lists; short Preferences detail rows and picker captions stay clear.
- Settings saves validate staged files before replacement, with startup guidance when recovering a saved copy.
- Stalled plugins no longer trigger duplicate restarts; bounded IPC batches keep dock input and shutdown responsive.
- SAVER caches Boing frames, repaints changed areas and batches raster bars; adaptive render sizes reduce work in deep screen modes.
- Preferences reports missing/damaged artwork and per-instance runtime problems with Refresh and repair guidance.
- HASS discards replies and control capabilities when changing targets; offline controls are not replayed after recovery.
- HASS scenes, scripts and groups open filtered lists without a physical-device step.
- HASS type changes reset incompatible actions; artwork states trigger a repaint independently of text.
- HASS can be added from the plugin picker, including independent instances.
- HASS uses the current AmiHASS bridge SDK, limits startup retries and restores readings after reconnecting.
- HASS settings cover individual tiles; shared connection settings are managed only in AmiHASSPrefs.
- HASS device and entity lists fill all visible rows instead of stopping at sixteen.
- SENSOR reads dotted and coloured WarpTool diagnostic output.
- ZZ9000 temperature is correctly labelled as the Zynq chip reading.
- Dock icons retain embedded and classic MagicWB colours without changing the Workbench palette.

## NovaDock 0.3

### Features

- AMINET with a native browser for recent uploads, categories, package details, readmes and downloads.
- PLACES with system drawers, mounted volumes and permanent folder bookmarks.
- SAVER with Boing Ball, copper waves, vector shapes, fireworks, a solving maze and selectable demo shuffle.
- AQUARIUM with a fullscreen animated fish tank.
- DOWNLOAD with a session queue and HTTP(S) progress.
- SYSTEM with CPU, Workbench version and boot uptime.
- ACTIONS tiles with independent system actions, custom icons and optional confirmation.
- Restart NovaDock directly from the dock context menu.
- Optional automatic network connection when the Network plugin starts.
- Independent WHDLoad tiles with custom labels/icons, executable selection, recursive folders and cached indexes.
- Independent SENSOR tiles for ZZ9000, AK_Sensors and Warp readings, with selectable channels; physical-board validation is pending.
- MUSIC collections with independent folders, format checkboxes including SID, and a configurable player.
- Collection count badges including zero, live title filters and automatic custom artwork preference.
- WHDLoad folder warnings and automatic instance settings/cache cleanup on removal.
- Illustrated icon chooser, live settings previews and 28 new collection/status icons.

### Enhancements

- CPU tooltip shows the detected processor model and current usage.
- AMINET downloads show a progress bar, percentage and received/total size.
- Christmas theme with evergreen tiles, cream text and red/gold accents.
- Weather forecast table with condition icons, precipitation chances and rain/snow amounts.
- Add Item lists plugin names simply; Preferences lists unusable plugins with repair reasons.
- Updated user documentation, plugin guides and website screenshots for 0.3.

### Bug fixes

- Plugin settings save reliably with larger profiles and apply changed options atomically.
- MUSIC settings use one vertical scrollbar and new instances select every supported format.
- MUSIC and SENSOR appear after updates; bundled manifests are validated during testing.
- WHDLoad explains invalid drawers, restores transparent tile artwork and hides unnecessary icon paging buttons.
- Rapid network stop/start reconnects the selected interface without reloading Roadshow.
- Release packaging requires compiled German and Swedish catalogs.
- Planning documents stay out of the application Help guide.
- Release builds check generated files without dirtying the committed source tree.

## NovaDock 0.2

### Features

- Weather with current conditions, five-day forecasts and an offline cache.
- Timer with countdowns, pause, reset and reminders.
- Permanent Notes with formatting, Markdown view and recoverable deletion.
- Archiver creates ZIP/LHA archives from files and drawers, and offers extraction destinations.
- Screen Switcher lists and switches between public screens.

### Enhancements

- LumiAMP playback, track information and volume controls.
- Direct screen-mode selection with the current mode highlighted.
- True-colour RTG screenshots, Workbench icons and capture-status icons.
- Empty/full Trash icons that refresh after external changes.
- Drag-and-drop placement with a visible insertion marker.
- Consistent plugin settings, shorter names and automatic default-name migration.
- Lower idle CPU use and faster running-app checks, graphs and Notes selection.

### Bug fixes

- Tile menus open on right-click even when the dock is inactive.
- The plugin picker includes CPU and RAM; scrollbar arrows respond to clicks and holds.
- SHOT, SMODE and SCREENS start reliably and perform their intended click actions.
- Multi-item archive drops create one archive, include existing .info files and return to idle when finished.
- Notes save, browse and close correctly, fit the tile and keep clean Markdown.
- Open plugin windows close cleanly before restarting the dock.
- Installers include every bundled plugin, preserve custom paths and saved Notes, and clean up startup entries.
- Uninstall logs stay outside removed application folders, and package files are excluded from maintenance detection.

## NovaDock 0.1 - First public release

- Application, drawer, volume and command launchers with drag-and-drop support.
- Dock placement on any screen edge, multiple rows, auto-hide and keyboard controls.
- Running-application indicators, drawer popups and removable-media tiles.
- Twelve themes, custom theme imports and configurable tile artwork.
- CPU, memory and network meters with live graphs.
- Clock and calendar, music controls, AHI mixer and Screen Mode presets.
- Clippy clipboard history, screenshots, archive tools, Trash and WHDLoad games.
- BitSkim, AmIRC and Directory Opus 4 integration.
- ARexx commands, script tiles and event hooks.
- Unified Preferences, Plugin Manager, diagnostics and settings recovery.
- Public Plugin SDK with configurable plugin settings.
- English, German and Swedish dock text.
- Graphical installer and AmigaGuide help for Classic AmigaOS 3.0 or later.
