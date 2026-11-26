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

The native build produces the dock, Preferences, helpers and plugin executables.
Host tests exercise the
portable logic; native syntax checking requires AmigaOS headers and a
m68k-amigaos GCC installation. Set M68K_CC and OPENNDK when they are not found
in the standard locations.

The syntax check covers every C source under `src/`, `prefs/`, `plugin_src/`
and `tools/` except the few written in vbcc-only inline assembly (listed in
`VBCC_ONLY`); it must report nothing. The host suite also runs on Linux:
generated PNG artwork is compared by decoded content, so zlib-ng and stock
zlib give the same result. Inside a Git work tree,
`tests/test_tracked_sources.py` fails when a file the build or installers
reference is not tracked, so a release from a clean checkout cannot miss it.

To stage a runnable application drawer on a transfer volume:

```sh
sh scripts/build-and-stage-amiga.sh /path/to/transfer-volume
```

This creates a NovaDock drawer with programs, Workbench icons, themes, setup
utilities, installer and defaults. Existing novadock.cfg settings are retained;
updated defaults are supplied as novadock.cfg.default for comparison.

Cross-builds use `scripts/compile-amiga.py` to keep compiler assembly and object
files in isolated workspaces below ignored `build/intermediates/`. Workspaces
are removed on success or failure. Handwritten assembly remains in the source
folders; generated assembly is not tracked or distributed as source.

## Linux toolchain

```sh
sudo apt-get install lhasa
sh scripts/setup-vbcc-toolchain.sh ~/amiga-toolchain
export VBCC=~/amiga-toolchain/vbcc
export PATH="$VBCC/bin:$PATH"
sh scripts/build-amiga.sh -B
```

The setup script uses pinned tool sources and obtains the required NDK.

### vbcc startup exit race

vbcc 0.9's stock `startup.o` restores the entry stack pointer in `___exit`
before reading the two words it saved there. A task switch at that point lets
Exec save registers over them, and the program then frees SysBase on exit
(alerts `0100000F`, `81000005`). NovaDock therefore links a patched copy:
`setup-vbcc-toolchain.sh` writes `lib/startup-novadock.o` and points the
`aos68k` config at it. For another vbcc installation, including a native Amiga
one, create the patched object and link it instead of `startup.o`:

```sh
python3 tools/vbcc_startup.py patch "$VBCC/targets/m68k-amigaos/lib/startup.o" startup-novadock.o
```

Release builds refuse any program that still contains the unsafe sequence;
`python3 tools/vbcc_startup.py check NovaDock Plugins/*` runs the same check.
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

Use `scripts/release.py` from the project root. Build and check a fresh output
directory, prepare its explicit version tag, then review the manifest before
publishing with `--confirm-publish`. The complete commands, credentials and
recovery rules are in [docs/RELEASING.md](docs/RELEASING.md).

The release checks validate plugin artwork after LHA extraction and verify the
complete SDK after ZIP extraction, including rebuilding ArtHello's artwork from
its JSON source. Runtime testing on AmigaOS remains a separate check.

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
Script tile results, Network settings, MusicPlayer replies, the calculator,
Weather and geocoding replies, Notes Markdown, the Monitor log reader and
Aminet HTTP and archive headers. Crashes are
kept as `fuzz-out/<target>/crash-*`; rerun one with
`fuzz-out/fuzz_<target> fuzz-out/<target>/crash-...`.

## Plugin versions

Bundled plugins have their own releases in `include/nd_plugin_versions.h`.
`VERSION` controls the dock and its utilities only. For each changed plugin,
bump its minor version once for the next release and update its date. Changes
to linked shared client code count as plugin changes. New plugins start at 0.1.
CPU uses Activity's release; RAM, Chip RAM and Fast RAM share Memory's release.

Run `python3 tools/update_plugin_versions.py` after editing the header. This
updates the version in each shipped `.ndplugin` manifest. The native executable
and built-in Preferences catalogue read the same header. The release version
check rejects stale plugin manifests without changing plugin releases.

## Runtime validation limits

Host tests and a successful 68000 build do not replace runtime checks. Use
`tools/run_native_fixture.py` with a disposable AmigaOS 3.2.3 installation
(Workbench library 47.42 or newer) for native checks. Never use a directory
name alone as evidence of the running OS version.

Animated magnification, wallpaper palette sampling, profile switching, theme
backdrop import and the PET artwork need further physical-hardware coverage,
especially on 68000/68020 and low-colour AGA screens. The reported slowdown
with fullscreen DOpus4 in the background remains a hardware investigation;
enable capture under Preferences > Support and collect a report before restarting.

The magnified canvas uses logical viewport coordinates for input and offscreen
painting, but its physical Intuition layer stays unscrolled. Present frames in
window-local coordinates. A viewport resize acknowledgement must paint the
existing animation state before the timer advances it; otherwise another
ChangeWindowBox can move old contents before a frame is presented. The native
hover-underlay fixture checks completed headroom pixels, zero layer scroll and
exposed foreign-window pixels with opaque and translucent backgrounds.

Support settings retain a validated staged file and previous-generation backup.
If the ENV handler rejects rename, publish by complete-file copy and recover the
backup after a failed write. SupportReport and Support logging display the
failing path, operation and DOS error; do not replace that detail with a guessed
permission diagnosis.


Native support-report regression fixtures use the normal runtime at QATests:Dock.
`tests/native_support_check.c` covers the wizard, ASL Save/Cancel, runtime capture
and retaining both pre-crash disk and fresh RAM logs. Its no-LhA case temporarily
shadows the disposable guest's C: assign and restores it afterwards.
`tests/native_support_export.c` links `tools/nd_support_save.c` and
`src/interaction.c` with `-Iinclude -lamiga`. Build
`tests/native_support_fake_lha.c` as FailLhA and RaceLhA beside that regression to
exercise compressor errors and a destination created during compression. The
fixture requires the isolation marker and a guest with LhA installed. The host
can extract the resulting archive to validate report and log contents.

Plugin release QA: run `python3 tools/run_plugin_qa.py --output outputs/plugin-qa-new-run` before packaging. The release workflow requires passing evidence for all bundled plugins. See [Plugin QA](docs/PLUGIN_QA.md) and the [performance audit](docs/PERFORMANCE_AUDIT_2026-11-23.md).
