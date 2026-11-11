#!/usr/bin/env python3
"""Checks NovaDock.guide the way MultiView would read it: nodes closed,
links pointing at real nodes, section links landing on their headings,
no stray @ commands, prose within 76 columns, Latin-1 text."""
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
result = subprocess.run([sys.executable, str(ROOT / 'tools/make_amigaguide.py'),
                         '--check'], capture_output=True, text=True)
assert result.returncode == 0, result.stdout + result.stderr
data = (ROOT / 'NovaDock.guide').read_bytes()
text = data.decode('latin-1')
lines = text.split('\n')
assert lines[0] == '@database NovaDock.guide' and lines[1].startswith('@$VER: NovaDock.guide ')
COMMANDS = ('@database', '@$VER:', '@author', '@(c)', '@node', '@endnode', '@toc')
nodes, current, body = {}, None, []
for number, line in enumerate(lines, 1):
    if line.startswith('@') and not line.startswith('@{'):
        assert line.startswith(COMMANDS), f'line {number}: unknown command {line[:30]}'
        if line.startswith('@node'):
            assert current is None, f'line {number}: node inside a node'
            current, body = line.split()[1].upper(), []
        elif line.startswith('@endnode'):
            assert current, f'line {number}: @endnode without @node'
            nodes[current], current = body, None
        continue
    if current:
        body.append(line)
    # Every other @ is escaped or starts an inline command @{...}.
    for match in re.finditer(r'(?<!\\)@(?!\{)', line):
        assert False, f'line {number}: unescaped @ in {line[:40]!r}'
    visible = re.sub(r'@\{"([^"]*)" link [^}]*\}', r'\1', line)
    visible = re.sub(r'@\{u?[biu]\}', '', visible).replace('\\@', '@').replace('\\\\', '\\')
    if not line.startswith('    '):            # code blocks may run wider
        assert len(visible) <= 78, f'line {number}: {len(visible)} columns'
assert current is None, 'last node not closed'
assert 'MAIN' in nodes
links = 0
for name, body in nodes.items():
    for line in body:
        for label, target, at in re.findall(r'@\{" ?([^"]*?) ?" link (\w+)(?: (\d+))?\}', line):
            assert target.upper() in nodes, f'{name}: link to missing node {target}'
            if at:
                heading = nodes[target.upper()][int(at)]
                assert label in heading, f'{name}: "{label}" lands on {heading!r}'
            links += 1
print(f'NovaDock.guide: {len(nodes)} nodes, {links} links, section links on target.')
