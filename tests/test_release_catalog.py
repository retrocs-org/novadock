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
assert release['binary_url']==plan['binary_url'] and release['source_url'] is None
assert m.with_upcoming([release], '0.1')==[release]
assert m.with_upcoming([release], '0.2')[0]['tag']=='v0.2'
record['assets'][0]['browser_download_url']='https://other.example/file.lha'
assert m.normalize(record)['binary_url'] is None
print('Planned releases, published assets and release-list synchronization passed.')
