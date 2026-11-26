# Using NovaDock

NovaDock is an application dock for Classic AmigaOS Workbench 3.x. It keeps
applications, drawers, disks and useful plugin controls within easy reach.
It requires AmigaOS 3.0 or newer and a 68k Amiga. Individual plugins may need
other software; their help pages describe those requirements.

## Install, update or remove

Unpack the application archive, open its drawer and double-click
**Install**. The installer needs Commodore Installer version 42 or
newer. Keep the supplied StartInstaller launcher beside the installer.

- **Novice** uses the standard locations, keeps existing settings and starts
  NovaDock automatically with Workbench and after installation.
- **Intermediate** lets you select the application and Preferences drawers.
- **Expert** also lets you select the shared-data parent drawer.

When NovaDock is already installed, the installer offers **Update** or
**Uninstall**. Updates preserve your configuration, custom installation paths,
imported plugins and themes. **Reset** restores the default configuration
and keeps the previous file as `novadock.cfg.old`. Uninstall asks whether to
keep your settings and saved Notes. **Remove settings** deletes the default
Notes collection in `ENVARC:NovaDock/Notes`, including deleted-note backups.
Notes stored in a custom drawer are kept. Use **Abort Install** to cancel
the installer. An empty settings drawer may remain after removal; choosing
**Remove settings** leaves no saved configuration or default Notes inside it.

The standard locations are:

- Application: `SYS:Utilities/NovaDock`
- Preferences: `SYS:Prefs/NovaDockPrefs`
- Plugins, themes, artwork and scripts: `SYS:NovaDock/`
- Help: `HELP:NovaDock.guide`
- Active settings: `ENV:NovaDock/novadock.cfg`
- Permanent settings: `ENVARC:NovaDock/novadock.cfg`

The installer adds startup entries and the required assignments. Help,
translations and settings keep their standard locations even when you choose
custom application or shared-data drawers. Danish, German, Italian and
Swedish translations follow the language selected in Locale preferences.

## Start and stop

Double-click NovaDock to start the dock. Double-click NovaDockPrefs to open
its settings. After installation, the dock can start with Workbench.

Right-click the dock background for its menu, including Preferences and Quit.
From a Shell, `NovaDock QUIT` asks the running dock to stop. Quit and restart
after manually replacing plugin executables so their resident processes are
also replaced. The installer handles this when updating the application.

## First use

A fresh configuration uses the Nord theme and eight tiles: System, Shell,
Music, Trash, CPU, RAM, Clock and Preferences. Existing installations retain their layout.

Click a launcher to open its application or drawer. Plugin clicks perform
their own actions: the clock opens a calendar, Notes opens its collection,
SHOT takes a screenshot and MODES opens the resolution picker. CPU and RAM
clicks switch between their meter and graph views.

Labels apply to application, drawer and plugin captions. **Always** keeps every name at the bottom. **Hover** shows only the focused tile's name: at the bottom with magnification disabled, or in a caption bubble above the tile when magnification is enabled. **Hidden** suppresses names. Status values, controls, meters and badges remain visible.

Long captions use an ellipsis rather than cutting off letters. Preferences and MusicPlayer use the shorter names Prefs and Music when space is tight. Full names remain available in floating Hover captions and enabled tooltips.

Normal artwork shares a 48-pixel area while retaining its proportions. Plugin artwork ignores unused transparent padding; all animation states share one envelope so their size stays steady. Notification badges and running dots follow the same artwork anchors.

Plugin health markers appear at the top-left of the artwork: an hourglass means loading, an outlined minus means offline, a question mark means unavailable or setup is required, and an exclamation mark means an error. Tooltips explain the state, and Plugins shows corresponding health information. These markers are separate from notification counts and running-app dots. Healthy idle, empty and zero readings do not produce an error marker. A recovered plugin clears its marker on the next update.

The magnification background refreshes when nearby windows move or close, including while you hover over a tile or its tooltip. Refresh remains deferred during dragging, popup menus and dock keyboard control.

CPU and RAM meter views use processor and memory-module artwork with live readings. Digital Clock uses a clock display; analogue Clock and graph views retain their own displays. Running-app dots sit below the artwork and follow it during magnification.

**Colourful icons** gives bundled artwork recognizable colours: a purple MusicPlayer, blue Preferences, green Trash and circuit boards, an amber digital Clock, and a blue Network interface. Third-party artwork keeps its original RGB colours. Turn it off to follow the theme. Launcher artwork keeps its own colours in either mode. The preview and live dock share the same icon palettes. Appearance groups its controls under Layout, Icons and labels, Background and effects, and Behaviour on larger screens; smaller screens keep a compact form. Network shows separate incoming and outgoing packet rates inside its interface icon, and retains its existing graph view.

