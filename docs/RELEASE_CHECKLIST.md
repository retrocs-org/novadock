# Release checklist

## Prepare the 0.5 candidate

- Keep VERSION, the application, current changelog and release notes at 0.5.
- Check the AmiHASS STATUS feature declarations against the bundled HASS SDK;
  verify that an older service offers an explicit upgrade message.
- Pair with AmiHASS 0.2 for room/filter/subscription support; record its independent installer and SDK checks.
- Check independent plugin versions and the bundled manifest inventory.
- Regenerate the user/SDK guide; keep planning and publishing material source-only.
- Compile German and Swedish catalogs with FlexCat and include them in the LHA.
- Run the full host suite with memory checks, then build all native programs.
- Verify the LHA extracts correctly, all files match the staged drawer and the SDK is complete.
- Review the four release assets, checksums, licences and public uploader contact.

## Test on Workbench

- Fresh install: check every bundled plugin, Help, artwork, themes and catalogs.
- Upgrade from published 0.4 with custom paths, Notes, imported plugins/themes
  and several WHDLoad/MUSIC/SENSOR instances. Check preservation and new plugins.
- Uninstall keeping settings, reinstall and verify restoration; then uninstall
  removing settings. Unrelated files and explicitly external Notes must survive.
- Check Novice always offers Update/Uninstall; verify Average/Expert path choices.
- Verify Plugins opens before inventory finishes, remains closeable, and Refresh revalidates packages in the background.
- Test Save versus Use, recovery, restart, low-resource settings and small screens.
- Check large collection scans, cached startup, filtering and instance removal.
- Exercise startup, reload, plugin removal and shutdown in the disposable 68000,
  8 MiB Fast RAM profile at 640×256; repeat native controls at 640×480.
- Measure startup, idle CPU and memory on representative physical configurations.
- Test AMINET downloads, Weather, DOWNLOAD and rapid network reconnects on a working LAN.
- Compare SENSOR readings with the matching utilities on physical hardware;
  retain the documented validation limitation for boards that cannot be tested.

## Freeze and publish

- Review README, changelog, release notes, website information and screenshots.
- Keep Downloads driven by published stable GitHub tags; do not advertise an
  unpublished package as available.
- Review source for sensitive files and commit the exact release inputs.
- Build and prepare in a fresh output directory from the clean committed checkout.
- Review the manifest, exact tag 0.5 and destinations before explicitly publishing.
- Publish a normal latest release with the same four assets on both repositories.
- Verify the public downloads; submit the same LHA and matching readme to Aminet.

Portable checks and native compilation do not replace interactive Workbench or
physical hardware tests. Record results privately, including skipped tests,
configuration details and unresolved limitations. Never mark a deferred test passed.
See [RELEASING.md](RELEASING.md) for the release commands and recovery rules.

Installer QA derives Previous/Current versions from the supplied native executables,
records both archive hashes, and accepts the legacy and current installer names.
Use `tools/prepare_release_qa.py --previous PATH_TO_PREVIOUS_LHA --current PATH_TO_CURRENT_LHA --output FRESH_DIRECTORY --compiler "$VC"`.
The version updater checks this candidate number and selects the previous
published version from the local release catalogue, so future bumps cannot
silently leave this checklist on an old release.


## Write-failure checks

`tests/native_write_failures.c` calls the packaged COLOUR, QR and BENCH workers
through their normal task protocol. Run it only with Amiga Workbench `--test`
and the `QATests:ISOLATED-QA-ONLY` marker. The fixture must contain the extracted
`NovaDock` package beside the compiled `WriteFailures` harness. It redirects
ENVARC to its private test drawer and uses `C:Lock QATests: ON/OFF` to test real
DOS write protection. RAM: does not implement this lock in the test environment.
The harness restores write access on failure and never targets user settings.

Verify that errors identify the destination problem, edits/results remain,
previous files survive, no partial output remains and saving works after unlock.
The sanitized host suite also injects disk-full, short-write, close and rename
failures and checks recovery when palette rollback itself fails. Those injected
checks do not claim a physical full-disk test.

The Installer fixture exercises all three logical user-level branches at standard
locations. It substitutes questions, notices and logical user-level references, and keeps
primitive-operation user mode at Novice to suppress built-in Expert prompts;
real file/startup/update/removal operations remain intact. Its isolated welcome
helper acknowledges only the initial welcome for the generated QA scripts.
Interactive path selection and physical-hardware checks remain separate.


## Local verification — 2026-11-21

NovaDock 0.5 preflight passed the sanitized host suite and native write-failure
checks. The final 0.4-to-0.5 Installer fixture passed all three logical branches
at standard locations (3,726 file/preservation checks). Archive SHA-256:
`8cb98b4a09b4e2394b7300e5b01c403e1c7e6fddeef2872ffacd5e53572a2daa`.
Selected logs and scope are retained locally in
`outputs/qa-0.5-write-preflight-20261121`. Physical hardware and interactive
custom-path checks are not marked complete by these automated tests.


## Custom paths and BENCH retry

Use the same `tools/prepare_release_qa.py` command with `--layout custom` to
exercise Average application/Preferences paths and Expert shared-data paths.
The fixture selects drawer names containing spaces on SYS: and on the separate
copied QATests: volume. Questions receive scripted answers; native Installer
file, startup and removal operations remain intact. The harness checks installed
bytes, active assigns, quoted persistent assignments and WBStartup's DefaultTool.
Unrelated files in the selected parent drawers must survive every removal.

The 2026-11-21 custom-location run passed 2,494 file/preservation checks, including
0.4-to-0.5 update, repeated update, keep-settings removal/reinstall and full removal.
Its exact archive identity and selected transcripts are recorded in
`outputs/qa-0.5-retry-custom-20261121`. This extends the earlier default-location
coverage; physical Amiga testing remains separate.

The native write-failure harness also retries the retained BENCH report while
protected and after unlock. Its timestamp, measurements, path and text must
remain identical; no benchmark stages run during save retry. BENCH's
PANEL-SELFTEST checks the actual Stop/Retry save enabled states and action-row
bounds through GadTools on the selected Workbench screen.


BENCH retry and actual control bounds passed on the exact package in native
68000/640×256 and 68060/1024×600 guests. The current local archive is
`outputs/release-0.5-retry-custom-final-20261121/NovaDock-0.5.lha`, SHA-256
`a4373acdde6a56cc46f38d433a5783a1c300fa12f95f9fb22bc2319684919e10`. No production Installer fix was required for the
custom-location cases. The earlier default-location record remains historical;
use this newer archive for subsequent local checks.


## AmiHASS lock and door integration

The NovaDock 0.5 / AmiHASS 0.2 SDK pairing check and sanitized host checks pass.
The native sample-service run exercises Lock/Unlock/latch actions, garage
open/stop/close, and the actual HASS confirmation popup. Cancel, Escape and
window-close preserve the sample lock state; Confirm alone changes it.
The service reports zero HTTP requests. No real locks or doors are tested.
Selected reports are retained in the local release QA output.

The HASS UI test requires the ISOLATED-QA-ONLY marker and a verified sample
service. It may stop an autostarted NovaDock only in the disposable test guest
before creating its simulated dock host. Physical-hardware checks remain
unverified; these results do not claim live lock/door integration testing.
