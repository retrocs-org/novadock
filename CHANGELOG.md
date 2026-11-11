# Changelog

- Fixed: the SAVE_FAILED event hook ran after every completed settings save, including successful ones. It now runs only when ENV: or ENVARC: could not be written.

- Preferences' Start/Restart escapes `*` and `"` in the dock's path, so a dock installed in a drawer whose name contains them starts correctly.

- Build: every program in `Makefile` and `Makefile.cross` now lists all headers it includes, and the host tests check this (`tools/make_deps.py`), so incremental builds no longer keep stale binaries after a protocol or configuration header change. Removed the retired NovaDockLauncherPrefs and NovaDockPluginPrefs targets from `Makefile.cross`.

- The 68000 type check now reports misleading indentation that spans lines (the shape of a missing brace) while still allowing several statements on one line. Host test runs no longer leave `.good`/`.bak` configuration files in `tests/`.

- Installer launcher resolves the fixed Install-NovaDock script beside its executable, rejects directory/empty script paths and starts Installer with one command and an explicit working directory. It no longer depends on Workbench project arguments or NovaDock installation assigns after uninstall.

- Shortened installer introduction, update summary, completion screen and removal prompt to fit classic Workbench windows; detailed explanations remain in Help.

- Removed the redundant Cancel maintenance choice; use Installer's Abort Install button. Update and Uninstall remain visible in Novice, Average and Expert.

- Novice now explicitly offers Update, Uninstall or Cancel for an installed copy. Maintenance and removal questions remain visible at every level; the original level is restored before installation/removal proceeds. Cancel is the maintenance default.

- Uninstall automatically resolves recorded/default/legacy directories without path questions; shared-data detection also handles partial installs with a missing Plugins drawer.

- Installer: Average/Intermediate selects only app and Preferences drawers; Expert also selects the shared-data parent. Both ask separate boot-start and launch-after-installation questions. Novice remains automatic.

- Installer: Novice now uses default paths, enables Workbench startup and launches NovaDock automatically; Average/Expert retain the installation choices. Updates preserve recorded locations and Novice keeps existing settings.

- Release drawer icons explicitly select Show Only Icons and View By Icon rather than inheriting the parent drawer settings. The runnable LHA and source/full ZIPs include this outer drawer icon; files without .info remain available through Show All Files.

- Optional Aminet release preparation produces a stable novadock.lha/readme pair, validates metadata and checksums, and previews the submission offline. Explicit upload commands submit the checked archive/readme to Aminet incoming; default builds never upload. FTP behavior is tested with a fake transport.

- Release script regenerates manual/catalog metadata, rebuilds and validates all 25 Amiga programs, creates a classic-compatible LH5/level-1 LHA installer archive, verifies CRCs and extraction byte-for-byte, and publishes a SHA-256 checksum without replacing the previous archive on build/verification failure.

- The single manual and project icon now install directly as HELP:NovaDock.guide and HELP:NovaDock.guide.info. Help lookup prefers that path; updates migrate the old manual subdrawer and remove it only when empty, leaving unrelated Help files untouched.

- Standardized layout: only app, Preferences and shared-data parent directories are selectable. Shared data defaults to SYS:NovaDock without a drawer icon; SaveConfig lives in Tools. Documentation always uses HELP:NovaDock with drawer/project icons. ENV/ENVARC settings keep the compatible novadock.cfg format and receive drawer icons. Older default shared data and manual locations migrate during updates.

- Installer adds independent application and Preferences directory choices, default SYS:Utilities and SYS:Prefs. Each executable and icon goes directly into its selected drawer. NovaDockApp: and NovaDockPrefs: persist these locations for launching and maintenance; only shared resources receive a NovaDock subdrawer.

- Installer maintenance detects the installed executable, displays installed locations, and offers only Update or Uninstall. Updates reuse existing shared/manual directories; legacy Utilities drawer installs migrate to the split layout. Uninstall safely handles a missing shared drawer.

