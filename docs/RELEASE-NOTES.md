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

### Bugs

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
