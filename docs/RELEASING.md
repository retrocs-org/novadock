# Releasing NovaDock

Use Python 3.9+ from the project root. The entry point is `scripts/release.py`;
`tools/release.py` remains a compatible entry point. Build once, prepare an exact
manifest, then publish those same verified bytes. Review the notes, destinations
and four download files before publishing.

NovaDock remains **0.4** until a version change is agreed. Version 0.4 is in development. Published tags through 0.3
already exist; the new-release examples below do not authorize replacing them.
Normal release commands never move tags, overwrite conflicting assets, create
repositories, deploy the website or roll back another service's release. Source
branch pushes are a separate, explicitly confirmed command.

## Contents

- [GitHub and GitLab release guidelines](#github-and-gitlab-release-guidelines)
- [Aminet submission rules](#aminet-submission-rules)
- [Configure locally](#configure-locally)
- [Committed source only](#committed-source-only)
- [Build, check and prepare](#build-check-and-prepare)
- [Credentials and publication](#credentials-and-publication)
- [Receipts and interrupted runs](#receipts-and-interrupted-runs)
- [Testing and limits](#testing-and-limits)
- [Build output](#build-output)
- [GitHub App installation credentials](#github-app-installation-credentials)
- [Submit an already published release to Aminet](#submit-an-already-published-release-to-aminet)

## GitHub and GitLab release guidelines

### Version, tag and publication status

- VERSION, the application, Preferences, package metadata and current release
  notes must agree. Plugins have independent versions: update a changed plugin's
  own version and metadata rather than copying the dock version to every plugin.
- Use the exact version as the public tag, such as `0.4`, without a `v` prefix.
  New preparations default to the bare VERSION. Pass `--tag` explicitly to pin
  the intended tag; subsequent commands reuse an existing manifest's tag.
- Both repositories receive the same annotated tag object and source commit.
  Never force-push, move or recreate a published tag through the normal workflow.
- Release titles use the application and version, for example **NovaDock 0.4**;
  the Git tag remains **0.4**. Publish normal releases, including versions
  below 1.0. GitHub uses `prerelease: false` and explicitly selects Latest with
  `make_latest: "true"` after all assets verify. A draft is only an upload stage.
- GitLab uses its ordinary published-release API, with no future release date or
  experimental status. GitLab orders the latest release by `released_at`; it has
  no equivalent prerelease/Latest checkbox in this API. Do not future-date releases.

### Release notes

CHANGELOG.md is the historical change list. docs/RELEASE-NOTES.md contains only
the complete current-version changelog for the GitHub and GitLab descriptions.
Keep the application overview, installation, requirements and source/licence
information in README.md. Refresh and check the release description before
committing release inputs:

```sh
python3 tools/release_notes.py --write
python3 tools/release_notes.py --check
```

Use this structure on both services:

```markdown
### Features

- New functionality introduced in this version.

### Enhancements

- Improvements to existing functionality or performance.

### Bugs

- Problems corrected in this version.
```

Start directly with Features (or the first nonempty change group). Do not add
Version Changelog, a version heading, an application overview or installation
text; the release title already identifies the version. Keep every current-version
change, without older releases or an unfinished placeholder. The generator maps CHANGELOG's `Bug fixes` heading to
`Bugs`; nonempty groups retain the order shown above. The first public release
lists its initial features and omits empty fix groups. A custom --notes file
must retain this complete changelog-only structure.

Aminet remains separate: its plain-text readme still includes the required
metadata, application features, installation, source/licence and changelog.
Published title/description corrections do not require moving tags or rebuilding
downloads. Retain a separate correction receipt; do not rewrite old package
manifests or publication receipts to disguise a metadata-only update.

### Assets and filenames

Upload exactly these four files, substituting the agreed VERSION:

```text
NovaDock-0.4.lha
NovaDock-0.4.lha.sha256
NovaDock-0.4-plugin-sdk.zip
NovaDock-0.4-plugin-sdk.zip.sha256
```

The LHA contains the installable application, all bundled plugins, themes, help
and licence. The SDK ZIP contains the standalone public Plugin SDK. Each checksum
is its SHA-256 digest, two spaces, exact filename and a newline. Both repositories
receive identical package and checksum bytes. GitLab exposes verified generic
package files through release asset links.

Use the hosts' automatic tagged source ZIP/tar archives. Do not upload another
source ZIP, website ZIP, aggregate checksum list, readme, notes, log, manifest or
receipt. NovaDock still generates extra source/site packages locally to validate
archive contents and retain build evidence; these are not hosted release assets.
Hosted CI configuration is not a release prerequisite. Checks run locally.

### Preparation and publication

1. Finish code, documentation, translations, website information and changelog.
   Refresh generated inputs, review for sensitive files, then commit them.
2. Confirm both configured repository destinations and access. Push reviewed
   source separately if needed.
3. Build in a fresh output directory. Check packages, native verification,
   installer and SDK inventory. Complete target-system runtime testing separately.
4. Prepare the exact tag and notes. Review manifest.json and the four hashes.
   Changes to release inputs after building require a fresh build.
5. Publish explicitly using the same output directory and tag. Preserve receipts
   and verify the release links. Website deployment remains a separate manual task.

Example for a new 0.4 tag:

```sh
python3 scripts/release.py source --to gitlab,github --dry-run
python3 scripts/release.py source --to gitlab,github --confirm-push
python3 scripts/release.py build --output outputs/release-0.4-new
python3 scripts/release.py check --output outputs/release-0.4-new
python3 scripts/release.py prepare --output outputs/release-0.4-new \
  --tag 0.4 --tag-message "NovaDock 0.4 release" --notes docs/RELEASE-NOTES.md
python3 scripts/release.py publish --output outputs/release-0.4-new \
  --tag 0.4 --to gitlab,github --confirm-publish
```

`--notes` belongs to `prepare` or the combined `release` command, not `publish`.
`--tag-message` sets the annotation and is preserved in the manifest; if passed
again, it must match. Changing the tag does not change embedded versions or names.
Custom tag names are validated by Git and get matching links in generated notes.

### Interrupted publication and corrections

Resume the same prepared release after inspecting its failure. Matching verified
assets may be reused; conflicting assets or notes stop publication. Do not delete
receipts, edit fingerprints or disable checks to force a retry. One completed
service is not automatically rolled back if another fails.

Prefer a new patch release for package corrections. A reviewed notes-only edit
can update a release description without changing downloads or moving its tag;
that is a separate maintenance operation. Explicitly authorized package replacement
is also a separate recovery operation: retain originals and hashes, explain the
correction, verify identical replacements everywhere and preserve recovery records.
Check whether changed distributed bytes require another Aminet submission.

## Aminet submission rules

Aminet uses stable, case-sensitive **lowercase** filenames:

```text
novadock.lha
novadock.readme
```

No version belongs in these names; the readme's Version field identifies it.
Repository assets retain their versioned names. The Aminet LHA is an unchanged copy
of the verified runtime archive, renamed without rebuilding its contents. Never
submit the SDK ZIP as the application archive.

### Readme format

`tools/release_notes.py` generates the readme before the runtime LHA is packed;
`scripts/make-release.sh` invokes it during the normal build. The same readme is
retained beside the LHA and staged under the stable Aminet name. The formatter
supports an optional reviewed `AMINET_REPLACES` header.

New readmes use plain ASCII, readable indentation and at most 76 columns. No
Markdown links, heading markers or decorative Unicode belong in this text.
Headers use this order, with the configured public author/uploader:

```text
Short:        Extensible application dock
Author:       NovaDock contributors
Uploader:     frossmant@icloud.com
Type:         util/wb
Version:      <VERSION>
Architecture: m68k-amigaos >= 3.0.0
Requires:     AmigaOS 3.0 or later, any 68000 or better
Distribution: Aminet
URL:          https://novadock.retrocs.org
```

A short description follows, then FEATURES, INSTALLATION, SOURCE AND LICENCE,
and VERSION CHANGELOG. Installation explains `novadock.lha`, Install-NovaDock,
Installer 42+, the Shell alternative and help location. VERSION CHANGELOG includes
every current-version entry, grouped under Features, Enhancements and Bugs with
indented bullets; older releases are excluded. The uploader contact is public.

### Preparing and uploading

Build prepares the pair locally without a network upload. `aminet` previews a
prepared bundle; `publish --to aminet --confirm-publish` explicitly submits it.
Use a fresh output directory when changing readme or naming rules. Frozen previous
submissions retain their original checksummed bytes and compatibility validation;
they are not silently rewritten to match a newer template.

The FTP adapter inspects the incoming listing before transferring. It refuses an
unreadable listing or either existing stable filename rather than overwriting an
unknown pending submission. Resolve blocked incoming files with Aminet before
retrying; do not remove local receipts to bypass a partial upload.

FTP acknowledgement means awaiting moderator review, not public publication.
Changing to stable filenames does not automatically remove historical versioned
entries. Review any migration/removal separately with Aminet.

## Configure locally

Default external configuration:

```text
~/Development/tools/git-release/projects/novadock.json
```

Use `NOVADOCK_RELEASE_CONFIG` or `--config PATH` to select another file.
An existing ignored `work/release-config.json` is a compatibility fallback when
that shared file is absent. `examples/release-config.json` is the public template.
Keep real machine paths, private repository destinations and credential selectors
outside public source. The JSON supports `github`, `github_git`, `gitlab`,
`gitlab_git`, `author`, `uploader`, `source_branch`, `env` and `credentials`.
There is no `aminet` enable switch: selecting the destination controls submission.

Public repository: https://github.com/retrocs-org/novadock.
Configure the private GitLab HTTPS URL and matching SSH push URL externally.
Credential-bearing URLs, non-HTTPS API origins, traversal, queries and mismatched
push destinations are rejected. Prepared manifests record destinations and must
remain private if they identify an internal repository.

Allowed `env` entries are `VBCC`, `VC`, `LHA`, `CC`, `PHP`, `FLEXCAT`, `AUTHOR`,
`AMINET_REPLACES` and `NOVADOCK_RELEASE_PYTHON`. Provision vbcc/vasm/vlink for
68000 AmigaOS and a compatible NDK, a host C compiler, PHP and compression-capable
LHa for UNIX. The preferred Python interpreter is used for shell staging and
host checks too. This Amiga-only build has no Mac signing/notarization target.

Set `uploader` to `frossmant@icloud.com`, or the reviewed real release contact;
this value appears in the readme and anonymous FTP login. `--uploader` can override
it during build/preparation. The default contact is frossmant@icloud.com.

## Committed source only

```sh
python3 scripts/release.py source --to gitlab,github --dry-run
python3 scripts/release.py source --to gitlab,github --confirm-push
```

The source command preflights all selected branches, pushes the exact committed
source without force/mirror/follow-tags, and verifies the remote commit. It does
not compile, create release tags or upload assets. `source_branch` selects the
remote branch; empty uses the current branch. Existing repositories are preserved.

For an extracted source directory, `source --init --dry-run` checks that every
selected remote is empty, Git identity exists and .gitignore is reviewed, without
creating local metadata or a commit. An explicitly confirmed `--init` imports
only the selected public source inventory. It never replaces existing history.
Use `--commit-message` to customize the first commit. Clone an established remote
and import changes there instead of initializing a competing history.

## Build, check and prepare

Release packaging requires [FlexCat](https://github.com/adtools/flexcat). Set
`FLEXCAT` in the external configuration to its executable, or put `flexcat` in
PATH. Missing compilers and invalid compiled catalogs stop the release; local
English-only staging remains available. Check the packaged German and Swedish
catalogs as well as the built-in English text.

When their source inputs change, refresh generated files before committing:

```sh
python3 tools/update_version.py
python3 tools/update_site_chrome.py
python3 tools/release_notes.py --write
python3 tools/generate_catalog.py
python3 tools/make_amigaguide.py
python3 tools/generate_info_icons.py
python3 tools/package_release.py
```

Review independent plugin versions with
`python3 tools/update_plugin_versions.py --check` when plugins change. Commit source and generated files, then run:

```sh
python3 scripts/release.py plan
python3 scripts/release.py build --output outputs/release-0.4-new
python3 scripts/release.py check --output outputs/release-0.4-new
python3 scripts/release.py prepare --output outputs/release-0.4-new --tag 0.4
```

Use the same output directory on publication. Default output is `outputs/release`;
custom paths must be outside source or under ignored `outputs/`. Build requires
an empty directory and always compiles fresh binaries. Preserve failed output and
use a fresh directory after fixing inputs; there is no unified skip-build option.
For compatibility, `prepare` builds if no build record exists.

Build preflight is read-only: it checks catalogs, Help, Workbench icons and the
SDK copies without regenerating tracked files. It does not add upcoming releases
to public-web/releases.json. The download page discovers published GitHub releases
automatically; refreshing that fallback list is a separate website-maintenance
operation. Stale generated inputs produce an actionable error before the output
directory is created. Review and regenerate those inputs before committing.

Build checks the version, committed release-note template, translations and host
tests, then compiles and verifies all **39 native programs**. A release needs
manual target-system validation recorded against `docs/RELEASE_CHECKLIST.md` too.
Tests and compilation do not certify every supported configuration.

Source fingerprints include code, tests, tools, docs, website inputs and plugin
metadata. Generation that changes tracked inputs must be reviewed and committed
before another build. Changing any input, including documentation, makes existing
builds stale. Known credentials, common repository-token/private-key formats and
tracked private configuration files block build/source publication. Matched secret
values are not printed; this is not a comprehensive source/history security audit.

Records bind source commit, compiler identity, native binaries and package hashes.
The runtime LHA uses classic level-1 headers and LH5 compression, with extraction
and per-file byte comparisons; LHa for UNIX is required. Source/SDK/site ZIPs are
also checked against expected contents. There is no stored-archive fallback in
NovaDock's runtime packaging path.

Cross-compilation keeps temporary assembly and object files in isolated,
ignored `build/intermediates/` workspaces; compiler outputs are excluded from
Git and source packages. Handwritten assembly remains part of the source.

`tools/check_package_assets.py` verifies each declared plugin artwork bank,
its executable and matching manifest after LHA extraction. The SDK check
extracts the exact standalone inventory and rebuilds ArtHello's artwork from
its included JSON using the shipped format. Missing, changed or invalid banks,
incomplete examples and unexpected ZIP entries stop verification.

An extracted tree may build local packages with a null commit. Repository
publication requires the original clean committed checkout, tracked inputs and
exact prepared commit. Do not edit provenance to bypass this requirement.

## Credentials and publication

API credentials come from environment variables or 1Password. They are fetched
only for selected repository publication destinations. Builds and source pushes
do not fetch API tokens. Never put tokens, passwords or private keys in JSON,
notes, command arguments or shell history; do not enable shell tracing.

```json
"credentials": {
  "github": {"provider": "env", "variable": "GITHUB_TOKEN"},
  "gitlab": {
    "provider": "1password",
    "vault": "YourVault",
    "item": "gitlab.example.com",
    "field": "your-group/novadock-rw",
    "account": ""
  }
}
```

Alternatively configure `provider: "1password"` with a copied `reference:
"op://YourVault/YourItem/YourField"` and optional account. Use one lookup form;
keep vault/item/field separate from a reference. Sign the CLI in or connect its
desktop app. Missing, cancelled or ambiguous lookups stop without exposing output.
Use project-specific credentials rather than another project's token or a broader
administration credential. Git source/tag pushes use normal SSH keys or helpers.

GitHub requires Contents write permission. GitLab needs `api` scope and Developer
access or higher for releases and generic packages; protected tags may need further
access. Repositories must already exist with the intended visibility and GitLab
package registry enabled. The tool does not configure hosting or access policies.

```sh
python3 scripts/release.py publish --output outputs/release-0.4-new \
  --tag 0.4 --to gitlab,github --confirm-publish
python3 scripts/release.py publish --output outputs/release-0.4-new \
  --tag 0.4 --to aminet --confirm-publish
```

Choose distinct destinations, optionally `--to gitlab,github,aminet`. Default is
GitLab and GitHub; `--confirm-publish` is mandatory (`--execute` is a compatibility
alias). The combined `release` command prepares then publishes the same manifest.

Before repository mutations the publisher checks source, clean commit, destinations,
credentials, remote tags, existing assets and notes. It pushes only the pinned
annotated tag object and verifies its remote commit. Every destination rechecks
local artifacts. It does not push branches as part of release publication.

GitHub stages files in a draft, verifies returned SHA-256 digests, then publishes
as a normal Latest release and checks the Latest endpoint before success.
Identical existing complete releases may be reused
and their normal/Latest status set explicitly. Unknown hashes, changed notes,
unexpected assets or an incomplete published release stop the run. Draft discovery
checks the most recent 100 entries before looking up the tag. No `gh` is required.

GitLab uploads versioned generic packages and stream-downloads their bytes to
verify hashes before creating the normal release and its four links. Existing
bytes, notes and links must match. The publisher also confirms this tag is the
latest by release date before recording success; future-dated releases are refused.
API writes reject redirects; package downloads
do not forward API credentials to another origin. No `glab` is required.

Aminet uses anonymous, unencrypted FTP to main.aminet.net `/new`, separate from
the HTTPS repository APIs. Only an explicit Aminet destination submits the pair.

## Receipts and interrupted runs

The output directory holds `manifest.json`, its `release.json` compatibility alias,
`BUILD-INFO.json`, `build-state.json` and `receipt.json`. The publication receipt
binds the manifest hash, completed destinations and current destination. A changed
manifest cannot reuse it. Keep all records and inspect failures before resuming.

Aminet has a separate `aminet/upload-receipt.json`, bound to its submission hashes.
It records `in_progress` before the first FTP transfer and acknowledgement after
each file; `completed` means the pair was acknowledged, awaiting moderation.
An incomplete receipt blocks automatic retry because a transfer might have arrived
without its acknowledgement. A completed receipt prevents duplicate submission.

Do not delete receipts, rebuild into a published directory or edit fingerprints to
force a retry. The checked inventory also prevents unknown files entering a bundle.
Use a new output directory for subsequent builds; retain old bundles as evidence.
There is no automatic retry or cross-service transaction.

## Testing and limits

```sh
python3 tests/test_release_notes.py
python3 tests/test_release_tool.py
python3 tests/test_release_provenance.py
python3 tests/test_release_integration.py
python3 tests/test_aminet_release.py
python3 tests/test_release_catalog.py
```

These checks use temporary Git repositories, local files and mocked APIs/FTP;
they never publish real releases. They cover the complete notes/readme layout,
normal/Latest GitHub flags, bare tags, four-asset inventory, provenance, interrupted
runs and pending Aminet filename protection. Exercise hosted adapters against
dedicated test repositories before production use after substantial API changes.

Checks establish matching source and bytes, not reproducible toolchains or
universal runtime compatibility. Services cannot publish atomically, and remote
changes after preflight can still stop a later destination. Credential checks
cannot replace a repository/history audit.

API references: [GitHub releases](https://docs.github.com/en/rest/releases/releases),
[GitHub assets](https://docs.github.com/en/rest/releases/assets),
[GitLab releases](https://docs.gitlab.com/api/releases/),
[GitLab generic packages](https://docs.gitlab.com/user/packages/generic_packages/).

## Build output

Four hosted assets sit directly in the selected output directory. Extra local
source/site packages, notes, readme, checksums and build records remain there for
verification. The stable Aminet pair and submission.json are under its `aminet/`
subdirectory, not attached to repository releases. The SDK ZIP has one
`NovaDock-PluginSDK` drawer, public headers, C/ARexx examples, guides, packaging
tools and MIT licence; private settings and generated example binaries are excluded.

Build prints compiler/test output directly; NovaDock currently has no `--verbose`
flag or automatic retained build log. Capture terminal output externally if needed,
without shell tracing or secret-bearing environment dumps. Credentials are stripped
from build child environments; API credential responses are not printed.

Deploy the PHP website manually from `public-web/`. After publication,
`python3 tools/sync_github_releases.py` can refresh its fallback release list;
the live download page discovers stable GitHub releases automatically.
Package download counts come from GitHub release assets and share the 15-minute
release cache. Cache format updates trigger one full refresh, so existing server
caches do not hide newly added information. Unknown counts are omitted; source
archive counts are unavailable. Updating website inputs affects future build
fingerprints, not already prepared packages.

## GitHub App installation credentials

A helper-created GitHub App installation token uses the same configured 1Password
field. It is not a personal access token and does not appear in personal-token
settings. Installation access tokens expire after one hour; refresh them with the
existing token helper when needed, using the same vault/item/field as the release
configuration. A valid installation token needs access to this repository and
Contents write permission for release creation and asset upload. Repository
identity and API access are checked before mutations; API permission failures stop
the run. No broader credential fallback is automatic.

Reference: [GitHub installation access tokens](https://docs.github.com/en/apps/creating-github-apps/authenticating-with-a-github-app/generating-an-installation-access-token-for-a-github-app).

## Submit an already published release to Aminet

Use the retained, prepared output directory. Aminet-only publication verifies the
frozen manifest, original LHA, checksums and embedded source/build provenance,
independently of current HEAD. Newer website or documentation edits can remain on
main; detaching to the old tag is unnecessary.

```sh
python3 scripts/release.py aminet --output outputs/release-0.4-corrected
python3 scripts/release.py publish --output outputs/release-0.4-corrected \
  --tag 0.4 --to aminet --confirm-publish
```

The first command previews the pair without uploading. There is no `prepare-aminet`
command in NovaDock. A completed Aminet receipt makes publication a no-op; this
example does not resend an already submitted release.

The current external uploader contact can create a separate checksummed
`aminet-<version>-<contact-hash>` submission directory beside the retained output
bundle. It does not modify the runtime LHA, repository manifest or old receipts.
Contact changes during an interrupted submission are refused. Changing the actual
application/readme inside the LHA requires a reviewed rebuild and correction, not
an edit to frozen files. Preserve original submissions and resolve recovery
separately with Aminet when necessary.
