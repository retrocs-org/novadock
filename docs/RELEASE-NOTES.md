### Features

- HASS supports advertised smart-lock and motorised-door actions through AmiHASS.
- AGENDA keeps persistent tasks and recurring reminders with a due badge.
- PICTURES displays local slideshows with larger previews and bounded background image decoding.
- HASS exposes supported media playback, volume and track information through AmiHASS.
- BENCH runs cancellable CPU, memory-copy and off-screen drawing comparisons with dated saved reports.
- QR generates local UTF-8 codes from text or clipboard contents, with readable previews and ILBM export.
- MOON shows offline moon phases, approximate illumination and location-based sunrise/sunset estimates.
- COLOUR samples Workbench pixels, copies hex/RGB values and keeps a protected 32-colour named palette.
- HASS adds read-only room summaries for lights, open contacts/covers and unavailable entities through AmiHASS.

### Enhancements

- Confirm HASS unlock, latch and door-opening actions; show clear door states and load supported actions in the background.
- Include Trash between Music and CPU in the default dock layout.
- Show percentage changes beside BENCH saved-result comparisons.
- Add optional bounded plugin thumbnails without host-side image decoding or palette changes.
- Retry saving a completed BENCH report after a destination error without rerunning its measurements.
- Open Plugins immediately while inventory, cache writes and third-party details load in a cancellable background worker.
- Derive installer QA versions from package executables and pair the HASS client explicitly with AmiHASS 0.2.
- Cache validated plugin inventory with source fingerprints and a forced Refresh; corrupt caches fall back to normal scans.
- Remove idle palette rebuilding and unchanged list copies; wait on signals in idle tool windows.
- Share HASS room aggregations, coalesce refreshes and transfer ownership when a room tile closes.
- Keep tool result windows and user edits intact during background updates; add COLOUR swatches, BENCH stages, QR export gating and MOON input validation.
- Add paired native startup/window-latency, idle-wakeup and memory budgets with a regression checker.
- Speed up built-in option lookup and animated artwork validation; read each artwork bank only once during package health checks.
- Open General and bundled plugin settings without scanning unrelated plugins; use a lightweight, checksummed temporary preview backup.
- Rename the installer and its Workbench icon to Install and Install.info.
- New package drawer and installer icon.
- Use the current packaging time for LHA and ZIP file dates and verify extracted LHA timestamps.

### Bugs

- Use factory defaults for fresh installs from a runnable development drawer, preserving existing installed settings on updates.
- Synchronize the AmiHASS SDK and require confirmed open/closed readback after stopping a cover.
- Keep QR/BENCH staging filenames within classic filesystem limits; report write-protected, full and invalid destinations without losing results or replacing existing files.
- Preserve COLOUR edits and saved palettes on failed writes, with actionable destination and recovery errors.
- Correct HASS setup instructions to AmiHASS 0.2 and Install, and check the documentation against the paired SDK.
- Avoid false plugin repair warnings for absent optional artwork on AmigaOS, restoring affected plugins to Add Item.
- Reject release builds with missing, invalid or incompatible plugin packages after archive extraction.
