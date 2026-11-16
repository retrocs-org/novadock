# Changelog

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
