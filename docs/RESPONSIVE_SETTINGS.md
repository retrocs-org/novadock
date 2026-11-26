# Responsive settings and diagnostics

Moving, adding and removing dock items, switching the CPU/RAM view, and changing
folder view/sort update the dock immediately. Rapid edits share an in-memory
full configuration, including disabled plugins and their settings. After about
two seconds without another edit, the separate `SaveConfig` program writes a
snapshot to ENV: and ENVARC:. It runs at a lower priority than the dock.

Only one writer runs at a time. Changes made during a write remain queued for
a later snapshot. A failed write keeps the latest changes in memory and retries
after roughly ten seconds; it never reports those changes as saved.

Preferences synchronizes queued edits before loading a session. Its live
configuration writes acquire a short dock writer lease, preventing overlap
with the background writer. Tile edits are refused briefly while Preferences
owns that lease. Saved configuration files retain the existing validation,
backup and known-good recovery handling. Manual external writes made during a
pending dock save do not participate in this coordination; use Preferences or
wait until Support reports no pending changes.

Quit flushes the latest edits before removing the dock's public ports. If a
save fails or takes longer than the shutdown wait, the dock stays running and
shows a message so the unsaved changes can be inspected or retried. `SaveConfig`
must stay beside the NovaDock executable; the installer copies it there.

Support diagnostics separate these operations:

- **Last update / Update paths**: whether the window was reused, rebuilt,
  skipped because nothing changed, or an update failed.
- **Update time**: configuration/layout application, excluding the later save.
- **Settings save**: saved, queued, writing in the background, or waiting to retry.
- **Save batches / Save failures**: completed snapshots, edits combined into
  batches, and failed attempts.
- **Save time / Last ENV write / Last ENVARC write**: the most recent writer's
  elapsed time and individual file results. A prior successful write does not
  imply that newer queued edits have already been saved.

Older docks are identified as lacking the additional telemetry instead of
showing a fabricated clean-save state.

## Status dots

Plugin process liveness is reported in Support, not as an automatic tile
dot. AmIRC publishes a confirmed IRC connection; Roadshow publishes its stack
state; Directory Opus publishes whether the application is resident. Music uses
its detected player's advertised controls. These marks follow the running-app
indicator setting. A traffic sampler does not claim an Internet connection
merely because its process or network driver is running.

Explicit notification badges, such as a Trash item count, remain available.
Third-party plugins can deliberately publish notification badges through the
existing negotiated badge API. No plugin wire ABI was changed.

## Recovery and low-resource preset

Support offers **Recover settings**. Choose ENV or ENVARC and then the
known-good or previous backup generation. NovaDock validates the selected file
before offering to load it into the Preferences session. Existing session
changes are replaced only after that confirmation. Use or Save applies the
recovered configuration through the normal settings workflow.

**Low-resource preset** selects simple refresh, list-style drawer popups, meter
views in place of graphs, two-second CPU/RAM/Network polling, and disables
launcher scans and always-on-top. These changes stay in Preferences until Use
or Save. The preset is applied atomically; unavailable memory or configuration
capacity leaves the session unchanged. Plugin updates can be less frequent.

**IPC recovery** reports late replies reclaimed by the dock (saturating at
65,535). Existing per-plugin rows show update age, stale/offline state and
restart counts. Full native timing and input behavior still require testing on
the target Workbench setup.

## Preferences startup and preview backups

In 0.5, General, Appearance, Updates and bundled plugin settings open without
scanning every installed plugin or decoding unrelated artwork. Organize performs
discovery when first opened; Add Item and Plugins scan their catalogues when
needed. A focused third-party settings lookup validates only the matching
plugin's executable and artwork. Explicit catalogue refresh still reads disk.

The private preview backup uses an exact, checksummed same-process snapshot in
T:, with a separate name for each Preferences process. This avoids applying the
permanent text-save routine and its repeated validation passes to a temporary
copy. Cancel after Test restores the starting settings, including pooled text
and plugin options. If T: cannot be written, Preferences retains its in-memory
fallback. The temporary snapshot is removed on exit; it is not an import format.
Use, Save and preview restoration to ENV: retain normal validated text writes
and coordination with the running dock's save worker.

Artwork validation uses bounded lookup tables for names and frame states instead
of comparing every pair of frames. It still rejects duplicate states and name
hash collisions, including at the 256-frame limit. Package checks read and
validate one bank without opening it twice or building renderer-only key arrays.
Built-in option assignment stops at the matching definition while keeping custom
schema overrides and value validation authoritative. These optimizations also
apply to dock startup/reload and plugin settings outside Preferences.

## Cached plugin inventory in 0.5

The Plugins window opens before inventory discovery begins. A lower-priority,
independently loaded worker reads and validates packages, then sends bounded
results through Exec messages. Navigation and Close stay available while loading.
Refresh forces a new background validation scan; imports and removal wait for
current results. Third-party metadata also loads in the worker, with rapid
selection changes coalesced. Closing or leaving the page removes the reply port;
workers stop cooperatively between records and discard undeliverable replies.

Repeated plugin-list openings reuse validated package metadata and health results
from compact, checksummed disposable files in T:. Unused option slots and
fixed-string padding are omitted to keep cache memory and I/O bounded. Program, manifest, Workbench icon and artwork-bank
fingerprints are checked on each scan. Bounded directory snapshots avoid
repeated file queries; a cold scan checks a second snapshot before saving its
packed records. Ordinary icons and duplicate installations are cached too.
Filename matching follows AmigaDOS case rules. Changed or missing sources invalidate the
entry; missing, truncated or corrupt caches fall back to normal validation.
Refresh on the Plugins page forces a complete validation scan. Cache files do
not change saved settings and can be discarded at any time.

