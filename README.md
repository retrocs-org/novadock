# NovaDock 0.3

NovaDock is a lightweight, extensible dock for Classic AmigaOS Workbench 3.x.

[Website](https://novadock.retrocs.org) | [Downloads](https://novadock.retrocs.org/get.php) | [Changelog](CHANGELOG.md) | [Source](https://github.com/retrocs-org/novadock)

## Features

- Launch applications, commands, drawers and volumes; arrange tiles by drag and drop.
- Choose from thirteen themes, thirty built-in icons, two tile sizes and multiple rows on any screen edge.
- Browse Aminet, read package information and download files with a progress bar.
- Build separate MUSIC and WHDLoad collections with recursive indexing, cached results, filters and custom icons.
- See current weather and five-day forecasts, save permanent Markdown Notes and set Timer reminders.
- Pack files and drawers into ZIP/LHA archives or extract an archive to a chosen destination, keeping Workbench icons.
- Control LumiAMP, EaglePlayer, AmigaAMP and HippoPlayer; capture screenshots and switch screen modes or public screens.
- Add system meters, clipboard history, Trash, folder bookmarks, hardware sensors and system action tiles.
- Take a break with SAVER's Boing Ball, copper waves, vectors, fireworks and maze, or a fullscreen aquarium.
- Manage the dock through unified Preferences, diagnostics, ARexx and the public Plugin SDK.

Version 0.3 adds AMINET, PLACES, SAVER, AQUARIUM, DOWNLOAD, SYSTEM, ACTIONS,
MUSIC and SENSOR, plus independent WHDLoad instances, improved forecasts and
network reconnects. See [CHANGELOG.md](CHANGELOG.md) for the complete changes.
Plugins keep their own version numbers independently of NovaDock.

## Requirements

- AmigaOS 3.0 or newer and a 68000 or better.
- Installer 42 or newer for the graphical installer; included with AmigaOS 3.1 and later.
- No MUI or RTG graphics card required for the dock.
- Weather needs TCP/IP and HTTPS-capable curl or Wget. AMINET needs TCP/IP;
  DOWNLOAD needs compatible curl. Music, games and app integrations need their corresponding programs.
- SENSOR supports ZZ9000 telemetry, AK_Sensors output and WarpTool readouts.
  Physical-board validation remains pending; direct TF1260 sensing is not supported.

## Install, update or remove

Download `NovaDock-0.3.lha`, unpack it and open the **NovaDock** drawer.
Double-click **Install-NovaDock**. All installation modes include every bundled
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

Fresh installs use Nord with six tiles: the boot disk (`SYS:`), the installation's
default shell (`NewShell`), MusicPlayer, CPU, RAM and Clock. Right-click the dock
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

## Plugin guides

[AMINET](docs/AMINET_PLUGIN.md), [MUSIC collections](docs/MUSIC_LIBRARY_PLUGIN.md),
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
For verified release builds, the four repository assets and Aminet submission,
use `scripts/release.py` as described in [docs/RELEASING.md](docs/RELEASING.md).

## Licence

NovaDock is released under the [MIT licence](LICENSE).
Bundled third-party components retain their accompanying notices.
