# Building and publishing NovaDock

Run `python3 scripts/release.py` from the project root with Python 3.9 or newer.
`tools/release.py` is the same entry point and remains supported. Build once,
review the prepared manifest, then publish the same verified bytes to each
selected destination. No command deploys the website or creates hosted repositories.

## Commands

| Command | Purpose | Uploads |
| --- | --- | --- |
| `plan` | Print ordered commands and target information | None |
| `build` | Run host tests, compile all native programs, package and validate | None |
| `check` | Verify current source, binaries, provenance, packages and checksums | None |
| `prepare` | Seal notes, destinations and exact assets in a manifest | None |
| `publish` | Preflight every selected destination, then publish | Explicit confirmation required |
| `release` | Prepare and publish, reusing an identical prepared manifest | Explicit confirmation required |
| `source` | Preflight or push a committed source branch | Separate source confirmation required |

NovaDock has one available build target: Classic AmigaOS m68k, baseline 68000,
AmigaOS 3.0 or later. Signing and notarization do not apply to this target.
Compilation and host checks are recorded separately from native runtime checks.
Complete `docs/RELEASE_CHECKLIST.md` on your supported configurations before
publishing; a successful compiler invocation does not certify runtime behaviour.

## Configure once

The default configuration is external to the project:
`~/Development/tools/git-release/projects/novadock.json`. This matches the shared
release-tool layout. Override it with `NOVADOCK_RELEASE_CONFIG` or `--config PATH`.
If the shared file is absent, an existing ignored `work/release-config.json` is
accepted for compatibility. Start from `examples/release-config.json` and set the
real private GitLab HTTPS and SSH URLs in the external copy. Never commit local
machine paths or credential settings. The earlier nested URL format from
`docs/release-targets.example.json` remains accepted.

The public destination is https://github.com/retrocs-org/novadock. The tool
validates HTTPS and Git push URLs as the same repository, including nested
GitLab groups. Credential-bearing URLs, query strings, encoded paths, traversal,
custom ports and non-HTTPS API destinations are rejected. The prepared manifest
records the exact destinations; changing local configuration after preparation
blocks publication. Keep manifests and receipts private if they identify an
internal repository. They are not part of the public source packages.

Allowed local build environment overrides are `VBCC`, `VC`, `LHA`, `CC`, `PHP`,
`FLEXCAT`, `AUTHOR`, `AMINET_REPLACES` and `NOVADOCK_RELEASE_PYTHON`. Use executable
paths for tool overrides. The selected Python interpreter is used for every
release step, including the shell build/staging scripts and host checks. Without
an override, the interpreter running release.py is used.
Configure vbcc/vasm/vlink with the AmigaOS m68k target and a compatible NDK.
LHA must be LHa for UNIX with archive creation support. PHP and a host C compiler
are needed for validation. The uploader defaults to support@retrocs.org; set
`uploader` in the local JSON or pass `--uploader` to override it.

API credentials can come from environment variables or 1Password CLI, matching
the shared release configuration. The default provider reads `GITHUB_TOKEN` or
`GITLAB_TOKEN`. To use 1Password, configure a copied secret reference or an exact
vault/item/field lookup in the external file:

```json
"credentials": {
  "github": {
    "provider": "1password",
    "vault": "Your vault",
    "item": "github.com",
    "field": "your-org/novadock-rw"
  },
  "gitlab": {
    "provider": "1password",
    "reference": "op://vault/item/field"
  }
}
```

Optional `account` selects a 1Password account. Use `provider: "env"` with
`variable: "YOUR_TOKEN_VARIABLE"` for a different environment variable. Values
in the JSON are references, never token contents. A configured provider is used
explicitly; an environment token does not silently override a 1Password setting.

Secrets are read only for confirmed repository publication, held in memory and
redacted from release errors. They are not copied into environment variables,
notes, manifests or receipts. Build subprocesses receive no API tokens or
1Password session/service-account credentials. Planning, checking, preparation,
source pushing and the build itself do not retrieve 1Password secrets.

GitHub needs repository Contents write access; GitLab needs API access and a role
allowed to create releases and generic packages. SSH keys or Git credential
helpers provide separate Git push access. Configure/sign in to the 1Password CLI
before publication if using that provider. Do not put literal tokens in command
arguments, URLs, configuration, notes, source files or shell history, and avoid
shell tracing. Unknown or ambiguous secret fields stop before uploads and their
output is not displayed.

The repositories must already exist, and the internal GitLab project must already
be private with its generic package registry enabled. The tool does not configure
repository visibility, credentials, keys, DNS or hosting.

## Finish and commit release inputs

Update VERSION and its CHANGELOG section, then refresh generated source before
committing:

```sh
python3 tools/update_version.py
python3 tools/update_site_chrome.py
python3 tools/generate_catalog.py
python3 tools/make_amigaguide.py
python3 tools/generate_info_icons.py
python3 tools/sync_github_releases.py --upcoming
python3 tools/package_release.py
```