Tool result panels update their fields in place. Background readings preserve
uncommitted text edits and keyboard focus; an explicit Paste or palette Use
replaces the selected input. Idle tools wait on UI/worker signals and keep their
normal dock heartbeat, rather than running frequent polling timers.

## Support logging and traces

Open **Preferences > Support > Advanced... > Logging**. Choose **Off**, **Logging** or
**Detailed trace**. Use applies this session; Save also enables it after a
restart. Support settings have their own ENV/ENVARC file and are independent
of appearance previews and dock profiles. The normal installation defaults
to Off. A separate diagnostic installer is no longer required.

Logging records timing summaries about every five seconds. Detailed trace
records summaries about every second, ordered state changes, native startup
phases and a four-per-second heartbeat in plugin editors. Calls taking at
least 100 ms are recorded separately. Capture covers the dock, Preferences, the bundled DOpus4 bridge and shared
resource/activity telemetry from bundled plugins.

**Save logs to disk** queues output to the low-priority SupportLog worker in
`NovaDock:SupportLogs`. The dock never waits for that worker's disk writes.
RAM capture remains in `RAM:NovaDock-Trace`. Each component keeps current and previous files: 64 KiB per RAM log and
512 KiB per disk log, plus bounded retained sessions. Queues hold at most eight
records per producer; a slow or failed writer is visible in the queued,
dropped and error counters. Capture cannot guarantee the last queued disk
record survives a power failure. RAM logs are lost on reboot.

Use **Mark event** just before or after a slow action to add a timestamped
reproduction marker to the dock and Preferences logs. Wall-clock stamps on
each summary correlate separate processes even after logs rotate. Start/stop records
distinguish capture disabled or normal exit from an interrupted recording.

Choose **Create report...** to start a 60-second recording automatically.
Reproduce the issue while the countdown runs; plugin CPU estimates, tracked RAM,
activity counters and disk traces are gathered without extra setup. Previous
logging settings are restored when it ends. **Continue now** keeps a partial
recording; **Use existing logs** is useful after a crash that cannot be repeated.
The previous recording and resource samples remain available alongside the new
ones. See [Diagnostics](DIAGNOSTICS.md) for sample fields and limitations.

Double-clicking `SupportReport` or using `SupportReport FORM` opens the same
flow. Without arguments the Shell command saves an anonymous technical snapshot
for automated collection. **Advanced... > More... > Snapshot** keeps the quick
RAM-only text action. The collector includes current, previous and retained
RAM and disk logs separately, including when the dock is no longer running.
The save chooser selects the location, and LhA compression is used when available.
No files are uploaded. The interactive form has five pages after recording:

1. **Prepare:** choose the problem type and review the recording status.
   Partial or existing logs are accepted; the form never claims that completing
   a countdown proves the issue was reproduced.
2. **Details:** optional name/email, a required short summary, frequency and
   optional setup notes (accelerator, graphics card, network stack, etc.).
   Next stays disabled until a non-blank summary is entered.
3. **Description:** a large multiline text editor for up to 8191 characters,
   with a vertical scrollbar. Text over that limit must be shortened before
   continuing; it is never silently truncated. The system texteditor.gadget
   supplies native editing on AmigaOS 3.2. If that class is unavailable on an
   older installation, the wizard still allows Steps/Actual on the next page.
4. **Reproduction:** steps, timing/markers, expected/actual behaviour and
   recent changes.
5. **System / save:** labeled read-only fields show the detected CPU,
   AmigaOS/Workbench version, Kickstart release, separate running Exec
   version, NovaDock version, total/free Chip and Fast RAM, screen dimensions,
   bitmap bit depth, display-mode ID and display-mode name. Detection occurs
   automatically at report start; editable setup notes stay separate.
   Unknown OS releases retain their exact Workbench library version rather
   than guessing a marketing release; unknown Kickstart is marked unavailable.

Back and Next retain edits; Cancel creates no bundle. Save report writes the
chosen details and detected system information to `problem.txt` beside the
technical `support.txt` and logs. Nothing is sent automatically. Turn capture
Off after a short recording to avoid unnecessary logging overhead.

Automatic trace data includes build/system identification, memory, timings
and numeric state/counters. It excludes typed text, setting values,
credentials, window titles and configuration files. Contact and problem text
entered deliberately in the report form is included in `problem.txt`. Collection is local and never uploads.

Trace capture adds overhead, particularly Detailed trace. Turn it off after
collecting a short reproduction and compare responsiveness with capture off.
Use **Refresh** to inspect current capture/writer health. Existing recordings
are retained by installer updates and when capture is disabled.

The capture design was also compared with the
[SnoopDos 3.10 source](https://aminet.net/util/moni/SnoopDos_Src.lha), especially
`buffer.c`, the logging routines in `mainwin.c`, its options and roadmap.
Bounded buffering, explicit capture controls and log lifecycle markers are
useful patterns here. NovaDock uses its own implementation and does not patch
system-wide DOS/library calls or copy SnoopDos functions. A separate worker
can claim startup records while the traced process is blocked; short message
transfers never perform disk I/O while holding the scheduler lock.
