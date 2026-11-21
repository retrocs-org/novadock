#!/usr/bin/env python3
"""Reject extracted releases whose bundled plugins would need repair."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def check_plugins(drawer, root=ROOT):
    drawer, root = Path(drawer).resolve(), Path(root).resolve()
    expected = {path.name for path in (root/'Plugins').glob('*.ndplugin')}
    actual = {path.name for path in (drawer/'Plugins').glob('*.ndplugin')}
    if not expected or actual != expected:
        raise ValueError('Plugin manifest inventory differs: missing '+
                         ', '.join(sorted(expected-actual))+'; unexpected '+
                         ', '.join(sorted(actual-expected)))
    manifests = []
    for name in sorted(expected):
        path = drawer/'Plugins'/name
        if path.is_symlink() or not path.is_file():
            raise ValueError('Missing or unsafe plugin manifest: '+name)
        if path.read_bytes() != (root/'Plugins'/name).read_bytes():
            raise ValueError('Packaged plugin manifest differs: '+name)
        program = path.with_suffix('')
        if program.is_symlink() or not program.is_file():
            raise ValueError('Missing or unsafe plugin program: '+program.name)
        asset_dir = program.with_name(program.name+'.assets')
        bank = asset_dir/'Artwork.ndart'
        if asset_dir.is_symlink() or bank.is_symlink():
            raise ValueError('Unsafe plugin artwork: '+program.name)
        manifests.append(str(path))
    with tempfile.TemporaryDirectory(prefix='novadock-plugin-health-') as tmp:
        validator = Path(tmp)/'PluginHealth'
        compiler = shlex.split(os.environ.get('CC', 'cc'))
        command = compiler+['-std=c89', '-Wno-deprecated-declarations',
                            '-I'+str(ROOT/'include'), str(ROOT/'tools/check_plugin_health.c'),
                            str(ROOT/'src/nd_manifest.c'), str(ROOT/'src/config.c'),
                            str(ROOT/'src/nd_options.c'), '-o', str(validator)]
        subprocess.run(command, check=True)
        subprocess.run([str(validator), *manifests], check=True)
    return len(manifests)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('drawer', type=Path, help='Extracted NovaDock drawer')
    args = parser.parse_args()
    try:
        check_plugins(args.drawer)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, 'Plugin health check failed: '+str(error)+'\n')


if __name__ == '__main__':
    main()
