#!/usr/bin/env python3
"""Static legacy-browser website checks; host-side test only."""
import hashlib
from html.parser import HTMLParser
from pathlib import Path
import subprocess, shutil
from urllib.parse import unquote
from zipfile import ZipFile

root = Path(__file__).resolve().parents[1]
web = root / 'public-web'

class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.targets = []
        self.scripts = 0
        self.styles = 0
        self.doctype = False

    def handle_decl(self, decl):
        if decl.upper().startswith('DOCTYPE HTML PUBLIC') or decl.upper() == 'DOCTYPE HTML':
            self.doctype = True

    def handle_starttag(self, tag, attrs):
        attr = dict(attrs)
        if tag == 'script':
            self.scripts += 1
        if tag == 'style' or (tag == 'link' and attr.get('rel', '').lower() == 'stylesheet'):
            self.styles += 1
        if tag == 'img' and 'alt' not in attr:
            raise AssertionError('image missing alt attribute')
        for key in ('href', 'src'):
            if attr.get(key):
                self.targets.append(attr[key])

pages = ['index.php', 'features.php', 'marketplace.php', 'plugin-sdk.php', 'theme-development.php', 'support.php',
         'download.php', 'developers.php', 'source.php', 'screenshots.php']
official_links = {
    'https://novadock.retrocs.org',
    'https://github.com/retrocs-org/novadock',
    'https://bitskim.retrocs.org',
    'mailto:support@retrocs.org',
}
for file in pages:
    path = web / file
    import os
    php = os.environ.get('PHP') or shutil.which('php')
    assert php, 'PHP is required to validate the website'
    render = subprocess.run([php, str(path.resolve())], capture_output=True, check=True)
    assert not render.stderr, render.stderr.decode()
    contents = render.stdout
    contents.decode('ascii')
    assert len(contents) < (16000 if file == 'marketplace.php' else 11000), f'{file} is too large'
    parser = Links()
    parser.feed(contents.decode('ascii'))
    assert parser.doctype, f'{file} lacks an HTML DOCTYPE'
    assert 'FAQ' in contents.decode('ascii'), f'{file} omits FAQ navigation'
    assert parser.scripts == 0 and parser.styles == 1, f'{file} must use only the shared stylesheet'
    assert '<link rel="stylesheet" href="site.css"' in contents.decode('ascii')
    for link in parser.targets:
        if link in official_links or link.startswith(('https://github.com/retrocs-org/novadock/releases/', 'https://github.com/retrocs-org/novadock/tree/', 'https://github.com/retrocs-org/novadock/blob/')):
            continue
        assert not link.lower().startswith(('http:', 'https:', 'javascript:', '//')), (file, link)
        rel = unquote(link.split('#', 1)[0].split('?', 1)[0])
        if not rel:
            continue
        dest = (path.parent / rel).resolve()
        assert dest.is_relative_to(web.resolve()) and dest.is_file(), (file, link)

# Shared header, menu and footer come from tools/update_site_chrome.py.
import subprocess, sys
chrome = subprocess.run([sys.executable, str(root / 'tools/update_site_chrome.py'),
                         '--check'], capture_output=True, text=True)
assert chrome.returncode == 0, chrome.stdout + chrome.stderr
for file in pages:
    text = subprocess.check_output([php, str((web/file).resolve())]).decode('ascii')
    # A 600-pixel column fits a 640-pixel Workbench screen.
    assert 'width="600"' in text and 'class="site-shell"' in text and '<!-- content -->' in text, file
# Site artwork: real GIFs, small enough for a slow modem.
for name, limit in (('logo', 2048), ('dock', 6144), ('px', 128)):
    raw = (web / 'images' / f'{name}.gif').read_bytes()
    assert raw[:6] in (b'GIF87a', b'GIF89a') and len(raw) < limit, name
assert (web / 'images' / 'px.gif').read_bytes()[:6] == b'GIF89a'  # transparent

