#!/usr/bin/env python3
"""A release is built from a clean Git checkout, so every file the build and
installer reference must be tracked.  Skipped outside a Git work tree (for
example in a source archive)."""
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
try:
    inside = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', '--is-inside-work-tree'],
                            capture_output=True, text=True)
except FileNotFoundError:
    inside = None
if not inside or inside.returncode or inside.stdout.strip() != 'true':
    print('Tracked sources: skipped (not a Git work tree).')
    raise SystemExit(0)

tracked = set(subprocess.run(['git', '-C', str(ROOT), 'ls-files', '-z'], check=True,
                             capture_output=True).stdout.decode().split('\0'))
referenced = set()
for build in ('Makefile', 'Makefile.cross', 'build.amiga'):
    for word in re.findall(r'[A-Za-z0-9_./-]+', (ROOT / build).read_text(encoding='latin-1')):
        if not word.startswith('/') and word.endswith(('.c', '.h', '.asm', '.py')) and (ROOT / word).is_file():
            referenced.add(word)
for script in ('install.amiga', 'scripts/make-release.sh'):
    for word in re.findall(r'[A-Za-z0-9_./-]+', (ROOT / script).read_text(encoding='latin-1')):
        # "$project_root/tools/x.py" -> "tools/x.py": drop leading components
        # until the rest names a file in the project.
        parts = [part for part in word.split('/') if part]
        while len(parts) > 1 and not (ROOT / '/'.join(parts)).is_file():
            parts = parts[1:]
        if len(parts) > 1:
            referenced.add('/'.join(parts))
missing = sorted(path for path in referenced if path not in tracked)
assert not missing, 'Referenced by the build but not tracked by Git:\n  ' + '\n  '.join(missing)
print(f'Tracked sources: all {len(referenced)} files referenced by the build and installers are in Git.')