Review and commit the intended source. These generators are repeated during
`build`; if they change tracked inputs, finish the generated changes and commit
before building again. All selected source inputs must be tracked, including
website captions, documentation, manifests and SDK helpers. Ignored or untracked
build inputs cannot silently enter a publishable release.

## Build, check and prepare

```sh
python3 scripts/release.py plan
python3 scripts/release.py build
python3 scripts/release.py check
python3 scripts/release.py prepare
```

The default output directory is `outputs/release`. Use `--output PATH` on each
command to select another directory; it must be outside the source tree or below
ignored `outputs/`. `build` requires an empty directory and always compiles fresh
binaries. Preserve failed output as evidence and use a fresh directory to retry.
There is no unified skip-build option. The older standalone packaging scripts
remain available for local maintenance, but their files are not automatically
trusted as a publishable build.

For compatibility, `prepare` also runs `build` when its output has no build record.
For the explicit workflow above, it only seals already checked artifacts.
Use `prepare --notes docs/RELEASE-NOTES.md` to seal a reviewed notes file instead
of notes generated from CHANGELOG. This deliberately changes the checksums and
manifest; it is refused once the manifest has been prepared.

Build records connect the exact source fingerprint and commit to all native
executables, compiler identity and artifact hashes. The fingerprint covers the
explicit source inventory, documentation, build tools, tests, plugin metadata and
website inputs. Generated downloads, compiled binaries, local configuration and
provenance records are excluded from their own input fingerprint. The source ZIPs
include `build/provenance/amiga.json`, identical to the release's BUILD-INFO.json.

Changing any release input after building requires another build. Checks reject
stale fingerprints, changed binaries, stale source archive contents, missing or
unexpected assets, duplicate entries and symlinked files. Package checks also
compare the website's downloads and checksum list with the release bytes.
These checks establish matching provenance; they do not promise bit-for-bit
reproducibility across toolchains.

An extracted source tree may build and prepare local files with a null commit.
Publishing requires a clean, committed Git checkout at the source root, with the
same commit and tracked inputs as the build. Do not edit build records to bypass
this requirement.

## Review the manifest

Review `outputs/release/manifest.json`, `RELEASE-NOTES.md`, BUILD-INFO.json and the
Aminet readme. `release.json` is a compatibility alias and must match manifest.json.
The manifest contains version/tag derived from VERSION, exact source commit,
source fingerprint, configured repository destinations, notes and every asset hash.
It is immutable: preparing again may confirm identical data, but cannot replace it
with different notes or destinations. Use a new release directory for changed input.

Each release uploads exactly four assets:

- NovaDock-VERSION.lha
- NovaDock-VERSION.lha.sha256
- NovaDock-VERSION-plugin-sdk.zip
- NovaDock-VERSION-plugin-sdk.zip.sha256

GitHub and GitLab provide source archives from the tag automatically; source ZIP
and tar.gz files are not uploaded separately. Both checksum sidecars contain the
SHA-256 and filename of their corresponding download.

Source/site packages, the Aminet readme, release notes, BUILD-INFO.json and the
complete SHA256SUMS.txt are retained locally for validation and deployment. The
manifest distinguishes this full build inventory from its four publication assets.
Manifests, local configuration and receipts are never release downloads.

## Publish selected destinations

```sh
python3 scripts/release.py publish --to gitlab,github --confirm-publish
python3 scripts/release.py publish --to aminet --confirm-publish
```

Or select `--to gitlab,github,aminet` to preflight all three before any upload.
The default destinations are GitLab and GitHub. Destination names must be distinct.
Without `--confirm-publish`, publication is refused before network access. The
previous `--execute` flag remains an explicit confirmation alias; `--target github`
or `--target gitlab` remains available for older operator commands.

Before any push or upload, the tool checks the manifest, current source and
binaries, clean commit, tokens, destination identity, existing assets, release notes
and remote tags. Annotated remote tags are peeled to their source commit. A
conflicting tag stops publication. It creates an annotated local tag and pushes
only that exact annotated tag object, without force or automatic tag following,
then verifies the remote commit. It does not push
branches or change default branches; the separate source command below is used
for branch publication.

GitHub uploads into a draft, verifies every returned SHA-256 digest and only then
publishes the release. Existing assets are reused only with matching known hashes;
unknown or conflicting digests, unexpected assets and changed notes stop the run.
Published releases missing assets are not silently modified. Versions beginning
with 0. are marked prerelease. No `gh` installation is needed.

GitLab uses versioned generic-package paths. Each existing or uploaded file is
stream-downloaded and hashed before its link is advertised in a release. Matching
packages are reused; conflicting bytes, notes or links stop publication. GitLab has
no matching prerelease flag. No `glab` installation is needed.

GitHub draft discovery checks the latest 100 releases before looking up the tag;
very old interrupted drafts may need manual inspection.

## Combined release command

