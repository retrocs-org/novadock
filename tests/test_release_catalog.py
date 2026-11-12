#!/usr/bin/env python3
"""Release-page data must distinguish planned URLs from verified published assets."""
import importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('release_catalog',Path(__file__).resolve().parents[1]/'tools/sync_github_releases.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
plan=m.upcoming('0.1')
assert plan['status']=='upcoming' and plan['binary_url'].endswith('/v0.1/NovaDock-0.1.lha')
assert m.normalize({'tag_name':'v0.1','draft':True}) is None
assert m.normalize({'tag_name':'not-a-version'}) is None
record={'tag_name':'v0.1','published_at':'2026-11-11T10:00:00Z','prerelease':True,
        'assets':[{'name':'NovaDock-0.1.lha','browser_download_url':plan['binary_url']}]}
release=m.normalize(record)
assert release['status']=='published' and release['date']=='2026-11-11'
assert release['binary_url']==plan['binary_url'] and release['source_url'].endswith('/archive/refs/tags/v0.1.zip')
assert m.with_upcoming([release], '0.1')==[release]
assert m.with_upcoming([release], '0.2')[0]['tag']=='v0.2'
record['assets'][0]['browser_download_url']='https://other.example/file.lha'
assert m.normalize(record)['binary_url'] is None
print('Planned releases, published assets and release-list synchronization passed.')

# Exact tags need not use a v prefix. Asset versions still determine filenames.
for tag in ('0.1', 'preview/0.1'):
    planned = m.upcoming('0.1', tag)
    entry = m.normalize({'tag_name': tag, 'assets': [{'name': 'NovaDock-0.1.lha', 'browser_download_url': planned['binary_url']}]})
    assert entry['tag'] == tag and entry['version'] == '0.1'
    assert entry['binary_url'] == planned['binary_url'] and entry['release_url'] == planned['release_url']
# Verify rendered developer links against a published bare version tag.
import json, shutil, subprocess, tempfile
php = shutil.which('php')
assert php
with tempfile.TemporaryDirectory() as tmp:
    site = Path(tmp)/'site'
    shutil.copytree(m.ROOT/'public-web', site, ignore=shutil.ignore_patterns('downloads'))
    (site/'releases.json').write_text(json.dumps([{'version': '0.1', 'tag': '0.1', 'status': 'published'}]))
    for name in ('developers.php', 'plugin-sdk.php', 'theme-development.php'):
        result = subprocess.run([php, str(site/name)], capture_output=True, check=True)
        assert not result.stderr, result.stderr
        html = result.stdout.decode('ascii')
        assert '/tree/0.1' in html and '/tree/v0.1' not in html, name
    future = m.upcoming('0.2', '0.2')
    future.update(status='published', date='2026-12-01', prerelease=False)
    (site/'releases.json').write_text(json.dumps([future]))
    result = subprocess.run([php, str(site/'get.php')], capture_output=True, check=True)
    assert not result.stderr, result.stderr
    html = result.stdout.decode('ascii')
    assert 'NovaDock 0.2' in html and '/0.2/NovaDock-0.2.lha' in html
    assert '/0.2/NovaDock-0.2-plugin-sdk.zip.sha256' in html
print('Custom release tags and matching website source/SDK links passed.')

assert m.upcoming('0.1', '0.1')['checksums_url'].endswith('/NovaDock-0.1.lha.sha256')
assert m.upcoming('0.1', '0.1')['source_tar_url'].endswith('/archive/refs/tags/0.1.tar.gz')
