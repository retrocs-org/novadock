# Screen Mode plugin

The **MODES** tile (`Plugins/ScreenMode`, ID `SCREENMODE`) opens a direct
resolution picker. It is separate from **SCREENS**, which switches between
already open public screens.

## Choose a resolution

1. Click MODES to list available Workbench-compatible display modes.
2. Select a resolution and colour depth, then click **Use** or press Return.
3. The picker closes and Workbench changes mode. **Cancel**, Escape or the
   close gadget dismisses the list without changing anything.

Each row shows width, height, bit depth and the display driver's mode name.
Unavailable modes are excluded. The active mode is highlighted and scrolled
into view when the picker opens. Its **CURRENT** marker remains visible when
you select a different row. Native modes
retain the current depth where supported. Fixed-depth display modes use the
depth reported by their driver. The list holds up to 128 modes, sorted by
resolution and depth; reopen it to refresh the available modes.

The mode list is allocated and scanned only while the picker opens, then
released when it closes. No extra idle scan or large resident list is kept.

## Applying a mode

The plugin writes a temporary ScreenMode preference file and applies it
through the system ScreenMode command, without opening its Preferences GUI.
NovaDock closes its own visitor windows during the change and reopens them
afterward. Other applications' Workbench windows must also be closed if the
system asks you to close them.

**Use** changes the current session only. To keep a mode after reboot, save
it through the system ScreenMode preferences. Selecting the exact current
mode closes the picker without reopening Workbench.

The temporary preference file belongs to the worker until the command exits,
including when the resident times out or quits. Failed or abandoned choices
cannot delete another operation's files. Only one mode switch runs at a time.

## Saved presets

Set **Click opens** to **PRESETS** in the plugin settings to list saved
ScreenMode preference files instead. Create these with the system ScreenMode
editor's **Project > Save As...** command. Select a preset and choose **Use**.
Set **Click opens** back to **MODES** for the direct resolution list.

## Settings

| Setting | Meaning | Default |
|---|---|---|
| Click opens | Available display modes or saved presets | `MODES` |
| Presets drawer | Drawer containing saved ScreenMode files | `SYS:Prefs/Presets` |
| ScreenMode program | System command used to apply the selection | `SYS:Prefs/ScreenMode` |

Hover over the monitor icon to see the current dimensions or error details.
A command that has not completed after 15 seconds reports a timeout and
NovaDock resumes. The worker is allowed to finish safely; another switch
waits until it exits.
