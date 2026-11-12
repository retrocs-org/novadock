#!/usr/bin/env python3
"""Shared PHP header, menu and footer for the public website.

Each PHP page keeps its own hand-written content between <!-- content --> and
<!-- /content -->; this tool writes everything around it from one template,
so the menu and footer are changed in one place.

    python3 tools/update_site_chrome.py          rewrite all pages
    python3 tools/update_site_chrome.py --check  exit 1 if a page is stale

The site renders HTML5 with layout tables, colour attributes
and small GIFs as a fallback for classic browsers. A local stylesheet adds
responsive layout and modern typography where supported. No JavaScript or
external fonts are required. The fallback column is 600 pixels wide.
"""
from pathlib import Path
import re
import sys

WEB = Path(__file__).resolve().parents[1] / 'public-web'
VERSION = next(line.split('=', 1)[1].strip()
               for line in (WEB.parent / 'VERSION').read_text('ascii').splitlines()
               if line.startswith('VERSION='))
FACE = 'Helvetica,Arial,sans-serif'   # helvetica.font exists on Workbench too
PAGES = (
    ('index.php', 'Home', 'Home',
     f'NovaDock {VERSION} - a Classic AmigaOS Workbench dock with plugin support.'),
    ('features.php', 'Features', 'Features', None),
    ('marketplace.php', 'Marketplace', 'Marketplace', None),
    ('plugin-sdk.php', 'Plugin SDK', 'Plugin SDK', None),
    ('theme-development.php', 'Theme development', 'Theme development', None),
    ('support.php', 'Support', 'Support', None),
    ('get.php', 'Get NovaDock', 'Get NovaDock', None),
    ('developers.php', 'Developers', 'Developers', None),
)
CARD_OLD = re.compile(
    r'<table width="100%" border="0" cellpadding="10" cellspacing="0" '
    r'bgcolor="#313244">\s*<tr><td>(?:<font color="(#[0-9A-Fa-f]{6})">'
    r'<b>(.*?)</b></font><hr noshade size="1" color="#585B70">)?(.*?)'
    r'</td></tr></table>', re.S)


def card(match):
    """Title-coloured accent strip on the left, title above the text."""
    colour, title, body = match.group(1) or '#89B4FA', match.group(2), match.group(3)
    heading = (f'<font color="{colour}"><b>{title}</b></font><br>'
               if title else '')
    return ('<table width="100%" border="0" cellpadding="0" cellspacing="0" '
            'bgcolor="#313244"><tr>'
            f'<td width="4" bgcolor="{colour}"><img src="images/px.gif" '
            'width="4" height="1" alt=""></td><td>'
            '<table width="100%" border="0" cellpadding="10" cellspacing="0">'
            f'<tr><td><font face="{FACE}">{heading}{body.strip()}'
            '</font></td></tr></table></td></tr></table>')


def code(match):
    """Dark code panel; a smaller font keeps commands inside 600 pixels."""
    return ('<table width="100%" border="0" cellpadding="4" cellspacing="0" '
            'bgcolor="#11111B"><tr><td><pre><font size="-1" color="#F9E2AF">'
            f'{match.group(1)}</font></pre></td></tr></table>')


def migrate(text):
    """Content of a page still in the old layout, modernised once."""
    lines = text.split('\n')
    nav = next(i for i, l in enumerate(lines) if l.startswith('<p align="center">')
               and '&nbsp; | &nbsp;' in l)
    footer = max(i for i, l in enumerate(lines)
                 if l.startswith('<p align="center"><small>NovaDock'))
    body = '\n'.join(lines[nav + 2:footer - 1])
    body = CARD_OLD.sub(card, body)
    body = re.sub(r'<pre>(.*?)</pre>', code, body, flags=re.S)
    return body.strip('\n')


def content_of(text):
    found = re.search(r'<!-- content -->\n(.*?)\n<!-- /content -->', text, re.S)
    return found.group(1) if found else migrate(text)


def menu(current):
    order = ('Home', 'Features', 'Marketplace', 'Developers', 'Support')
    items = {label: file for file, label, _, _ in PAGES}
    links = []
    for label in order:
        condition = "in_array($current, ['Developers', 'Plugin SDK', 'Theme development'], true)" if label == 'Developers' else f"$current === '{label}'"
        links.append(f'<a href="{items[label]}"<?php if ({condition}): ?> class="active" aria-current="page"<?php endif; ?>>{label}</a>')
    main_links = ' '.join(links)
    return ('<nav class="site-nav" aria-label="Main navigation">'
            '<div class="nav-links">'+main_links+'</div>'
            '<details class="mobile-menu"><summary>Menu</summary><div class="mobile-links">'+main_links+'</div></details>'
            '<a class="nav-cta" href="get.php"<?php if ($current === \'Get NovaDock\'): ?> aria-current="page"<?php endif; ?>>Get NovaDock <span aria-hidden="true">&#8599;</span></a>'
            '</nav>')


