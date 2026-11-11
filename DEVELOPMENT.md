# NovaDock development

NovaDock targets Classic AmigaOS 3.x and baseline 68000 processors.
The source uses C89, with AmigaOS-specific code checked against NDK headers.

## Build and test

Set VBCC to a configured m68k-amigaos vbcc installation, with vasm and vlink
available in its bin directory or PATH. VC can select a custom compiler driver.

```sh
sh scripts/build-amiga.sh -B
SANITIZE=1 sh tests/run_host_tests.sh
sh tests/amiga_syntax_check.sh
```

The native build produces 25 Amiga Hunk executables. Host tests exercise the
portable logic; native syntax checking requires AmigaOS headers and a
m68k-amigaos GCC installation. Set M68K_CC and OPENNDK when they are not found
in the standard locations.

To stage a runnable application drawer on a transfer volume:

```sh
sh scripts/build-and-stage-amiga.sh /path/to/transfer-volume
```

This creates a NovaDock drawer with programs, Workbench icons, themes, setup
utilities, installer and defaults. Existing novadock.cfg settings are retained;
updated defaults are supplied as novadock.cfg.default for comparison.

## Linux toolchain

```sh
sudo apt-get install lhasa
sh scripts/setup-vbcc-toolchain.sh ~/amiga-toolchain
export VBCC=~/amiga-toolchain/vbcc
export PATH="$VBCC/bin:$PATH"
sh scripts/build-amiga.sh -B
```

The setup script uses pinned tool sources and obtains the required NDK.
This source distribution does not assume a hosted CI workflow.

## Header dependencies

vbcc has no automatic dependency output, so each program's header list in
`Makefile` and `Makefile.cross` is maintained by hand.  The host tests run
`python3 tools/make_deps.py`, which follows every target's `#include` tree
and fails if a header is missing; `python3 tools/make_deps.py --fix` adds
them.  Without a complete list, an incremental build after a change to, for
example, `nd_protocol.h` or `nd_config.h` can leave the dock and its plugins
built against different structures.

## Packing a source tarball on macOS

Use `COPYFILE_DISABLE=1 tar --no-xattrs -czf ...` so the archive carries no
AppleDouble `._*` files or `com.apple.provenance` attributes, which other
systems extract as hundreds of stray files and warnings.

## Making and publishing releases

Use `python3 tools/release.py prepare --output ../dist/release-0.1` to compile,
test and package a release. `publish` previews GitHub and GitLab publication;
add `--execute` to push the version tag and attach verified release assets.
The `aminet` subcommand separately checks or submits the Aminet files.

See [docs/RELEASING.md](docs/RELEASING.md) for setup, credentials, complete
commands, retry behavior and the native test requirements. No hosted CI or
automatic tag-triggered upload is assumed by these scripts.

## Dock source layout

The dock is split by topic; every file includes the private
`src/dock_internal.h` (the `NDDock` structure, constants and the functions
shared between files). Functions used by only one file stay `static` there.

| File | Contents |
| --- | --- |
| `src/novadock.c` | start-up, main loop, shutdown, library bases, diagnostics |
| `src/dock_draw.c` | colours, fonts, tile geometry, every tile, badges, graphs |
| `src/dock_popups.c` | calendar, About window, drawer popup |
| `src/dock_window.c` | window size and opening, hit-testing, scrolling |
| `src/dock_launch.c` | starting programs and companions, quiet requesters |
| `src/dock_icons.c` | loading and scaling icons, reuse across reloads |
| `src/dock_input.c` | AppWindow, AppMenus, Commodities hot keys, paths |
| `src/dock_plugins.c` | plugin processes, the plugin protocol, watchdog |
| `src/dock_timers.c` | timer work: auto-hide, hover, tooltips, media and task scans |
| `src/dock_reload.c` | configuration reload, control port, moving tiles |
| `src/dock_menu.c` | context menus and their actions, tasks |
| `src/dock_events.c` | drops, clicks, keyboard control, the window event loop |
| `src/dock_arexx.c` | the ARexx host |

A new dock file must be added to `Makefile`, `Makefile.cross` and
`build.amiga` (`tests/test_build_scripts.py` checks the last two agree);
`tools/generate_catalog.py`, `tests/amiga_syntax_check.sh` and the review
checks in `tests/run_host_tests.sh` pick up `src/dock_*.c` automatically.


## Sanitizers and fuzzing

`SANITIZE=1 sh tests/run_host_tests.sh` builds every host test with
AddressSanitizer and UndefinedBehaviorSanitizer (`CC=clang` picks another
compiler). `sh scripts/fuzz.sh [seconds] [target ...]` builds the libFuzzer
targets in `tests/fuzz/nd_fuzz.c` with clang and runs each for the given time
(default 60 s): the configuration, manifest and ARexx parsers, the ILBM
loader, clipboard IFF, BitSkim status text, AHI and ScreenMode preferences,
Script tile results, Network settings and MusicPlayer replies. Crashes are
kept as `fuzz-out/<target>/crash-*`; rerun one with
`fuzz-out/fuzz_<target> fuzz-out/<target>/crash-...`.
