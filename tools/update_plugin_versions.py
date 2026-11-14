#!/usr/bin/env python3
"""Sync bundled plugin manifests from their independent version header."""
from datetime import datetime
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def versions():
    text = (ROOT / 'include/nd_plugin_versions.h').read_text('ascii')
    releases = dict(re.findall(r'^#define ND_PLUGIN_VERSION_(\w+) "([^"]+)"$', text, re.M))
    dates = dict(re.findall(r'^#define ND_PLUGIN_DATE_(\w+) "([^"]+)"$', text, re.M))
    if not releases or releases.keys() != dates.keys():
        raise ValueError('Every plugin needs its own version and date.')
    for key, value in releases.items():
        if not re.fullmatch(r'\d+\.\d+(?:\.\d+)?', value):
            raise ValueError('Invalid plugin version: ' + key)
        datetime.strptime(dates[key], '%d.%m.%Y')
    return releases


def sync(check=False):
    releases = versions()
    problems = []
    manifests = sorted((ROOT / 'Plugins').glob('*.ndplugin'))
    manifests += sorted((ROOT / 'plugin_src/runningapps').glob('*.ndplugin'))
    for path in manifests:
        text = path.read_text('ascii')
        key = re.search(r'^NOVADOCK_ID=(\w+)$', text, re.M)
        if not key or key[1] not in releases:
            raise ValueError('Missing independent plugin version: ' + str(path))
        line = 'NOVADOCK_VERSION=' + releases[key[1]]
        if re.search(r'^NOVADOCK_VERSION=.*$', text, re.M):
            updated = re.sub(r'^NOVADOCK_VERSION=.*$', line, text, flags=re.M)
        else:
            updated = text.replace(key[0], key[0] + '\n' + line, 1)
        if updated != text:
            if check:
                problems.append(str(path.relative_to(ROOT)))
            else:
                path.write_text(updated, 'ascii')
    return problems


if __name__ == '__main__':
    problems = sync(check='--check' in sys.argv[1:])
    if problems:
        print('Stale plugin versions: ' + ', '.join(problems))
        sys.exit(1)
    print('Independent plugin versions and manifests are consistent.')
