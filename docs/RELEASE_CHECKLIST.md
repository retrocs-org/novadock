# Release checklist

- Set VERSION and its release date, update CHANGELOG, and regenerate version metadata.
- Build all programs and run the host and native source checks.
- Verify the LHA extracts correctly and matches the staged drawer.
- Test a fresh install, updating existing settings and uninstalling with settings retained.
- Open Preferences, Help, Diagnostics and each bundled app on supported screen sizes.
- Check the low-resource preset, configuration recovery and Save/Use behavior.
- Test Clipboard search, empty results, pinning and copying; test Apps title selection.
- Check drawers, icon loading, keyboard navigation and auto-hide.
- Verify optional hardware and network dependencies on the target configuration.
- Review release notes, uploader details, repository targets and download checksums.
- Publish from a clean committed checkout using the release tool.

Host tests and compilation do not replace interactive checks on the supported
AmigaOS configurations. Record test results privately and retain any open issues.
