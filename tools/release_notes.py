#!/usr/bin/env python3
"""Release notes for the version in VERSION, from its CHANGELOG.md section.

    python3 tools/release_notes.py > notes.md

Prints the section that starts with "## NovaDock <version>" (up to the next
"## " heading), then installation steps.  Exits 1 if the CHANGELOG has no
section for the version, so a release is never published without notes.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    version = next(line.split('=', 1)[1].strip()
                   for line in (ROOT / 'VERSION').read_text('ascii').splitlines()
                   if line.startswith('VERSION='))
    text = (ROOT / 'CHANGELOG.md').read_text('utf-8')
    found = re.search(rf'^## NovaDock {re.escape(version)}\b.*?$\n(.*?)(?=^## |\Z)',
                      text, re.M | re.S)
    if not found or not found.group(1).strip():
        print(f'CHANGELOG.md has no "## NovaDock {version}" section.', file=sys.stderr)
        sys.exit(1)
    print(found.group(1).strip())
    print(f'''
### Installation

1. Download **NovaDock-{version}.lha** and unpack it anywhere, for example in RAM:.
2. Open the **NovaDock** drawer and double-click **Install-NovaDock**
   (Installer 42 or later, included with AmigaOS 3.1 and newer).
3. Without the Installer, run `Execute install.amiga` in that drawer.

Running the installer again updates NovaDock and keeps your settings, or
removes it. Requires AmigaOS 3.0 or later on any 68000 or better.''')


if __name__ == '__main__':
    main()
