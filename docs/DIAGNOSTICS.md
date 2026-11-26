# Support recordings and reports


## Create a report

Choose **Preferences > Support > Create report...**. A 60-second recording
starts automatically. Use NovaDock to demonstrate the problem while the
countdown runs. Detailed tracing, disk logs and repeated five-second plugin CPU
and tracked-allocation samples are collected automatically. Your previous
logging settings are restored when recording ends, even if the dock restarts.
An expiry lease restores settings if the countdown helper stops unexpectedly
while NovaDock remains running. Later manual logging changes take precedence.

**Continue now** keeps a partial recording and opens the report form.
**Use existing logs** does the same for issues that already happened, especially
crashes that cannot safely be repeated. The previous recording and resource
samples are retained alongside the new recording. Completed and partial results
are labelled; missing CPU samples are shown as unavailable, never as zero load.

Describe the problem, review the detected system details, then choose where to
save. A fresh five-second plugin sample is gathered at save time as well.
Reports include `capture.txt`, `capture.previous.txt`, `plugin-samples.txt`, and
`plugin-samples.previous.txt` when available. Every sample row contains elapsed
time, numeric plugin slot, tracked RAM, activity/I/O counters, and CPU hit/sample
counts. These are statistical estimates of the main plugin task, not total
process accounting or separately launched worker programs.

The main Support page has Create report, Advanced, recovery, and the low-resource
preset. **Advanced...** contains manual logging, plugin activity, snapshots,
counter reset and event markers. The temporary DOpus4 benchmark was removed.

Disk component logs retain a 512 KB current and previous file. RAM logs use
64 KB per file to limit memory use. Starting a new capture archives the preceding
capture into four owned disk session slots, or two RAM slots, per component. The newest unfinished archived capture is protected when
older slots are pruned. Unknown files in a slot prevent its deletion. Reports
collect RAM and disk sessions separately. Logging must already have been
running to capture events before a crash; data never recorded cannot be recovered.

Magnification uses measured rendering cost to choose a 40–200 ms timer period.
With Animate off the selected tile still jumps directly to its full configured
size. Optional scaled icon and widget caches share an automatic 128–1024 KB
image-data budget. `HOVER_CACHE_KB=128` through `8192` sets an explicit budget;
`0` restores automatic sizing. Necessary background buffers are retained to
preserve the pixels of windows underneath the dock. Support reports include
frame cost, target period, cache size, budget and eviction count.

If saving a report fails, entered answers and the last selected destination
are retained for another attempt. Partially saved files are kept. Collection
and LhA compression show a progress window; failed compression retains the
uncompressed report drawer.

## Plugin activity

Open **Preferences > Support > Advanced... > Activity** for a paged list of enabled plugins,
tracked RAM, a sampled CPU estimate and the latest fixed operation. The view
starts one five-second sample; **Sample 5s** repeats it. Measuring does not turn
logging on. `--` means unavailable, and `+` marks partial allocation tracking.
Tracked RAM excludes libraries, stacks, code and graphics buffers. CPU sampling
can miss short bursts and does not include separately launched applications.

All bundled clients negotiate the same capture policy. Off sends no extra
support packets. Logging adds bounded numeric snapshots; Detailed trace permits
more frequent snapshots. Allocation failures, file I/O counts, transfer bytes,
scan/cache state and user actions use fixed numeric events. WHDLoad includes
indexing state, entry count and scan errors. SENSOR includes available-channel
and provider counts. Aminet reports transfer phase and byte counts. These records
use the dock's existing bounded/retained logs and are collected with reports.
Resource snapshots and CPU sample counts are also included in `support.txt`.