- Split installed layout: the dock and icon live directly in SYS:Utilities, Preferences in SYS:Prefs. Installer-selectable shared components default to SYS:Resources/NovaDock; the separately selectable manual defaults to HELP:english. Persistent NovaDock: and NovaDockHelp: assignments resolve custom locations. Legacy Utilities drawers are backed up, imported components preserved, and old absolute bundled-plugin paths migrate on load.

- Standard launch paths now use SYS:Utilities/NovaDock, SYS:Prefs/NovaDockPrefs and SYS:Utilities/BitSkim/BitSkim. The former shipped BitSkim Bin: path migrates on config load; custom paths stay intact. Installer upgrades remove obsolete local Preferences and installer tools from a separate installed drawer.

- Clipboard History is now named Clippy, with the shorter CLIPPY dock label. Its executable, plugin ID, settings and session-history storage remain compatible.

- Removed selected-image hover effects and zoom windows, including the General preference control. Ordinary tooltips remain; legacy HOVER entries load harmlessly and disappear on the next save.
- Application launchers and launcher-style plugins now share an aspect-preserving 48x40 artwork limit and a common vertical center above the label. Wide DOpus artwork no longer fills most of its tile or sits above neighboring icons.

- NovaDock package/installed drawer and installer use the supplied generic Amiga icons, including their selected images. Drawer icons contain valid DrawerData; installer icons retain StartInstaller and NovaDock tool types. Source and runnable archives include the outer drawer .info.

- Script tiles keep watchdog-safe heartbeats independently of their refresh interval.
- Mixer stages complete preference files before replacement, preserves .bak recovery copies, checks close/write errors, and reports permanent-save failures while keeping them pending for retry.
- BitSkim recovers outstanding requests when its registered host disappears or changes owner, safely retaining late-reply targets with a bounded retired-request pool. Modal requester waits are not retried.
- Screen Mode runs the editor in a separate guarded worker. The plugin keeps answering dock IPC, resumes the dock on a 15-second command deadline, and prevents overlapping switches until the worker exits.

- Testing: `SANITIZE=1 sh tests/run_host_tests.sh` runs the host tests under AddressSanitizer and UndefinedBehaviorSanitizer; `scripts/fuzz.sh` runs libFuzzer targets for every parser that reads files, ARexx, plugin output or network text.
- The quit-while-unsaved rule is now a host-tested policy function (`nd_save_quit_step`).
- Clipboard History: a malformed clip no longer leaves part of its text in the decode buffer (found by fuzzing; the plugin itself only used successful decodes).

- Script tiles: a new tile type (Add item > Application > Script tile, `Plugins/Script`) shows what an ARexx script returns: value, caption, meter and badge. Scripts run in RexxMast's process, are called with UPDATE or CLICK and the tile ID, and have NOVADOCK as their default host. Example: `Rexx/ScriptTile.rexx`.
- Event hooks: `HOOK=EVENT|command` lines run a command on STARTUP, QUIT, DISK_IN, DISK_OUT, NET_ONLINE, NET_OFFLINE, PLUGIN_STALE and SAVE_FAILED, with the event and a quoted detail appended.
- Room for 128 plugin settings (was 96), for Script and Command tiles.

- New plugin: Mixer. Shows the AHI output volume; a click opens a slider with Mute. Changes go to every unit in ENV:Sys/ahi.prefs, are applied at once where the driver has a volume control and the card is free, and are copied to ENVARC: when the window closes.
- New plugin: Screenshot. Click to save the front screen or Workbench as a ByteRun1 IFF ILBM (palette and display mode included, up to 256 colours) after a countdown; the badge counts the shots.
- New plugin: Screen Mode. Shows the Workbench size and switches between ScreenMode presets saved in a drawer, closing the dock's windows during the Workbench reset.
- ARexx: SUSPEND and RESUME close and reopen the dock's windows (with a 30-second safety limit), so Workbench can change its screen mode.

