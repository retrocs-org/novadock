# AGENDA plugin

Add AGENDA from Add Item → Plugin. Click its tile to list tasks, then use New,
Edit, Complete or Remove. Tasks are saved independently for each instance under
`ENVARC:NovaDock/Agenda`. Up to 64 tasks are supported per instance.

Enter a task title, a date in `YYYY-MM-DD` and a time in `HH:MM`, using the Amiga
local clock. Leave the date blank for an undated task. Repeat accepts NONE,
DAILY, WEEKLY or MONTHLY, without regard to case. Dates cover 1978–2099.
Completing a recurring task schedules its next occurrence after today; monthly
recurrence retains its original day, clamping to shorter months when necessary.
Completing an ordinary task toggles its completed state.

The badge counts incomplete tasks due today or overdue. Reminder alerts use the
dock's ALERTS history and are enabled by default in tile settings. They are
checked once a minute and recorded once per occurrence before notification, so
restarting the dock does not repeat an acknowledged reminder. A failed reminder
save is retried later. AGENDA must be running for reminders to appear.

Loading and saving run in background workers. Failed saves retain the current
list and editor contents. Successful replacements keep a `.bak` copy; an
unreadable or malformed existing task file is not replaced. Keep the backup
when recovering a damaged file. Uninstalling with Keep settings retains tasks;
Remove settings deletes this default storage with the other NovaDock data.
