#!/usr/bin/env python3
"""Check native version strings, manifest drift and dock-version independence."""
from pathlib import Path
import importlib.util
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('plugin_versions', ROOT / 'tools/update_plugin_versions.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
releases = module.versions()
assert not module.sync(check=True)
# Use the real C preprocessor: the native string must expand without the dock header.
for path in (ROOT / 'plugin_src').glob('*/*.c'):
    source = path.read_text()
    found = re.search(r'ND_PLUGIN_VER_STRING\((\w+),"([^"]+)"\)', source)
    if found:
        key, name = found.groups()
        assert key in releases
        snippet = '#include "nd_plugin_versions.h"\nND_PLUGIN_VER_STRING(' + key + ',"' + name + '")\n'
        expanded = subprocess.check_output(['cc', '-E', '-P', '-I' + str(ROOT / 'include'), '-x', 'c', '-'], input=snippet, text=True)
        literals = re.findall(r'"([^"]*)"', expanded)
        assert ''.join(literals).startswith('$VER: ' + name + ' ' + releases[key] + ' (')
    assert 'ND_VER_STRING(' not in source, str(path)
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    (root / 'include').mkdir()
    shutil.copy(ROOT / 'include/nd_plugin_versions.h', root / 'include')
    shutil.copytree(ROOT / 'Plugins', root / 'Plugins', ignore=shutil.ignore_patterns('*'))
    for path in (ROOT / 'Plugins').glob('*.ndplugin'):
        shutil.copy(path, root / 'Plugins')
    website = ROOT / 'public-web/includes/plugin_versions.php'
    if website.exists():
        (root / 'public-web/includes').mkdir(parents=True)
        shutil.copy(website, root / 'public-web/includes')
    module.ROOT = root
    # A later dock release must leave every independent plugin unchanged.
    (root / 'VERSION').write_text('VERSION=9.9\nDATE=01.01.2030\n')
    assert not module.sync(check=True)
    header = root / 'include/nd_plugin_versions.h'
    header.write_text(header.read_text().replace('ND_PLUGIN_VERSION_CLIPBOARD "' + releases['CLIPBOARD'] + '"', 'ND_PLUGIN_VERSION_CLIPBOARD "9.8"'))
    expected = ['Plugins/Clipboard.ndplugin']
    if website.exists(): expected.append('public-web/includes/plugin_versions.php')
    assert module.sync(check=True) == expected
    module.sync()
    assert not module.sync(check=True)
    if website.exists():
        assert "'Clipboard' => '9.8'" in (root / 'public-web/includes/plugin_versions.php').read_text()
    assert 'NOVADOCK_VERSION=9.8' in (root / 'Plugins/Clipboard.ndplugin').read_text()
print('Independent native plugin versions, manifest drift and dock-version independence passed.')
