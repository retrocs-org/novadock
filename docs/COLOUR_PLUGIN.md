# COLOUR plugin

Add COLOUR from Add Item → Plugin and click its tile. Pick (3s) starts a
three-second countdown: move the pointer over the desired Workbench pixel.
No click is needed. The window shows `#RRGGBB` and decimal RGB values; you can
also type a six-digit hex colour and press Return. A live swatch previews the
colour using the nearest available screen pen; the RGB/hex values remain exact.
Cancel pick stops the countdown. Copy hex writes to the Amiga
clipboard. Change COPY_FORMAT to RGB in tile settings for decimal copying.

Sampling supports indexed Workbench screens, including Extra Halfbrite, and
compatible CyberGraphX RTG screens. Workbench must be the front screen; HAM and
unsupported RTG sampling report a failure rather than an invented colour.
Sampling and clipboard operations run in bounded background workers.

Enter a palette name and choose Save colour. Up to 32 named colours are stored
in `ENVARC:NovaDock/Colours/Palette`. Saving the same name replaces its value.
Palette opens a list with Use and Remove controls; removing a colour affects
only this saved palette. Successful replacements retain a `.bak` copy and
validate the new file before replacing the original. A malformed existing
palette is preserved and reported as an error. Colour.assets belongs to the
plugin and is installed beside it.

Palette rows are built only when the list opens or its contents change. Sampling
results update the existing window and preserve uncommitted text edits.

Write-protected or full destinations report an error without discarding the typed
colour/name or replacing the saved palette. Choose Save colour again after fixing
the destination. Failed saves leave no partial palette; if rollback itself fails,
the error identifies the retained original in `.bak`.
