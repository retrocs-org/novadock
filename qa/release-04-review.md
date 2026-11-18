# NovaDock 0.4 release review

The release date is 18 November 2026. NovaDock remains 0.4; AmiHASS remains 0.1.

The HASS chooser now browses the service's paged room catalogue, retains stable
area IDs, and provides All rooms including unassigned devices. Room, domain,
device and search filters combine. STATUS feature keys explicitly negotiate
room discovery, catalogue filters and subscriptions; an older service reports
that it needs updating instead of silently returning an unfiltered catalogue.
The wire ABI and message size remain unchanged. Stage the matching AmiHASS
service and SDK from the sibling AmiHASS project.

## Preliminary verification

- Full sanitized host suites passed for NovaDock and AmiHASS.
- Independent plugin versions cover all 42 bundled plugins changed since 0.3.
- Workbench 3.2.1, 68000, 8 MiB Fast RAM plus 2 MiB Chip RAM, actual 640×256:
  native room selection, sample controls, utilities and four complete dock
  startup/reload/remove/window/shutdown cycles passed. Public free memory was
  9,325,272 / 9,325,200 / 9,325,272 / 9,325,248 bytes after the four cycles.
  Evidence: `/private/tmp/nd-final-68000-runtime-03/junit.xml`.
- Native HASS UI tests dispatch refresh events before destroying each window,
  following the production event loop. Early test-only stalls led to this fix.
- Memory checks wait for the captured dock/plugin tasks to exit before comparing
  settled memory. Removing the public control port alone does not mean every
  asynchronous child has finished unloading.

- Workbench 3.2.1 with an explicitly selected 640×480, 8-bit screen passed the
  full native suite. After shutdown settled, public free memory remained
  approximately 12,236,700 bytes through all four cycles.
  Evidence: `/private/tmp/nd-final-640x480-runtime-02/junit.xml`.

## Exact final archive procedure

Build from the clean committed source using `scripts/release.py build`.
Prepare runtime inputs with `tools/prepare_runtime_qa.py --package <final-lha>
--bridge <matching-AmiHASSService> --output <new-input-folder>`.
Run its test.json with the 68000 profile and test480.json with the 68060 profile.
The latter explicitly applies a native 640×480 display mode and asserts the
actual dimensions; the emulator window size alone is insufficient.

Use `tools/prepare_release_qa.py` with the final LHA and the published 0.3 LHA.
Compile tests/native_release_check.c with src/control_client.c, src/config.c,
src/nd_options.c and src/interaction.c, then run its test.json in a disposable
guest only. Questions and notices receive fixed answers; original file/startup
operations remain intact. The helper compares installed files byte for byte,
tests fresh install, keep-settings removal/reinstall, complete removal,
0.3→0.4 upgrade and repeated update. It also checks Notes, imported plugin
artwork/themes/scripts, custom Notes and unrelated-file preservation.

Keep the final package SHA-256, JUnit reports and native transcripts beside the
release output. These final results are build artifacts, separate from this
source review. Publishing, tagging, pushing and website deployment are separate
explicit actions; the website keeps published downloads at 0.3 until 0.4 exists.

## Remaining physical checks

Physical WARP/ZZ9000/Prism behavior, actual light controls and physical-system
performance remain unverified. Sample IPC and emulation do not certify them.
AmiHASS's independent installer checklist remains in the AmiHASS project.
