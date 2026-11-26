# Sensors plugin

Add **SENSOR** from **Add item > Plugins**. Add multiple independent tiles, such
as Temperatures and Voltages, each with its own label and selected channels.
It detects supported readings and
shows the active channel name, value and sensor-type artwork above the instance
label. CPU, memory, FPGA, temperature, voltage, battery and fan readings have
distinct artwork. Unknown devices use a generic sensor picture.

**Auto-switch seconds** controls rotation (default five seconds). Set it to **0**
to hold the current channel. Right-click for **Next sensor**, **Previous sensor**
or **Sensor readings...**. **Click action > Next sensor** makes an ordinary tile
click advance immediately; the default still opens the readings list. The list
also has a **Next sensor** button. Missing/unselected readings are skipped.
Changing the channel does not run the vendor diagnostic again.

Each detected channel has an optional **title** in settings. Leave it empty to
use the device's normal name. I2C titles and choices follow the device's stable
identity rather than its discovery position. **Tile label** remains independent
of channel titles, so several SENSOR instances can have different purposes.

Right-click and choose **Edit settings**. Freshly detected channels appear as
checkboxes, checked initially. Untick readings you do not want displayed.
**Tile label** names each instance. A sad chip icon appears when no available
readings are selected, or no sensors are detected. **Use** applies for the current session; **Save** keeps the selection after
restart. Missing channels are hidden without erasing their saved choices.
Open settings again after connecting hardware or starting a sensor driver.
The title says **no sensors detected** when no supported readings are available.

## Supported sources

### MNT ZZ9000

Reads the Zynq chip's internal temperature, core voltage (VCCINT) and
auxiliary voltage (VCCAUX) directly from a ZZ9000 found through
expansion.library. Firmware must expose the telemetry registers (1.7 or later).
No extra sensor utility needs to be running. This is the ZZ9000 chip's
reading, not the Amiga CPU temperature.

NovaDock only reads the board registers; it does not change firmware, video
modes or clock settings. Temperature is shown in Celsius and voltage in volts.
Out-of-range readings are omitted. The interface matches official firmware
1.13 and its earlier 1.7 implementation:

| Register offset | Reading | Conversion |
| --- | --- | --- |
| 0xC0 | Firmware major/minor | High byte / low byte; 1.13 is 0x010D |
| 0xE0 | Zynq temperature | Raw value / 10, Celsius |
| 0xE2 | Auxiliary voltage | Raw value / 100, volts |
| 0xE4 | Core voltage | Raw value / 100, volts |

Offsets are relative to the detected board base. NovaDock uses 16-bit reads
on the Zorro II or Zorro III device (manufacturer 0x6D6E, product 3 or 4).
The firmware serves temperature and auxiliary voltage as the upper and lower
halves of one 32-bit response; the separate word offsets above match ZZTop.
No additional temperature, fan or current channels were found in this reviewed
telemetry interface.

