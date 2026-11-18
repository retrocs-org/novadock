# LOGS plugin

LOGS follows a selected text log file, marks warnings and errors and counts
unseen matches. Add LOGS from Add Item → Plugin, then use Edit settings to
choose the file. Independent instances can follow separate files.

Checks default to every 10 seconds and accept 2–300 seconds. The first check
reads at most the last 8 KiB, discarding an incomplete leading line. Later
checks read at most 8 KiB from the saved offset. This limits I/O per pass;
a fast-growing log can take several checks to catch up.

Click for the native viewer. Use Filter to search the retained rows, Copy line
to copy the selected displayed entry to the Amiga clipboard, Open file,
Mark read, Clear history, Check now, Pause view, severity filter, Export and Close. Copying includes the displayed
warning/error marker. Nothing writes to or truncates the monitored file.

The latest 64 completed, non-empty lines are retained in memory. Rows are
limited to 159 displayed characters and long lines are clipped. Split lines
are carried across checks; an unterminated final line appears after its
newline arrives. ANSI/Amiga colour sequences and other control characters
are stripped. Case-insensitive error, fatal and panic matches get an [ERROR]
marker; warn matches get [WARN]. This is text matching, not a structured-log
parser. Asterisks identify unread matching rows.

The badge counts warning/error lines since Mark read, capped at 65535,
including matches that have left the bounded history. Clear history clears
the badge too. Optional notifications, enabled by default, summarize matching
lines in ALERTS. History and offsets reset when the plugin restarts or settings are applied.

A low-priority worker owns file reads, opening and clipboard operations.
At most one runs per instance. The viewer and dock remain responsive while
an unavailable filesystem or clipboard owner is slow. A busy check reports
its status after 15 seconds; no duplicate workers pile up.

Missing files retry automatically without losing the current offset. File
size, disk key and a short prefix detect truncation and common rotation or
replacement. Those events discard any partial line, add a history marker and
start at the new file's bounded tail. Rewrites preserving the same key,
prefix and size can be indistinguishable from the previous file. AmigaDOS
signed offsets limit supported files to less than 2 GiB. Plugin-owned artwork
shows when no file is configured or a check fails.

## Live view and export

The viewer follows its newest matching row automatically. Select a row or
press Pause view to freeze the displayed snapshot while capture, unread counts
and notifications continue. Resume view shows the current retained history;
older lines may have left its 64-row bound while the view was paused.

The severity button cycles All lines → Warnings/errors → Errors only. The
text Filter works within that selection. Export writes the selected displayed
line, or all displayed lines if none is selected, to a new text file. It refuses
to replace an existing destination. Copy and Export use the frozen snapshot
when the view is paused, keeping selection stable as the log grows.

Notifications are batched using Quiet seconds for alerts (default 3, range
1–30), with a 30-second maximum wait during continuous activity. The in-memory
history is a ring buffer, avoiding repeated full-history moves on busy logs.
