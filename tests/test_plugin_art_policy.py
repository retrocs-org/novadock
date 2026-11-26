#!/usr/bin/env python3
"""SDK rejects missing/corrupt banks and artwork that vanishes at Tiny size."""
from pathlib import Path
import sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from check_plugin_art import check_bank,check_project
from pack_plugin_art import build_bank
from package_release import SDK_FILES
palette=[[i*16]*3 for i in range(16)]
full=[[1]*32 for _ in range(32)];dot=[[0]*32 for _ in range(32)];dot[0][0]=1
with tempfile.TemporaryDirectory(prefix='nd-art-policy-',dir='/private/tmp') as tmp:
 p=Path(tmp);manifest=p/'Example.ndplugin';bank=p/'Example.assets/Artwork.ndart'
 manifest.write_text('NOVADOCK_PROGRAM=Example\nNOVADOCK_FEATURES=artwork\n')
 assert check_project(p)[0]
 bank.parent.mkdir();bank.write_bytes(build_bank([dict(name='icon',state=0,pixels=full),dict(name='icon',state=1,pixels=full)],palette))
 assert not check_project(p)[0] and check_bank(bank)==(2,8)
 good=bank.read_bytes()
 for bad in (good[:55],good[:100],good[:56]+good[56:80]+b'\0\1'+good[82:],good[:84]+b'\xff\xff\xff\xff'+good[88:]):
  bank.write_bytes(bad)
  assert check_project(p)[0]
 bank.write_bytes(build_bank([dict(name='icon',state=0,pixels=full),dict(name='icon',state=1,pixels=dot)],palette))
 problems=check_project(p)[0];assert problems and any('disappears in' in s for s in problems),problems
 bank.write_bytes(good)
 # The standalone SDK places authoring manifests in examples, with runtime
 # assets alongside the built executable at project root.
 (p/'examples').mkdir();manifest.rename(p/'examples/Example.ndplugin');assert not check_project(p)[0]
 assert (ROOT/'plugin-sdk/tools/check_plugin_art.py').read_bytes()==(ROOT/'tools/check_plugin_art.py').read_bytes()
 assert ('tools/check_plugin_art.py','tools/check_plugin_art.py') in SDK_FILES
 assert not check_project(ROOT/'Plugins')[0]
 assert not check_project(ROOT/'plugin-sdk')[0]
print('Artwork policy: missing/corrupt banks, vanished Tiny states, all bundled banks, SDK project layout and SDK validator packaging passed.')