**Always on top**, **Auto hide** and **Hide delay** are in Appearance. Hide delay is 1–60 seconds and is enabled when Auto hide is on. With Always on top off, application windows stay above the dock. With it on, the topmost layer follows the visible shelf and magnified artwork instead of reserving an invisible rectangle above it.

The **Tooltips** switch in Appearance controls hover popups for both plugins and normal launchers independently of these captions. Popups sit close to the current artwork, including magnified widgets.

**Transparency %** accepts 0–100: zero keeps an opaque background and 100 shows only the desktop behind the tiles. RTG screens use cached colour blending; palette screens use an ordered transparency mask. The desktop snapshot refreshes after startup, when the dock window is rebuilt and when nearby windows move or close. Capture waits until a window drag ends. Icons and labels stay opaque.

**macOS Modern** provides a translucent rounded shelf; **Mac OS X Glass** adds a sloping reflective shelf. Their transparency defaults can be adjusted in Appearance. Preferences is now a theme-aware plugin; the original default launcher upgrades in place. Idle MusicPlayer artwork follows the theme too. All plugin widgets, including Eyes, support hover magnification.

Hover over a tile for its tooltip. Right-click a tile for its actions,
settings and removal. Plugin help pages explain extra controls and software
requirements.

## Add and arrange tiles

Open Preferences and use **Add item** to choose an application, drawer or
disk, installed plugin, or spacer. Enable the desired plugins through their
catalogue entries; their settings are available from the tile's right-click
menu or the Preferences plugin pages.

Unlock the dock from its background menu to arrange it. Drop an application,
drawer or disk from Workbench onto the dock to add it. An insertion line
shows whether it will be placed before or after the tile. Drag existing tiles
to rearrange them. Removing a tile does not uninstall its application.
Lock the dock again when you finish arranging it.

Preferences controls screen edge, alignment, rows, tile size, themes, labels,
auto-hide and keyboard shortcuts. The other chapters explain launchers,
groups, removable media, running applications and drawer popups.

## Small screens

At 640x256 with the standard 8-point Workbench font, Appearance, Add Item and
launcher editing use compact spacing so the buttons remain visible. At
640x480 they use the taller layout. Plugin settings show a scrollbar only
when their options exceed the available rows.

Use Previous and Next in the icon chooser to reach more icons. APPS shows
a page footer when running applications fill its grid; click either half
to change pages, or use the arrow keys. Search also moves to the matching
application's page. Larger Workbench fonts need a larger screen mode.

## Save settings

**Use** applies settings to the current session. **Save** also stores them
permanently for the next restart. Updates retain saved settings. When an
invalid configuration is found, recovery choices help you restore a working
configuration or use the built-in defaults.

## Update notifications (0.4)

Open **Preferences -> Update**, between Support and About. Both checkboxes
are off on new installations and when older configurations have no update opt-in:

- **Check for updates on startup** checks once when the network is available.
- **Check for updates while running** enables the interval selector: **2, 4 or 6 hours**.
  The default interval is 4 hours. With startup checking off, the first check
  waits for that interval rather than checking immediately.

Choose **Use** for this session or **Save** to keep the settings. Either option
works independently. Turning both off stops future checks and dismisses an
unanswered popup. Checks use GitHub's latest published stable release. They do
not bring the network online. Periodic failures are silent and retried no sooner
than an hour later; startup-only checking makes one attempt. An unanswered or
postponed version is offered at most once per day.

**Check now** runs one check without enabling either automatic option, even
when the dock itself is stopped. The button is disabled while a check or its
update prompt is open. **Last check** records the time of the latest attempt;
the status reports Checking, up to date, a newer version, offline or failure.
The record is saved across restarts. Manual checks may offer a version again
even if you postponed it earlier that day.

The checker needs HTTPS-capable `C:curl`, a correct system clock and a working
CA certificate configuration. It uses normal certificate verification and
never disables it. Neither a GitHub account nor an API token is required.

A newer version opens a popup showing the installed version, available version
and LHA filename. Choose **Download...** to select a save drawer and filename,
or **Later** to continue working. Download and verification run separately
from the dock, so its tiles remain usable. A status window reports received
bytes and allows cancellation. Existing destination files are never replaced.
Use a destination drawer whose path is at most 100 characters long.

NovaDock keeps the archive only when its size and SHA-256 match the release
metadata and checksum. Downloading does not install or run anything. Unpack
the saved archive and run **Install -> Update** when ready. Your saved
settings are retained by the installer.

Text configuration uses `CHECK_UPDATES_STARTUP=YES|NO`,
`CHECK_UPDATES_RUNNING=YES|NO` and `UPDATE_INTERVAL_HOURS=2|4|6`.
The old combined `CHECK_UPDATES=YES` remains readable and enables both checks.
Notification timestamps live in
`ENVARC:NovaDock/UpdateState`. Keeping settings during uninstall also keeps
this record; removing settings deletes it. Checks contact GitHub directly,
which receives the usual request information, such as your IP address and
NovaDock version. No local configuration or credentials are uploaded.