- The dock window opens at once: launcher artwork now loads a few tiles at a time after start-up instead of before the window appears.
- Drags, removals and other dock gestures use about 116 KB less memory at peak: the edited configuration is handed over instead of copied, and SaveConfig reads the snapshot in place instead of receiving its own copy (an older SaveConfig still gets a copy).
- While another screen completely covers the dock (same monitor, not dragged down), tile repaints wait and plugins back off as they do while the dock is hidden.
- ARexx: `LIST STEM name` fills an ARexx stem with every tile's label, kind, ID and value. Plain `LIST` is no longer cut short on a full dock.
- Plugin ABI 5: a second 32-bit event word, advertised through the dock port's priority so new plugins still work with older docks. Plugins built with the current client publish a wake port, and the dock never signals a task pointer that may be stale.
- Quitting while the last settings save finishes keeps the dock painted and shows "Saving settings...".
- Network Details and the BitSkim helper can no longer open twice from two quick clicks. ARexx `ADD` with a path on an absent volume no longer raises a requester.
- Messages with counts use separate singular and plural catalog strings (Swedish and German updated).
- Removed unused code (`add_command_tile`).

- Fixed: the dock could no longer be quit while settings could not be saved (read-only ENVARC:, SaveConfig missing), which also made Install-NovaDock abort. The first quit still stops and explains; a second quit now leaves without the unsaved change.
- Fixed: the native Makefile, build.amiga and the published source ZIP built NovaDock from compat/novadock_vbcc.c, which is not part of the source. They now compile src/novadock.c, dock_save.c, save_policy.c and indicator.c directly; Makefile.cross uses compat/ only when present.
- Media controls pressed on a repeated plugin tile now reach the plugin in the same reply instead of the next poll.
- Launcher working directories, the Preferences companion and the SaveConfig helper are probed with requesters quietened, so an absent volume can no longer stop the dock behind a "Please insert volume" requester.
- Removed stale vbcc output (src/dock_save.asm, src/save_policy.asm) that hard-coded NovaDock structure offsets.

- Apps clicks directly launch a single-instance popup helper independently of resident status polling. Console windows without an application message port are included individually, and selection revalidates the exact window before raising it. Ignore stale inactive notifications on an active popup.

- BitSkim never switches to graph mode, including Shift-click; saved graph settings are treated as progress-meter mode. Clicking opens or restores BitSkim.

- Network traffic tiles now align their heading with CPU/RAM and show only IN/OUT meters or graphs; hover for packet rates and status. Active BitSkim tiles align with CPU/RAM and show direction/percentage, with speed in the hover tooltip.

- BitSkim shows WAIT BITSKIM while a modal accept/save requester delays status replies, without reporting a transfer failure or showing a stale completion. A valid status clears a previous STATUS FAILED notice immediately.

- Simplified the BitSkim context menu to Open, Send file, Toggle receiving, Cancel transfer, Edit settings and Remove. Detailed information, device refresh and application preferences are accessed in BitSkim itself.

- Fixed BitSkim STATUS FAILED on the tested rexxsyslib 47.2 setup: supply a request-owned REXX message marker when CreateRexxMsg leaves it unset, so BitSkim recognizes commands and returns results. Native queries now pass.

- Enhanced BitSkim with protocol-2 capability negotiation, transfer speed, session-aware retained-result alerts, transfer details, and a stable-ID recipient picker through Send file or a single-file drop. Older clients retain basic controls.

- Added optional BitSkim integration: launch/restore, nearby-computer badges, incoming-offer alerts, SEND/RECV progress meters, and context-menu refresh, receiving, preferences and cancel controls using its local ARexx host. Includes BitSkim artwork and license.

- Network Online reloads the saved interface in its new worker instead of trusting resident discovery caches, and passes the exact selected DEVS or Storage profile path to Roadshow. Missing profiles fail without falling back to another interface.

