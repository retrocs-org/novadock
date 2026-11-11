# Plugin settings interface — NovaDock 0.1

All user configuration is in the single `novadock.cfg`, not one file per plugin.
`ENV:NovaDock/novadock.cfg` is the session copy and
`ENVARC:NovaDock/novadock.cfg` is the permanent copy.

Open a plugin tile's settings from its right-click menu. Add plugins from
right-click → Add on the dock. The Plugins page in Preferences lists installed
plugins and their details, with Import and Remove for third-party packages.
The dock can still be started independently from WBStartup.

Option metadata comes from built-in definitions or third-party `OPTIONDEF`
entries, not hidden external preference files. Supported editor fields:

| Field | Config type | UI |
|---|---|---|
| Toggle | BOOL | Yes/No cycle |
| Choice | CHOICE | Cycle of listed choices |
| Bounded integer | UINT | Validated text input |
| Text | TEXT | Text input |
| File | FILE | Editable path and a Select file button |

Third-party plugins normally describe their settings themselves with a
manifest (`NOVADOCK_OPTION` lines, see [Self-describing plugins](PLUGIN_MANIFEST.md));
Preferences then writes the `OPTIONDEF` lines below automatically. Writing
them by hand still works:

```ini
PLUGIN=Example|Plugins/Example|EXAMPLE|METER|ON
OPTIONDEF=EXAMPLE|ENABLED|Enable widget|BOOL||YES
OPTIONDEF=EXAMPLE|DISPLAY|Display mode|CHOICE|COMPACT,FULL|COMPACT
OPTIONDEF=EXAMPLE|REFRESH|Refresh ticks|UINT|25,1000|100
OPTION.EXAMPLE.ENABLED=YES
OPTION.EXAMPLE.DISPLAY=FULL
OPTION.EXAMPLE.REFRESH=100
```

A third-party executable should call `nd_plugin_options_effective()` and
provide a callback `(void *ctx, const char *key, const char *value)` returning
1 for accepted values and 0 for invalid values; see
`include/nd_option_runtime.h` and `plugin_src/common/nd_option_runtime.c`.
The helper reads only `OPTION.EXAMPLE.*` records and ignores unrelated fields.
`ND_EVENT_SETTINGS` is an optional additive event flag; existing ABI 1/2
message structures are unchanged. Plugins should reread settings when they
receive this event. Disabled plugins remain in the config on save.

The dock does not execute option strings as shell commands. Plugins must
validate external command mappings and file/device paths independently.
Third-party plugin-specific custom controls are not automatically available;
this editor supports the five schema types above. The manifest has no hook to
replace Edit settings with an external preferences executable.

The focused editor adapts to the Workbench screen height and scrolls when
its fields cannot fit. The manifest supports up to 12 option definitions.

**Use** writes the session copy and **Save** writes both session and permanent
copies, closes the editor, then asks the running dock and plugins to reload.
Cycle and string values are collected directly from the live GadTools controls,
so changes such as Clock Digital / Analog take effect without restarting the
dock. Invalid values leave the editor open with an error title; write failures
are reported separately.

The editor, save path and live reload have been compiled for 68k and exercised
on AmigaOS Workbench.

## Save and recovery behavior

NovaDock closes a complete `.new` file before replacing `novadock.cfg`, and
retains the preceding fully valid generation as `novadock.cfg.bak`. A malformed
current file is never copied over that backup. Effective loading accepts only
zero-error parses and tries `ENV:`, its `.good` and `.bak` generations,
`ENVARC:` and its generations, then packaged `PROGDIR:` settings and their
generations, and finally `SYS:NovaDock/novadock.cfg.default`. The Diagnostics page identifies a selected backup
as **Recovered config**. Saving the recovered working copy repairs the active
file through the same replacement path.

## Bundled Eyes widget

The new `EYES` plugin uses the exact same built-in schema mechanism as Clock,
RAM, Network and MusicPlayer. Right-click the Eyes tile and choose
**Edit settings...**. No extra configuration file or separate editor is
installed. See [Eyes plugin](EYES_PLUGIN.md) for the full options.

Focused settings windows choose their visible row count from the Workbench
height. Network shows seven normal options; its Advanced button exposes three sampling
override controls. Taller screens fit these together without a scrollbar;
smaller screens keep scrolling so all options remain reachable.

For a complete SDK walkthrough, see [Creating your plugin settings window](PLUGIN_SDK_SETTINGS.md).
