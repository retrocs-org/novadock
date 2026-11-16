# NovaDock 0.3

NovaDock is a lightweight, extensible dock for Classic AmigaOS Workbench.

## Features

- Application, command, drawer and volume launchers, with drag-and-drop placement, groups and drawer popups.
- Thirteen themes, third-party theme imports, two tile sizes, auto-hide and multi-row layouts on any screen edge.
- Current weather and five-day forecasts, permanent Markdown Notes and countdown Timer reminders.
- ARCHIVE creates ZIP/LHA files from multiple files and drawers, or extracts archives to a chosen destination, preserving Workbench icons.
- Music controls for LumiAMP, EaglePlayer, AmigaAMP and HippoPlayer; screen capture, resolution selection and public-screen switching.
- System meters, clipboard history, Trash and application integrations; unified Preferences, diagnostics, ARexx control and a public plugin SDK.

Bundled plugins: Weather, SCREENS, Clock, Eyes, CPU, RAM, Network, MusicPlayer, DOpus4, ARCHIVE, Command, APPS, Trash, WHDLoad, MUSIC, SENSOR, AQUARIUM, PLACES, SAVER, AMINET, DOWNLOAD, SYSTEM, ACTIONS, AmIRC, BitSkim, Clippy, Mixer, SHOT, SMODE, Script, Timer, Notes.

## Installation

Download **NovaDock-0.3.lha**, unpack it and open the **NovaDock** drawer.
Double-click **Install-NovaDock** and follow the graphical installer. It can
install, update or remove NovaDock, preserving settings on update.

Requires AmigaOS 3.0 or later and a 68000 or better. The graphical installer
needs Installer 42 or newer (included with AmigaOS 3.1 and later). For a
standard-location Shell install, run `Execute install.amiga`.

Help: `HELP:NovaDock.guide`. Settings and permanent Notes live under
`ENVARC:NovaDock` by default. Weather needs internet access and an
HTTPS-capable curl or Wget; other integrations need their corresponding apps.

## Source and Licence

[Source and Plugin SDK](https://github.com/retrocs-org/novadock) |
[Tagged release](https://github.com/retrocs-org/novadock/releases/tag/0.3) |
[Website](https://novadock.retrocs.org) |
[MIT licence](https://github.com/retrocs-org/novadock/blob/0.3/LICENSE)

## Version Changelog

### 0.3

#### Features

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
- Independent SENSOR tiles with detected ZZ9000, AK_Sensors and Warp readings, channel checkboxes and a sad empty-state icon.
- MUSIC collections with independent folders, format checkboxes including SID, and a configurable player.
- Collection count badges including zero, live title filters and automatic custom artwork preference.
- WHDLoad folder warnings and automatic instance settings/cache cleanup on removal.
- Illustrated icon chooser, live settings previews and 28 new collection/status icons.

#### Enhancements

- CPU tooltip shows the detected processor model and current usage.
- AMINET downloads show a progress bar, percentage and received/total size.
- Christmas theme with evergreen tiles, cream text and red/gold accents.
- Weather forecast table with condition icons, precipitation chances and rain/snow amounts.
- Add Item lists plugin names simply; Preferences lists unusable plugins with repair reasons.

#### Bugs

- Plugin settings save reliably with larger profiles and apply changed options atomically.
- MUSIC settings use one vertical scrollbar and new instances select every supported format.
- MUSIC and SENSOR appear after updates; bundled manifests are validated during testing.
- WHDLoad explains invalid drawers, restores transparent tile artwork and hides unnecessary icon paging buttons.
- Rapid network stop/start reconnects the selected interface without reloading Roadshow.
