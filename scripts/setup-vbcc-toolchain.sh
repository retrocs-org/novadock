#!/bin/sh
# Build a Linux-hosted vbcc/vasm/vlink cross toolchain for Classic AmigaOS and
# pair it with the AmigaOS 3.2 NDK.  Used by CI; also handy on any Linux box.
#
#   sh scripts/setup-vbcc-toolchain.sh [DEST]          (default: ~/amiga-toolchain)
#   export VBCC=DEST/vbcc PATH=$VBCC/bin:$PATH
#   sh scripts/build-amiga.sh -B
#
# Sources are pinned: kusma/amiga-dev bundles vbcc, vasm and vlink as
# submodules together with the vbcc m68k-amigaos target (startup code,
# vc.lib, amiga.lib).  NDK 3.2 comes from Aminet (Hyperion's public release;
# it is downloaded at build time, never redistributed).
#
# Optional environment:
#   NDK_INCLUDE    use this Include_H directory instead of downloading NDK 3.2
#   EXTRA_INCLUDE  searched before everything else (local header overrides)
#   AMIGA_DEV_REV  amiga-dev commit to build (default pinned below)
#   NDK_URL        NDK 3.2 archive (default Aminet)
set -eu

DEST=${1:-$HOME/amiga-toolchain}
AMIGA_DEV_REPO=https://github.com/kusma/amiga-dev
AMIGA_DEV_REV=${AMIGA_DEV_REV:-b2e26df28335cd575c30df3b867f6667797b72a7}
NDK_URL=${NDK_URL:-http://aminet.net/dev/misc/NDK3.2.lha}

say() { printf '\n== %s\n' "$*"; }
mkdir -p "$DEST"
DEST=$(CDPATH= cd "$DEST" && pwd)
SRC=$DEST/src
VBCC_ROOT=$DEST/vbcc

if [ -x "$VBCC_ROOT/bin/vc" ] && [ -f "$VBCC_ROOT/config/aos68k" ]; then
    say "toolchain already present in $VBCC_ROOT"
    exit 0
fi

say "fetching vbcc, vasm and vlink sources ($AMIGA_DEV_REV)"
rm -rf "$SRC"
git clone --quiet "$AMIGA_DEV_REPO" "$SRC"
git -C "$SRC" checkout --quiet "$AMIGA_DEV_REV"
git -C "$SRC" submodule --quiet update --init --depth 1

say "building vasm"
make -C "$SRC/src/vasm" CPU=m68k SYNTAX=mot >"$DEST/vasm.log" 2>&1 ||
    { tail -20 "$DEST/vasm.log"; exit 1; }

say "building vlink"
mkdir -p "$SRC/src/vlink/objects"
make -C "$SRC/src/vlink" >"$DEST/vlink.log" 2>&1 ||
    { tail -20 "$DEST/vlink.log"; exit 1; }

say "building vbcc"
VB=$SRC/src/vbcc
mkdir -p "$VB/bin"
make -C "$VB" TARGET=m68k bin/dtgen </dev/null >"$DEST/vbcc.log" 2>&1 ||
    { tail -20 "$DEST/vbcc.log"; exit 1; }
# dtgen asks which host C types match each target type.  Its defaults are
# detected for this host, so Enter is the right answer to every question -
# but it loops forever at end of input, so feed a bounded number of lines.
rm -f "$VB/machines/m68k/dt.h" "$VB/machines/m68k/dt.c"
(cd "$VB" && yes "" | head -n 60 |
    bin/dtgen machines/m68k/machine.dt machines/m68k/dt.h machines/m68k/dt.c \
    >>"$DEST/vbcc.log" 2>&1)
[ -s "$VB/machines/m68k/dt.h" ] || { echo "dtgen failed" >&2; exit 1; }
make -C "$VB" TARGET=m68k </dev/null >>"$DEST/vbcc.log" 2>&1 ||
    { tail -20 "$DEST/vbcc.log"; exit 1; }

say "assembling $VBCC_ROOT"
rm -rf "$VBCC_ROOT"
mkdir -p "$VBCC_ROOT/bin" "$VBCC_ROOT/config"
cp -R "$SRC/targets/m68k-amigaos/lib" "$SRC/targets/m68k-amigaos/include" \
      "$SRC/targets/m68k-amigaos/netinclude" "$VBCC_ROOT/"
for tool in vc vbccm68k vprof; do cp "$VB/bin/$tool" "$VBCC_ROOT/bin/"; done
cp "$SRC/src/vasm/vasmm68k_mot" "$SRC/src/vlink/vlink" "$VBCC_ROOT/bin/"

if [ -n "${NDK_INCLUDE:-}" ]; then
    NDK_H=$NDK_INCLUDE
else
    say "fetching NDK 3.2 from Aminet"
    command -v lha >/dev/null 2>&1 ||
        { echo "lha not found (Debian/Ubuntu: apt-get install lhasa)" >&2; exit 1; }
    mkdir -p "$DEST/ndk"
    curl -fsSL "$NDK_URL" -o "$DEST/ndk/NDK3.2.lha"
    (cd "$DEST/ndk" && lha -xq NDK3.2.lha)
    NDK_H=$(find "$DEST/ndk" -type d -name Include_H | head -n 1)
    [ -n "$NDK_H" ] || { echo "NDK3.2: Include_H not found" >&2; exit 1; }
fi
NDK_H=$(CDPATH= cd "$NDK_H" && pwd)

say "writing $VBCC_ROOT/config/aos68k (NDK: $NDK_H)"
# NDK 3.2 headers come first so OS 3.2 calls and its vbcc inlines are used;
# vbcc's own directory still supplies the C library headers.
INC=""
[ -n "${EXTRA_INCLUDE:-}" ] && INC="-I\"$EXTRA_INCLUDE\" "
INC="$INC-I\"$NDK_H\" -I\"$VBCC_ROOT/include\" -I\"$VBCC_ROOT/netinclude\""
L="-L\"$VBCC_ROOT/lib\""
cat >"$VBCC_ROOT/config/aos68k" <<EOF
-cc=vbccm68k -quiet -hunkdebug %s -o= %s %s -O=%ld $INC
-ccv=vbccm68k -hunkdebug %s -o= %s %s -O=%ld $INC
-as=vasmm68k_mot -quiet -Fhunk -phxass -opt-fconst -nowarn=62 %s -o %s
-asv=vasmm68k_mot -Fhunk -phxass -opt-fconst -nowarn=62 %s -o %s
-ld=vlink -bamigahunk -x -Bstatic -Cvbcc -nostdlib "$VBCC_ROOT/lib/startup.o" %s %s $L -lvc -o %s
-l2=vlink -bamigahunk -x -Bstatic -Cvbcc -nostdlib %s %s $L -o %s
-ldv=vlink -bamigahunk -t -x -Bstatic -Cvbcc -nostdlib "$VBCC_ROOT/lib/startup.o" %s %s $L -lvc -o %s
-l2v=vlink -bamigahunk -t -x -Bstatic -Cvbcc -nostdlib %s %s $L -o %s
-ldnodb=-s -Rshort
-ul=-l%s
-cf=-F%s
-ml=500
EOF

say "smoke test"
cat >"$DEST/hello.c" <<'EOF'
#include <proto/exec.h>
#include <proto/dos.h>
int main(void) { PutStr("hello\n"); return 0; }
EOF
VBCC=$VBCC_ROOT PATH=$VBCC_ROOT/bin:$PATH \
    vc +aos68k -notmpfile -o "$DEST/hello" "$DEST/hello.c" -lamiga
# Every AmigaOS executable starts with the HUNK_HEADER magic 0x000003F3.
[ "$(od -An -tx1 -N4 "$DEST/hello" | tr -d ' \n')" = "000003f3" ] ||
    { echo "smoke test did not produce an AmigaOS executable" >&2; exit 1; }
rm -f "$DEST/hello" "$DEST/hello.c"
say "ready: export VBCC=$VBCC_ROOT PATH=\$VBCC/bin:\$PATH"
