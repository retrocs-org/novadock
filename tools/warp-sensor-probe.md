# Experimental Warp sensor probe

This standalone AmigaOS tool exercises a recovered diagnostic call in
cswarp.library **30.74 only**. It is excluded from the dock, installer and
normal release targets. The interface is inferred from the supplied vendor
binaries, not a vendor-supported SDK. Physical hardware comparison is pending.

## Recovered interface

The library function table at text offset `0x51B0` has the four standard library
entries followed by application functions. Entry 6, at `0x51C8`, points to
`0x0808`: library vector **-42**, library base in **A6**, output buffer in
**A0**, success in the **low byte of D0** (zero means failure).

WarpTool independently confirms the call at text offset `0x80B0`, after
allocating and clearing **428 bytes**. The library converts the ARM diagnostic
frame into a different result layout; do not pass the raw mailbox structure.
The highest observed result write is a longword at byte 424.

| Result byte offset | Reading | Representation |
| --- | --- | --- |
| 37 | Fan output | Unsigned byte, percent |
| 44 | Internal 68060 temperature | Big-endian IEEE-754 binary32, Celsius |
| 48 | External CPU probe | Big-endian IEEE-754 binary32, Celsius |
| 52 | ARM temperature | Big-endian IEEE-754 binary32, Celsius |
| 56 | FPGA temperature | Big-endian signed 32-bit whole Celsius |
| 60 | Main supply | Big-endian IEEE-754 binary32, volts |
| 64 | Backup battery | Big-endian IEEE-754 binary32, volts |

The library writes these fields at text offsets `0x08CE` through `0x08FC`.
WarpTool reads the matching fields at `0x81FA` through `0x82B8`; it formats
FPGA temperature as an integer and the other temperatures/voltages as floats.
The probe's decoder uses integer arithmetic, handles invalid floats, preserves
valid zero readings and checks the same channel bounds as SENSOR. Values are
rounded to the displayed precision: 0.1 C or 0.01 V.

## Limits and private data

The call also fills unrelated configuration fields, including Wi-Fi credentials.
The probe never writes or prints the full result or board settings: it decodes
only the seven fields above, then clears the complete buffer through volatile
writes. Guard bytes check the observed buffer size. Never add a raw frame dump
to a report or test fixture.

The inspected command path uses a semaphore, the library's shared interrupt
channel and blocking Exec Wait calls. No bounded timeout or reliable Ctrl-C
escape was verified in that path. A stalled board can therefore leave this
standalone probe waiting; do not invoke it from the dock's input loop or run
repeated probes after a stall. This is why automatic native sampling has not
been enabled. The diagnostic call requests information; it does not issue
firmware-update, reset, clock or fan-setting commands. Library initialization
and communication are still managed by the installed vendor library.

Only version 30.74 is accepted, including its revision check. Other versions
return an unsupported message without calling the vector. Establish the layout,
write footprint and cancellation behaviour for each additional supported version.
The probe does not install or replace cswarp.library or firmware.

## Build and test

With the project's Amiga cross-compiler configured, from the project root:

```sh
sh scripts/build-amiga.sh outputs/diagnostics/WarpSensorProbe
```

Copy that executable to the Warp-equipped Amiga running the matching library.
From its Shell, request one sample:

```text
WarpSensorProbe --read-sensors
```

Compare all seven values with **WarpTool Diag** taken shortly afterward. Without
arguments, the probe prints usage without opening the library. Keep the short
sensor-only output and any error message; do not save a raw diagnostic buffer.
Record the board model, library and firmware versions with the result.

Portable decoder tests run as part of `tests/run_host_tests.sh`. They cover the
version gate, mixed float/integer layout, units, rounding, signed values, zero,
invalid floats, bounds, short buffers, stale readings and unrelated-field
isolation. Native compilation and those tests do not certify the recovered ABI
on a physical board.

## Inspected inputs

- cswarp.library 30.74, 64108 bytes, SHA-256
  `22fa4d8a1ce36f0b53f2c7eeb348dd730c7bf6dc6e2a14b13177210045d3226b`.
- WarpTool 30.74, 117064 bytes, SHA-256
  `030f1fe59cf2b22b49096991d590a8cb895d993559acd0ddaee253ac39cebf77`.
- Both identify as 8 November 2026, from firmware pack `warp_fmw_3074.zip`.

Analysis used IRA 2.09 and m68k-amigaos-objdump with the 68060 instruction set.
Addresses above are code-section offsets, not file offsets or runtime addresses.
Vendor binaries and their full disassemblies are not included in the project.
