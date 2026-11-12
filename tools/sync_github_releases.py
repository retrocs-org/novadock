#!/usr/bin/env python3
"""Refresh website release links from GitHub; --upcoming works offline."""
import argparse
import json
from pathlib import Path
import re
import sys
import urllib.request
from urllib.parse import quote

ROOT = Path(__file__).resolve().parents[1]
REPO = 'https://github.com/retrocs-org/novadock'
OUTPUT = ROOT/'public-web/releases.json'


def upcoming(version, tag=None):
    tag = 'v'+version if tag is None else tag
    encoded = quote(tag, safe='')
    base = REPO+'/releases/download/'+encoded+'/'
    return {'version': version, 'tag': tag, 'status': 'upcoming', 'prerelease': version.startswith('0.'),
            'date': None, 'release_url': REPO+'/releases/tag/'+encoded,
            'binary_url': base+'NovaDock-'+version+'.lha',
            'source_url': REPO+'/archive/refs/tags/'+encoded+'.zip',
            'source_tar_url': REPO+'/archive/refs/tags/'+encoded+'.tar.gz',
            'sdk_url': base+'NovaDock-'+version+'-plugin-sdk.zip',
            'sdk_checksums_url': base+'NovaDock-'+version+'-plugin-sdk.zip.sha256',
            'checksums_url': base+'NovaDock-'+version+'.lha.sha256'}


def normalize(release):
    tag = release.get('tag_name', '')
    if release.get('draft') or not isinstance(tag, str) or not tag or any(ord(c) < 32 for c in tag):
        return None
    assets = {item['name']: item.get('browser_download_url', '') for item in release.get('assets', [])}
    match = re.fullmatch(r'v?([0-9]+(?:\.[0-9]+)*(?:[-.][A-Za-z0-9]+)*)', tag)
    versions = {m.group(1) for name in assets for m in [re.fullmatch(r'NovaDock-([0-9]+(?:\.[0-9]+)*(?:[-.][A-Za-z0-9]+)*)\.lha', name)] if m}
    if match:
        version = match.group(1)
    elif len(versions) == 1:
        version = versions.pop()
    else:
        return None
    entry = upcoming(version, tag)
    for key, name in [('binary_url', 'NovaDock-'+version+'.lha'),
                      ('sdk_url', 'NovaDock-'+version+'-plugin-sdk.zip'),
                      ('sdk_checksums_url', 'NovaDock-'+version+'-plugin-sdk.zip.sha256'),
                      ('checksums_url', 'NovaDock-'+version+'.lha.sha256')]:
        expected = REPO+'/releases/download/'+quote(tag, safe='')+'/'+name
        entry[key] = expected if assets.get(name) == expected else None
    entry.update(status='published', prerelease=bool(release.get('prerelease')),
                 date=(release.get('published_at') or '')[:10] or None)
    return entry


def with_upcoming(entries, version):
    published = [entry for entry in entries if entry['status'] == 'published']
    if not any(entry['version'] == version for entry in published):
        published.insert(0, upcoming(version))
    return published


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upcoming', action='store_true', help='Keep published entries; add the VERSION tag locally')
    args = parser.parse_args()
    version = next(line.split('=', 1)[1].strip() for line in (ROOT/'VERSION').read_text().splitlines() if line.startswith('VERSION='))
    if args.upcoming:
        entries = json.loads(OUTPUT.read_text()) if OUTPUT.exists() else []
    else:
        entries = []
        page = 1
        while True:
            request = urllib.request.Request('https://api.github.com/repos/retrocs-org/novadock/releases?per_page=100&page='+str(page),
                                             headers={'Accept': 'application/vnd.github+json', 'User-Agent': 'NovaDock-release-list'})
            with urllib.request.urlopen(request, timeout=30) as response:
                records = json.load(response)
            for record in records:
                item = normalize(record)
                if item:
                    entries.append(item)
            if len(records) < 100:
                break
            page += 1
    entries = with_upcoming(entries, version)
    candidate = OUTPUT.with_suffix('.json.tmp')
    candidate.write_text(json.dumps(entries, indent=2)+'\n')
    candidate.replace(OUTPUT)
    print('Website release list updated:', len(entries), 'release(s). No uploads performed.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError) as error:
        print('Release list unchanged:', error, file=sys.stderr)
        sys.exit(1)
