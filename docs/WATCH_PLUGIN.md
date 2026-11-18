# WATCH plugin

WATCH records new or changed files in a selected drawer, with an unread badge
and a native, filterable recent-changes list. Add WATCH from Add Item → Plugin,
then use Edit settings to select a drawer. Add another instance for a different
drawer; each has its own label, settings, baseline and history.

The first successful scan establishes a baseline without reporting everything
as new. Subsequent scans compare names, sizes, file dates, types and disk keys.
Monitoring covers immediate children, including drawer entries; it does not
recursively index their contents or report deletions. It does not read file
contents or calculate hashes. Checks default to every 10 seconds; the setting
accepts 2–300 seconds.

Click the tile to see the latest 64 changes. Unread rows have an asterisk.
Use the Filter field, Open, Open drawer, Mark read, Clear history, Check now,
Pause/Resume and Close. Open drawer opens the watched drawer. Clearing the history does
not remove files or reset the baseline. Mark read clears the unseen badge;
opening the viewer does not acknowledge changes automatically. The badge
counts events since Mark read, capped at 65535, even if older rows have left
the bounded history. History resets when the plugin restarts or settings are applied.

Optional dock notifications are enabled by default and appear in ALERTS.
Notifications summarize batches of changes after the configured quiet period. Disable Send dock
notifications to retain the history and badge without notifications.

A separate low-priority worker does the filesystem work. At most one worker
runs per instance; a slow filesystem cannot accumulate extra scans. A check
taking over 15 seconds shows a busy status while the native viewer and dock
remain responsive. Missing drawers retry automatically. Recovery establishes
a fresh baseline, avoiding a flood of false new-file events.

Scans are limited to 256 entries, with paths up to 239 characters. Larger
drawers show a limit message and do not report a partial scan as complete;
choose a smaller drawer. Plugin-owned artwork includes an unavailable state.

## Name filters, pause and notifications

Include names defaults to `#?` (all names). Ignore names defaults to empty.
Patterns are case insensitive and separated with semicolons: `#?.log;#?.txt`.
Both `#?` and `*` match any sequence; `?` matches one character. This is a
bounded wildcard subset, not the full AmigaDOS pattern language. For example,
ignore `#?.info;#?.uaem;#?.tmp` to hide icon sidecars and temporary files.
Filtering affects recorded changes; the complete drawer still has a 256-entry
scan limit. Applying settings establishes a new baseline.

Pause stops new scans and discards an in-flight result. Existing history and
unread counts remain available. Resume takes a fresh baseline so activity
while paused does not cause a notification flood.

Quiet seconds for alerts defaults to 3 (range 1–30). Badges/history update
immediately, while notifications wait for that quiet period. Continuous copy
activity is summarized at least every 30 seconds. Pausing cancels a pending
summary without clearing the recorded changes.
