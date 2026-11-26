# Native performance regression checks

Compare two staged runtime drawers containing the 0.5 tools. Use the same
Workbench installation, CPU, memory and display profile for both. The runner
creates a fresh guest for each runtime, copies runtime assets and supplies its own fixed ten-tile,
two-row layout. It stops any dock auto-started by the guest image, uses private
RAM ENV:/ENVARC: assigns and verifies that the intended ten tiles are running.
It does not copy the user's configuration or control HASS devices.
Close other Workbench emulator sessions before running it.

With the normal Amiga toolchain environment configured:

```sh
python3 tools/run_performance_qa.py --baseline /path/to/baseline/NovaDock --candidate /path/to/candidate/NovaDock --profile /path/to/workbench-profile.json --compiler "$VC" --output /private/tmp/novadock-performance-new
```

For cycle-exact 68000 profiles, add `--timeout-seconds 1200` to allow the slower
guest to finish; this changes the wall-clock watchdog, not the regression budgets.

Plugins-page timings measure window availability, including its loading state;
`tests/native_catalog_manager.c` separately waits for completed inventory, forces
Refresh, navigates away during loading and checks worker-port cleanup. The
window-latency measurements do not claim that a full scan has finished.

The probe measures dock startup, first and repeat Plugins-page opening,
General and QR-settings window latency, five seconds of idle dock wakeups and
full paints, and retained memory above that guest's pre-launch baseline (including
RAM cache files). Baseline and final system totals are retained for diagnosis. It closes its own Preferences windows and
stops each dock before the guest closes. Separate guests avoid measuring cached
segments or unfinished cleanup from the other runtime. The probe waits for an initialized settings-window port before closing its own
windows and fails explicitly if the guest restarts. Native input hashes, profile,
compiler output, JUnit and measured before/after values are retained.

`tools/performance_check.py` applies relative and fixed-noise budgets and exits
nonzero for a regression or an incomplete report. The same check can run again:

```sh
python3 tools/performance_check.py /path/to/qa-output/performance.log --output performance.json
```

The budgets are in ticks at 50 Hz and bytes, not assumed CPU percentages.
They permit bounded first-scan cache costs and 512 KiB of added memory, while
checking repeat openings more strictly. Avoid comparing different emulator
profiles or interpreting emulator speed as physical Amiga speed. Physical
68000/68060 timing and RTG testing remain separate release checks.

The host suite tests budget failures and cache invalidation. Native runs remain
explicit because they require a licensed Workbench installation and an idle
emulator. They do not publish or upload anything.


## Release polish checks

`tools/run_release_polish_qa.py` validates an extracted candidate against the
current native binary hashes and source build ID. Use a fresh output directory
for each invocation. All guest drives are disposable; no user installation is
mounted. `--hidden` keeps the host emulator window out of the way while
preserving the guest display and normal rendering. Hidden runs also stay in the
macOS background and disable host mouse integration/input grabbing; the harness
injects its pointer and keyboard events through the guest input.device. Native
runs are serialized because the bundled emulator can deliver
Cocoa/SDL quit events across concurrent instances.

Examples (substitute the exact current and previous archives):

```sh
python3 tools/run_release_polish_qa.py --candidate outputs/candidate/NovaDock-0.6.lha --previous outputs/previous/NovaDock-0.5.lha --check installer --output outputs/polish-installer
python3 tools/run_release_polish_qa.py --candidate outputs/candidate/NovaDock-0.6.lha --check ui --variant RTG --output outputs/polish-rtg
python3 tools/run_release_polish_qa.py --candidate outputs/candidate/NovaDock-0.6.lha --check ui --variant LARGE --output outputs/polish-large
python3 tools/run_release_polish_qa.py --candidate outputs/candidate/NovaDock-0.6.lha --check ui --variant PLANAR --output outputs/polish-planar
python3 tools/run_release_polish_qa.py --candidate outputs/candidate/NovaDock-0.6.lha --check soak --hidden --soak-seconds 1800 --output outputs/polish-soak
```

The UI cases check 640x480 RTG, larger 10x9 screen font metrics at 800x600,
and a 640x256 planar 68000/8 MiB configuration. The planar profile runs at
maximum emulation speed; it is an instruction and rendering compatibility
check, not a cycle-exact performance measurement. They inspect window and
gadget bounds for every main Preferences page, exercise supported 31-character
labels, and inject guest keyboard events for typing, Tab, Shift-Tab, Enter and
Escape in the guided report. Screenshots remain in the evidence directory.

The Installer cases retain the real file, startup, upgrade and removal
expressions while substituting question answers. Standard locations cover
all three logical user levels; custom paths cover Average and Expert, spaces
in paths, and shared data on a second volume. Interactive question selection
and physical Amiga testing remain separate.

The soak repeatedly starts the dock, opens and rescans WHDLoad and MUSIC
collections, opens Appearance, changes the plugin set and dock side, moves the
pointer over magnifying tiles, then shuts down. After warmup it checks public
ports, task count, total free RAM and the largest free allocation. Its bounded
allowances are 64 KiB total free RAM, 128 KiB largest-block change, and two
ports/tasks above the warm baseline. It also requires every selected plugin
wake port to disappear. The CSV preserves each sample, including CPU tile
readings and dock wake, paint and update counts. These CPU readings describe
the active workload; they are not per-plugin execution-time measurements.

A successful release soak requires at least 1800 seconds in both the guest
clock and host monotonic clock. A short `--soak-seconds 20` smoke run is marked
as lacking release duration. An interrupted or timed-out run never passes.
The complete all-plugin gate separately exercises SDK resource sampling,
tracked allocations, logging/trace policy, events and shutdown for every client.

A reboot marker now rejects a restarted soak immediately and preserves its earlier
CSV samples. Cycles start at least ten seconds apart, exercising popup worker
cleanup along with the existing workload; the full 1800 seconds remains required
in both guest and host clocks. Accelerated reset-reproducer runs are recorded
separately and never substitute for this duration check.
