# Screen Mode plugin

The Screen Mode tile (`Plugins/ScreenMode`, ID `SCREENMODE`) shows the size of
the Workbench screen, for example `800x600x8`, and switches between saved
ScreenMode presets.

## Making presets

Open **Prefs/ScreenMode**, choose a mode, then **Project > Save As...** and
save it in the presets drawer (by default `SYS:Prefs/Presets`) under a name
such as `PAL Laced` or `RTG 1024`. Repeat for each mode you want to switch
between.

## Switching

1. Click the tile. A window lists every ScreenMode preset in the drawer with
   its size and display mode name.
2. Select one and choose **Use** (or press Return).
3. The plugin runs `ScreenMode FROM <preset> USE`, the same as opening the
   preset in ScreenMode and choosing **Use**, and Workbench reopens in the new
   mode. The setting is not saved to ENVARC:; open ScreenMode and choose
   **Save** to keep it after a reboot.

Workbench can only change its screen while no other window is open on it.
The plugin therefore asks NovaDock to close its own windows first (the ARexx
command `SUSPEND`) and to reopen them (`RESUME`) once the new Workbench screen
is up, or after 12 seconds. Close other programs' windows on Workbench
first, or Intuition asks you to.

The tile shows `SWITCHING` meanwhile, and `FAILED` or `NOT STARTED` if
ScreenMode reported an error or could not be found.

## Settings

| Setting | Meaning | Default |
|---|---|---|
| Presets drawer | Drawer searched for ScreenMode preset files | `SYS:Prefs/Presets` |
| ScreenMode program | The ScreenMode preferences editor | `SYS:Prefs/ScreenMode` |

The editor command runs in a separate worker while the tile keeps answering
the dock. A command that has not completed after 15 seconds shows COMMAND
TIMEOUT and the dock resumes. The worker is not forcibly killed: another mode
switch is refused until it exits (SWITCH STILL RUNNING). A late completion is
discarded after the waiting plugin has left; it cannot access deleted reply
ports. A failed SUSPEND aborts the operation before launching the editor.
