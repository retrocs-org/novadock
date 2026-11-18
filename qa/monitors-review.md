# WATCH, LOGS and SENSOR status review

Target: NovaDock 0.4 development build, 18 November 2026.

Implemented WATCH and LOGS as independent-instance plugins with bundled artwork,
64-row native filtered histories, unread badges and optional ALERTS notices.
Filesystem and clipboard operations run in one guarded worker per instance.
Histories use a ring buffer; slow paths cannot accumulate duplicate workers.

SENSOR reports no selection, stale readings, unavailable devices and optional
I2C library absence. Detail rows distinguish disabled sources/channels and
current measurements. Preferences retains previously discovered native
channels and annotates cached stale/absent readings.

Verification performed:

- Full sanitized host regression suite: `/private/tmp/nd-monitors-host-complete.log`.
- Final focused sanitized model tests cover baseline/change detection, ring
  ordering, unread acknowledgement, split/long/control-coloured log lines,
  rotation fingerprints and SENSOR states.
- Production worker I/O tests cover appends, truncation, replacement, tail
  limits, oversized drawers and unavailable paths through a POSIX DOS adapter.
- Configuration tests cover real manifest discovery, independent instances,
  separate paths/labels, interval validation, save/reload and removal.
- All 46 Amiga programs build. Twenty plugin artwork banks validate.
- The final uninstrumented native Workbench integration passed in
  `/private/tmp/nd-monitors-native-production/junit.xml` with a disposable cloned
  AmigaOS 3.2.1 / 68060 guest. It opened the six-button native viewer and verified
  WATCH baseline/new/changed files, LOGS normal/append/truncate/replace, dock
  status IPC and delivery to the ALERTS journal.
- Website source and generated source/SDK download checks pass locally.

The native test exposed a large worker-request structure-copy problem. The
resident now copies only explicit request fields; WATCH snapshots copy only
initialized entries, keeping individual copies below 32 KiB. The final native
run used production binaries, with no diagnostic logging added to the plugins.

Physical sensor hardware was not exercised by this status-only change. Manual
checks still useful: unavailable network drawers, a slow clipboard owner,
opening files through their Workbench default tools, and visual inspection of
new plugin artwork in the user's preferred screen modes.

The runnable build is staged under
`/Users/frossmant/Development/builds/NovaDock`.
Nothing was committed, uploaded, deployed or published for this work.
