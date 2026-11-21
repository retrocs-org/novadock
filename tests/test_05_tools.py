#!/usr/bin/env python3
"""Portable behavioral checks for the 0.5 tools and read-only HASS summaries."""
from pathlib import Path
import os,subprocess,tempfile,hashlib,json
ROOT=Path(__file__).resolve().parents[1]
provenance=json.loads((ROOT/'plugin_src/qr/vendor/upstream.json').read_text())
for name,digest in provenance['files'].items():
 assert hashlib.sha256((ROOT/'plugin_src/qr/vendor'/name).read_bytes()).hexdigest()==digest
notice=(ROOT/'Plugins/QR.assets/QR-Code-generator.LICENSE').read_text()
assert 'Copyright (c) Project Nayuki.' in notice and 'all copies or substantial portions' in notice

with tempfile.TemporaryDirectory(prefix='novadock-05-') as tmp:
 p=Path(tmp)
 sources=['tests/test_05_tools.c','plugin_src/bench/nd_bench.c','plugin_src/moon/nd_moon.c','plugin_src/moon/nd_moon_settings.c','plugin_src/qr/nd_qr.c','plugin_src/qr/vendor/qrcodegen.c','plugin_src/colour/nd_colour.c','plugin_src/hass/nd_hass_summary.c','plugin_src/hass/nd_hass.c','src/ilbm.c']
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'include')]+['-I'+str(ROOT/'plugin_src'/name) for name in ['bench','moon','qr','colour','hass']]+[str(ROOT/name) for name in sources]+['-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check'),str(p/'qr.iff')],cwd=p,check=True)
 import verify_qr
 verify_qr.verify(p/'qr.iff')

with tempfile.TemporaryDirectory(prefix='novadock-colour-rollback-') as tmp:
 p=Path(tmp)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-DND_COLOUR_RENAME=qa_colour_rename','-include',str(ROOT/'tests/colour_store_hooks.h'),'-I'+str(ROOT/'plugin_src/colour'),str(ROOT/'tests/test_colour_rollback.c'),str(ROOT/'plugin_src/colour/nd_colour.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],cwd=p,check=True)
