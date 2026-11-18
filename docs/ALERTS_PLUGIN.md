# ALERTS plugin

Add **ALERTS** in Preferences > Organize > Add > Plugin. Its corner badge
counts unread notifications. Click to open the native history window.

The dock retains the newest 64 notifications during its current session.
The window shows newest first, with source and elapsed session time. Previous
and Next browse older entries. **Mark read** acknowledges notifications from
the last snapshot; arrivals after that snapshot stay unread. **Clear** removes
the history. Closing the window does not mark notifications as read.

Timer expiry, ARCHIVE results, DOWNLOAD and Aminet transfers, low disk space,
new NovaDock releases and AmiHASS connection loss/recovery feed the history.
AmiHASS alerts begin only after a connection has succeeded. Routine state
polling does not generate notifications. Identical consecutive notices within
30 seconds are coalesced. Notification text is bounded and strips controls.

**Edit settings > System beep for new alerts** is off by default. This uses
the system beep and may be visual depending on your system settings. No sound
files, network access or persistent log are required. Restarting the dock
clears the history. Features that already show their own reminders keep them.

Plugin developers can call `nd_client_notify(client, title, details, severity)`
with severity 0 (information), 1 (warning) or 2 (error). OR the returned event
bits into your normal update result, so pending actions are retained. The
client sends notices only after the host advertises support. Older docks
continue working without notifications. Details currently fit the 31-character
IPC text field; never include passwords or tokens in a notice.
