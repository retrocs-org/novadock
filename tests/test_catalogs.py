#!/usr/bin/env python3
"""Translations follow the dock's own rules (src/nd_locale.c nd_trf):
format strings keep their %s/%d/%u conversions in order and fit 80
characters, or NovaDock silently falls back to English.  Files must be
Latin-1 and free of blank lines (FlexCat reads one as a message without an
identifier).  With FLEXCAT set, every catalog is compiled as well."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
cd = (ROOT / 'catalogs' / 'novadock.cd').read_text('latin-1')
english = dict(re.findall(r'^MSG_(\d{4}) \(\d+//\)\n(.*)$', cd, re.M))
assert '\n\n' not in cd, 'novadock.cd has a blank line'


def conversions(text):
    # nd_trf compares only the conversion letters, skipping flags/precision.
    return [m[-1] for m in re.findall(r'%[^sdu]*[sdu]', text)]


catalogs = sorted((ROOT / 'catalogs').glob('*.ct'))
assert catalogs, 'no translations'
for ct in catalogs:
    text = ct.read_text('latin-1')
    assert '\n\n' not in text, f'{ct.name} has a blank line'
    assert re.search(r'^## language \w+', text, re.M), f'{ct.name} has no ## language'
    translated = dict(re.findall(r'^MSG_(\d{4})\n([^\n]*)\n; ', text, re.M))
    assert set(translated) == set(english), f'{ct.name}: messages differ from novadock.cd'
    for ident, source in english.items():
        target = translated[ident]
        if '%' in source:
            assert conversions(target) == conversions(source), \
                f'{ct.name} MSG_{ident}: conversions {conversions(target)} != {conversions(source)}'
            assert len(target) <= 80, f'{ct.name} MSG_{ident}: {len(target)} > 80 characters'
flexcat = os.environ.get('FLEXCAT')
compiled = 0
if flexcat:
    with tempfile.TemporaryDirectory() as tmp:
        for ct in catalogs:
            out = Path(tmp) / (ct.stem + '.catalog')
            result = subprocess.run([flexcat, str(ROOT / 'catalogs/novadock.cd'), str(ct),
                                     'CATALOG', str(out)], capture_output=True)
            # 5 is FlexCat's warning for a deliberate %.0s plural (see
            # catalogs/README.txt); the catalog is still written.
            assert result.returncode in (0, 5) and out.read_bytes()[:4] == b'FORM' and \
                out.read_bytes()[8:12] == b'CTLG', f'FlexCat failed on {ct.name}'
            compiled += 1
names = ', '.join(c.stem for c in catalogs)
print(f'Catalogs ({names}): {len(english)} messages, formats valid'
      + (f', {compiled} compiled with FlexCat.' if flexcat else '.'))
