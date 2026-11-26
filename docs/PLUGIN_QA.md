# Plugin QA before packaging

Run from the repository with the configured Amiga compiler and an imported
AmigaOS FS-UAE test system:

```sh
python3 tools/run_plugin_qa.py --output outputs/plugin-qa-new-run
```

This builds the programs, runs the complete portable host suite, then boots an
isolated offline FS-UAE Workbench. The output directory must be new. No installed
user configuration is used. Allow several minutes for the native run.

Every bundled client has an explicit case in `tests/plugin_qa.json`. Adding a new
client without a case fails coverage validation. Each case names its portable
feature tests; the native harness also checks startup, heartbeat, five-second
CPU/resource sampling, bounded idle update traffic, Logging and Detailed trace
metadata, and orderly shutdown. The native gate also runs eight ARexx/Preferences save-barrier sequences, so queued script edits cannot shadow a later external Save. Interactive utility cases open and close their
real native window and check screen bounds. The report includes each plugin's
RAM, CPU hits/sample count and activity counters; the full logs are retained.

To test and package the same binaries in one command:

```sh
python3 tools/run_plugin_qa.py --output outputs/plugin-qa-new-run --package outputs/package-new-run
```

`tools/release.py build` runs this gate automatically. Evidence binds the host
and native results to the current source, plugin inventory, binaries and runtime
assets, and checks the saved log digests. A changed executable or stale/partial
report fails release packaging. Standalone developer `make-release.sh` always
checks coverage; use the command above for the complete release gate.

A focused development check is also available:

```sh
python3 tools/run_plugin_qa.py --output outputs/qa-sensor-new-run --no-build --skip-host --plugin SENSORS
```

Repeat `--plugin` for additional IDs or program names. Focused runs and
`--skip-host` produce partial evidence and cannot pass the release gate.
Use `--system` and `--resources` to select imported FS-UAE resources explicitly.

## What still needs separate testing

CPU is a statistical estimate of the plugin main task. RAM counts tracked
allocations, excluding executable code, task stacks, libraries and graphics.
A short idle sample is useful for regressions, not a physical-hardware benchmark.

Offline checks validate unavailable-provider behavior and portable feature
logic. They do not prove an AHI device, Warp/I2C sensors, Home Assistant server,
external application or Internet service works on a user's machine. Use
`tools/run_sensor_qa.py` for the deterministic sensor display fixture and
`tools/run_aminet_qa.py` for the separate live Aminet search/download test.
Keep real-hardware smoke checks for providers and low-memory configurations.

The release gate also launches the actual dock from Shell and Workbench in a portable drawer, a split installation with an empty sibling `Plugins` drawer, and one containing only stale metadata. Each launch must receive live updates from CPU, RAM, Clock, Archive and Pet. `tools/run_program_directory_qa.py` runs these six cases independently; it uses disposable settings and never installs into the user profile.

## State icon regression

`python3 tests/test_plugin_icon_states.py` renders every packaged artwork frame through the production Tiny/Normal bank renderer, including the cached shelf, one-bit transparency mask and both colour modes. Every bundled plugin must have a bank or a dedicated live/launcher icon.

`python3 tools/run_plugin_icon_qa.py --resources /path/to/isolated/resources --profile a4000-rtg --output outputs/plugin-icons-rtg` replays the packaged state banks over the real client ABI in a disposable offline FS-UAE installation. It checks painted pixels and screen bounds at both sizes, idle and magnified, and checks Tiny EYES. Repeat with `--profile a500-ecs` for classic planar graphics. Test clients replace only copies inside the isolated fixture and never run network jobs or user commands.

## Side placement and backdrop refresh

Use `tools/run_plugin_icon_qa.py --side LEFT --height 46 --programs Sensors ...` and repeat with `--side RIGHT`. The default height remains 76; minimum Normal tiles may be compressed to 46 pixels by dock fitting. The fixture identifies the actual diagnostics viewport, so it also follows magnified side docks.

`tools/run_side_redraw_qa.py --mode BACKDROP --alpha 100 --resources /path/to/isolated/resources --output outputs/side-redraw` supplies a deliberately delayed SIMPLE_REFRESH desktop and static launcher artwork. It compares the composed dock before and after desktop refresh and rapid hover movement on all four edges, with animation off/on. Repeat with `--mode TOP` and opaque/transparent backgrounds. A configuration guard rejects fallback/default layouts, and all output remains in a disposable guest.

## AmiNotis forwarding and retired plugins

`tools/run_aminotis_qa.py --resources /path/to/resources --profile a4000-rtg --output outputs/notis-rtg`
checks the real dock, General checkbox and protocol-v2 SDK against an isolated
receiver. Repeat with `--profile a500-ecs` for planar UI bounds. Add
`--service-drawer /path/to/AmiNotis/build/Amiga` to copy the independently built
service, presenter and history writer into the guest and observe delivery
through its public SDK. The real-service binaries are hashed in the report.
No installed AmiNotis or user configuration is changed.

The cases cover local collection with forwarding off, enabled delivery,
coalescing, service absence/restart without history replay, checkbox Save and
retired enabled profiles. Host tests cover queue limits, timeout/disable
cleanup, retained source options and chunked legacy tile data. Runtime package
checks reject the three retired programs, manifests, icons, artwork and hook.
