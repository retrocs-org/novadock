# Running applications

NovaDock does not automatically add tiles for applications that are not part
of the configured dock. Starting MultiView, a Shell tool or another program
therefore never changes the dock's size, ordering or row layout.

The **Apps** setting in **Preferences > Appearance** controls only configured
launcher indicators:

| Apps | Effect |
|---|---|
| Off | Do not inspect configured launchers for running applications. |
| Mark | Show a running mark and let a launcher click bring its application to front. |

Older configurations may contain `RUNNING_SECTION=YES` from the retired
automatic taskbar mode. NovaDock accepts the line for compatibility, treats it
as `NO`, and writes `NO` on the next save.

## Optional Running Apps plugin

Add **Running Apps** through the dock's **Add item... > Plugins** page when an
application browser is useful:

- The plugin occupies one stable, explicitly configured dock item.
- Clicking it directly opens a native popup containing the currently open
  applications, sorted by window title.
- Clicking a popup entry closes the popup and brings that application's
  window and screen to the front.
- Workbench drawer windows, NovaDock and Preferences are omitted.
- Multiple windows owned by one application task are represented by one entry.
- Console windows without an application message port (including Shells) are
  listed individually and selecting one raises that exact window.
- Closing the popup discards the list. The next click performs a fresh scan.

The popup list is collected when opened. A low-rate window count updates the
optional badge between openings. Popup opening runs in a separate helper,
independent of the resident badge/status poll; repeated clicks raise the one
existing popup. A stale inactive notification does not close an active popup.

## Native testing checklist

1. Start an application that is not configured in the dock. Confirm that no
   new main-dock tile appears.
2. Add and enable the Running Apps plugin, then click its tile.
3. Confirm the new application's window title appears in the popup.
4. Click the entry and confirm its screen and window move to the front.
5. Close the application, reopen the popup and confirm the entry is gone.

Repeated clicks on Apps keep an already-open popup visible rather than closing
it again. The empty state has enough width to display its full message. Apps
lists running applications with eligible open windows, not installed programs.

Type in the Apps popup to jump to a case-insensitive title match; Backspace
shortens the search. Arrow keys and Return retain their existing navigation
behavior. Search does not change the order of the running-window snapshot.
