# Small-screen layout review, NovaDock 0.4

Target modes: 640x256 and 640x480, standard 8x8 Workbench font and
11-pixel screen title bar. Larger system fonts require a larger mode.

## Changes

- Add Item is 620 pixels wide, with a 230-pixel compact layout.
- Appearance uses a 239-pixel compact layout and shorter live preview.
- Launcher editors use 238-pixel compact layouts; sidebar spacing is retained.
- About has enough bottom clearance for its seventh navigation button.
- Short Preferences text rows no longer overlap the following row.
- Shared detail/control panels reserve space for the longest field caption.
- Prism Wi-Fi uses compact fields/list/status areas; Warp Wi-Fi and turbo
  controls retain their explanation and cooling guidance.
- APPS pages long lists, including pointer hit targets, keyboard navigation
  and search across pages.
- Network picker hints and AMINET Search button text fit their allocated space.

## Coverage

| Window family | Review/check |
| --- | --- |
| General and Update | Production field helpers and authored coordinates; bounds/captions |
| Appearance | Production helpers; compact preview; bounds/captions/overlaps |
| Organize and launcher actions | Production helpers; both focused and list editors |
| Add Item | All item-type authored fields; compact and normal layouts |
| Plugin inventory, Diagnostics, About, Layouts | Production helpers and authored bounds |
| Spacer editor | Focused/nonfocused layout and bottom controls reviewed |
| Bundled plugin settings | Expanded built-in schemas plus installed descriptors; every label/choice, picker column and page height |
| Network profile and icon pickers | List/page height, footer buttons and captions reviewed |
| Warp Wi-Fi, Prism Wi-Fi, CPU turbo | Production constructor gadget capture; bounds/captions/overlaps |
| AmiHASS device/entity/scene pickers | Production constructor capture for five/six-button layouts |
| APPS | Production grid and hit testing, 0-48 entries, all pages |
| Notes/Clippy/PLACES/SCREENS quick pickers | Shared production grid/hit testing, 0-64 entries, all pages |
| Notes editor/context menu | Toolbar, edit area, scrollbar, buttons and clamped menu reviewed |
| Weather | Production painting checked against both heights, empty/full data |
| ARCHIVE create/extract | Destination/options and bottom buttons reviewed |
| AMINET search/categories/readmes/download | Shared production constructor capture; list/footer/progress bounds |
| Clipboard history | Search/list/six-button footer reviewed |
| WHDLoad/MUSIC browser | Filter/list/footer height clamp reviewed |
| SMODE | Screen-limited rows and automatic window placement reviewed |
| BitSkim recipients | Font-aware list/footer size clamp reviewed |
| Network details, MusicPlayer, Mixer | List/control/button bounds reviewed |
| Timer/SENSOR/HASS/DOWNLOAD/SYSTEM/ACTIONS details | Shared panel captions and row/button layout reviewed |
| File/drawer/font requesters | System-managed ASL requesters, no custom fullscreen dimensions |

`tests/test_small_screens.py` executes extracted production helpers and the
critical browser constructors with captured GadTools rectangles. It also
checks all bundled schemas. `tests/test_weather_window.py` checks actual
painting operations. These run with the portable test suite; no network or
hardware commands execute in the layout tests.

This is a source/geometry and native-compilation review. It does not certify
visual rendering or pointer operation on physical Workbench hardware. The
remaining manual pass is to open each family above at both modes, tab through
fields and reach every bottom button; test full/empty lists, page changes,
Use/Save/Cancel and closing requesters. No physical-board functions were invoked.
