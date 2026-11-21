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
