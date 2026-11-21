#!/usr/bin/env python3
"""Propagate the release version from VERSION to everything that shows it.

    python3 tools/update_version.py          apply VERSION everywhere (also
                                             refreshes the website chrome and
                                             its download archives)
    python3 tools/update_version.py --no-package  update metadata without archives
    python3 tools/update_version.py --check  exit 1 if anything disagrees

include/nd_version.h is generated (dock programs build their $VER strings and
version texts from it).  Shell scripts, the release tools, CI and the
website chrome read VERSION themselves.  Release text that names the
version in prose (website pages, README, installer) is checked here, and
rewritten from the previous version when VERSION changes.
"""
from pathlib import Path
import re
import json
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'include' / 'nd_version.h'
# Current build mentions. Screenshot captions retain the captured release version.
TEXT_FILES = ['README.md', 'Install', 'novadock.cfg'] + \
    sorted(str(p.relative_to(ROOT)) for p in (ROOT / 'public-web').glob('*.php')
           if p.name != 'features.php')
MENTION = re.compile(r'(NovaDock[ -])(\d+\.\d+(?:\.\d+)?)\b')


def read_version():
    values = {}
    for line in (ROOT / 'VERSION').read_text('ascii').splitlines():
        if line.strip() and not line.startswith('#'):
            key, value = line.split('=', 1)
            values[key.strip()] = value.strip()
    version, date = values['VERSION'], values['DATE']
    assert re.fullmatch(r'\d+\.\d+(\.\d+)?', version), f'bad VERSION {version}'
    # AmigaOS $VER dates are DD.MM.YYYY.
    assert re.fullmatch(r'\d\d\.\d\d\.\d{4}', date), f'bad DATE {date}'
    return version, date


def header_text(version, date):
    return f'''#ifndef ND_VERSION_H
#define ND_VERSION_H
/* Generated from VERSION by tools/update_version.py - do not edit. */
#define ND_VERSION "{version}"
#define ND_VERSION_DATE "{date}"
/* "$VER: <name> <version> (<date>)" for the AmigaOS Version command. */
#define ND_VER_STRING(name) "$VER: " name " " ND_VERSION " (" ND_VERSION_DATE ")"
#endif
'''


def previous_version():
    if not HEADER.exists():
        return None
    found = re.search(r'#define ND_VERSION "([^"]+)"', HEADER.read_text('ascii'))
    return found.group(1) if found else None


def checklist_text(text,version,previous):
    text=re.sub(r'(## Prepare the )\d+\.\d+(?:\.\d+)?( candidate)',lambda m:m[1]+version+m[2],text)
    text=re.sub(r'(current changelog and release notes at )\d+\.\d+(?:\.\d+)?',lambda m:m[1]+version,text)
    text=re.sub(r'(exact tag )\d+\.\d+(?:\.\d+)?',lambda m:m[1]+version,text)
    if previous:text=re.sub(r'(Upgrade from published )\d+\.\d+(?:\.\d+)?',lambda m:m[1]+previous,text)
    return text

def main():
    check = '--check' in sys.argv[1:]
    version, date = read_version()
    from update_plugin_versions import sync as sync_plugins
    problems = sync_plugins(check=check)
    header = header_text(version, date)
    old = previous_version()
    if not HEADER.exists() or HEADER.read_text('ascii') != header:
        problems.append('include/nd_version.h')
        if not check:
            HEADER.write_text(header, 'ascii')
    installer = ROOT / 'Install'
    script = installer.read_text('latin-1')
    managed = re.sub(r'; \$VER: Install \S+ \(\S+\)',
                     f'; $VER: Install {version} ({date})', script)
    managed = re.sub(r'\(set #version "[^"]*"\)', f'(set #version "{version}")',
                     managed)
    if managed != script:
        problems.append('Install ($VER / #version)')
        if not check:
            installer.write_text(managed, 'latin-1')
    for name in TEXT_FILES:
        path = ROOT / name
        text = path.read_text('latin-1')
        if not check and old and old != version:
            text = MENTION.sub(lambda m: m.group(1) + version
                               if m.group(2) == old else m.group(0), text)
            path.write_text(text, 'latin-1')
        wrong = sorted({m.group(0) for m in MENTION.finditer(text)
                        if m.group(2) != version})
        if wrong:
            problems.append(f'{name} ({", ".join(wrong)})')
    checklist=ROOT/'docs/RELEASE_CHECKLIST.md'
    catalog=json.loads((ROOT/'public-web/releases.json').read_text())
    key=lambda v:tuple(map(int,v.split('.')))
    released=[r['version'] for r in catalog if r.get('status')=='published' and key(r['version'])<key(version)]
    previous=max(released,key=key) if released else None
    wanted=checklist_text(checklist.read_text(),version,previous)
    if wanted!=checklist.read_text():
        problems.append('docs/RELEASE_CHECKLIST.md')
        if not check:checklist.write_text(wanted)
    if check:
        if problems:
            print(f'Not version {version}: ' + '; '.join(problems))
            sys.exit(1)
        print(f'Version {version} ({date}) is consistent everywhere.')
    else:
        remaining = [p for p in problems if not p.startswith(('include/', 'Install (', 'docs/RELEASE_CHECKLIST.md'))]
        if remaining:
            print('Please update by hand: ' + '; '.join(remaining))
            sys.exit(1)
        # The website chrome and its download archives carry the version too.
        import subprocess
        commands = [['tools/make_amigaguide.py'], ['tools/update_site_chrome.py']]
        if '--no-package' not in sys.argv[1:]:
            commands.append(['tools/package_release.py', '--out', str(ROOT.parent / 'dist')])
        for tool in commands:
            subprocess.run([sys.executable, str(ROOT / tool[0])] + tool[1:],
                           check=True, stdout=subprocess.DEVNULL)
        print(f'Version {version} ({date}) applied: programs, installer, '
              + ('website.' if '--no-package' in sys.argv[1:] else 'website and downloads.'))


if __name__ == '__main__':
    main()
