#!/usr/bin/env python3
"""Verify plugin artwork and the standalone SDK after package extraction."""
import argparse
from pathlib import Path
import tempfile
import struct
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
ART_EXAMPLE = {'Makefile', 'include/nd_client.h', 'include/nd_protocol.h',
               'include/nd_reply.h', 'include/nd_artwork.h', 'src/nd_client.c',
               'tools/pack_plugin_art.py', 'PLUGIN_SDK_ARTWORK.md',
               'examples/arthello.c', 'examples/ArtHello.ndplugin',
               'examples/artwork.json', 'ArtHello.assets/Artwork.ndart'}


def regular(path):
    if path.is_symlink() or not path.is_file():
        raise ValueError('Missing or unsafe package file: '+str(path))
    return path.read_bytes()


def validate_bank(payload):
    from pack_plugin_art import build_bank
    if not 56 <= len(payload) <= 262144 or payload[:5] != b'NDAR\0' or payload[5] not in (1, 2):
        raise ValueError('Invalid packaged artwork bank header')
    modern = payload[5] == 2
    header = 72 if modern else 56
    count = int.from_bytes(payload[6:8], 'big')
    table_end = header+count*32
    if not 1 <= count <= 256 or table_end > len(payload):
        raise ValueError('Invalid packaged artwork frame table')
    frames = []
    for index in range(count):
        name, state, flags, runs, offset = struct.unpack_from('>24sBBHI', payload, header+index*32)
        if b'\0' not in name or not 1 <= runs <= 1024 or offset < table_end or offset+runs*5 > len(payload) or (not modern and flags):
            raise ValueError('Invalid packaged artwork frame')
        pixels = [[0]*32 for _ in range(32)]
        for position in range(offset, offset+runs*5, 5):
            x, y, width, height, colour = payload[position:position+5]
            if not width or not height or x+width > 32 or y+height > 32 or not 1 <= colour <= 15:
                raise ValueError('Invalid packaged artwork rectangle')
            for row in range(y, y+height):
                pixels[row][x:x+width] = [colour]*width
        frames.append({'name': name.split(b'\0', 1)[0].decode('ascii'),
                       'state': state, 'flags': flags, 'pixels': pixels})
    palette = [list(payload[start:start+3]) for start in range(8, 56, 3)]
    build_bank(frames, palette, list(payload[56:72]) if modern else None)


def runtime_artwork(root=ROOT):
    root = Path(root)
    expected = {}
    for manifest in sorted((root/'Plugins').glob('*.ndplugin')):
        fields = dict(line.split('=', 1) for line in manifest.read_text('ascii').splitlines()
                      if '=' in line and not line.lstrip().startswith(';'))
        features = {part.strip() for part in fields.get('NOVADOCK_FEATURES', '').split(',')}
        bank = manifest.with_suffix('.assets')/'Artwork.ndart'
        if 'artwork' in features or bank.is_file():
            payload = regular(bank)
            validate_bank(payload)
            expected[bank.relative_to(root).as_posix()] = payload
    # An asset directory without a manifest cannot declare its requirements.
    for bank in (root/'Plugins').glob('*.assets/Artwork.ndart'):
        manifest = bank.parent.with_suffix('.ndplugin')
        if not manifest.is_file():
            raise ValueError('Artwork package has no plugin manifest: '+bank.parent.name)
    return expected


def check_runtime(drawer, root=ROOT):
    drawer, root = Path(drawer), Path(root)
    for program in ('Alerts','AmIRC','AmiRocket'):
        for suffix in ('','.info','.ndplugin','.assets'):
            retired=drawer/'Plugins'/(program+suffix)
            if retired.exists():
                raise ValueError('Retired notification component in runtime package: '+str(retired))
    if (drawer/'Rexx/NovaDockNotify.amirx').exists():
        raise ValueError('Retired notification hook in runtime package')
    expected = runtime_artwork(root)
    for name, payload in expected.items():
        bank = drawer/name
        if regular(bank) != payload:
            raise ValueError('Packaged plugin artwork differs: '+name)
        program = Path(name).parent.with_suffix('')
        regular(drawer/program)
        manifest = Path(str(program)+'.ndplugin')
        if regular(drawer/manifest) != regular(root/manifest):
            raise ValueError('Packaged artwork manifest differs: '+str(manifest))
    return len(expected)


def sdk_inventory(root=ROOT):
    from package_release import SDK_FILES
    inventory = {member: regular(Path(root)/source) for member, source in SDK_FILES}
    if not ART_EXAMPLE <= set(inventory):
        raise ValueError('SDK inventory omits part of the artwork example: '+
                         ', '.join(sorted(ART_EXAMPLE-set(inventory))))
    return inventory


def check_sdk(drawer, root=ROOT):
    from pack_plugin_art import build_bank
    import json
    drawer = Path(drawer)
    expected = sdk_inventory(root)
    for member, payload in expected.items():
        if regular(drawer/member) != payload:
            raise ValueError('Extracted SDK file differs: '+member)
    # The example must be rebuildable using its shipped authoring source.
    document = json.loads((drawer/'examples/artwork.json').read_text())
    frames = []
    for frame in document['frames']:
        frame = dict(frame)
        frame['pixels'] = [[int(value, 16) for value in row] if isinstance(row, str)
                           else row for row in frame['pixels']]
        frames.append(frame)
    bank = build_bank(frames, document['palette'], document.get('palette_roles'))
    if bank != regular(drawer/'ArtHello.assets/Artwork.ndart'):
        raise ValueError('Extracted SDK artwork does not match its authoring source')
    return len(expected)


def check_sdk_zip(archive, root=ROOT):
    expected = sdk_inventory(root)
    prefix = 'NovaDock-PluginSDK/'
    with ZipFile(archive) as package, tempfile.TemporaryDirectory(prefix='novadock-sdk-extract-') as tmp:
        names = package.namelist()
        if len(names) != len(set(names)) or set(names) != {prefix+name for name in expected}:
            raise ValueError('SDK ZIP has missing, duplicate or unexpected entries')
        destination = Path(tmp)
        # Only the explicit, reviewed inventory is extracted, never arbitrary paths.
        for member in expected:
            target = destination/member
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(package.read(prefix+member))
        return check_sdk(destination, root)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-drawer', type=Path)
    parser.add_argument('--sdk-zip', type=Path)
    args = parser.parse_args()
    if not args.runtime_drawer and not args.sdk_zip:
        parser.error('Choose --runtime-drawer or --sdk-zip')
    try:
        if args.runtime_drawer:
            print('Verified %d plugin artwork banks after extraction.' % check_runtime(args.runtime_drawer))
        if args.sdk_zip:
            print('Verified %d SDK files after extraction, including rebuildable ArtHello artwork.' % check_sdk_zip(args.sdk_zip))
    except (OSError, ValueError) as error:
        parser.exit(1, 'Package assets check failed: '+str(error)+'\n')


if __name__ == '__main__':
    main()