Verified against the official MNT
[firmware sources](https://source.mnt.re/amiga/zz9000-firmware)
(tag 1.13, commit cc484d0abb19097f3a225c24813568ede8fe7960), including the
main.c register handler and adc.c XADC readings. Cross-checked with
[ZZTop](https://source.mnt.re/amiga/zz9000-drivers)
(commit 7434d253f1d143c1ae4697a62a42b92f6a1efa87).
The 1.7.0-beta3 source uses the same registers and scaling. Physical-board
comparison with ZZTop remains necessary; source verification does not replace
that test.

### AmigaKit clockport RTC and Sensors module

Install and start **AK_Sensors** using its Workbench icon, as described in its
readme. NovaDock reads the published ENV values:

| Value | Display |
| --- | --- |
| AK_Internal_temp_C | Module temperature |
| AK_CPU_temp_C | Optional external CPU probe |
| AK_5V_supply_V | 5 V supply |
| AK_RTC_battery_percent | RTC battery charge percentage |

The optional probe appears only when the driver publishes it. A CPU probe can
monitor a TF1260 CPU when connected through this module; it remains labelled
as an external probe rather than a built-in TF1260 sensor.

NovaDock does not start the driver, change its clockport address or set the
clock. AK_Sensors 1.3 supports a CLOCKPORT_ADDRESS icon ToolType for compatible
third-party ports. Leave hardware access and calibration to that driver.
Its readme specifies temperature accuracy of +/-2.5 C and voltage accuracy
of +/-2%; displayed precision does not improve that accuracy.

[Driver announcement and documentation archive](https://forum.amiga.org/index.php?topic=75107.0)
([AK_Sensors 1.3](http://amiga.serveftp.net/temp/AK_Sensors_v1.3.lha)).
The third-party driver is not bundled with NovaDock.

### CS-Lab Warp accelerators

Install the official Warp software. Set **Warp diagnostic tool** if WarpTool
is not at C:WarpTool. NovaDock runs only **WarpTool Diag**, in a separate
background process. Automatic polling uses a 30-second interval, with recent
results shared across instances and only one active diagnostic at a time. It reads temperatures,
supply/battery voltages and fan output when those rows are reported. Available
channels depend on the board and firmware. Fan output is a percentage, not RPM.
WarpTool output with dotted separators and coloured values is supported,
including Warp1260 diagnostic rows. No board settings, fan controls,
overclocking or WiFi settings are changed.
Other diagnostic fields are ignored; temporary output is removed after reading.
A stalled diagnostic cannot block dock input or start overlapping hardware jobs.

[Official Warp firmware and guide](https://amigawarp.eu/download/download-warp-firmware/).
The vendor tools are not bundled with NovaDock.

## Refresh and stale data

**Refresh seconds** controls local sampling (2-60 seconds, default 5), independently
of the dock's Smooth/Low mem redraw preference. Warp commands remain limited to
one per 30 seconds. **RTC max age seconds** defaults to 120; increase it if your
AK_Sensors POLL_INTERVAL exceeds this. Only ENV values within that age are used.
Warp values expire after 90 seconds without a successful update. Missing data
shows **NO SENSORS**, never a fabricated zero. With every available channel
unchecked, the tile shows **NO SELECTION**.

Runtime snapshots are kept under ENV:NovaDock/Sensors/ for each instance
and removed when that instance exits. Removing a tile clears only its own
settings and runtime state. A shared, timestamped Warp readout avoids repeated
diagnostic runs across tiles; selecting channels on one does not affect another. It is not a permanent sensor log.

## Hardware validation and TF1260

The plugin compiles for classic AmigaOS; conversion, parsing, firmware gates,
selection, snapshot and stale-data checks have portable regression tests.
Readings still need comparison with ZZTop, AK_Sensors and WarpDiag on physical
boards before treating this as hardware-validated support.

The TF1260 manual identifies FAN/THERM connectors but does not document a
software-readable sensor interface. Direct TF1260 sensor support is pending a
verified controller/API and calibration details. A connector alone is not
reported as a detected sensor. The board designer recommends a Solas controller
for temperature monitoring in [this hardware discussion](https://www.exxosforum.co.uk/forum/viewtopic.php?start=30&t=4947).

Next hardware work: compare readings on real boards, verify older WarpTool
output formats, and add a documented Solas/TF1260 provider when available.

## Plugin-owned status artwork

In NovaDock 0.4, the sad status icon ships in
`Plugins/Sensors.assets/Artwork.ndart`, beside the plugin executable.
Update the artwork and executable together, then reload or restart NovaDock.
Missing or damaged artwork falls back to status text.

## Optional i2csensors.library v2 provider

Install and configure i2csensors.library separately using its own sensor
and bus definitions in `DEVS:Sensors`. NovaDock does not install drivers,
change bus addresses or invent hardware definitions. Enable the provider in
SENSOR settings (enabled by default). Missing libraries do not affect native
ZZ9000, WarpTool or AK_Sensors readings.

The standalone `Plugins/SensorsI2C` helper enumerates configured voltage,
current, temperature, fan, pressure, power, humidity and miscellaneous classes.
Only successful reads first introduce a channel. A valid numeric zero is a
reading; two NULL output strings indicate the library's documented error.
Names and units are copied before closing the library. Signed 16.16 values
are converted without floating-point operations.

Reopen settings after the first sample to see discovered I2C checkboxes.
Selections use stable `I2C_XXXXXXXX` IDs rather than enumeration positions,
so reordering sensor definitions does not switch your selected channel.
Existing choices survive temporary disconnection and remain independent for
each SENSOR instance. Selected current channels rotate with native readings.
Unavailable or stale values are excluded from the live tile and detail view.

All instances share `ENV:NovaDock/Sensors/I2C.status`. One background helper
samples at most every five seconds; absence of the library uses a 30-second
retry interval. The dock and Preferences never call the bus API. A guard port
prevents overlapping helper calls; if a driver blocks indefinitely, readings
expire after 60 seconds and the dock remains usable. The helper bounds each
pass to 64 attempted reads and 32 cached channels. It prioritises successful
current readings over older unavailable ones.

The provider is a separate GPL-3.0-or-later executable, with its licence in
`Plugins/SensorsI2C.LICENSE`. NovaDock communicates through its text cache;
it does not link to the sensor library or redistribute the library binary.
See the [upstream library interface](https://gitlab.com/HenrykRichter/i2csensors).
Physical-board validation and calibration comparisons remain required. The
library's configured definitions alone do not establish TF1260 support.

## Reading and provider status

The tile reports No channels selected, Stale readings, Device unavailable,
I2C library missing or No sensors detected when there is no current selected
reading. Click for per-channel status, including previously discovered native
and I2C readings. Old measurements are labelled stale instead of being shown
as current values. Disabled channels and a disabled I2C provider are explicit.

Preferences retains known native channel choices while their readings are
unavailable, annotates stale/absent channels and cached I2C rows, and shows the
last resident status. Status older than 30 seconds is labelled stale; start
SENSOR to populate live discovery information. Settings are preserved while
hardware is unavailable. Optional I2C library absence does not prevent native
ZZ9000, RTC or Warp readings from being used.

## Repeatable FS-UAE validation

Run `python3 tools/run_sensor_qa.py` from the source tree with the normal Amiga
build toolchain and an imported AmigaOS 3.2.3 installation configured. Use
`--no-build` to test binaries already built. The tool creates a disposable,
offline guest, redirects its ENV assign, and supplies known AK_Sensors inputs.
It never mounts persistent guest drives or executes WHDLoad games.

The fixture checks the real SENSOR client's channel names, values, artwork keys,
manual switching and missing readings. It also runs the real WHDLoad indexer
against 129 dummy project drawers, then checks the normal dock's rendered
three-digit badge, channel text and independent footer. Before/after PPM images
and PASS/FAIL output are saved in the temporary QA output directory printed by
the runner. This tests the data and rendering paths without sensor hardware;
physical ZZ9000, Warp and I2C hardware still require device-specific validation.

## Side docks and compact tiles

SENSOR supports Bottom, Top, Left and Right placements. A crowded side dock may shorten Normal tiles to 46 pixels. The compact artwork layout keeps the selected sensor name and reading visible, and omits the redundant plugin-name footer when it cannot fit. Tiny mode keeps its icon and reading. Test both Normal and Tiny, including live and sad/unavailable states, idle and magnified on both side docks.
