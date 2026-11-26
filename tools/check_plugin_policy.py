#!/usr/bin/env python3
"""Check shared diagnostics wiring for bundled clients and the source SDK."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
def main():
 count=0
 for name in ('Makefile','Makefile.cross','build.amiga'):
  for line in (ROOT/name).read_text().splitlines():
   if 'nd_client.c' in line and ('$(COMPILE)' in line or '$(VC)' in line or line.lstrip().startswith('vc ')):
    assert '-DND_PLUGIN_RESOURCES' in line,(name,line)
    count+=1
    for source in re.findall(r'plugin_src/[^ ]+\.c',line):
     p=ROOT/source
     if p.name=='nd_client.c' or source in ('plugin_src/hass/nd_hass_client.c','plugin_src/hass/nd_hass.c'):continue # frozen upstream library interface
     text=p.read_text()
     if re.search(r'\b(?:AllocVec|FreeVec|AllocMem|FreeMem|malloc|calloc|realloc|free|Open|Read|Write|ExNext)\s*\(',text):assert '"nd_plugin_resources.h"' in text,source
 assert count>=40,count
 sdk=(ROOT/'plugin_src/template/Makefile.sdk').read_text();assert '-DND_PLUGIN_RESOURCES' in sdk
 for name in ('hello.c','arthello.c'):assert '"nd_plugin_resources.h"' in (ROOT/'plugin_src/template'/name).read_text()
 from package_release import SDK_FILES
 for member,source in SDK_FILES:assert (ROOT/'plugin-sdk'/member).read_bytes()==(ROOT/source).read_bytes(),member
 print('Plugin diagnostics policy: %d build recipes, allocation/I/O wrappers and SDK templates agree.'%count)
if __name__=='__main__':main()
