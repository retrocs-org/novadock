#!/bin/sh
# Build a ready-to-run NovaDock release for AmigaOS users:
#
#   OUTPUT/NovaDock-<version>.lha     unpack anywhere, double-click Install-NovaDock
#   OUTPUT/NovaDock-<version>.readme  Aminet-format description
#
#   sh scripts/make-release.sh OUTPUT_DIR [--aminet | --upload-aminet]
#
# Environment:
#   ND_SKIP_BUILD=1  package the binaries already built (default: rebuild)
#   LHA=path         an LHA archiver that can CREATE archives, such as
#                    "LHa for UNIX" (github.com/jca02266/lha); lhasa only
#                    extracts.  Default: lha in PATH.
#   VBCC=directory  vbcc installation with the m68k-amigaos target
#   VC=command      optional Makefile compiler override (exported to the build)
#   UPLOADER=e-mail  required for Aminet preparation/upload; optional otherwise
#   AUTHOR=name      author credit (default NovaDock contributors)
#   AMINET_REPLACES  optional full Aminet paths, separated by semicolons
set -eu
PYTHON=${PYTHON:-python3}
export PYTHON

project_root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
# VERSION is a list of NAME=value lines, readable by the shell.
. "$project_root/VERSION"
version=$VERSION
name="NovaDock-$version"
[ "$#" -ge 1 ] && [ "$#" -le 2 ] || {
    echo "Usage: $0 OUTPUT_DIR [--aminet | --upload-aminet]" >&2; exit 2;
}
aminet_mode=${2:-}
case "$aminet_mode" in
    ""|--aminet|--upload-aminet) ;;
    *) echo "Unknown release mode: $aminet_mode" >&2; exit 2 ;;
esac
if [ -n "$aminet_mode" ] && [ -z "${UPLOADER:-}" ]; then
    echo "Set UPLOADER to your real email before preparing an Aminet submission." >&2
    exit 2
fi
if [ -n "$aminet_mode" ]; then
    "$PYTHON" - "${UPLOADER:-}" <<'CHECK'
import re, sys
if not re.search(r'[^\s<>@()]+@[^\s<>@()]+\.[^\s<>@()]+', sys.argv[1]):
    raise SystemExit('Set UPLOADER to your real email before preparing an Aminet submission.')
CHECK
fi
# Standalone local packaging uses the agreed contact unless explicitly supplied.
UPLOADER=${UPLOADER:-frossmant@icloud.com}
export UPLOADER
mkdir -p "$1"
output=$(CDPATH= cd "$1" && pwd)

lha=${LHA:-lha}
if ! "$lha" --version 2>&1 | grep -q "LHa for UNIX"; then
    echo "NovaDock: $lha cannot create LHA archives." >&2
    echo "Build LHa for UNIX (github.com/jca02266/lha) and set LHA=/path/to/lha." >&2
    exit 1
fi

# Keep temporary output on the release filesystem for atomic publication.
work=$(mktemp -d "$output/.novadock-release.XXXXXX")
trap 'rm -rf "$work"' 0
trap 'exit 1' HUP INT TERM

# Regenerate documentation/catalog metadata before compiling the release.
"$PYTHON" "$project_root/tools/generate_catalog.py" >/dev/null
"$PYTHON" "$project_root/tools/make_amigaguide.py" >/dev/null
if [ "${ND_SKIP_BUILD:-0}" = 1 ]; then
    echo "NovaDock: packaging existing Amiga binaries."
else
    echo "NovaDock: compiling a fresh Amiga release."
fi

# The runnable drawer, with all required application files.
sh "$project_root/scripts/build-and-stage-amiga.sh" "$work" >/dev/null
drawer="$work/NovaDock"
rm -f "$drawer/novadock.cfg.default"
"$PYTHON" "$project_root/tools/generate_info_icons.py" --drawer-icon "$work/NovaDock.info" >/dev/null

# Generate the embedded readme before archiving, using the same changelog as
# repository release notes. Both Aminet files are reviewed before submission.
"$PYTHON" "$project_root/tools/release_notes.py" --format aminet \
    --output "$drawer/NovaDock.readme"

"$PYTHON" "$project_root/tools/aminet_release.py" format-readme \
    "$drawer/NovaDock.readme" --replaces "${AMINET_REPLACES:-}"

# Confirm all staged programs are real Amiga Hunk executables.
"$PYTHON" - "$project_root/Makefile" "$drawer" <<'CHECK'
from pathlib import Path
import re, sys
makefile = Path(sys.argv[1]).read_text()
drawer = Path(sys.argv[2])
programs = re.search(r'^all:(.*)$', makefile, re.M).group(1).split()
for name in programs:
    with (drawer / name).open('rb') as program:
        if program.read(4) != bytes.fromhex('000003f3'):
            raise SystemExit('Not an Amiga Hunk executable: ' + name)
print('NovaDock: verified %d Amiga programs.' % len(programs))
CHECK

# Explicit LH5 compression and level-1 headers for classic Amiga extractors.
archive="$work/$name.lha"
(cd "$work" && "$lha" -a1o5 "$archive" NovaDock NovaDock.info >/dev/null)
"$lha" -t "$archive" >/dev/null

# Unpack it again and compare every file, including Workbench .info data.
check="$work/check"
mkdir -p "$check"
(cd "$check" && "$lha" -x "$archive" >/dev/null)
diff -r "$drawer" "$check/NovaDock" >/dev/null
cmp -s "$work/NovaDock.info" "$check/NovaDock.info"

"$PYTHON" - "$archive" <<'CHECK'
from pathlib import Path
import hashlib, sys
archive = Path(sys.argv[1])
archive.with_suffix('.lha.sha256').write_text(
    hashlib.sha256(archive.read_bytes()).hexdigest() + '  ' + archive.name + '\n')
CHECK

# Leave an existing release intact if building, compression or verification
# fails. Publish only verified files; moving the archive is the final step.
cp "$drawer/NovaDock.readme" "$work/$name.readme"
mv -f "$work/$name.readme" "$output/$name.readme"
mv -f "$work/$name.lha.sha256" "$output/$name.lha.sha256"
mv -f "$archive" "$output/$name.lha"

echo "NovaDock: release written and extraction verified:"
echo "  $output/$name.lha ($(wc -c <"$output/$name.lha" | tr -d ' ') bytes)"
echo "  $output/$name.readme"
echo "  $output/$name.lha.sha256"

# Normal builds never connect to Aminet. Preparing also remains entirely local.
if [ -n "$aminet_mode" ]; then
    "$PYTHON" "$project_root/tools/aminet_release.py" prepare \
        "$output/$name.lha" "$output/aminet" --lha "$lha"
    if [ "$aminet_mode" = "--upload-aminet" ]; then
        "$PYTHON" "$project_root/tools/aminet_release.py" check \
            "$output/aminet" --lha "$lha" --upload
    else
        echo "NovaDock: Aminet files are prepared locally; review $output/aminet/novadock.readme."
        echo "To submit later, run tools/aminet_release.py check on that directory with --upload."
    fi
fi
