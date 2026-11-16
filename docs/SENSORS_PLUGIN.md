# Sensors plugin

Add **SENSOR** from **Add item > Plugins**. Add multiple independent tiles, such
as Temperatures and Voltages, each with its own label and selected channels.
It detects supported readings and
rotates selected channels in the dock every five seconds. Click for a live,
paged list with **Previous**, **Next**, **Refresh** and **Close**.

Right-click and choose **Edit settings**. Freshly detected channels appear as
checkboxes, checked initially. Untick readings you do not want displayed.
**Tile label** names each instance. A sad chip icon appears when no available
readings are selected, or no sensors are detected. **Use** applies for the current session; **Save** keeps the selection after
restart. Missing channels are hidden without erasing their saved choices.
Open settings again after connecting hardware or starting a sensor driver.
The title says **no sensors detected** when no supported readings are available.

## Supported sources

### MNT ZZ9000

Reads FPGA temperature, core voltage and auxiliary voltage directly from a
ZZ9000 found through expansion.library. Firmware must expose the telemetry
registers (1.7 or later). It does not probe arbitrary addresses, write board
registers, or change firmware, video modes or clock settings. Temperature is
shown in Celsius and voltage in volts. Invalid register values are omitted.

The register identities, firmware version field and engineering units are
verified against the public [ZZTop board code](https://github.com/BlitterStudio/zz9000-drivers)
and its register definitions (ZZTop/Core/board.c and include/zz9000_hw.h;
reviewed commit f60eafa28f36e64cee052c5c02d22a097a3553f9).

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
No board settings, fan controls, overclocking or WiFi settings are changed.
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
