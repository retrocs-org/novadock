# Self-describing plugins

A NovaDock plugin can describe itself, so users install it by copying files
instead of editing `novadock.cfg`. When NovaDockPrefs starts, it reads the
descriptions in the Plugins drawer and offers every new plugin in
**Add item... > Plugins**, together with a ready-made settings page.

## Writing a manifest

Put `NOVADOCK_` lines either in a text file named after the program with the
suffix `.ndplugin`, or as tool types in the program's own icon:

```text
Plugins/Weather            the plugin executable
Plugins/Weather.ndplugin   text manifest  (or)
Plugins/Weather.info       icon with the same lines as tool types
```

```text
NOVADOCK_ID=WEATHER
NOVADOCK_NAME=Weather
NOVADOCK_VIEW=GRAPH
NOVADOCK_ABI=2
NOVADOCK_MULTIPLE=YES
NOVADOCK_OPTION=CITY|City|TEXT||Stockholm
NOVADOCK_OPTION=UNITS|Units|CHOICE|C,F|C
NOVADOCK_OPTION=POLL_TICKS|Refresh ticks|UINT|250,30000|3000
```

| Key | Meaning |
|---|---|
| `NOVADOCK_ID` | Required. Unique ID (letters, digits, `_`, `-`, max 19). The dock passes it in `argv[1]`, and settings are stored as `OPTION.<ID>.<key>`. |
| `NOVADOCK_NAME` | Tile label; defaults to the program's file name. |
| `NOVADOCK_VIEW` | `METER` (default) or `GRAPH`, the initial view. |
| `NOVADOCK_ABI` | Lowest dock IPC ABI the plugin needs: 1 (basic), 2 (graphs), 3 (media controls) or 4 (badges). A newer requirement is reported, not installed. Badges degrade gracefully, so a plugin that only adds a badge can keep a lower ABI. |
| `NOVADOCK_MULTIPLE` | `YES` allows several dock tiles for this plugin; the default is `NO`. Repeated tiles share one plugin process, live state and settings namespace. |
| `NOVADOCK_VERSION` | Plugin release, shown in the Plugin Manager list and details (max 15 characters). |
| `NOVADOCK_AUTHOR` | Author or publisher, shown by the Plugin Manager. |
| `NOVADOCK_DESCRIPTION` | One line describing what the tile does; the Plugin Manager wraps it over two lines of about 46 characters. |
| `NOVADOCK_LICENSE` | Licence name or identifier, shown after the version. |
| `NOVADOCK_WEBSITE` | Optional project page. |
| `NOVADOCK_FEATURES` | Comma-separated capabilities, from: `meter` (shows a value or bar), `graph` (history graph), `click` (reacts to clicks), `popup` (opens a popup or window), `drop` (accepts dropped Workbench icons), `media` (player transport controls), `badge` (a label on the tile's corner). Unknown words are displayed as written. Do not list settings or several tiles: the Plugin Manager reports those from `NOVADOCK_OPTION` and `NOVADOCK_MULTIPLE`. |
| `NOVADOCK_PROGRAM` | Installed executable file name, when it differs from the manifest's name. |
| `NOVADOCK_OPTION` | `KEY|Label|TYPE|choices|default`, up to twelve. TYPE is `BOOL`, `CHOICE` (comma-separated choices), `UINT` (`min,max`), `TEXT`, or `FILE` (text plus an ASL file chooser). The default must be valid for its type. |

Lines that do not start with `NOVADOCK_` are ignored (comments, `DONOTWAIT`
and other tool types). Unknown `NOVADOCK_` keys are ignored so newer
manifests still work with this version. A text manifest beside the program
takes precedence over tool types in its icon.

## What Preferences does with it

- Drawers scanned: `NovaDock:Plugins` and, when Preferences runs
  from a development directory, `PROGDIR:Plugins`. A manifest whose program is
  missing is ignored.
- A **new** plugin is added to the working configuration **disabled**, as
  `PLUGIN=<Name>|Plugins/<file>|<ID>|<VIEW>|OFF|<MULTIPLE>`, with its option schema. The
  General page title says how many were found. Nothing is written until the
  user adds the plugin and chooses Use or
  Save. A new plugin never starts by itself.
- A plugin that is **already configured** gets its option schema refreshed
  from the manifest, so an upgraded plugin can add, rename or drop settings.
  Values already stored by the user are kept.
- IDs of bundled plugins (CLOCK, CPU, RAM, NET, ROADSHOW, MUSIC, EYES, DOPUS4,
  ARCHIVE, RUNNINGAPPS) cannot be claimed by a manifest.
- A plugin is rejected, with the reason shown in the Organize title, if its
  manifest is invalid, needs a newer ABI, would exceed the 32 third-party
  setting definitions, or the dock already has 24 items.

## Reading the settings in the plugin

Use the option reader from the SDK. Defaults in code should match the
manifest's defaults:

```c
char city[32];
nd_plugin_option_string(argv[1], "CITY", "Stockholm", city, sizeof(city));
```

Reread settings when an exchange returns `ND_EVENT_SETTINGS`. The SDK's
`examples/hello.c` and `examples/Hello.ndplugin` show the complete pattern.

`OPTIONDEF=` lines written by hand still work; a manifest simply generates
them.

## Plugin Manager

Preferences > Plugins lists every installed plugin with its version and dock
status. Selecting one shows its version and licence, author, capabilities,
dock status, program and description. Bundled plugins take this information
from NovaDock itself; third-party plugins from their manifest, so complete
`NOVADOCK_VERSION`, `NOVADOCK_AUTHOR`, `NOVADOCK_DESCRIPTION` and
`NOVADOCK_FEATURES` lines make a plugin look as finished as the bundled ones.

**Import...** installs a package from its `.ndplugin` manifest and
**Remove...** uninstalls a third-party plugin without tiles; see
PLUGIN_SDK_QUICKSTART.md for the package rules.

A plugin is active exactly when it has a tile in the dock. Right-click the
dock and choose Add to place a tile; use a tile's right-click menu for its
settings or to remove it. The Plugins page shows installed plugin information
and provides Import and Remove for third-party packages.
