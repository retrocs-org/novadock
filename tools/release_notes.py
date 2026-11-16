#!/usr/bin/env python3
"""Generate matching repository notes and plain-text Aminet metadata."""
import argparse
import os
from pathlib import Path
import re
import sys
import textwrap
from urllib.parse import quote

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = 'https://github.com/retrocs-org/novadock'
WEBSITE = 'https://novadock.retrocs.org'
GROUPS = ('Features', 'Enhancements', 'Bugs')
DESCRIPTION = 'NovaDock is a lightweight, extensible dock for Classic AmigaOS Workbench.'
FEATURES = (
    'Application, command, drawer and volume launchers, with drag-and-drop placement, groups and drawer popups.',
    'Thirteen themes, third-party theme imports, two tile sizes, auto-hide and multi-row layouts on any screen edge.',
    'Native Aminet browser with recent uploads, categories, readmes and download progress.',
    'Independent MUSIC and WHDLoad collections with recursive indexing, cached results and title filters.',
    'Current weather and five-day forecasts, permanent Markdown Notes and countdown Timer reminders.',
    'ARCHIVE creates ZIP/LHA files from multiple files and drawers, or extracts archives to a chosen destination, preserving Workbench icons.',
    'Music controls for LumiAMP, EaglePlayer, AmigaAMP and HippoPlayer; screen capture, resolution selection and public-screen switching.',
    'System meters, clipboard history, Trash and application integrations; unified Preferences, diagnostics, ARexx control and a public plugin SDK.',
)

def version(root=ROOT):
    return next(s.split('=', 1)[1].strip() for s in (root/'VERSION').read_text('ascii').splitlines() if s.startswith('VERSION='))

def changes(root=ROOT):
    v = version(root)
    text = (root/'CHANGELOG.md').read_text('utf-8')
    sections = list(re.finditer(r'^## NovaDock ([^\s]+)([^\n]*)\n', text, re.M))
    found = [m for m in sections if m[1] == v]
    if len(found) != 1:
        raise ValueError('CHANGELOG.md must contain exactly one NovaDock '+v+' section.')
    match = found[0]; end = next((m.start() for m in sections if m.start() > match.start()), len(text))
    body = text[match.end():end].strip()
    if re.search(r'\bin development\b|\bTBD\b|\bTODO\b', match[2]+'\n'+body, re.I):
        raise ValueError('Current release changelog contains an unfinished placeholder.')
    groups = {name: [] for name in GROUPS}; group = 'Features'
    for line in body.splitlines():
        if not line.strip(): continue
        if line.startswith('### '):
            group = line[4:].strip()
            if group == 'Bug fixes': group = 'Bugs'
            if group not in groups: raise ValueError('Unknown current changelog section: '+group)
        elif line.startswith('- '): groups[group].append(line[2:].strip())
        elif line[:1].isspace() and groups[group]: groups[group][-1] += ' '+line.strip()
        else: raise ValueError('Current changelog entries must be grouped bullet lists.')
    if not any(groups.values()): raise ValueError('Current release changelog is empty.')
    return groups

def plugin_names(root=ROOT):
    match = re.search(r'^all:\s*(.+)$', (root/'Makefile').read_text(), re.M)
    if not match: raise ValueError('Missing native plugin inventory.')
    fallback = {'Activity':'CPU', 'Memory':'RAM', 'ArchiveExtractor':'ARCHIVE', 'RunningApps':'APPS', 'Games':'WHDLoad'}
    names = []
    for program in match[1].split():
        if not program.startswith('Plugins/'): continue
        executable = program.split('/', 1)[1]
        if executable == 'Christmas': continue
        path = root/'Plugins'/(executable+'.ndplugin')
        name = fallback.get(executable, executable)
        if path.is_file():
            found = re.search(r'^NOVADOCK_NAME=(.+)$', path.read_text('ascii'), re.M)
            if found: name = found[1].strip()
        names.append(name)
    return names

def changelog_markdown(root=ROOT):
    lines = ['### '+version(root)]
    for name, entries in changes(root).items():
        if entries: lines += ['', '#### '+name, ''] + ['- '+entry for entry in entries]
    return '\n'.join(lines)

def markdown(root=ROOT, tag=None):
    v = version(root); encoded = quote(tag or v, safe='')
    return '\n'.join([
        '# NovaDock '+v, '', DESCRIPTION, '', '## Features', '',
        *['- '+entry for entry in FEATURES], '',
        'Bundled plugins: '+', '.join(plugin_names(root))+'.', '',
        '## Installation', '',
        'Download **NovaDock-'+v+'.lha**, unpack it and open the **NovaDock** drawer.',
        'Double-click **Install-NovaDock** and follow the graphical installer. It can',
        'install, update or remove NovaDock, preserving settings on update.', '',
        'Requires AmigaOS 3.0 or later and a 68000 or better. The graphical installer',
        'needs Installer 42 or newer (included with AmigaOS 3.1 and later). For a',
        'standard-location Shell install, run `Execute install.amiga`.', '',
        'Help: `HELP:NovaDock.guide`. Settings and permanent Notes live under',
        '`ENVARC:NovaDock` by default. Weather needs internet access and an',
        'HTTPS-capable curl or Wget; other integrations need their corresponding apps.', '',
        '## Source and Licence', '',
        '[Source and Plugin SDK]('+REPOSITORY+') |',
        '[Tagged release]('+REPOSITORY+'/releases/tag/'+encoded+') |',
        '[Website]('+WEBSITE+') |',
        '[MIT licence]('+REPOSITORY+'/blob/'+encoded+'/LICENSE)', '',
        '## Version Changelog', '', changelog_markdown(root), '',
    ])

