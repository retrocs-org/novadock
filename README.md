# NovaDock 0.5

NovaDock is a lightweight, extensible dock for Classic AmigaOS Workbench 3.x.

**0.5 is in development.** The latest published release is 0.4.

[Website](https://novadock.retrocs.org) | [Downloads](https://novadock.retrocs.org/get.php) | [Changelog](CHANGELOG.md) | [Source](https://github.com/retrocs-org/novadock)

AmiRocket integration shows aggregate unread counts and connection status, with a click to start or restore the client. Configure its executable in the tile settings; accounts remain in AmiRocket. See [AmiRocket plugin](docs/AMIROCKET_PLUGIN.md).

## Features

- Keep tasks and recurring reminders in [AGENDA](docs/AGENDA_PLUGIN.md), and run a bounded local [PICTURES](docs/PICTURES_PLUGIN.md) slideshow.
- Compare performance with [BENCH](docs/BENCH_PLUGIN.md), create local [QR codes](docs/QR_PLUGIN.md), sample [COLOUR](docs/COLOUR_PLUGIN.md) and follow offline [MOON phases](docs/MOON_PLUGIN.md).
- Review read-only [HASS room summaries](docs/HASS_PLUGIN.md) for lights and open contacts through AmiHASS.

- Calculate decimal/hex/binary expressions with [CALC](docs/CALC_PLUGIN.md), or edit Workbench icon metadata with backup protection in [TOOLTYPES](docs/TOOLTYPES_PLUGIN.md).
- Monitor drawer changes with [WATCH](docs/WATCH_PLUGIN.md) and follow warning/error lines with [LOGS](docs/LOGS_PLUGIN.md), using bounded histories and unread badges.
- Keep permanent shortcut collections in [SHELF](docs/SHELF_PLUGIN.md) and review session notifications in [ALERTS](docs/ALERTS_PLUGIN.md).
- Inspect [volume capacity](docs/VOLUME_SPACE.md), set low-space warnings and use optional [I2C sensor readings](docs/SENSORS_PLUGIN.md).
- Launch applications, commands, drawers and volumes; arrange tiles by drag and drop.
- Choose from thirteen themes, seventy-five built-in icons, two tile sizes and multiple rows on any screen edge.
- Browse Aminet, read package information and download files with a progress bar.
- Build separate MUSIC and WHDLoad collections with recursive indexing, cached results, filters and custom icons.
- See current weather and five-day forecasts, save permanent Markdown Notes and set Timer reminders.
- Pack files and drawers into ZIP/LHA archives or extract an archive to a chosen destination, keeping Workbench icons.
- Control LumiAMP, EaglePlayer, AmigaAMP and HippoPlayer; capture screenshots and switch screen modes or public screens.
- Add system meters, clipboard history, Trash, folder bookmarks, hardware sensors and system action tiles.
- Take a break with SAVER's Boing Ball, copper waves, vectors, fireworks and maze, or a fullscreen aquarium. SAVER offers adaptive render sizes for slower systems. Optional performance diagnostics show FPS and frame timing.
- Manage the dock through unified Preferences, diagnostics, ARexx and the public Plugin SDK.

Version 0.3 adds AMINET, PLACES, SAVER, AQUARIUM, DOWNLOAD, SYSTEM, ACTIONS,
MUSIC and SENSOR, plus independent WHDLoad instances, improved forecasts and
network reconnects. See [CHANGELOG.md](CHANGELOG.md) for the complete changes.
Plugins keep their own version numbers independently of NovaDock.

Version 0.4 adds native Warp1260 Wi-Fi and CPU clock controls, plus a
Prism Wi-Fi chooser with scanning and WirelessManager profiles. HASS adds
Home Assistant tiles through the independent AmiHASS service, with room/type/search
filters, selected-entity subscriptions, read-only tiles and confirmed light,
thermostat, fan and cover controls. Automatic update checks remain off by default.
HASS sample controls are tested on Workbench; physical-device and physical-card
validation remain pending. See
[Network](docs/NETWORK_PLUGIN.md) and [CPU controls](docs/USING_NOVADOCK.md).

Version 0.5 opens the Plugins page while inventory loads in the background, caches validated plugin inventory, updates tool windows in place
and shares HASS room snapshots. Performance comparisons and regression budgets
are documented in [native performance QA](docs/QA_PERFORMANCE.md).

Preferences uses compact layouts on 640x256 screens with the standard 8-point
Workbench font. Longer plugin settings scroll; APPS and icon pickers use pages.
Preferences -> Plugins offers Refresh and repair guidance for the selected
instance, including stopped services, missing setup and unavailable readings.

## Requirements

- AmigaOS 3.0 or newer and a 68000 or better.
- Installer 42 or newer for the graphical installer; included with AmigaOS 3.1 and later.
- No MUI or RTG graphics card required for the dock.
- Weather needs TCP/IP and HTTPS-capable curl or Wget. AMINET needs TCP/IP;
  DOWNLOAD needs compatible curl. Music, games and app integrations need their corresponding programs.
- SENSOR supports ZZ9000 telemetry, AK_Sensors output and WarpTool readouts,
  including dotted and coloured Warp1260 diagnostics. An optional isolated
  helper adds channels from i2csensors.library v2; the library is installed separately.
  The helper carries its own GPL-3.0 licence in `Plugins/SensorsI2C.LICENSE`.
  Physical-board validation remains pending; direct TF1260 sensing is not supported.

## Install, update or remove

Download the latest published LHA from [Downloads](https://novadock.retrocs.org/get.php),
unpack it and open the **NovaDock** drawer.
Double-click **Install**. All installation modes include every bundled
plugin; add the tiles you want afterward in Preferences.

- **Novice** uses standard locations, enables Workbench startup and opens the dock.
  When NovaDock is installed, it still offers **Update** or **Uninstall**.
- **Average** also lets you choose application and Preferences drawers, startup
  and whether to launch after installation.
- **Expert** additionally lets you choose the shared-data parent.

Updates preserve existing settings, saved Notes, per-instance plugin options and
imported plugins and themes. New bundled plugins are installed automatically.
Uninstall asks for confirmation and whether to keep settings and saved Notes.
Reinstalling after keeping settings restores that configuration.

Fresh installs use Nord with seven tiles: the boot disk (`SYS:`), the installation's
default shell (`NewShell`), MusicPlayer, CPU, RAM, Clock and Trash. Right-click the dock
to add items, open Preferences, restart or quit. Right-click a plugin for its settings.

Default installed locations:

| Item | Location |
| --- | --- |
| Application | `SYS:Utilities/NovaDock` |
| Preferences | `SYS:Prefs/NovaDockPrefs` |
| Shared plugins, themes, icons and tools | `SYS:NovaDock/` |
| Help | `HELP:NovaDock.guide` |
| Active settings | `ENV:NovaDock/novadock.cfg` |
| Permanent settings and default Notes | `ENVARC:NovaDock/` |
| German and Swedish dock catalogs | `LOCALE:Catalogs/<language>/novadock.catalog` |

Press **Help** in Preferences to open the manual. Uninstall progress is recorded
in `ENVARC:NovaDock-Uninstall.log`. For a standard-location Shell install, run
`Execute install.amiga`. See [installation and usage](docs/USING_NOVADOCK.md)
for the complete instructions.

## Update notifications (0.4)

Automatic update checks are **off by default**. Open **Preferences -> Update**
to enable **Check for updates on startup** and/or **Check for updates while running**.
Running checks can repeat every **2, 4 or 6 hours** (default 4). Choose **Save**
to keep your choices. Checks wait for an available TCP/IP stack.

**Check now** works independently of the automatic checkboxes. The Update page
shows when the last attempt ran, its result and a live checking status.

A newer version opens a **Download... / Later** popup. Choose a destination;
the LHA is checked against its SHA-256 checksum before it is kept. Unpack it
and run **Install -> Update** yourself. Nothing installs automatically.
This optional feature requires an HTTPS-capable **C:curl** with working CA
certificates. See [usage](docs/USING_NOVADOCK.md#update-notifications-04).

## Plugin guides

[HASS / Home Assistant](docs/HASS_PLUGIN.md), [AMINET](docs/AMINET_PLUGIN.md), [MUSIC collections](docs/MUSIC_LIBRARY_PLUGIN.md),
[WHDLoad](docs/GAMES_PLUGIN.md), [Weather](docs/WEATHER_PLUGIN.md),
[Notes and Timer](docs/TIMER_AND_NOTES.md), [ARCHIVE](docs/ARCHIVE_PLUGIN.md),
[MusicPlayer](docs/MUSICPLAYER_PLUGIN.md), [Network](docs/NETWORK_PLUGIN.md),
[SENSOR](docs/SENSORS_PLUGIN.md), [ACTIONS](docs/ACTIONS_PLUGIN.md),
[PLACES](docs/PLACES_PLUGIN.md), [DOWNLOAD](docs/DOWNLOAD_PLUGIN.md),
[SYSTEM](docs/SYSTEM_PLUGIN.md), [SAVER](docs/SAVER_PLUGIN.md) and
[AQUARIUM](docs/AQUARIUM_PLUGIN.md).

## Build from source

Install vbcc with an AmigaOS m68k target and a compatible Amiga NDK, then run:

```sh
sh scripts/build-amiga.sh -B
```

To build and stage an installable drawer in a shared development folder:

```sh
FLEXCAT=/path/to/flexcat sh scripts/build-and-stage-amiga.sh /path/to/shared
```

The result is `/path/to/shared/NovaDock`. FlexCat compiles the German and Swedish
catalogs; local staging without it remains English-only. Production release
packaging requires FlexCat and refuses to omit these translations.

Start plugin development with the [SDK quick start](docs/PLUGIN_SDK_QUICKSTART.md),
[protocol reference](docs/PLUGIN_API.md) and [standalone SDK source](plugin-sdk/README.md).
NovaDock 0.5 also supports [plugin-owned artwork and state icons](docs/PLUGIN_SDK_ARTWORK.md),
with an ArtHello example and a standalone artwork packer.
For verified release builds, the four repository assets and Aminet submission,
use `scripts/release.py` as described in [docs/RELEASING.md](docs/RELEASING.md).

## Licence

NovaDock is released under the [MIT licence](LICENSE).
Bundled third-party components retain their accompanying notices.

The optional standalone SensorsI2C helper is GPL-3.0-or-later; see `Plugins/SensorsI2C.LICENSE`. The remaining NovaDock sources retain their stated licences.