def page(label, title, description, content):
    return f'''<!DOCTYPE html>
<html lang="en">
<head>
<title>NovaDock - {title}</title>
<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
<meta name="description" content="{description}">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="site.css" type="text/css">
</head>
<body bgcolor="#1E1E2E" text="#CDD6F4" link="#89B4FA" vlink="#CBA6F7" alink="#F9E2AF">
<center>
<table class="site-shell" width="600" border="0" cellpadding="0" cellspacing="0"><tr><td>
<table class="site-header" width="100%" border="0" cellpadding="0" cellspacing="0"><tr>
<td width="120"><a href="index.php"><img src="images/logo.gif" width="108" height="44" border="0" alt="NovaDock"></a></td>
<td><font face="{FACE}"><font size="+3"><b>NovaDock</b></font><br><font color="#A6ADC8">A dock for Classic AmigaOS Workbench</font></font></td>
</tr></table>
<br>
{menu(label)}
<br>
<font class="site-content" face="{FACE}">
<!-- content -->
{content}
<!-- /content -->
</font>
<br>
<table class="site-footer" width="100%" border="0" cellpadding="10" cellspacing="0" bgcolor="#181825"><tr><td align="center"><font face="{FACE}" size="-1" color="#A6ADC8">NovaDock {VERSION} &middot; Classic AmigaOS 3.x &middot; <a href="https://github.com/retrocs-org/novadock">Source on GitHub</a> &middot; <a href="marketplace.php">Marketplace</a> &middot; <a href="features.php#screenshots">Screenshots</a> &middot; <a href="theme-development.php">Theme guide</a> &middot; <a href="developers.php">Developers</a> &middot; <a href="developers.php#source">Build from source</a> &middot; <a href="support.php#faq">Support &amp; FAQ</a></font></td></tr></table>
</td></tr></table>
</center>
</body>
</html>
'''


def php_quote(text):
    return "'" + text.replace("\\", "\\\\").replace("'", "\\'") + "'"


def main():
    check = '--check' in sys.argv[1:]
    stale = []
    marker = '__NOVADOCK_PAGE_CONTENT__'
    frame = page('', '<?= site_escape($title) ?>', '<?= site_escape($description) ?>', marker)
    head, foot = frame.split(marker)
    head = head.replace('<!-- content -->\n', '')
    foot = foot.replace('\n<!-- /content -->', '')
    layout = (f"<?php\ndeclare(strict_types=1);\nconst NOVADOCK_VERSION = '{VERSION}';\n"
              "function site_escape(string $text): string {\n"
              "    return htmlspecialchars($text, ENT_QUOTES | ENT_SUBSTITUTE, 'ISO-8859-1');\n}\n"
              "function site_release_tag(): string {\n"
              "    $entries = json_decode((string) @file_get_contents(__DIR__.'/../releases.json'), true);\n"
              "    foreach (is_array($entries) ? $entries : [] as $entry) {\n"
              "        if (($entry['version'] ?? '') === NOVADOCK_VERSION && ($entry['status'] ?? '') === 'published' && is_string($entry['tag'] ?? null)) return $entry['tag'];\n"
              "    }\n"
              "    return 'v'.NOVADOCK_VERSION;\n}\n"
              "function site_header(string $title, string $current, string $description): void {\n"
              "    header('Content-Type: text/html; charset=ISO-8859-1');\n?>\n"
              + head + "<?php\n}\nfunction site_footer(): void {\n?>"
              + foot + "<?php\n}\n")
    outputs = {WEB/'includes/layout.php': layout}
    for file, label, title, description in PAGES:
        path = WEB/file
        text = path.read_text('ascii')
        if description is None:
            found = re.search(r'<meta name="description" content="([^"]*)"', text)
            if found:
                description = found.group(1)
            else:
                # Existing route metadata is stored in its header call.
                import ast
                call = re.search(r'site_header\((.*?)\);', text)
                description = ast.literal_eval('('+call.group(1)+')')[2]
        body = content_of(text)
        outputs[path] = ("<?php\nrequire_once __DIR__ . '/includes/layout.php';\n"
                         + 'site_header('+', '.join(php_quote(v) for v in (title, label, description))+');\n?>\n'
                         + '<!-- content -->\n'+body+'\n<!-- /content -->\n<?php site_footer(); ?>\n')
    for path, new in outputs.items():
        if not path.exists() or path.read_text('ascii') != new:
            stale.append(path.relative_to(WEB).as_posix())
            if not check:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(new, 'ascii')
    if check and stale:
        print('Website templates need tools/update_site_chrome.py:', ', '.join(stale))
        sys.exit(1)
    if not check:
        print('Updated' if stale else 'Already current:', ', '.join(stale) or 'all templates')


if __name__ == '__main__':
    main()
