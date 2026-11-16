#!/usr/bin/env python3
"""Keep seasonal surprise content out of public prose and generated notes."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
paths=[ROOT/'README.md',ROOT/'CHANGELOG.md',ROOT/'NovaDock.guide']
paths+=list((ROOT/'docs').rglob('*.md'))
paths+=list((ROOT/'public-web').rglob('*.php'))
for path in paths:
    assert not re.search(r'\b(?:Christmas plugin|Christmas tree|XMAS|CHRISTMAS_PLUGIN|Wintery loop)\b',path.read_text('latin-1' if path.suffix=='.guide' else 'utf-8'),re.I), str(path)
print('Seasonal surprise remains absent from public documentation, release notes and website prose.')
