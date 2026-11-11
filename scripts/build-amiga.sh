#!/bin/sh
set -eu

project_root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
task_root=$(CDPATH= cd "$project_root/../.." && pwd)
bundled_toolchain="$task_root/work/amiga-toolchain/vbcc-0.9hp3"

if [ -n "${VBCC:-}" ] && [ -x "$VBCC/bin/vc" ]; then
    PATH="$VBCC/bin:$PATH"
elif command -v vc >/dev/null 2>&1; then
    : # Use the caller's configured vbcc installation.
elif [ -x "$bundled_toolchain/bin/vc" ]; then
    VBCC=$bundled_toolchain
    PATH="$VBCC/bin:$PATH"
else
    echo "NovaDock: vbcc was not found." >&2
    echo "Set VBCC to its installation directory or put vc in PATH." >&2
    exit 1
fi

# VC can select a custom target configuration; Makefile.cross uses it as-is.
if [ -n "${VC:-}" ]; then export VC; fi
export VBCC PATH

mkdir -p "$project_root/Plugins"
exec make -C "$project_root" -f Makefile.cross "$@"
