NovaDock theme packs
====================

Each theme lives in its own directory. theme.cfg defines its display name,
default tile shape, placement, typography, spacers and eight RGB colours. The
dock uses native colour fills; graphic backdrop images are intentionally
unsupported for now.

Supported theme.cfg keys:
  NAME, SHAPE, SIDE, POSITION, FONT, LABELS
  SPACER_WIDTH, SPACER_LINE, SPACER_LINE_COLOR, SPACER_LINE_WIDTH
  BG, TILE, HOVER, TEXT, SUBTLE, ACCENT, BAR, ACCENT2

Colours use decimal R,G,B values (0--255). NovaDock has safe built-in
fallbacks if a theme file is missing or partially edited.

The twelve bundled directories are selected from Appearance. Choosing a preset
also selects its default shape and placement. The regular themes are bottom
centred; NeXTSTEP is vertically centred on the right edge. Appearance controls
can override these defaults.

NovaDock 0.1 recognizes only these directory names:
  Mocha, Light, Workbench, Graphite, HighContrast, Copper, Ocean, Emerald,
  NeXTSTEP, BeOS, Nord and Vapor.

A third-party pack must replace one of those slots. Back up the original
directory first. A recommended package contains README.txt plus
Themes/<target-slot>/theme.cfg. Quit NovaDock, copy that directory into the
NovaDock Themes drawer, restart, select the target slot and use Test before
Save. Restore the backup to uninstall it.
