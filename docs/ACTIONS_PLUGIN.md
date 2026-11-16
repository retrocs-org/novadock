# ACTIONS plugin

Add **ACTIONS** from **Add item > Plugins**. Each tile has its own action,
label, icon and confirmation setting. Add more tiles for different actions.

Right-click a tile and choose **Edit settings**:

- **Tile label**: the name to show on the dock, up to 31 characters.
- **System action**: select a predefined action from the list below.
- **Confirm before running**: enabled by default. Clicking opens a window
  describing the selected action, with **Run action** and **Cancel**.
- **Tile icon**: choose a bundled icon from the gallery, with a live preview.
  New tiles use Utilities.
- **Custom icon or picture**: an existing custom file overrides the gallery
  choice. Clear the field to return to the selected bundled icon.

Choose **Use** for the current session or **Save** for permanent settings.
Changing settings never runs an action. Actions run only after a tile click
and, when enabled, explicit confirmation. A failed confirmation window does
not run the action. Closing it or pressing Esc cancels.

## Available actions

- **Restart Amiga**: reboot the whole machine. Save your work first.
- **Restart Workbench**: close and reopen the Workbench screen. Applications
  remain running. NovaDock temporarily closes its own dock window and then
  restores it. Other windows or applications holding the screen can prevent
  reopening; close those windows and retry. This does not terminate programs
  or force a busy screen to close.
- **Restart NovaDock**: use the normal dock shutdown and relaunch sequence.
- **Open Shell**: open a new AmigaDOS Shell.
- **Workbench front**: bring Workbench in front of other screens.
- **Workbench back**: send Workbench behind other screens.

The Workbench and NovaDock restart actions need NovaDock's ARexx host and
rexxsyslib.library. The plugin reports a failed action in its tooltip.
No shell command is accepted from configuration: only the predefined actions
can run. Multiple tiles use unique IDs, such as ACTIONS_2, in novadock.cfg.
Removing a tile removes only that instance's settings; the default template
remains available for adding a fresh ACTIONS tile.
