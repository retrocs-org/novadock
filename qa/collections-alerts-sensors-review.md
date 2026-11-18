# Collections, alerts and sensor-provider review

Target: NovaDock 0.4, Classic AmigaOS Workbench, 68060 test profile.

The disposable native test in `tests/native_collections_alerts.c` passed:

- Large 32-bit volume capacities, invalid readings and threshold configuration.
- Notification bounds, unread acknowledgement and delivery through the dock.
- Shelf deduplication, backup recovery and one multi-item drop into one instance.
- Shelf shortcuts survive a dock restart without altering their target files.
- Alerts history resets on restart.
- Stable sensor selection keys and an unavailable optional library without fake readings.
- The native filtered collection window opens successfully.

Portable checks additionally cover malformed/truncated cache records, numeric
overflow, notice eviction and duplicate suppression. The volume performance
regression verifies four queries for four visible volumes, no hidden-volume
queries and one coalesced repaint per changed tile.

The optional i2csensors.library provider still needs physical-board validation
of sensor names, calibration and missing-channel behavior. Native absent-library
checks and fixture data do not certify those hardware readings.

Interactive collection layout at each supported font/screen size and physical
low-space transitions remain useful follow-up checks. No production release was
published by these tests.
