# NovaDock

NovaDock is a lightweight, extensible dock for Classic AmigaOS Workbench 3.x.

- Website: [novadock.retrocs.org](https://novadock.retrocs.org)
- Source: [github.com/retrocs-org/novadock](https://github.com/retrocs-org/novadock)

Fresh installations start with the Nord theme and six tiles: the boot disk
(`SYS:`), the installation's default shell (`NewShell`), Music, CPU, RAM and Clock.
Add other tiles or open their settings from the dock's right-click menus.
Updates keep existing settings; choose Reset in the installer to use this layout.

## Features

- Application, command, drawer, and volume launchers with Workbench icons.
- Application groups, normal and tiny layouts, multiple rows, themes, auto-hide, and screen-edge placement.
- Running-application indicators, tooltips, drag-and-drop, and drawer popups.
- Bundled CPU, memory, clock, music, archive, Directory Opus 4, and running-app plugins.
- A unified Preferences application with Plugin Manager, organization, diagnostics, and About pages.
- Recoverable atomic configuration saving, adaptive plugin pacing, and dock-capacity diagnostics.
- A documented plugin SDK with a small example plugin.

## Requirements

- AmigaOS 3.0 or newer.
- A classic 68k Amiga.
- Optional plugin dependencies are described in the files under `docs/`.

## Build

Install vbcc with an AmigaOS m68k target and a compatible Amiga NDK, then run:

```sh
sh scripts/build-amiga.sh -B
```

To build and copy the required Amiga files into a shared development directory:

```sh
sh scripts/build-and-stage-amiga.sh "/path/to/shared/bin"
```

The staging script creates a `NovaDock` subdirectory at the destination. Its outer `NovaDock.info` sets Workbench to Show Only Icons and View By Icon, so files without icons stay out of the normal package view. On AmigaOS, double-click **Install-NovaDock** in that drawer to install with the standard Installer, or start the programs directly from it.

## Building an Amiga LHA release

Configure `VBCC` for your vbcc installation and `LHA` for **LHa for UNIX** (the archiver that can create archives; lhasa only extracts), then run:

```sh
sh scripts/make-release.sh /path/to/releases
```

The script regenerates the manual/catalog definitions, rebuilds all Amiga programs, stages a clean installer package and creates `NovaDock-0.1.lha`, its SHA-256 checksum and an accompanying readme. The archive contains the `NovaDock` installation drawer and its outer `NovaDock.info`, including the installer and prebuilt icons. It uses LH5 compression with level-1 headers, tests CRCs, extracts the archive and compares every file before replacing a previous release.

`VC` can override the compiler command for a custom vbcc configuration. `ND_SKIP_BUILD=1` packages existing binaries when a rebuild has already completed. `UPLOADER` optionally fills the readme's uploader field. Normal builds never upload anything.

### Preparing an Aminet submission

```sh
UPLOADER="your-email@example.com" AUTHOR="Your name" \
  sh scripts/make-release.sh /path/to/releases --aminet
```

This builds the usual versioned release and prepares `aminet/novadock.lha`, `aminet/novadock.readme` and a local `submission.json` containing hashes. This command only prepares and previews files; it makes no connection to Aminet. Set your real uploader email when you are ready. The readme is wrapped to 78 columns, with required metadata and a Short field of at most 40 characters.

Review the prepared files, then run a local check:

```sh
python3 tools/aminet_release.py check /path/to/releases/aminet
```

Only when ready to submit, run the same command with `--upload`. Alternatively, `make-release.sh /path/to/releases --upload-aminet` explicitly builds, prepares and submits in one invocation (with `UPLOADER` set). Upload transfers only the archive and matching readme, with the readme last. The manifest and checksums stay local. Submission goes to Aminet incoming; publication depends on Aminet processing. No upload is performed by the default build or by double-clicking the local Mac launcher.

Aminet recommends a stable filename, so its copy is `novadock.lha` with the version in the readme; local releases remain versioned. For replacing older differently named releases, set `AMINET_REPLACES` to full Aminet paths separated by semicolons, for example `util/wb/NovaDock-0.1.lha`. Omit it for the first release or updates to the same `novadock.lha` filename. Edited or rebuilt prepared files fail the hash check; prepare the submission again before uploading.

See [Aminet's upload instructions](https://wiki.aminet.net/Uploading_instructions) and [readme format](https://wiki.aminet.net/The_Readme_file). The default target is anonymous FTP at `main.aminet.net/new`, as documented there. Recheck their instructions before a future public release.

## Installing on AmigaOS

**Install-NovaDock** is a GUI installer for the Commodore Installer included with AmigaOS 3.x (version 42 or later). **Novice** is selected by default: it uses the standard paths, enables startup with Workbench and opens NovaDock after installation, without asking for these choices. Existing settings are kept, and updates retain recorded installation paths.

**Average (Intermediate)** asks for the application drawer (default `SYS:Utilities`) and Preferences drawer (default `SYS:Prefs`). Each binary and its `.info` file goes directly into the selected drawer. Shared components use `SYS:NovaDock`.

**Expert** also asks for the shared-data parent (default `SYS:`), where a `NovaDock` drawer holds plugins, artwork, themes, scripts and helper tools. Help, settings and translations use their standard locations in every mode.

Both Average and Expert ask two separate Yes/No questions, in order: start NovaDock automatically with Workbench, then start NovaDock when installation finishes. Updates retain recorded locations in every mode.

The dock's messages, menus and calendar are translated into German and Swedish; the installer adds these translations to `LOCALE:Catalogs`, and NovaDock uses the language chosen in Locale preferences.

The AmigaGuide manual, `NovaDock.guide`, is always installed as `HELP:NovaDock.guide`: press **Help** in any NovaDock Preferences window, or the **Help...** button on its About page, to open it in MultiView.

Existing settings are kept unless you choose to reset them (the old file is saved as `novadock.cfg.old`). Run the installer again to update NovaDock, keeping your settings and imported plugins and themes, or to remove it. A running NovaDock is stopped first: `NovaDock QUIT` asks a running dock to quit from the Shell.

The Workbench icon uses the bundled `StartInstaller` launcher. It prefers `SYS:System/Installer`, falls back to `C:Installer`, and runs with a 64 KB stack. Keep the launcher beside `Install-NovaDock`. Uninstall progress is recorded in `ENVARC:NovaDock-Uninstall.log` so the last completed step can be checked after a restart.

The Shell script `install.amiga` installs the standard locations. Use the GUI installer to select custom locations. The installer records `NovaDock:`, `NovaDockApp:` and `NovaDockPrefs:` in `S:User-Startup`. Help uses `HELP:` without an extra assignment. Updates preserve imported components. The old Utilities drawer is backed up before the application replaces it.

Adding, moving and removing tiles updates the existing dock window when its
window resources and fitted tile dimensions remain compatible. Existing scaled
artwork and plugin state are reused; only new artwork is loaded. Changes to the
theme, font, refresh mode or fitted tile size use the full rebuild path.

The optional [BitSkim plugin](docs/BITSKIM_PLUGIN.md) starts or restores BitSkim,
shows nearby computers, incoming offers, transfer speed and retained results.
Right-click Send file or drop one file to choose a recipient using protocol 2;
Open BitSkim for detailed transfer information. BitSkim is installed separately.

The optional [AmIRC plugin](docs/AMIRC_PLUGIN.md) shows IRC client status and
can start or restore AmIRC. Add it through the dock background menu.

[Settings saves and Diagnostics](docs/RESPONSIVE_SETTINGS.md) now separate
background file writes from live dock updates.

Network replaces the standalone Roadshow plugin and combines its controls with packet meters/graphs in one tile. Its
right-click menu offers Online or Offline, a Details window, and settings;
the main Network interface selector derives its driver/unit automatically.
Sampling overrides require explicit opt-in under Advanced. Failed connection
changes open an error popup. See [Network](docs/NETWORK_PLUGIN.md),
including requirements for compatible TCP/IP stacks.

[Clippy](docs/CLIPBOARD_PLUGIN.md) offers a centered picker and a
separate management window. [AmIRC unread alerts](docs/AMIRC_PLUGIN.md) use
numbered badges through the supplied event hook. Repeated Apps clicks keep
its running-application popup open.

After manually replacing executables, quit and start NovaDock to replace
resident plugin processes as well. The installer handles stopping the dock
for an update; copying binaries alone does not update a running process.

## Plugin development

Start with [the Plugin SDK quick start](docs/PLUGIN_SDK_QUICKSTART.md). The complete protocol is documented in [the Plugin API](docs/PLUGIN_API.md).

## License

NovaDock is released under the [MIT License](LICENSE).


Network full-stack shutdown first puts the selected Roadshow interface offline,
then runs NetShutdown. The native bridged A2065 comparison completed with this
sequence; direct NetShutdown hung with the dock stopped. A failed interface
step produces an error and leaves full shutdown unstarted. Network uses separate
IN/OUT meters by default; online clicks still switch to the two-channel graph.

Default installed layout:

- `SYS:Utilities/NovaDock` and `.info`: visible application.
- `SYS:Prefs/NovaDockPrefs` and `.info`: visible Preferences application.
- `SYS:NovaDock/`: shared data, with no drawer icon. `Plugins`, `Themes`, `Icons`, `Rexx`, and `Tools` live here; `Tools/SaveConfig` writes saved settings.
- `HELP:NovaDock.guide` and `.info`: the manual goes directly into `HELP:`, without a subdrawer. The default tool is `SYS:Utilities/MultiView`.
- `ENV:NovaDock/novadock.cfg`: active settings; Use updates ENV.
- `ENVARC:NovaDock/novadock.cfg`: permanent settings; Save updates ENV and ENVARC. Settings drawers have icons, and the existing config filename/format is preserved.

Only the application drawer, Preferences drawer and shared-data parent are selectable. Help, settings and translations use their standard assignments. The shared directory has no `.info`, so it stays out of Workbench's normal icon view; Show All Files can still reveal it. The unpacked distribution retains its drawer and installer icons.

When an installed executable is detected, every user level, including Novice, displays its locations and offers Update or Uninstall. Use the Installer's Abort Install button to cancel. Uninstall always asks for confirmation and whether to keep settings; Novice does not bypass these questions. Update is selected by default, but the choice is always shown. Updates retain custom binary/shared-data locations, migrate the earlier default shared-data location to `SYS:NovaDock`, and move older manuals directly into `HELP:`. Imported plugins and themes are preserved. Leftover resources alone do not count as an installed application. Uninstall automatically resolves recorded application, Preferences and shared-data locations, falling back to documented standard and legacy directories. It never asks you to select directories, including when the Plugins drawer is missing but Themes or Tools remains. It only asks for removal confirmation and whether to keep settings. Uninstall deletes only NovaDock's files from old shared Help locations and never removes the HELP: root.

BitSkim defaults to `SYS:Utilities/BitSkim/BitSkim`; custom executable paths remain configurable. The installer stays in the distribution package.

## Maintainer release tool

Compile, test, package and publish the same tagged release to GitHub and GitLab
with `python3 scripts/release.py` (plan, build, check, prepare, confirmed publish and separate source push). Aminet submission is supported by
the same tool. See [docs/RELEASING.md](docs/RELEASING.md) for setup and commands.

The standalone plugin SDK source tree is in [plugin-sdk/](plugin-sdk/README.md).
