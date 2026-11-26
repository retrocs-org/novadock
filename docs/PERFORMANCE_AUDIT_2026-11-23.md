# Performance audit - 23 November 2026

Scope: bundled plugin event loops, idle IPC, support capture and shared native
controls. The new plugin QA gate records all 49 clients on an isolated AmigaOS
3.2.3 FS-UAE fixture. It measures update packets and tracks RAM, activity and
main-task CPU samples. These are emulator regression measurements, not physical
Amiga benchmark results.

## Changes

| Area | Previous behavior | Change and validation |
| --- | --- | --- |
| System tile | One timer wake every second even for static CPU/Workbench information | With capture off, closed static views now use a ten-second watchdog: 60 to 6 timer wakes/minute. Rotation wakes at its five-second deadline, and uptime respects minute boundaries. Dock/window signals still wake immediately. Portable timing boundary tests cover each mode. |
| Automatic support recording | Resource IPC during active CPU sampling, and capture-lease file reads, about ten times/second | Probe once/second while retaining the 10 Hz UI/event loop. Minimum-free-RAM observations now have one-second resolution. A final non-blocking probe retains samples completed between the last periodic poll and the deadline. Native report capture verifies the resulting sample files and policy restoration. |
| Shared percentage sliders | Repeated GadTools redraw requests for unchanged values | Read the current knob/disabled state and update only on change. Preserve selected/dragged knobs. A host harness checks 100 identical updates cause zero level or enable-state writes, as well as offline and re-enable transitions. |
| Mixer controls | Window could overlap the Workbench title bar | Clamp the initial top position. The all-plugin native window-bounds check caught this during the audit. |

## Retained behavior and next candidates

Interactive text fields and pending remote controls still use short polling
intervals where validation/debounce depends on them. Slowing these blindly can
make typing or acknowledgements feel delayed. A further pass should instrument
those specific states before changing their cadence.

CPU's idle sampler and animated Eyes/Pet/Aquarium tiles have intentional work;
the QA inventory gives animated clients a separate idle packet allowance.
Existing text/list controls already avoid unchanged text updates.

Short CPU samples commonly contain zero hits for a sleeping plugin. They establish
that sampling works, but cannot rank tiny differences reliably. Update packets
are also not timer-wakeup counts: unchanged state may produce no packet. The
System scheduling reductions above follow directly from tested timer deadlines.
Hardware/provider latency and long-duration memory growth need real hardware
and longer workloads. New QA evidence makes those follow-up comparisons repeatable.

## Recorded plugin QA results

The complete candidate run passed all 49 client cases and 19 owned-window
open/close checks. Each plugin had 94-100 CPU samples; the largest five-second
idle update count was 9 (Pet), within its animated-client allowance. All clients
exposed version-1 resource metadata and removed their wake ports on shutdown.
The complete portable host suite and minimum-68000 syntax checks also passed.

Evidence: `outputs/plugin-qa-2026-11-23-final/report.json`, with full host and
native logs beside it. The first baseline run exposed Mixer's title-bar overlap;
the corrected candidate passed that same bounds assertion.

## Before/after runtime comparison

Both runs used fresh copies of the same imported Workbench 47.42 / Kickstart
47.115, A4000/68060 and uaegfx profile. The fixed ten-tile workload and existing
noise budgets are defined in `tools/run_performance_qa.py` and
`tools/performance_check.py`.

| Metric | Previous installer | Candidate |
| --- | ---: | ---: |
| Startup (50 Hz ticks) | 17 | 17 |
| General Preferences (ticks) | 4 | 4 |
| Plugins page, first / repeat (ticks) | 2 / 2 | 1 / 1 |
| QR settings (ticks) | 2 | 1 |
| Idle dock wakeups, five seconds | 11 | 7 |
| Full idle paints, five seconds | 0 | 0 |
| Retained memory above pre-launch baseline (bytes) | 2,051,288 | 2,051,888 |

All eight regression budgets passed. The 600-byte memory difference is small;
one run and one-tick window measurements do not establish a general speedup.
These results check for regressions in this workload, which does not contain the
System tile. Its timer policy is tested separately above. Full input hashes,
runtime identities and raw results are in
`outputs/plugin-performance-2026-11-23/`.

The native report regression also passed the seven-second CPU-sample edge case,
the complete 60-second automatic recording, retained previous samples, capture
setting restoration, cancellation, uncompressed saving and LhA saving.
