# Release checklist

## Prepare the 0.3 candidate

- Keep VERSION, the application, current changelog and release notes at 0.3.
- Check independent plugin versions and the bundled manifest inventory.
- Regenerate the user/SDK guide; keep planning and publishing material source-only.
- Compile German and Swedish catalogs with FlexCat and include them in the LHA.
- Run the full host suite with memory checks, then build all native programs.
- Verify the LHA extracts correctly, all files match the staged drawer and the SDK is complete.
- Review the four release assets, checksums, licences and public uploader contact.

## Test on Workbench

- Fresh install: check every bundled plugin, Help, artwork, themes and catalogs.
- Upgrade from published 0.2 with custom paths, Notes, imported plugins/themes
  and several WHDLoad/MUSIC/SENSOR instances. Check preservation and new plugins.
- Uninstall keeping settings, reinstall and verify restoration; then uninstall
  removing settings. Unrelated files and explicitly external Notes must survive.
- Check Novice always offers Update/Uninstall; verify Average/Expert path choices.
- Test Save versus Use, recovery, restart, low-resource settings and small screens.
- Check large collection scans, cached startup, filtering and instance removal.
- Measure startup, idle CPU and memory on representative slower configurations.
- Test AMINET downloads, Weather, DOWNLOAD and rapid network reconnects on a working LAN.
- Compare SENSOR readings with the matching utilities on physical hardware;
  retain the documented validation limitation for boards that cannot be tested.

## Freeze and publish

- Review README, changelog, release notes, website information and screenshots.
- Keep Downloads driven by published stable GitHub tags; do not advertise an
  unpublished package as available.
- Review source for sensitive files and commit the exact release inputs.
- Build and prepare in a fresh output directory from the clean committed checkout.
- Review the manifest, exact tag 0.3 and destinations before explicitly publishing.
- Publish a normal latest release with the same four assets on both repositories.
- Verify the public downloads; submit the same LHA and matching readme to Aminet.

Portable checks and native compilation do not replace interactive Workbench or
physical hardware tests. Record results privately, including skipped tests,
configuration details and unresolved limitations. Never mark a deferred test passed.
See [RELEASING.md](RELEASING.md) for the release commands and recovery rules.
