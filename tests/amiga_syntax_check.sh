#!/bin/sh
# Optional: type-check every AmigaOS source with a 68000 GCC and the open
# AmigaOS 3.2 headers (OpenNDK from github.com/DalsinAI/openamigasdk).
# This is a compile-only check; it does not link or replace a vbcc build.
#   OPENNDK=/path/to/openamigasdk/OpenNDK/Include_H tests/amiga_syntax_check.sh
# Requires m68k-linux-gnu-gcc (Debian/Ubuntu: gcc-m68k-linux-gnu).
set -eu
cd "$(dirname "$0")/.."
: "${OPENNDK:?set OPENNDK to the OpenNDK Include_H directory}"
CC=${M68K_CC:-m68k-linux-gnu-gcc}
STUBS=tests/amiga-stubs
# Several statements on one line ("if(a)Close(a);if(b)Close(b);") is the
# house style and harmless, so a misleading-indentation warning is dropped
# when gcc's note points at the same line.  A guarded statement followed by
# an indented statement on the NEXT line - the shape of a missing brace
# (see the SAVE_FAILED hook fix in dock_save.c) - is still reported.
SAME_LINE_FILTER='
/warning: this .* clause does not guard/ {
  if (held!="") print held
  split($0,w,":"); held=$0; held_file=w[1]; held_line=w[2]; next
}
held!="" && /: note: / {
  # The note may point into an OS macro header first; the location that
  # counts is the next note in the warned file ("in expansion of macro").
  split($0,n,":"); if (n[1]!=held_file) next
  if (n[2]!=held_line) print held
  held=""; next
}
{ print }
END { if (held!="") print held }'
status=0
# vbcc register syntax (__reg("a0")) has no GCC equivalent in a syntax check.
# Sources written in vbcc-only inline assembly, or which #error outside the
# supported vbcc build, are compiled by vbcc alone.
VBCC_ONLY='plugin_src/sensors/i2c_provider.c plugin_src/warp/nd_warp_ui.c plugin_src/warp/warp.c tools/warp_sensor_probe.c'
for f in src/*.c prefs/*.c plugin_src/*/*.c tools/*.c; do
  case " $VBCC_ONLY " in *" $f "*) continue;; esac
  case $f in
    src/nd_trace.c|tools/save_trace.c) extra=-DND_TRACE ;;
    *) extra= ;;
  esac
  out=$($CC -nostdinc -isystem "$($CC -print-file-name=include)" -I"$STUBS" \
        -std=gnu89 -fsyntax-only -fno-diagnostics-show-caret -m68000 -Wall -Wdeclaration-after-statement -Wno-unused-function \
        -Wno-pointer-sign -Wno-missing-braces -Wno-format \
        -Wmisleading-indentation -D__amigaos__ -DAMIGA -DND_PREFS_EMBEDDED '-D__reg(x)=' $extra \
        -I"$OPENNDK" -Iinclude -Iplugin_src/common -Iplugin_src/archive -Iplugin_src/command -Iplugin_src/warp -Iplugin_src/sensors \
        -I"$(dirname "$f")" \
        "$f" 2>&1 | awk "$SAME_LINE_FILTER" | grep -E '(^|[[:space:]])(fatal error|error|warning):' | grep -v "$OPENNDK" || true)
  if [ -n "$out" ]; then echo "$out"; status=1; fi
done
[ $status -eq 0 ] && echo "AmigaOS sources type-check cleanly (68000, OS 3.2 headers)."
exit $status
