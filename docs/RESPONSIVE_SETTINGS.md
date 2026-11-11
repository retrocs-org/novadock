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
wait until Diagnostics reports no pending changes.

Quit flushes the latest edits before removing the dock's public ports. If a
save fails or takes longer than the shutdown wait, the dock stays running and
shows a message so the unsaved changes can be inspected or retried. `SaveConfig`
must stay beside the NovaDock executable; the installer copies it there.

Diagnostics separates these operations:

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

Plugin process liveness is reported in Diagnostics, not as an automatic tile
dot. AmIRC publishes a confirmed IRC connection; Roadshow publishes its stack
state; Directory Opus publishes whether the application is resident. Music uses
its detected player's advertised controls. These marks follow the running-app
indicator setting. A traffic sampler does not claim an Internet connection
merely because its process or network driver is running.

Explicit notification badges, such as a Trash item count, remain available.
Third-party plugins can deliberately publish notification badges through the
existing negotiated badge API. No plugin wire ABI was changed.

## Recovery and low-resource preset

Diagnostics offers **Recover settings**. Choose ENV or ENVARC and then the
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
