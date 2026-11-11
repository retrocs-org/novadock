#!/usr/bin/env python3
"""Propagate the release version from VERSION to everything that shows it.

    python3 tools/update_version.py          apply VERSION everywhere (also
                                             refreshes the website chrome and
                                             its download archives)
    python3 tools/update_version.py --check  exit 1 if anything disagrees

include/nd_version.h is generated (C programs build their $VER strings and
version texts from it).  Shell scripts, the release tools, CI and the
website chrome read VERSION themselves.  Release text that names the
version in prose (website pages, README, installer) is checked here, and
rewritten from the previous version when VERSION changes.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / 'include' / 'nd_version.h'
# Text that mentions the release as "NovaDock <version>" / "NovaDock-<version>".
TEXT_FILES = ['README.md', 'Install-NovaDock'] + \
    sorted(str(p.relative_to(ROOT)) for p in (ROOT / 'public-web').glob('*.php'))
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


def main():
    check = '--check' in sys.argv[1:]
    version, date = read_version()
    problems = []
    header = header_text(version, date)
    old = previous_version()
    if not HEADER.exists() or HEADER.read_text('ascii') != header:
        problems.append('include/nd_version.h')
        if not check:
            HEADER.write_text(header, 'ascii')
    installer = ROOT / 'Install-NovaDock'
    script = installer.read_text('latin-1')
    managed = re.sub(r'; \$VER: Install-NovaDock \S+ \(\S+\)',
                     f'; $VER: Install-NovaDock {version} ({date})', script)
    managed = re.sub(r'\(set #version "[^"]*"\)', f'(set #version "{version}")',
                     managed)
    if managed != script:
        problems.append('Install-NovaDock ($VER / #version)')
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
    if check:
        if problems:
            print(f'Not version {version}: ' + '; '.join(problems))
            sys.exit(1)
        print(f'Version {version} ({date}) is consistent everywhere.')
    else:
        remaining = [p for p in problems if not p.startswith(('include/', 'Install-NovaDock ('))]
        if remaining:
            print('Please update by hand: ' + '; '.join(remaining))
            sys.exit(1)
        # The website chrome and its download archives carry the version too.
        import subprocess
        for tool in (['tools/make_amigaguide.py'], ['tools/update_site_chrome.py'],
                     ['tools/package_release.py', '--out', str(ROOT.parent / 'dist')]):
            subprocess.run([sys.executable, str(ROOT / tool[0])] + tool[1:],
                           check=True, stdout=subprocess.DEVNULL)
        print(f'Version {version} ({date}) applied: programs, installer, '
              'website and downloads.')


if __name__ == '__main__':
    main()
