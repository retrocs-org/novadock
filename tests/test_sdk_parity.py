#!/usr/bin/env python3
"""The Plugin SDK ships copies of dock headers and client sources.

Every SDK copy must be byte-identical to the file the dock and the bundled
plugins are built from, otherwise SDK plugins compile against a different
protocol or artwork layout than the dock reads.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SDK = ROOT / 'plugin-sdk'
SEARCH = (ROOT / 'include', ROOT / 'plugin_src' / 'common')

checked = 0
for copy in sorted(list((SDK / 'include').glob('*.h')) + list((SDK / 'src').glob('*.c'))):
    origin = next((base / copy.name for base in SEARCH if (base / copy.name).is_file()), None)
    if origin is None:
        continue  # SDK-only helper (none today)
    assert copy.read_bytes() == origin.read_bytes(), \
        f'{copy.relative_to(ROOT)} differs from {origin.relative_to(ROOT)}; copy it into the SDK'
    checked += 1
assert checked >= 8, checked
print(f'Plugin SDK: {checked} shared headers and sources match the dock build.')
