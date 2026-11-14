# Using NovaDock

NovaDock is an application dock for Classic AmigaOS Workbench 3.x. It keeps
applications, drawers, disks and useful plugin controls within easy reach.
It requires AmigaOS 3.0 or newer and a 68k Amiga. Individual plugins may need
other software; their help pages describe those requirements.

## Install, update or remove

Unpack the application archive, open its drawer and double-click
**Install-NovaDock**. The installer needs Commodore Installer version 42 or
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
custom application or shared-data drawers. German and Swedish translations
follow the language selected in Locale preferences.

## Start and stop

Double-click NovaDock to start the dock. Double-click NovaDockPrefs to open
its settings. After installation, the dock can start with Workbench.

Right-click the dock background for its menu, including Preferences and Quit.
From a Shell, `NovaDock QUIT` asks the running dock to stop. Quit and restart
after manually replacing plugin executables so their resident processes are
also replaced. The installer handles this when updating the application.

## First use

A fresh configuration uses the Nord theme and six tiles: System, Shell,
Music, CPU, RAM and Clock. Existing installations retain their layout.

Click a launcher to open its application or drawer. Plugin clicks perform
their own actions: the clock opens a calendar, Notes opens its collection,
SHOT takes a screenshot and MODES opens the resolution picker. CPU and RAM
clicks switch between their meter and graph views.

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

## Save settings

**Use** applies settings to the current session. **Save** also stores them
permanently for the next restart. Updates retain saved settings. When an
invalid configuration is found, recovery choices help you restore a working
configuration or use the built-in defaults.

## Help and development

Press **Help** in Preferences or use **Help...** on its About page to open
this manual in MultiView. Start with the bundled-plugin chapters for each
tile's controls and settings.

The Plugin SDK chapters cover writing a plugin, its protocol, manifest and
settings window. The Themes chapter explains creating and importing themes.
ARexx and event-hook chapters describe automating the dock and integrating
applications.
