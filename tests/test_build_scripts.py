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