## Help and development

Press **Help** in Preferences or use **Help...** on its About page to open
this manual in MultiView. Start with the bundled-plugin chapters for each
tile's controls and settings.

The Plugin SDK chapters cover writing a plugin, its protocol, manifest and
settings window. The Themes chapter explains creating and importing themes.
ARexx and event-hook chapters describe automating the dock and integrating
applications.

The CPU tile tooltip shows the detected 68000-family processor and the latest
measured usage percentage, in both meter and graph views. The percentage is
a sampled estimate of activity, rather than a per-task CPU accounting figure.

## Classic icon colours

Embedded ColorIcon and NewIcon palettes are mapped to the current screen when
loading artwork. Untagged three-plane icons use the standard MagicWB palette
locally, so NovaDock does not require enabling Set MagicWB Colors or changing
the Workbench palette. Classic four-colour icons retain their screen colours.


## Warp1260 CPU controls (0.4)

On a detected Warp control board, right-click CPU and choose
**Set Turbo Level...**. The window enables writes only after confirming a
Warp1260 and `cswarp.library` 30.74. Other models and versions are refused.
The physical-board test is still pending.

The window shows the current clock when available. Choose a level and click
**Apply** to change and save the board clock. There is no default selection
and NovaDock never changes the clock automatically. **Read again** checks the
result. Higher clocks require a suitable processor and cooling.

| Level | Warp1260 clock |
| --- | --- |
| 0 | 50 MHz |
| 1 | 66.6 MHz |
| 2 | 75 MHz |
| 3 | 85 MHz |
| 4 | 90 MHz |
| 5 | 95 MHz |
| 6 | 100 MHz |
| 7 | 105 MHz |

Firmware 30.74 uses frequency settings, so these are legacy turbo-level
presets translated to MHz, rather than a call to the older CPUTurboLevel
command. Accepted settings are confirmed separately from clock readback.
A busy library call runs independently; closing its window does not interrupt
a submitted change or permit another competing call.

Network offers corresponding Warp Wi-Fi controls and a Prism network chooser.
See [Network](NETWORK_PLUGIN.md#configure-wi-fi-04) for setup and limitations.

Plugin artwork travels in a `.assets` drawer beside its program. If Preferences
marks a plugin **Needs repair**, select it to see whether its program or artwork
is missing, damaged or requires a newer NovaDock. Reinstall the complete plugin
package, then reopen Preferences.

## Plugin runtime health

Open Preferences -> Plugins, select a dock instance and click Refresh to check
its runtime status. The details distinguish a missing heartbeat from setup,
connection and dependency problems, and suggest a next step. Check Support
for unresponsive processes and AmiHASSPrefs for HASS connection errors. Empty
collections, idle tools and lights reporting off are normal states.

## Additional 0.5 tools

BENCH, QR, MOON and COLOUR are available under Add Item → Plugin. See the
[benchmark](BENCH_PLUGIN.md), [QR](QR_PLUGIN.md), [moon](MOON_PLUGIN.md) and
[colour](COLOUR_PLUGIN.md) chapters for use, settings and limits. HASS Type room
adds read-only room summaries through AmiHASS; see [HASS](HASS_PLUGIN.md).


## Sending a support report

Open Preferences > Support > Create report. Recording starts automatically for
60 seconds; use NovaDock to demonstrate the problem. Logs, plugin CPU estimates
and tracked RAM are gathered for you, and prior logging settings are restored.
Choose Continue now to keep a partial recording, or Use existing logs after a
crash you cannot repeat. Then choose the kind of problem and describe it.

Use your own words on the What happened page. The questions are optional and
an example follows the kind of problem you chose. The final page lists detected
system details. Choose Save report to pick where the report should go; cancelling
the chooser returns to review and keeps your answers.

If C:LhA or a LhA program beside SupportReport is available, the report is saved
as one .lha archive. Otherwise choose a drawer and NovaDock saves a new report
folder there. If compression fails, the uncompressed folder is retained. Send
the saved archive or all the files in the report folder to support. Nothing is
uploaded automatically.

Fresh installations use Nord, a rounded artwork background, colourful icons,
Always labels and 100% magnification with Animate on. Always on top and Auto hide
start off. Updating an existing installation retains its saved preferences.

Create report enables detailed disk tracing automatically. For an intermittent
crash before a report is started, use Advanced > Logging to enable Detailed trace
and Save logs to disk ahead of time. Disk logs survive a dock restart and a reboot; RAM logs do not
survive reboot. The final buffered events may be lost in a crash. Reports include
both available RAM logs and disk logs (named with -disk), plus their previous
rotation, so a fresh log after restart cannot hide the earlier disk record.