- Network interface chooser hides profiles whose drivers are not installed, and labels inactive Storage profiles as Template to avoid confusion with storage adapters.

- Network interface selection now uses a dedicated sorted profile list from DEVS and Storage, showing each driver beside its profile. Raw driver files cannot be selected as connection profiles.
- Network settings now use one main Network interface selector with automatic driver/unit discovery. Sampling overrides are hidden under Advanced and require explicit opt-in; stale old overrides are cleared in automatic mode.
- Offline Network tiles ignore left-clicks. Online clicks toggle and save meter/graph mode, synchronized with the new Default view preference (Meter initially), without reconfiguring the sampler.
- Network control is quiet by default using Roadshow's hidden NetLogViewer; a Debug log setting shows retained messages on demand. Actual control errors remain visible.
- Network settings now offer native Select buttons for installed Roadshow interface profiles and SANA-II drivers, preserving AUTO and editable fields.
- Network Offline takes the selected Roadshow interface offline before full-stack shutdown. This avoids the reproduced bridged A2065 shutdown hang; a failed preparatory step aborts shutdown and shows an error.
- Network now defaults to separate IN and OUT meters with a shared PPS scale; graph view remains available by clicking the online tile.

## NovaDock 0.1 — Upcoming first public release

- Network replaces the standalone Roadshow plugin. Legacy tiles migrate to Network and update installers remove the retired program.

### Release hardening

- Reliability fixes: protect plugin destinations, check copy close failures and report failed rollback recovery. Recover packaged settings consistently, reclaim late IPC replies in updated hosts, fix pre-1978 calendar weekdays and validate Script meters strictly.

- Added plugin import previews, settings recovery and an atomic low-resource preset in Diagnostics, Apps title search, drawer substring search, and clipboard search/pins. Batch plugin settings reads, defer drawer icons, skip hidden launcher scans, and remove unused taskbar state.

- Native build fixes: use the standard proto/wb.h header and link amiga.lib where its helper functions are required.


### Added

- Idle-batched background settings writer, with retry, shutdown flushing and
  Preferences write coordination. Diagnostics separates update paths from
  saving and reports pending writes, coalesced edits and failures.
- Application/connection dots no longer represent plugin-process liveness;
  DOpus and Roadshow publish their monitored state explicitly.

- Optional AmIRC tile: client/connection status, nickname, current channel or
  server, away indication, and click to launch or restore AmIRC via local ARexx.

- Live item/layout updates retain the dock window, rendered artwork and plugin
  task state when the theme, font, refresh mode and fitted tile size allow it.
  Add, move and remove operations apply their in-memory configuration directly.

- A native dock for Classic AmigaOS Workbench 3.x.
- Application, command, drawer, and volume launchers with drag-and-drop support.
- Configurable themes, normal and tiny sizes, auto-hide, positioning, and multi-row layouts.
- Application groups, per-layout behavior, and improved dock-capacity handling.
- Running-application indicators, tooltips, drawer popups, and removable-media tiles.
- CPU, memory, clock, music, archive, Directory Opus 4, running-app, and utility plugins.
- A unified Preferences application with Plugin Manager, organization, diagnostics, and About pages.
- Atomic configuration recovery and adaptive performance pacing.
- A public plugin SDK, manifest format, and example plugin.
- English catalog sources and a lightweight public website.
- A graphical installer (Install-NovaDock) that installs, updates and removes NovaDock, and a ready-to-run LHA release with an Aminet readme.
- Plugin Manager details (version, author, capabilities, dock status) with Import and Remove for third-party plugin packages.
- Imported themes can be removed again; layout profiles keep imported themes; any theme can use the Workbench bevel.
- `NovaDock QUIT` stops a running dock from the Shell.
- An AmigaGuide manual (NovaDock.guide): press Help in any Preferences window or use Help... on the About page.
- German translation of the dock's messages, menus and calendar; translations are now compiled and installed to LOCALE:Catalogs.
- The installer lets you choose where NovaDock is installed; outside SYS:Utilities it adds the assign NovaDock:.
- Plugins can show a badge on their tile, such as an unread count (ABI 4, `nd_client_set_badge()`); older docks simply do not show it. Archive Extractor shows running extractions, Running Apps the number of applications, and command tiles `!` when the command fails.
- Trash plugin: the Workbench Trashcan on the dock. Drop files and drawers to move them to their volume's Trashcan, click to open it, empty it from the menu after a confirmation; a badge shows the number of items.
- WHDLoad Games plugin: lists the WHDLoad games in a drawer, with type-ahead, and starts them as Workbench does, tool types included.
- ARexx `SETBADGE id [text]` puts a badge on any plugin tile, so scripts can use badges too.
- Keyboard control: a Dock hot key (General page) highlights a tile; cursor keys move, Return or Space acts, M opens the tile's menu, which now also works from the keyboard.
- Memory diagnostics: free and largest memory, and reload memory and icon reuse.

