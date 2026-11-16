# SYSTEM plugin

Add **SYSTEM** from Add item > Plugins to see CPU, Workbench and uptime.
The tile rotates between them every five seconds. Edit settings can keep one
field visible instead. Click the tile to see all three together.

CPU identifies the processor reported by Exec: 68000, 68010, 68020, 68030,
68040 or 68060. Workbench displays the installed `workbench.library` version
and revision. This is the library version, rather than a guessed OS edition.
Uptime is measured from the system timer's E-clock initialization, independently
of calendar-clock adjustments. The detailed view includes seconds.

Readings refresh once per second. SYSTEM does not change hardware, scan disks
or contact the network. It requires the standard timer device; the Workbench
version is shown as unavailable if its library cannot be opened.
