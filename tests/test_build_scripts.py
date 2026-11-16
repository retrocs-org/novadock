#!/usr/bin/env python3
"""build.amiga (native AmigaDOS build) compiles every program from the same
source files as the native Makefile, so neither can fall behind."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
makefile = (ROOT / 'Makefile').read_text()
programs = re.search(r'^all:(.*)$', makefile, re.M).group(1).split()
script = {}
for line in (ROOT / 'build.amiga').read_text().splitlines():
    if line.startswith('vc '):
        words = line.split()
        target = words[words.index('-o') + 1]
        script[target] = sorted(w for w in words if w.endswith(('.c', '.asm')))
for program in programs:
    recipe = re.search(r'^' + re.escape(program) + r':.*\n\t(.*)$', makefile, re.M)
    assert recipe, f'Makefile has no recipe for {program}'
    wanted = sorted(w for w in recipe.group(1).split() if w.endswith(('.c', '.asm')))
    assert program in script, f'build.amiga does not build {program}'
    missing = sorted(set(wanted) - set(script[program]))
    extra = sorted(set(script[program]) - set(wanted))
    assert not missing and not extra, \
        f'build.amiga {program}: missing {missing}, extra {extra}'
    for source in wanted:
        assert (ROOT / source).exists(), f'{program}: {source} does not exist'
print(f'build.amiga matches the Makefile for all {len(programs)} programs.')

# Switching development versions must also remove old runnable downloads.
import importlib.util
import tempfile
spec = importlib.util.spec_from_file_location('package_cleanup', ROOT/'tools/package_release.py')
package_cleanup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package_cleanup)
with tempfile.TemporaryDirectory() as temporary:
    folder = Path(temporary)
    obsolete = ['NovaDock-0.1-source.zip', 'NovaDock-0.1-plugin-sdk.zip',
                'NovaDock-0.1.lha', 'NovaDock-0.1.lha.sha256', 'NovaDock-0.1.readme',
                'NovaDock-0.1-plugin-sdk.zip.sha256',
                'NovaDock-0.1-public-web-updated.zip',
                'NovaDock-0.2-continued-source-and-site.zip']
    retained = ['NovaDock-0.2-source.zip', 'NovaDock-0.2-plugin-sdk.zip',
                'NovaDock-0.2.lha', 'NovaDock-0.2.readme', 'SHA256SUMS.txt', 'notes.txt']
    for name in obsolete + retained:
        (folder/name).write_text('fixture')
    package_cleanup.prune_downloads(folder, '0.2')
    assert sorted(p.name for p in folder.iterdir()) == sorted(retained)
print('Version changes remove old generated downloads and retain unrelated files.')

# Missing translation tools must stop production packaging before any build/output.
import os
import subprocess
with tempfile.TemporaryDirectory() as temporary:
    output = Path(temporary)/'release'
    env = dict(os.environ, FLEXCAT=str(Path(temporary)/'missing-flexcat'))
    result = subprocess.run(['sh', str(ROOT/'scripts/make-release.sh'), str(output)],
                            env=env, capture_output=True, text=True)
    assert result.returncode != 0 and 'release requires FlexCat' in result.stderr
    assert not output.exists(), 'failed prerequisite must not create release state'
print('Release packaging refuses missing translations before building.')
