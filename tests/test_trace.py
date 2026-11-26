#!/usr/bin/env python3
"""Normal builds evaluate none of the opt-in diagnostic hooks."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=r'''#include <assert.h>
#include "nd_trace.h"
int main(void){int changed=0;
 ND_TRACE_PHASE_BEGIN(ignored,++changed);ND_TRACE_PHASE_END(ignored,++changed);ND_TRACE_PAGE(++changed,++changed);
 ND_TRACE_INIT(++changed);ND_TRACE_START(undefined_token);
 ND_TRACE_END(NDT_PAINT,undefined_token);ND_TRACE_NOTE(++changed,++changed,++changed,++changed,++changed);
 ND_TRACE_QUEUE(++changed,++changed);ND_TRACE_SCENE(++changed,++changed);
 ND_TRACE_TILE_KIND(++changed,++changed);ND_TRACE_PACKET(++changed,++changed,++changed);ND_TRACE_PORT(++changed);
 ND_TRACE_PULSE();ND_TRACE_FLUSH();assert(changed==0);return 0;}
'''
with tempfile.TemporaryDirectory(prefix="nd-trace-off-") as temp:
    base=Path(temp);(base/"test.c").write_text(source)
    subprocess.run([os.environ.get("CC","cc"),"-std=c89","-Wall","-Wextra","-Werror","-I"+str(root/"include"),str(base/"test.c"),"-o",str(base/"test")],check=True)
    subprocess.run([str(base/"test")],check=True)
    symbols=subprocess.check_output(["nm",str(base/"test")],text=True)
    assert "nd_trace_" not in symbols
print("Trace hooks compile out without evaluating arguments or linking diagnostics.")