for name in ('workbench', 'drawer', 'games', 'internet', 'tools'):
    raw = (web/'images'/f'{name}.gif').read_bytes()
    assert raw[:6] in (b'GIF87a', b'GIF89a')
    assert int.from_bytes(raw[6:8], 'little') == 32
    assert int.from_bytes(raw[8:10], 'little') == 32
    assert len(raw) < 2048

VERSION = next(line.split('=', 1)[1].strip()
               for line in (root / 'VERSION').read_text('ascii').splitlines()
               if line.startswith('VERSION='))
download = web/'downloads'/f'NovaDock-{VERSION}-source.zip'
sdk_download = web/'downloads'/f'NovaDock-{VERSION}-plugin-sdk.zip'
with ZipFile(download) as z:
    assert z.testzip() is None
    contents = z.namelist()
    assert 'NovaDock/src/novadock.c' in contents
    assert 'NovaDock/plugin_src/eyes/eyes.c' in contents
    assert 'NovaDock/README.md' in contents
    assert 'NovaDock/NovaDock.info' in contents
    assert 'NovaDock/NovaDockPrefs.info' in contents
    assert 'NovaDock/Plugins/MusicPlayer.info' in contents
    assert 'NovaDock/Plugins/RunningApps.info' in contents
    assert 'NovaDock/Plugins/RunningApps.ndplugin' in contents
    assert 'NovaDock/Makefile.cross' in contents
    assert 'NovaDock/scripts/build-amiga.sh' in contents
    assert 'NovaDock/scripts/build-and-stage-amiga.sh' in contents
    assert z.read('NovaDock/README.md') == (root / 'README.md').read_bytes()
    assert z.read('NovaDock/NovaDock.info') == (root / 'NovaDock.info').read_bytes()
    assert z.read('NovaDock/NovaDockPrefs.info') == (root / 'NovaDockPrefs.info').read_bytes()
    assert z.read('NovaDock/Plugins/MusicPlayer.info') == \
        (root / 'Plugins' / 'MusicPlayer.info').read_bytes()
    assert z.read('NovaDock/Plugins/RunningApps.info') == \
        (root / 'Plugins' / 'RunningApps.info').read_bytes()
    assert z.read('NovaDock/Plugins/RunningApps.ndplugin') == \
        (root / 'Plugins' / 'RunningApps.ndplugin').read_bytes()
    assert z.read('NovaDock/CHANGELOG.md') == (root / 'CHANGELOG.md').read_bytes()
    assert z.read('NovaDock/tools/package_release.py') == (root / 'tools/package_release.py').read_bytes()
    assert z.read('NovaDock/scripts/build-and-stage-amiga.sh') == \
        (root / 'scripts' / 'build-and-stage-amiga.sh').read_bytes()
    assert not any('/public-web/' in item for item in contents)
with ZipFile(sdk_download) as z:
    assert z.testzip() is None
    sdk_contents = z.namelist()
    sdk_root = 'NovaDock-PluginSDK/'
    for item in ('README.md', 'Makefile', 'include/nd_protocol.h',
                 'include/nd_client.h', 'include/nd_reply.h', 'src/nd_client.c', 'examples/hello.c',
                 'include/nd_option_runtime.h', 'src/nd_option_runtime.c',
                 'examples/Hello.ndplugin'):
        assert sdk_root + item in sdk_contents
    assert z.read(sdk_root+'include/nd_protocol.h') == (root/'include'/'nd_protocol.h').read_bytes()
    assert z.read(sdk_root+'src/nd_client.c') == (root/'plugin_src'/'common'/'nd_client.c').read_bytes()
    assert z.read(sdk_root+'src/nd_option_runtime.c') == \
        (root/'plugin_src'/'common'/'nd_option_runtime.c').read_bytes()
    assert z.read(sdk_root+'examples/hello.c') == (root/'plugin_src'/'template'/'hello.c').read_bytes()