def validate_notes(text, root=ROOT, tag=None):
    headings = re.findall(r'^## (.+)$', text, re.M)
    expected = ['Features', 'Installation', 'Source and Licence', 'Version Changelog']
    if not text.startswith('# NovaDock '+version(root)+'\n') or headings != expected:
        raise ValueError('Release notes need the NovaDock title and Features, Installation, Source and Licence, Version Changelog sections.')
    if text.split('## Version Changelog\n', 1)[1].strip() != changelog_markdown(root).strip():
        raise ValueError('Release notes must contain the complete current-version changelog only.')
    url = REPOSITORY+'/releases/tag/'+quote(tag or version(root), safe='')
    if url not in text or 'NovaDock-'+version(root)+'.lha' not in text:
        raise ValueError('Release notes must link the exact tagged release and versioned LHA.')
    if re.search(r'\bin development\b|\bTBD\b|\bTODO\b', text, re.I):
        raise ValueError('Release notes contain an unfinished placeholder.')

def plain(text):
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', r'\1 (\2)', text)
    text = text.replace('`', '').replace('**', '')
    text = text.translate(str.maketrans({'\u2013':'-', '\u2014':'-', '\u2018':"'", '\u2019':"'", '\u201c':'"', '\u201d':'"', '\u2026':'...'}))
    try: text.encode('ascii')
    except UnicodeEncodeError: raise ValueError('Release text must have a plain ASCII Aminet representation.') from None
    return text

def wrap(text, indent='', bullet=False):
    return textwrap.fill(plain(text), width=76, initial_indent=indent+('- ' if bullet else ''), subsequent_indent=indent+('  ' if bullet else ''), break_long_words=False, break_on_hyphens=False)

def aminet(root=ROOT, uploader=None, author=None):
    v = version(root)
    if uploader is None: uploader = os.environ.get('UPLOADER', '')
    if author is None: author = os.environ.get('AUTHOR', 'NovaDock contributors')
    for value in (uploader, author):
        if not value or any(ord(c)<32 or ord(c)>126 for c in value): raise ValueError('Set a plain ASCII author and real uploader contact.')
    if not re.fullmatch(r'[^\s<>@()]+@[^\s<>@()]+\.[^\s<>@()]+', uploader): raise ValueError('Set UPLOADER to your real email before preparing an Aminet submission.')
    lines = [
        'Short:        Extensible application dock', 'Author:       '+author,
        'Uploader:     '+uploader, 'Type:         util/wb', 'Version:      '+v,
        'Architecture: m68k-amigaos >= 3.0.0',
        'Requires:     AmigaOS 3.0 or later, any 68000 or better',
        'Distribution: Aminet', 'URL:          '+WEBSITE, '', DESCRIPTION, '', 'FEATURES', '',
        *[wrap(entry, bullet=True) for entry in FEATURES], '',
        wrap('Bundled plugins: '+', '.join(plugin_names(root))+'.'), '', 'INSTALLATION', '',
        wrap('Unpack novadock.lha, open the NovaDock drawer and double-click Install-NovaDock. The graphical installer requires Installer 42 or newer, included with AmigaOS 3.1 and later. It supports installation, updating and removal; updates preserve settings.'), '',
        wrap('For a standard-location Shell install, run "Execute install.amiga" in the NovaDock drawer. Help is installed as HELP:NovaDock.guide. Settings and permanent Notes use ENVARC:NovaDock by default. Weather updates need internet access and an HTTPS-capable curl or Wget. Other integrations require their corresponding applications.'), '',
        'SOURCE AND LICENCE', '', REPOSITORY, REPOSITORY+'/releases/tag/'+quote(v, safe=''), WEBSITE,
        'MIT licence; see LICENSE in the archive.', '', 'VERSION CHANGELOG', '', '  '+v,
    ]
    for name, entries in changes(root).items():
        if entries: lines += ['', '  '+name] + [wrap(entry, indent='    ', bullet=True) for entry in entries]
    text = '\n'.join(lines)+'\n'
    if any(len(line)>76 for line in text.splitlines()): raise ValueError('Aminet metadata or an unbroken word exceeds 76 columns.')
    return text

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Check the committed docs/RELEASE-NOTES.md against its source')
    parser.add_argument('--write', action='store_true', help='Refresh docs/RELEASE-NOTES.md before committing release inputs')
    parser.add_argument('--format', choices=('markdown','aminet'), default='markdown')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--tag')
    args = parser.parse_args()
    try:
        if args.check and args.write: raise ValueError('Choose --check or --write.')
        if (args.check or args.write) and (args.output or args.format!='markdown' or args.tag): raise ValueError('Documentation check/write uses the default version tag and Markdown format.')
        content = aminet() if args.format=='aminet' else markdown(tag=args.tag)
        if args.check:
            path = ROOT/'docs/RELEASE-NOTES.md'
            if not path.is_file() or path.read_text('utf-8')!=content: raise ValueError('Refresh docs/RELEASE-NOTES.md with tools/release_notes.py --write, then review and commit it.')
        elif args.write: (ROOT/'docs/RELEASE-NOTES.md').write_text(content, encoding='utf-8')
        elif args.output: args.output.write_text(content, encoding='ascii' if args.format=='aminet' else 'utf-8')
        else: print(content, end='')
    except (ValueError,OSError) as error: parser.exit(1, str(error)+'\n')

if __name__ == '__main__': main()
