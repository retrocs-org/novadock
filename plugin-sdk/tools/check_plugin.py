#!/usr/bin/env python3
"""Check a source plugin project against the NovaDock SDK diagnostics policy.

This checks integration, not arbitrary program behavior or runtime correctness.
"""
from pathlib import Path
from check_plugin_art import check_project
import argparse,re

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('project',type=Path,nargs='?',default=Path('.'));args=parser.parse_args();root=args.project.resolve()
 make=(root/'Makefile').read_text();problems=[]
 if '-DND_PLUGIN_RESOURCES' not in make:problems.append('Enable ND_PLUGIN_RESOURCES in the build flags.')
 helpers={'nd_client.c','nd_plugin_resources.c','nd_option_runtime.c'}
 sources=[p for p in root.rglob('*.c') if p.name not in helpers]
 app='\n'.join(p.read_text() for p in sources)
 for call in ('nd_client_init','nd_client_close'):
  if call+'(' not in app:problems.append('Application sources must use '+call+'.')
 if not re.search(r'\bnd_client_update(?:_\w+)?\s*\(',app):problems.append('Use nd_client updates for host policy negotiation.')
 for p in sources:
  text=p.read_text()
  if re.search(r'\b(?:malloc|calloc|realloc|free|AllocVec|FreeVec|AllocMem|FreeMem|Open|Read|Write|ExNext)\s*\(',text) and '"nd_plugin_resources.h"' not in text:problems.append(str(p.relative_to(root))+': include nd_plugin_resources.h after library headers.')
 if 'nd_client_activity(' not in app:problems.append('Add fixed numeric nd_client_activity events around meaningful work.')
 for p in ('include/nd_plugin_support.h','include/nd_plugin_resources.h','src/nd_plugin_resources.c'):
  if not (root/p).is_file():problems.append('Missing SDK component: '+p)
 artwork_problems,banks,frames,cases=check_project(root);problems.extend(artwork_problems)
 for message in problems:print('FAIL',message)
 if problems:return 1
 print('PASS SDK client, resource wrappers, activity events and build flags are integrated.')
 print(f'PASS artwork size policy: {banks} banks, {frames} states, {cases} Tiny/Normal checks.')
 print('Visually verify every artwork state in Tiny/Normal, idle/magnified, before release.')
 print('Also test Off/Logging/Trace at runtime; do not write independent debug files or private text.')
 return 0
if __name__=='__main__':raise SystemExit(main())