# Release archives must never nest earlier releases inside the website.
assert sorted(p.name for p in (web / 'downloads').iterdir()) == sorted([
    f'NovaDock-{VERSION}-plugin-sdk.zip', f'NovaDock-{VERSION}-source.zip',
    f'NovaDock-{VERSION}.lha', f'NovaDock-{VERSION}.readme', 'SHA256SUMS.txt'])
checksum_lines = (web / 'downloads' / 'SHA256SUMS.txt').read_text('ascii').splitlines()
checksums = dict(line.split('  ', 1)[::-1] for line in checksum_lines)
assert checksums[download.name] == hashlib.sha256(download.read_bytes()).hexdigest()
assert checksums[sdk_download.name] == hashlib.sha256(sdk_download.read_bytes()).hexdigest()
print(f'Public website checks: OK ({len(pages)} PHP pages, stylesheet, GIFs, links, source and Plugin SDK ZIPs)')

# Real screenshot references, small previews and binary checksum coverage.
import json
manifest=json.loads((root/'docs/SCREENSHOTS.json').read_text('utf-8'))
assert len(manifest['images'])>=10
for item in manifest['images']:
    for suffix in ('','-thumb'):
        path=web/'screenshots'/(item['file'][:-4]+suffix+'.gif')
        raw=path.read_bytes();assert raw[:6] in (b'GIF87a',b'GIF89a')
        assert len(raw)<65536
        expected=(item['width'],item['height']) if not suffix else (item['thumb_width'],item['thumb_height'])
        assert (int.from_bytes(raw[6:8],'little'),int.from_bytes(raw[8:10],'little'))==expected
for name in (f'NovaDock-{VERSION}.lha',f'NovaDock-{VERSION}.readme'):
    assert checksums[name]==hashlib.sha256((web/'downloads'/name).read_bytes()).hexdigest()
assert 'fifteen-second' in (web/'plugin-sdk.php').read_text('ascii')
assert 'five-second' not in (web/'plugin-sdk.php').read_text('ascii')
print('Screenshot gallery, binary downloads and current protocol information passed.')

# Marketplace must list every bundled plugin program exactly once.
catalog = json.loads(subprocess.check_output([php, '-r',
    'echo json_encode(require $argv[1]);', str((web/'includes/plugin_catalog.php').resolve())]))
import re
programs = re.search(r'^all:(.*)$', (root/'Makefile').read_text(), re.M).group(1).split()
expected = {name.split('/', 1)[1] for name in programs if name.startswith('Plugins/')}
actual = [item['executable'] for item in catalog]
assert len(actual) == len(set(actual)) and set(actual) == expected
marketplace = subprocess.check_output([php, str((web/'marketplace.php').resolve())]).decode('ascii')
assert set(re.findall(r'data-executable="([^"]+)"', marketplace)) == expected
assert 'No community plugins are listed yet.' in marketplace
assert "header('Location: marketplace.php', true, 301)" in (web/'plugins.php').read_text()
print('Marketplace covers every bundled plugin; future community listings are clearly separated.')

# Theme listings and palette dots must agree with the actual bundled presets.
themes = json.loads(subprocess.check_output([php, '-r',
    'echo json_encode(require $argv[1]);', str((web/'includes/theme_catalog.php').resolve())]))
expected_themes = {p.parent.name: p for p in (root/'Themes').glob('*/theme.cfg')}
assert len(themes) == len(expected_themes) == len({t['id'] for t in themes})
assert {t['id'] for t in themes} == set(expected_themes)
assert set(re.findall(r'data-theme-id="([^"]+)"', marketplace)) == set(expected_themes)
for theme in themes:
    config = dict(line.split('=', 1) for line in expected_themes[theme['id']].read_text().splitlines() if '=' in line)
    palette = ['#' + ''.join(f'{int(n):02x}' for n in config[key].split(','))
               for key in ('BG', 'TILE', 'TEXT', 'ACCENT', 'ACCENT2')]
    assert theme['palette'] == palette and theme['name'] == config['NAME']
