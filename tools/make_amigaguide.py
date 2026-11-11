#!/usr/bin/env python3
"""Build NovaDock.guide, the AmigaGuide manual, from README.md and docs/*.md.

    python3 tools/make_amigaguide.py          write NovaDock.guide
    python3 tools/make_amigaguide.py --check  exit 1 if it is out of date

The guide is pre-wrapped at 76 columns and uses only AmigaGuide V39
features (nodes, links, bold), so it reads the same in MultiView on
AmigaOS 3.0 and later.  Every document becomes one node, grouped on a
contents page; long documents start with a clickable section list.
Markdown cross-links become AmigaGuide links and web links show their
address.  Text is ISO-8859-1; a character without a Latin-1 or ASCII
equivalent stops the build rather than appearing as garbage.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'NovaDock.guide'
WIDTH = 76
# Contents page: every Markdown document, with the title the manual uses.
GROUPS = (
    ('Using NovaDock', (
        ('README.md', 'About NovaDock and installation'),
        ('docs/LAUNCHERS_AND_ICONS.md', 'Launchers and icons'),
        ('docs/RESPONSIVE_SETTINGS.md', 'Responsive settings and diagnostics'),
        ('docs/AUTOHIDE_AND_DOCK_MENU.md', 'Auto-hide and the dock menu'),
        ('docs/DRAWER_POPUPS.md', 'Drawer popups'),
        ('docs/REMOVABLE_MEDIA.md', 'Removable media'),
        ('docs/TASKBAR.md', 'Running applications'),
        ('docs/THEMES.md', 'Themes'),
        ('docs/ARexx_AND_DROPS.md', 'ARexx, tooltips and drops'),
        ('docs/EVENT_HOOKS.md', 'Event hooks'))),
    ('Bundled plugins', (
        ('docs/AMIRC_PLUGIN.md', 'AmIRC'),
        ('docs/BITSKIM_PLUGIN.md', 'BitSkim'),
        ('docs/NETWORK_PLUGIN.md', 'Network'),
        ('docs/CLIPBOARD_PLUGIN.md', 'Clipboard History'),
        ('docs/ARCHIVE_PLUGIN.md', 'Archive Extractor'),
        ('docs/COMMAND_TILE.md', 'Command tiles'),
        ('docs/SCRIPT_TILE.md', 'Script tiles'),
        ('docs/DOPUS4_PLUGIN.md', 'Directory Opus 4'),
        ('docs/EYES_PLUGIN.md', 'Eyes'),
        ('docs/MUSICPLAYER_PLUGIN.md', 'MusicPlayer'),
        ('docs/MIXER_PLUGIN.md', 'Mixer'),
        ('docs/SCREENSHOT_PLUGIN.md', 'Screenshot'),
        ('docs/SCREENMODE_PLUGIN.md', 'Screen Mode'),
        ('docs/TRASH_PLUGIN.md', 'Trash'),
        ('docs/GAMES_PLUGIN.md', 'WHDLoad games'))),
    ('Writing plugins', (
        ('docs/PLUGIN_SDK_QUICKSTART.md', 'Plugin SDK quick start'),
        ('docs/PLUGIN_SDK_SETTINGS.md', 'Creating plugin settings windows'),
        ('docs/PLUGIN_API.md', 'Plugin protocol'),
        ('docs/PLUGIN_MANIFEST.md', 'Plugin manifests'),
        ('docs/PLUGIN_SETTINGS.md', 'Plugin settings'))),
    ('Maintaining NovaDock', (
        ('docs/RELEASING.md', 'Building and publishing releases'),
        ('docs/RELEASE_CHECKLIST.md', 'Release checklist'),
        ('docs/NETWORK_MIGRATION.md', 'Upgrading Network settings'),)),
)
# Typography the Amiga's Latin-1 fonts do not have.
PLAIN = {'\u2014': '--', '\u2013': '-', '\u2192': '->', '\u2190': '<-',
         '\u2026': '...', '\u2018': "'", '\u2019': "'", '\u201c': '"',
         '\u201d': '"', '\u2264': '<=', '\u2265': '>=', '\u2022': '*'}
BOLD_ON, BOLD_OFF = '\x01', '\x02'          # placeholders until escaping
ITALIC_ON, ITALIC_OFF = '\x03', '\x04'
# *italic*, but not a lone * such as a wildcard ("* and ?").
ITALIC = re.compile(r'(?<![\w*])\*(?=\S)([^*\n]+?)(?<=\S)\*(?![\w*])')


def node_name(path):
    return 'README' if path == 'README.md' else Path(path).stem.upper()


def version():
    values = dict(line.split('=', 1) for line in
                  (ROOT / 'VERSION').read_text('ascii').splitlines()
                  if '=' in line and not line.startswith('#'))
    return values['VERSION'].strip(), values['DATE'].strip()


def escape(text):
    """AmigaGuide treats @ and backslash specially."""
    return text.replace('\\', '\\\\').replace('@', '\\@')


def inline(text, nodes):
    """Markdown inline text -> list of words; each word is
    (guide text, visible width).  Links stay whole."""
    words, buffer, width = [], '', 0

    def flush():
        nonlocal buffer, width
        if buffer:
            words.append((buffer, width))
        buffer, width = '', 0

    pos = 0
    pattern = re.compile(r'\[([^\]]+)\]\(([^)]+)\)|`([^`]*)`|\*\*|' +
                         ITALIC.pattern)
    pieces = []
    for match in pattern.finditer(text):
        pieces.append(('text', text[pos:match.start()]))
        if match.group(0) == '**':
            pieces.append(('bold', None))
        elif match.group(4) is not None:
            pieces.append(('italic', match.group(4)))
        elif match.group(3) is not None:
            pieces.append(('text', match.group(3)))
        else:
            label, target = match.group(1), match.group(2)
            label = re.sub(r'`|\*\*', '', label)
            node = target.split('#')[0]
            node = node_name(node) if node.endswith('.md') else None
            if target.startswith(('http:', 'https:')):
                pieces.append(('text', f'{label} ({target})'))
            elif node in nodes:
                pieces.append(('link', label, node))
            else:
                pieces.append(('text', label))
        pos = match.end()
    pieces.append(('text', text[pos:]))
    bold = False
    for piece in pieces:
        if piece[0] == 'bold':
            bold = not bold
            buffer += BOLD_ON if bold else BOLD_OFF
        elif piece[0] == 'italic':
            buffer += ITALIC_ON
            for n, word in enumerate(piece[1].split()):
                if n:
                    flush()
                buffer += escape(word)
                width += len(word)
            buffer += ITALIC_OFF
        elif piece[0] == 'link':
            label = piece[1].replace('"', "'")
            buffer += '@{"' + escape(label) + '" link ' + piece[2] + '}'
            width += len(label)
        else:
            for ch in piece[1]:
                if ch in ' \t\n':
                    flush()
                else:
                    buffer += escape(ch)
                    width += 1
    if bold:
        buffer += BOLD_OFF
    flush()
    return words


def wrap(words, first='', rest=''):
    lines, line, width = [], first, len(first)
    started = False
    for text, size in words:
        if started and width + 1 + size > WIDTH:
            lines.append(line)
            line, width, started = rest, len(rest), False
        if started:
            line += ' '
            width += 1
        line += text
        width += size
        started = True
    lines.append(line)
    return lines


def plain(text):
    """Inline Markdown reduced to plain text (table cells)."""
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', r'\1', text)
    return re.sub(r'`|\*\*', '', text).strip()


def table(rows):
    cells = [[plain(c) for c in row.strip().strip('|').split('|')] for row in rows]
    cells = [r for r in cells if not all(re.fullmatch(r':?-+:?', c) for c in r if c)]
    if not cells:
        return []
    columns = max(len(r) for r in cells)
    cells = [r + [''] * (columns - len(r)) for r in cells]
    widths = [max(len(r[i]) for r in cells) for i in range(columns)]
    out = []
    if 2 + sum(widths) + 2 * (columns - 1) <= WIDTH:
        for n, row in enumerate(cells):
            text = '  '.join(c.ljust(widths[i]) for i, c in enumerate(row)).rstrip()
            text = escape(text)
            out.append('  ' + (BOLD_ON + text + BOLD_OFF if n == 0 else text))
            if n == 0:
                out.append('  ' + '-' * min(WIDTH - 2, sum(widths) + 2 * (columns - 1)))
        return out
    # Too wide: one entry per row.  With several value columns each value
    # is labelled by its header; a single value column needs no label.
    header = cells[0]
    for row in cells[1:]:
        out.append('  ' + BOLD_ON + escape(row[0]) + BOLD_OFF)
        for title, value in zip(header[1:], row[1:]):
            if value:
                text = value if columns == 2 else f'{title}: {value}'
                out += wrap(inline(text, ()), '      ', '      ')
    return out


def document(path, nodes):
    """Returns (title, lines, [(section title, line index)])."""
    lines = (ROOT / path).read_text('utf-8').split('\n')
    out, sections, title = [], [], None
    i = 0

    def blank():
        if out and out[-1] != '':
            out.append('')

    while i < len(lines):
        line = lines[i].rstrip()
        if line.startswith('```'):
            blank()
            i += 1
            while i < len(lines) and not lines[i].startswith('```'):
                out.append('    ' + escape(lines[i].rstrip().expandtabs(4)))
                i += 1
            i += 1
            out.append('')
            continue
        heading = re.match(r'(#{1,6})\s+(.*)', line)
        if heading:
            level, text = len(heading.group(1)), plain(heading.group(2))
            if title is None and level == 1:
                title = text
            else:
                blank()
                if level <= 2:
                    sections.append((text, len(out)))
                out.append(BOLD_ON + escape(text) + BOLD_OFF)
                if level <= 2:
                    out.append(('=' if level == 1 else '-') * min(len(text), WIDTH))
                out.append('')
            i += 1
            continue
        if line.startswith('|'):
            rows = []
            while i < len(lines) and lines[i].startswith('|'):
                rows.append(lines[i])
                i += 1
            blank()
            out += table(rows)
            out.append('')
            continue
        item = re.match(r'(\s*)([-*+]|\d+\.)\s+(.*)', line)
        if item:
            depth = len(item.group(1).expandtabs(4)) // 2
            marker = '*' if item.group(2) in '-*+' else item.group(2)
            text = item.group(3)
            i += 1
            while i < len(lines) and lines[i].strip() and \
                    not re.match(r'\s*([-*+]|\d+\.)\s+', lines[i]) and \
                    lines[i].startswith(' '):
                text += ' ' + lines[i].strip()
                i += 1
            first = '  ' + '  ' * depth + marker + ' '
            out += wrap(inline(text, nodes), first, ' ' * len(first))
            continue
        if re.fullmatch(r'-{3,}|\*{3,}', line.strip()):
            blank()
            out.append('-' * WIDTH)
            out.append('')
            i += 1
            continue
        if not line.strip():
            blank()
            i += 1
            continue
        quote = line.startswith('>')
        text = []
        while i < len(lines) and lines[i].strip() and \
                not re.match(r'(#{1,6}\s|```|\||\s*([-*+]|\d+\.)\s)', lines[i]):
            text.append(lines[i].lstrip('> ').strip())
            i += 1
        out += wrap(inline(' '.join(text), nodes), '  ' if quote else '',
                    '  ' if quote else '')
    while out and out[-1] == '':
        out.pop()
    return title or node_name(path), out, sections


def finish(text):
    text = text.replace(BOLD_ON, '@{b}').replace(BOLD_OFF, '@{ub}')
    text = text.replace(ITALIC_ON, '@{i}').replace(ITALIC_OFF, '@{ui}')
    for fancy, simple in PLAIN.items():
        text = text.replace(fancy, simple)
    bad = sorted({ch for ch in text if ord(ch) > 255})
    if bad:
        raise SystemExit('NovaDock.guide: no Latin-1 form for ' +
                         ', '.join(f'U+{ord(c):04X}' for c in bad) +
                         ' - add it to PLAIN in tools/make_amigaguide.py')
    return text


def build():
    titles = {p: t for _, entries in GROUPS for p, t in entries}
    listed = [p for _, entries in GROUPS for p, _ in entries]
    missing = sorted(set(['README.md'] + [str(p.relative_to(ROOT)) for p in
                     (ROOT / 'docs').glob('*.md')]) - set(listed))
    if missing:
        raise SystemExit('NovaDock.guide: add to GROUPS in '
                         'tools/make_amigaguide.py: ' + ', '.join(missing))
    nodes = {node_name(p) for p in listed}
    ver, date = version()
    docs = {p: document(p, nodes) for p in listed}
    out = ['@database NovaDock.guide',
           f'@$VER: NovaDock.guide {ver} ({date})',
           '@author "NovaDock contributors"',
           '@(c) "Copyright 2026 NovaDock contributors, MIT licence"',
           f'@node Main "NovaDock {ver} manual"', '',
           f'@{{b}}NovaDock {ver}@{{ub}}',
           'A lightweight, extensible dock for Classic AmigaOS Workbench.', '',
           'Select a topic. Press Help in any NovaDock Preferences window to',
           'return to this manual.']
    for group, paths in GROUPS:
        out += ['', f'@{{b}}{group}@{{ub}}', '']
        for p, _ in paths:
            out.append(f'  @{{" {escape(titles[p])} " link {node_name(p)}}}')
    out += ['', 'Website: novadock.retrocs.org',
            'Source:  github.com/retrocs-org/novadock', '@endnode']
    for p in listed:
        _, body, sections = docs[p]
        title = titles[p]
        out += ['', f'@node {node_name(p)} "{title}"', '@toc Main',
                f'@{{b}}{escape(title)}@{{ub}}', '=' * len(title), '']
        index = []
        if len(sections) >= 4:
            # Link lines count the node's displayed text: the three-line
            # title block, this index, then the document.
            index = ['@{b}Sections@{ub}']
            index += [''] * len(sections) + ['']
            for n, (name, line) in enumerate(sections):
                index[n + 1] = (f'  @{{" {escape(name).replace(chr(34), chr(39))} " '
                                f'link {node_name(p)} {3 + len(index) + line}}}')
        out += index + body + ['@endnode']
    return finish('\n'.join(out) + '\n').encode('latin-1')


def main():
    data = build()
    if '--check' in sys.argv[1:]:
        if not OUTPUT.exists() or OUTPUT.read_bytes() != data:
            print('NovaDock.guide is out of date: run tools/make_amigaguide.py')
            sys.exit(1)
        print('NovaDock.guide is current.')
        return
    OUTPUT.write_bytes(data)
    print(f'Wrote {OUTPUT.name} ({len(data)} bytes).')


if __name__ == '__main__':
    main()