```sh
python3 scripts/release.py release --to gitlab,github,aminet --confirm-publish
```

This prepares an unprepared build and then uses the same publisher. If no build
exists, preparation first performs the normal fresh build. An existing prepared
manifest is reused; the command cannot silently replace its notes, tag annotation,
assets or destinations. `prepare --tag-message "NovaDock 0.1"` records a custom
annotation in the manifest. Use `--tag 0.1` to choose an exact tag; without it a new preparation defaults to
`v<VERSION>`. Application version and package filenames still derive from VERSION.
Git validates the tag name. Once prepared, omitting --tag reuses the manifest tag;
an explicit different tag or annotation is refused before credential lookup.
Changing tags or annotations requires a fresh preparation directory.

```sh
python3 scripts/release.py prepare --tag 0.1 --tag-message "NovaDock 0.1 release"
python3 scripts/release.py publish --tag 0.1 --to gitlab,github --confirm-publish
```

The combined `release` command accepts the same --tag and --tag-message options.
After publication, refresh the website release list as described below. Website
source and SDK links then follow the actual published tag, including bare version
numbers, rather than assuming a v prefix.

## Upload source separately

```sh
python3 scripts/release.py source --to gitlab,github --dry-run
python3 scripts/release.py source --to gitlab,github --confirm-push
```

This preflights every selected branch push before uploading to any repository,
pushes the reviewed commit ID without force or automatic tag following, and verifies the resulting
branch commit. It performs no compilation, release-tag creation or binary upload.
`source_branch` in the external JSON selects the remote branch; an empty value
uses the current branch. The checkout must be clean with required source inputs
tracked. Tracked private configuration, build evidence or local output blocks it.

For a new extracted source tree, explicitly add `--init`. Initialization is
allowed only when all selected remotes have no refs and a Git author/committer
identity is configured. It stages only the selected public source inventory,
respects .gitignore and commits it. `--commit-message "Import NovaDock source"`
customizes that first commit. Existing repositories are never reset or replaced. An unfinished repository with
already staged files must be reviewed before initialization can continue.
If a selected remote already has history, clone it and import changes there.
`source --init --dry-run` only checks and describes the import; it creates no
metadata or commit. Source publication still requires `--confirm-push`.

Generated website downloads are ignored by Git and rebuilt during packaging.
Private machine toolchain adapters belong in ignored work/ or an external tools
directory. Cross-builds always select the same public C sources as native builds;
private compatibility source overrides are not used.

## Aminet and interrupted uploads

Aminet submission uses anonymous FTP to `main.aminet.net`, directory `/new`, with
the uploader email as the contact. The adapter verifies the LHA CRC and the prepared
pair's hashes against the repository release. It sends novadock.lha first and
novadock.readme last. Submission success means awaiting moderation, not publication
on the public archive. Repository publishing alone never submits to Aminet.

`outputs/release/receipt.json` records the manifest hash, destination currently
in progress and completed destinations. A different manifest cannot reuse it.
Rerun the same manifest for a failed repository destination; verified matching
assets are reused and successful releases are preserved. No cross-service rollback
or automatic network retry occurs.

Aminet also maintains `aminet/upload-receipt.json`, bound to that submission's
exact bytes. It records transfer progress before connecting and after each file.
A completed receipt prevents duplicate submission. An incomplete receipt blocks
automatic retry, including uncertain connection failures: inspect the incoming
server and receipt before deciding recovery. Do not delete receipts to bypass the
check. Re-preparing over an existing Aminet upload receipt is refused.

The standalone `tools/aminet_release.py` retains its own offline prepare/check and
explicit upload commands and observes the same receipt rules. The compatibility
`release.py aminet` command checks locally; `--execute` routes through the unified
confirmed publisher.

## Validation and website updates

Ordinary tests use mocked APIs, fake FTP transfers and temporary Git repositories;
they never publish real tags or submit files. Run:

```sh
python3 tests/test_release_tool.py
python3 tests/test_release_provenance.py
python3 tests/test_release_integration.py
python3 tests/test_aminet_release.py
```

Exercise publication against dedicated test repositories before a production
release. Preserve build records and receipts as evidence of the release run.
After publishing, run `python3 tools/sync_github_releases.py` and redeploy the PHP
website to show actual release dates and assets. Publishing attaches the website
ZIP; it does not deploy it. Refreshing website inputs changes the next build's
fingerprint and cannot silently change an already prepared release.

API references: [GitHub releases](https://docs.github.com/en/rest/releases/releases),
[GitLab generic packages](https://docs.gitlab.com/user/packages/generic_packages/)
and [GitLab releases](https://docs.gitlab.com/api/releases/).
Aminet references: [upload instructions](https://wiki.aminet.net/Uploading_instructions)
and [readme format](https://wiki.aminet.net/The_Readme_file).

Credential references: [1Password read command](https://developer.1password.com/docs/cli/reference/commands/read/).
