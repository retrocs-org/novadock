# BENCH plugin

Add BENCH from Add Item → Plugin. Click its tile and choose Run to measure
integer CPU throughput, a 64 KiB memory copy and rectangle fills in a private
320×128, two-plane bitmap. Each stage lasts about one second, with CPU, memory-copy and drawing stage
feedback in the window title. Stop cancels
between batches; partial results are not saved. The dock remains responsive.

Completed results include the date, CPU flags, available Chip/Fast memory,
allocation type, display dimensions/depth and EClock frequency. Dated text
reports live in `ENVARC:NovaDock/Bench`. History shows the newest sixteen
reports and Compare displays their rates beside the current result.

These are short comparative tests, not a hardware rating or a fullscreen
screen-mode benchmark. Compare the same NovaDock benchmark version, memory
allocation and workload with similar background activity. Off-screen fill speed
includes graphics.library and the available blitter implementation; emulator
results do not establish physical Amiga performance. Artwork ships in Bench.assets.

If the report cannot be saved, the results window keeps the completed measurements
and shows the destination error. Fix the destination, then choose **Retry save**
to save those same measurements without running the benchmark again. The button
is enabled only for an unsaved completed report. **Stop** cancels an active
benchmark; saving disables both controls until the write finishes. Existing reports remain intact and failed saves
leave no partial report. Report and temporary filenames fit classic Amiga filesystems.

Compare shows signed percentage changes alongside the current and saved rates.
A zero baseline is shown as n/a; extreme changes are capped in the display to
avoid overflowing the 68000's integer arithmetic. Positive rates mean more work
per second, not a percentage of CPU utilisation.
