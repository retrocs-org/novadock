# Warp1260 controls and Prism Wi-Fi interfaces

These are implementation/review notes for the 0.4 development tree. Usage is
in docs/NETWORK_PLUGIN.md and docs/USING_NOVADOCK.md. Physical-card testing is
still pending; host tests and compilation establish no hardware certification.

## Warp1260: recovered 30.74 ABI

Inputs and SHA-256 hashes match tools/warp-sensor-probe.md. Both the supplied
cswarp.library and WarpTool were inspected independently. No vendor executable,
firmware or disassembly is distributed with NovaDock.

The library has four standard entries before its application function table.
A6 is the library base, A0 points to the argument, and low D0 is a Boolean
result. The implementation rejects versions other than exactly 30.74.

| Operation | Vector | Library text target | WarpTool call |
| --- | --- | --- | --- |
| Diagnostics | -42 | 0x0808 | 0x80B0, 0x118DC |
| Set SSID | -66 | 0x0D58 | 0x167AA |
| Set password | -72 | 0x0DD4 | 0x167B6 |
| Set frequency | -282 | 0x4680 | 0x11980 |

SSID/password arguments are NUL-terminated strings. The library copies up to
128 bytes, stopping advancement at the first NUL and padding with NULs. Its
command opcodes are 10 and 11. WarpTool sends SSID before password, always as
a pair. NovaDock stops if SSID fails; a password failure is reported as a
potentially partial operation, without an automatic rollback or retry.

The diagnostic result is 428 bytes. Model is at 0..31, SSID at 84..211, and
current clock at 382..385 (big-endian IEEE binary32 MHz). Password occupies
212..339. Only model, SSID and validated clock are copied to replies. The full
result is cleared before any setter. No raw diagnostic output is logged.

WarpTool 30.74 uses CPUFreqHz, not its older CPUTurboLevel command. It converts
Hz to binary32 MHz and builds a 10-byte argument: float at 0, zero long at 4,
word 1 at 8. The library copies the float to wire offset 4 and argument byte 9
to wire offset 12, using command 36. NovaDock uses fixed binary32 representations
for the legacy Warp1260 levels, 50/66.6/75/85/90/95/100/105 MHz, and the same
save flag. It does not infer an active level from an approximate clock.

WarpTool finds the control device with expansion.library FindConfigDev using
manufacturer 0x1400 and product 60. This identifies the Warp family only.
The menu uses that passive detection; the worker additionally checks the exact
NUL-terminated Warp1260 model before enabling writes. Warp1240, Warp560,
unknown models and other library revisions are not accepted by this backend.

Library transport includes unbounded waits. Calls run only in a separately
loaded worker. A global port serializes NovaDock's Wi-Fi/clock operations.
The UI hands a private message to the worker and clears its password field.
After handing off a request, a closed or timed-out UI never frees worker-owned
data, kills the call or starts a competing retry. The worker clears credentials
and results before freeing memory; if the UI is gone it disposes of the reply.
A blocked library call can still require the user to resolve a board/library
fault before another native operation. Normal CPU/Network sampling never calls
these functions.

## Prism: documented interface, independent implementation

zenPrismWifi 0.4 includes GPL source. Its driver/scanning/configuration approach
was reviewed; no GPL implementation code, utility executable or MUI dependency
is included in NovaDock. NovaDock implements the public interface independently.
The Prism2 2.9 driver's request.c and unit.c provide the authoritative cross-check.

- S2_GETNETWORKS = 0xC011. ios2_Data is an Exec pool; ios2_StatData supplies the
  input taglist. On success, StatData is an array of taglist pointers and
  DataLength is its count. The worker destroys the result pool only after the
  completed I/O request and device close.
- S2INFO_SSID = TAG_USER+0, Channel = +7 and Signal = +8. SSID is a string.
  Use this tag; a BSSID is only a six-byte MAC address. The reviewed chooser's
  assumption that an SSID follows BSSID at byte 8 is not used by NovaDock.
- The driver requires an online interface to scan. NovaDock does not issue
  S2_ONLINE, S2_OFFLINE or S2_CONFIGINTERFACE. It shares the driver normally,
  leaves transfer callbacks unset because it does no packet I/O, and keeps all
  waits outside the resident/UI processes.
- Profiles use WirelessManager's network blocks: hex SSID, hex WPA PSK or
  key_mgmt=NONE, and priority=1000. PBKDF2-HMAC-SHA1 uses 4096 iterations and
  a 32-byte result; standard IEEE/password vectors are tested. SSID hex avoids
  configuration quoting/injection. Only the NovaDock-marked block is replaced.
- ENV and ENVARC inputs are read separately and preserved independently. Both
  new files are staged and the inputs rechecked before renames. Originals are
  kept until both commits succeed; a second-file failure triggers rollback.
  Unknown pending transaction files are refused. Derived keys remain sensitive.

The first implementation supports one managed profile, printable ASCII SSIDs
and WPA personal passwords, and at most 32 displayed scan results. It does not
restart WirelessManager, manage its startup entry or claim association success.

References:

- https://aminet.net/package/comm/net/zenPrismWifi_v0.4
- https://aminet.net/driver/net/prism2v2.lha
- https://wiki.amigaos.net/wiki/SANA-II_Revision_7
- FIPS 180-4 (SHA-1), RFC 2104 (HMAC), RFC 8018 (PBKDF2).