### Improved

- Fresh installs use Nord with System, Shell, Music, CPU, RAM and Clock only; optional plugins remain available from Add.
- Plugins Preferences uses the dock menus for settings and adding tiles; its list arrows now receive the GadTools input events they require.
- Launcher editors offer Cancel and restore their staged edits.
- Trash omits the online status dot while keeping its item-count badge.
- Plugin reply waits allow 15 seconds for slow reloads, and the watchdog gives live plugins a fresh heartbeat window after a blocking reload.

- Configuration copies use about 37% less memory (shared text pool); reloads hold one working copy instead of three and keep unchanged icons.
- Reloads answer at once and report "still applying" instead of a false timeout.
- Lower idle load on 68000 machines: running-program indicators take one pass over the system's process list per second instead of one per launcher (with interrupts off for one short pass instead of many), and skip launchers on hidden pages; the clock wakes 6 instead of 60 times a minute when seconds are hidden; graph tiles scroll in each new sample instead of redrawing every line.

### Fixed

- Stack overflows after Restart from Preferences and during plugin discovery.
- Swedish plurals ("arkivs", "objekts") and FlexCat compilation of the catalogs.
- Opening Appearance asked to insert volume Bin on systems without a Bin: assign.
- Tiles after the 32nd were never redrawn when their plugin updated.
- ArchiveExtractor crashed on AmigaOS 3.0-3.9; requesters for missing Bin: or PROGDIR: volumes; a signal leak after reload timeouts.

- Added AmIRC event-hook unread badges, with source filtering and acknowledgement on focus or successful SHOW.
- Added optional Clipboard History: bounded session-only FTXT history, deduplication, restore/delete/clear/pause popup and count badges.

- Clipboard: treat empty and superseded reads quietly, clear device errors per request, and finish reads with the documented null-buffer drain.
- Clipboard now uses an Apps-style icon and bottom title, a centered themed preview picker, and a separate Manage history context-menu entry.

- Unified Network and Roadshow: asynchronous Online/Offline controls, offline launcher artwork, online traffic meters/graphs, automatic interface device metadata, and legacy tile migration.

- Network: added a read-only Details context-menu window for interface/IP configuration, routes, gateway and DNS, with Refresh and offline handling.

- Network settings fit available screen height; show only the applicable Online/Offline action; failed control actions open a requester; unsupported Roadshow status queries are explained once.
- Apps popup stays open on repeated clicks and the empty state has enough width.

- Network AUTO prefers an active non-loopback Roadshow interface over unrelated interface files; missing device metadata is explained without guessing.

- Network shutdown: bound the foreground wait for claimed workers, explain pending shutdown, pause status/Details queries, and prevent overlapping control jobs. Automatically read the active interface's reported SANA-II device/unit.

- AUTO can identify an installed A2065 board before the stack starts and use its DEVS/Storage profile; accept Roadshow's Device unit number label.
