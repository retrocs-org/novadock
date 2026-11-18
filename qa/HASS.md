# HASS bridge interoperability

Use AmigaMac's isolated test mode with a fresh output directory and no saved
AmiHASS credentials in its bundled Workbench. Keep the normal desktop closed
before starting the test; never stop a user's session automatically.

Compile `tests/native_hass_bridge.c` with the same Amiga compiler/options as HASS,
linking `plugin_src/hass/nd_hass.c`, `plugin_src/hass/nd_hass_client.c` and
`src/interaction.c`. Put the executable in `outputs/hass-qa/NovaDockHASSBridgeCheck`.

```sh
'/Applications/Amiga Workbench.app/Contents/MacOS/AmigaWorkbench' \
  --workbench workbench-3.2.1 \
  --mount "QATests:$PWD/outputs/hass-qa" \
  --mount "Bridge:$HOME/Development/builds/AmiHASS" \
  --test "$PWD/qa/hass-bridge.json" \
  --output /tmp/novadock-hass-native-001
```

The test starts its own AmiHASSService, exercises the real dock client adapter,
checks status, unconfigured tile behavior, both diagnostic pages and shutdown.
It checks that neither HTTP requests nor device actions occurred. Existing
services or saved credentials cause refusal. Retain the resulting JUnit report
and guest output. Native IPC checks do not certify live entity controls or UI.

## Device-first light setup

`tests/test_hass_picker.py` exercises the device/entity chooser request model,
including scoped LIST calls, cancellation, paging and supported light actions.

Compile `tests/native_hass_devices.c` with the same sources as the check above,
outputting `outputs/hass-qa/NovaDockHASSDevicesCheck`. Run the launcher with
`--test qa/hass-devices.json` and a fresh output directory, keeping the same
QATests and Bridge mounts. This test refuses an existing service or configuration,
creates sample-only settings inside the disposable guest, discovers a device and
its light, then checks Turn on, Turn off, Toggle and brightness 25/100/0 through the real bridge.
A second client changes brightness to verify external readback. Three deliberate
service stop/start cycles verify recovery and rejection of offline actions.
It confirms that HTTP counters remain zero. No real device is controlled.

For interactive validation, update NovaDock and AmiHASS, open a HASS tile's
settings, use Type, Device and Click Action to select a device and its light entity. Check filter/page controls,
Cancel, changing the device/type, separate instances and saved settings after a
restart. Use AmiHASS sample data for simulated actions. Physical light control
requires a separate live check; the sample test does not certify it.

Automatic icon checks: turn a sample light off/on and confirm its artwork
changes only with the returned state. A pending read should show unknown,
while stopping AmiHASS should show the sad bridge. Repeat with two tiles for
the same entity, and with a switch, fan or door/contact sensor. The settings
window should show only Tile label, Type, Device and Click Action.

In Show controls, check a dimmable sample light's percentage slider. Confirm
that dragging sends only on release, server readback restores the level,
external actions update it, and stale/offline data disables it. An older bridge
or an on/off-only light must omit the slider. Check the four-field state window
and five-field brightness window at 640x256 and 640x480.

Latest automated check (2026-11-18): the 68060 Workbench 3.2.1 sample test
passed device/light discovery, on/off/toggle, brightness 25/100/0, external-client
brightness readback, three service restarts and zero-HTTP diagnostics. Local
results: `/private/tmp/novadock-hass-recovery-native-20261118-01/junit.xml`.
This certifies the sample IPC path; it does not certify the live UI or real bulbs.

## Shared parameter controls

From the sibling AmiHASS repository, run `make qa-novadock` with the configured
Amiga compiler, then its `qa/sample-novadock.json` through Workbench.app's
isolated test workflow. The helper links this production NovaDock adapter and
refuses to send actions unless STATUS confirms the shared sample source.
It checks brightness, RGB and climate readback from an independent SDK client,
then runs `Plugins/HASS CONTROL-SELFTEST` to validate native control inputs.
No real-device actions or HTTP requests are used.
