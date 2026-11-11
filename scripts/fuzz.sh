#!/bin/sh
# Builds and runs NovaDock's libFuzzer targets (needs clang with libFuzzer).
#   sh scripts/fuzz.sh [seconds-per-target] [target ...]
# Targets: config manifest rexx ilbm bitskim clipboard binary script network
# music.  Crashes are kept in fuzz-out/<target>/ as crash-* files; the seed
# corpus comes from the project's own files.
set -eu
cd "$(dirname "$0")/.."
SECONDS_EACH=${1:-60}
[ $# -gt 0 ] && shift
TARGETS=${*:-config manifest rexx ilbm bitskim clipboard binary script network music}
CC=${FUZZ_CC:-clang}
OUT=fuzz-out
FLAGS="-g -O1 -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined"
INC="-Iinclude -Iplugin_src/common -Iplugin_src/bitskim -Iplugin_src/clipboard \
 -Iplugin_src/mixer -Iplugin_src/screenmode -Iplugin_src/script"
mkdir -p "$OUT"
status=0
for t in $TARGETS; do
  case $t in
    config)    SRC="src/config.c src/nd_options.c"; SEEDS="novadock.cfg* tests/*.cfg* Layouts/*" ;;
    manifest)  SRC="src/config.c src/nd_options.c src/nd_manifest.c"; SEEDS="Plugins/*.ndplugin examples/*.ndplugin" ;;
    rexx)      SRC="src/interaction.c"; SEEDS="Rexx/*" ;;
    ilbm)      SRC="src/ilbm.c"; SEEDS="Icons/*.iff Backgrounds/*.iff Themes/*/*.iff" ;;
    bitskim)   SRC="plugin_src/bitskim/nd_bitskim.c"; SEEDS="" ;;
    clipboard) SRC="plugin_src/clipboard/nd_clipboard.c"; SEEDS="" ;;
    binary)    SRC="plugin_src/mixer/nd_ahiprefs.c plugin_src/screenmode/nd_scrmode.c"; SEEDS="" ;;
    script)    SRC="plugin_src/script/nd_script.c"; SEEDS="" ;;
    network)   SRC="plugin_src/network/nd_network.c"; SEEDS="" ;;
    music)     SRC="plugin_src/musicplayer/nd_musicplayer.c"; SEEDS="" ;;
    *) echo "unknown target: $t" >&2; exit 2 ;;
  esac
  upper=$(echo "$t" | tr 'a-z' 'A-Z')
  bin="$OUT/fuzz_$t"
  $CC $FLAGS $INC -DFUZZ_$upper tests/fuzz/nd_fuzz.c $SRC -o "$bin"
  corpus="$OUT/$t/corpus"
  mkdir -p "$corpus"
  for f in $SEEDS; do [ -f "$f" ] && cp "$f" "$corpus/" 2>/dev/null || true; done
  echo "== $t (${SECONDS_EACH}s)"
  if ! (cd "$OUT/$t" && "../fuzz_$t" -max_total_time="$SECONDS_EACH" \
        -max_len=8192 -print_final_stats=0 corpus >"log.txt" 2>&1); then
    echo "   FAILED: see $OUT/$t/log.txt"; status=1
  else
    grep -E "^#[0-9]+ *DONE" "$OUT/$t/log.txt" | tail -1 || true
  fi
done
exit $status
