# Changelog

## NovaDock 0.6

### Features

- AMINOTIS shows the AmiNotis inbox on a tile, with an unread badge, quiet mode and a paged reading panel.
- PET adds an animated companion tile: a cat, Gerald the stapler, a dog, a rabbit or Bob the ball.
- Drawer popups load entries and icons in the background, with loading states, Escape to cancel and R to retry.
- Reload artwork in the dock menu retries tile artwork without restarting plugins or changing the layout.
- Launch feedback names the application being started and includes DOS error details on failure.
- Dock profiles save and switch named layouts from the dock menu or with ARexx PROFILE commands.
- Optional animated magnification eases tiles in and out, with adjustable 0-100% hover magnification.
- Adjustable 0-100% dock background transparency, with RTG blending and a palette-screen fallback.
- Optional dock background artwork with seamless ILBM strips and rounded corners.
- An optional drag grip moves the dock along its screen edge, with Start, Center and End presets.
- Undo the last move or removal from the dock menu or with ARexx UNDO while the dock is unlocked.
- Wallpaper-matched dock background on classic Workbench, following the desktop's colours.
- Theme packs can bring their own backdrop artwork; Import can pick any small ILBM as the backdrop.
- New macOS Modern and Mac OS X Glass themes with native artwork and transparency defaults.
- Colourful icons option preserves original artwork colours across themes.
- Forty new built-in icons in a tilted, bold-outlined late-90s desktop style.
- Theme-coloured CPU, RAM and digital Clock artwork with live readings outside graph mode.
- A magnifying Network interface icon with live incoming and outgoing packet readings.
- A Preferences plugin replaces the old launcher, with themed artwork.
- New ARexx commands: GET, SET, THEME, INFO, SAVE, UNDO and STATUS DETAIL.
- Support section in Preferences with logging and trace modes, optional disk saving and rotating logs.
- Timed support recordings restore normal settings automatically and keep bounded capture sessions.
- SupportReport guides you through a report, collects system details and saves an LHA archive or drawer.
- Danish and Italian translations.
- The plugin SDK includes diagnostic wrappers, examples, a policy guide and an integration validator.

### Enhancements

- Danish, German, Italian and Swedish catalogues cover the new launch, drawer and disk messages.
- Launcher icons and DataTypes pictures load in background workers and are reused across reloads.
- Volume space queries run in a background worker and keep the last valid readings while disks are busy.
- Notifications support longer titles and bodies, with a fallback for older plugins.
- An opt-in setting forwards new dock alerts to the separately installed AmiNotis service.
- Support snapshots and reports include AmiNotis delivery and failure diagnostics.
- ALERTS, AMIRC and AMIROCKET are retired in favour of AmiNotis; old settings are kept but inactive.
- Plugin names appear in uppercase in the dock, catalogues and settings.
- About, support reports and trace logs show the exact source build ID.
- Support is simplified to reporting, advanced tools and recovery.
- All bundled plugins report bounded resource and activity snapshots for support reports.
- A run of ARexx SET and THEME commands is applied to the dock in a single reload.
- Plugin options use a shared pool, reducing memory while keeping older profiles intact.
- Updates explains when an earlier, cancelled download is still finishing.
- Appearance and General are reorganized into clearer groups, with a larger layout on big screens.
- Fresh installs use Nord with rounded artwork, colourful icons and animated magnification.
- The default dock includes a Preferences tile.
- Labels offer Always, Hover and Hidden; hover names move above magnified tiles.
- Magnification blends smoothly between tiles and stays stable across gaps and rows.
- Repaints reuse cached shelves, widgets and artwork and redraw only the tiles that changed.
- Idle tool windows, workers and auto-hide sleep until needed, lowering background CPU use.
- Quitting or restarting no longer waits for an ARexx launcher that stopped answering.
- Crowded captions are shortened with an ellipsis; full names stay in hover captions and tooltips.
- Tiles show consistent loading, offline, unavailable and error markers with explanatory tooltips.
- SENSOR keeps headings, readings and artwork visible on compressed side docks.
- Plugin artwork appears as soon as the dock confirms support, without waiting for a heartbeat.
- All artwork states must support Tiny and Normal sizes, checked by an SDK validator.
- The public website has its own changelog page, checked against this changelog.
- Release LHA archives are checked for compatibility with classic extractors, including OLHa.
- Release testing covers all bundled plugins, every dock edge and a 30-minute packaged soak.

### Bugs

- Fix intermittent "freed twice" and "memory list corrupt" alerts when helpers or plugins exit.
- Fix the Appearance preview shelf when switching to Tiny icons.
- Find shared NovaDock components when running from a portable or unpacked drawer.
- Queued ARexx edits no longer override a later Preferences Save.
- Escape cancels Preferences pages and report windows consistently.
- Support reports include both RAM and persistent disk logs.
- With Animate off, the hovered tile snaps directly to the chosen magnification.
- Fix magnification frames that could show layer scrolling or partial repaints.
- Capture settings save correctly on ENV handlers without rename support.
- Always on top raises the dock only when a window really overlaps it, and no longer hides new windows.
- Keep the desktop snapshot the same size as the dock after live changes.
- Clicks are no longer dropped or misplaced while the dock window resizes.
- MusicPlayer queues actions and stops waiting for players that do not answer ARexx.
- Repeated ARexx launcher clicks are ignored while their target is still busy.
- Notes no longer reads past the end of text that ends with a heading marker.
- Recover when Intuition never confirms a dock resize.
- Queued clicks keep their positions when the system clock is set backwards.
- The installer replaces an older NovaDock.previous drawer on a repeated update.
- macOS Modern and Mac OS X Glass use valid theme IDs and their own defaults.
- The Plugin Manager and Add Item list up to 64 installed plugins and say when the list is full.
- MusicPlayer buttons and the volume knob match the magnified widget and give hover feedback.
- Nearly full docks no longer bunch up abruptly when magnification reaches the screen edge.
- Grey backdrop pixels no longer remain around the dock after startup or closed windows.
- Colourful icons no longer turn into grey silhouettes after a reload or screen reopen.
- The normal dock stays behind application windows moved into its magnification area.
- Light themes use the same contrast-aware icon palette in Appearance and the live dock.
- The dock no longer becomes invisible in Low Memory mode after an update or startup.
- Label settings apply consistently to plugin artwork, launchers and widget headings.
- Running dots and notification badges stay attached to magnified artwork.
- The plugin catalogue holds 64 entries, so disabled plugins no longer block new drawers.
- Magnification stays responsive while another window is active.
- The drag grip no longer leaves a detached strip after reloads.
- SENSOR keeps channel names separate and adds sensor-type artwork and per-channel titles.
- Plugin badges show multi-digit counts such as 129.
- A queued click no longer dismisses a context menu just after it opens.
- Support reports keep your answers and the last save destination after failures.

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

- Show compact CPU, RAM, network and digital-clock artwork in Tiny mode and its Appearance preview. Keep palette pens within the screen bitmap depth and preserve live widget artwork during magnification on classic planar screens.

- Update HASS for AmiHASS 0.3, with current climate validation and clearer connection, capacity and blocked-action messages.

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
