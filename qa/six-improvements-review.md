# Six improvements review — 18 November 2026

NovaDock 0.4 development build; AmiHASS 0.1 and structured Exec protocol ABI 1.

## Delivered

- Shared guarded background jobs with bounded requests, cancellation, stale-result rejection and safe late replies. Monitor jobs use unique instance names and generations when settings change.
- WATCH include/ignore filename patterns, pause/resume and quiet-period notification batching. Resume establishes a new baseline; notifications do not replay paused activity.
- LOGS live follow, frozen selection/pause with continued capture, warning/error filters, clipboard copying and new-file-only export. Export freezes its displayed snapshot.
- HASS capability-gated thermostat targets, fan percentages and cover position/open/close/stop, all through the AmiHASS Exec SDK. Commands require confirmed readback; unavailable capabilities stay hidden.
- CALC expression evaluation, decimal/hex/binary output, session history and clipboard copying, using software math on 68000 systems.
- TOOLTYPES drop/browse, default tool, stack and ToolType editing. Save retains the original artwork and metadata and keeps a numbered original-file backup. Embedded NewIcons artwork ToolTypes are protected.

## Verification

- Full NovaDock portable host suite passed with sanitizers. Covers arithmetic/error bounds, protected ToolTypes, filename matching, controls/capabilities, artwork, packaging inventories, source website checks and existing regression checks.
- Actual worker receive paths exercised across 1,000 normal/cancelled/late exchanges with allocation and signal ownership checks. Old-target results and duplicate workers are rejected.
- Native list geometry verified at 640×256 and 640×480, with 8- and 16-pixel fonts and 3/6/7/9 buttons.
- Full current AmiHASS portable/core suite passed, including fan/cover shared sample mutations and independently confirmed readback.
- Native Workbench 3.2.1 test passed in a disposable cloned 68060 profile: `/private/tmp/nd-six-native-02/junit.xml`.
- Native checks covered software calculator arithmetic, simultaneous panels, nine-button list/follow, cancellation and late worker replies, non-overwriting export, and real ToolTypes save/backup. Original icon backup bytes matched exactly.
- Five native worker cycles showed no accumulating public-memory loss (15,619,448 bytes before; 15,619,608 after).
- Native AmiHASS sample service checks confirmed fan 65%, cover 70%, cover close and thermostat 22.5 through a second SDK client. No physical devices were operated.

## Packaging and staging

- Current native builds completed for both repositories.
- Local NovaDock LHA, Aminet readme, SDK ZIP, source and website archives passed their packaging and extraction checks in `/private/tmp/novadock-six-final-packages`. No upload was requested.
- Full website checks passed against refreshed generated archives.
- Staged NovaDock native binaries, manifests, artwork banks, guide and installer match canonical files; staged AmiHASS service, Preferences and installer also match.
- Staging retained the existing NovaDock configuration.

## Limits

Emulated Workbench checks do not replace physical 68000 performance or real integration tests. Fancy ColorIcon/NewIcon visual variants should still be checked on the target Workbench. WATCH uses bounded polling and its supported wildcard subset; LOGS keeps a bounded recent history. CALC history is session-only. TOOLTYPES declines unsupported or oversized icon metadata instead of truncating it. Install the matching updated AmiHASS service to expose the new fan/cover capabilities.

No commits, uploads, deployment or publishing were performed.
