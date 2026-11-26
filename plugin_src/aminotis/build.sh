#!/bin/sh
# Build only AMINOTIS through Makefile.cross and verify the patched startup.
# VC defaults to the local vbcc driver, which links the patched startup.o.
set -eu
root=$(CDPATH= cd "$(dirname "$0")/../.." && pwd)
cd "$root"
make -f Makefile.cross VC="${VC:-python3 $root/work/toolchain/vc-driver.py}" Plugins/AmiNotis
python3 tools/vbcc_startup.py check Plugins/AmiNotis