assert 'No community themes are listed yet.' in marketplace
print('Marketplace themes match all bundled presets and their real colour palettes.')

# Git source tree SDK must match the standalone SDK and upstream helpers.
for member in sdk_contents:
    name = member.removeprefix(sdk_root)
    if name and not name.endswith('/'):
        with ZipFile(sdk_download) as sdk_archive:
            assert (root/'plugin-sdk'/name).read_bytes() == sdk_archive.read(member)
release_entries = json.loads((web/'releases.json').read_text())
assert release_entries and all(e['release_url'].startswith('https://github.com/retrocs-org/novadock/releases/tag/') for e in release_entries)
for e in release_entries:
    assert e['status'] in ('upcoming', 'published')
    if e['status'] == 'upcoming':
        assert e['binary_url'].endswith('NovaDock-'+e['version']+'.lha')
        assert e['source_url'].endswith('NovaDock-'+e['version']+'-source.zip')
rendered_download = subprocess.check_output([php, str((web/'download.php').resolve())]).decode()
assert 'Build instructions' not in rendered_download and 'sh scripts/build-amiga.sh' not in rendered_download
assert 'https://bitskim.retrocs.org' in marketplace
assert 'data-executable="Roadshow"' not in marketplace
print('GitHub release links, browsable SDK tree and retired plugin checks passed.')

source_page = subprocess.check_output([php, str((web/'source.php').resolve())]).decode('ascii')
developer_page = subprocess.check_output([php, str((web/'developers.php').resolve())]).decode('ascii')
assert 'sh scripts/build-amiga.sh -B' in source_page and 'Execute build.amiga' in source_page
assert 'Build NovaDock from source' not in developer_page
assert 'Plugin SDK' in developer_page and 'Theme development' in developer_page
print('SDK documentation and application build instructions are separated.')

# FAQ lives on Support, with native collapsible questions and a legacy redirect.
home = subprocess.check_output([php, str((web/'support.php').resolve())]).decode('ascii')
assert 'id="faq"' not in subprocess.check_output([php, str((web/'index.php').resolve())]).decode('ascii')
assert '<details>' in home and home.count('<details') == home.count('</details>')
assert 'id="faq"' in home and '<summary>Does it run on AmigaOS 3.1?</summary>' in home
for removed in ('Native compilation and Workbench testing have not yet been completed',
                'Does it need a graphics card or X11?', 'Why does a settings window have a scrollbar?',
                'What does the Apps tile list?', 'What layout does a new installation use?',
                'Where are the website and source?'):
    assert removed not in home
assert "header('Location: support.php#faq', true, 301)" in (web/'faq.php').read_text()
print('Support FAQ is collapsible and excludes the removed entries.')

assert '<details class="home-faq" id="faq">' in home
assert '<summary class="faq-heading">Frequently asked questions</summary>' in home

assert sdk_root+'PLUGIN_SDK_SETTINGS.md' in sdk_contents
assert (root/'plugin-sdk/PLUGIN_SDK_SETTINGS.md').read_bytes() == (root/'docs/PLUGIN_SDK_SETTINGS.md').read_bytes()
print('Settings-window guide is included in both SDK distributions.')

assert "header('Location: theme-development.php', true, 301)" in (web/'themes.php').read_text()
assert 'href="plugin-sdk.php"' in developer_page and 'href="theme-development.php"' in developer_page
plugin_page = subprocess.check_output([php, str((web/'plugin-sdk.php').resolve())]).decode('ascii')
assert 'id="settings"' in plugin_page and 'ND_EVENT_SETTINGS' in plugin_page
assert 'FOCUSED=1' in plugin_page and 'Before sharing' in plugin_page
assert 'mailto:support@retrocs.org' in home
print('Dedicated development guides and Support navigation passed.')
