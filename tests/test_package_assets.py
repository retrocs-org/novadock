#!/usr/bin/env python3
"""Reject release packages that lose artwork, manifests or SDK example files."""
from pathlib import Path
import sys
import tempfile
import unittest
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import check_package_assets as checks
import package_release as package
from pack_plugin_art import build_bank


class PackageAssetsTests(unittest.TestCase):
    def test_required_runtime_bank_and_matching_manifest(self):
        with tempfile.TemporaryDirectory() as tmp:
            root, drawer = Path(tmp)/'source', Path(tmp)/'extracted'
            pixels = [[1]*32 for _ in range(32)]
            bank = build_bank([{'name': 'Lamp', 'state': 0, 'pixels': pixels}], [[0, 0, 0]]*16)
            files = {'Plugins/Lamp.v1': b'program',
                     'Plugins/Lamp.v1.ndplugin': b'NOVADOCK_FEATURES=click, artwork\n',
                     'Plugins/Lamp.v1.assets/Artwork.ndart': bank}
            for base in (root, drawer):
                for member, data in files.items():
                    target = base/member
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(data)
            self.assertEqual(checks.check_runtime(drawer, root), 1)
            target = drawer/'Plugins/Lamp.v1.assets/Artwork.ndart'
            target.unlink()
            with self.assertRaisesRegex(ValueError, 'Missing'): checks.check_runtime(drawer, root)
            target.write_bytes(b'broken')
            with self.assertRaisesRegex(ValueError, 'differs'): checks.check_runtime(drawer, root)
            target.write_bytes(bank)
            (drawer/'Plugins/Lamp.v1.ndplugin').write_bytes(b'old manifest')
            with self.assertRaisesRegex(ValueError, 'manifest differs'): checks.check_runtime(drawer, root)
            (root/'Plugins/Lamp.v1.assets/Artwork.ndart').unlink()
            with self.assertRaisesRegex(ValueError, 'Missing'): checks.runtime_artwork(root)

    def test_retired_notification_components_cannot_return(self):
        with tempfile.TemporaryDirectory() as tmp:
            drawer=Path(tmp)/'runtime';root=Path(tmp)/'source'
            (root/'Plugins').mkdir(parents=True)
            for program in ('Alerts','AmIRC','AmiRocket'):
                for suffix in ('','.info','.ndplugin','.assets'):
                    target=drawer/'Plugins'/(program+suffix)
                    target.parent.mkdir(parents=True,exist_ok=True)
                    if suffix=='.assets':target.mkdir()
                    else:target.write_bytes(b'old component')
                    with self.assertRaisesRegex(ValueError,'Retired'):checks.check_runtime(drawer,root)
                    if target.is_dir():target.rmdir()
                    else:target.unlink()
            target=drawer/'Rexx/NovaDockNotify.amirx';target.parent.mkdir();target.write_text('old hook')
            with self.assertRaisesRegex(ValueError,'Retired'):checks.check_runtime(drawer,root)

    def test_banks_are_valid_not_only_present(self):
        for payload in (b'bad', b'NDAR\0\3'+bytes(80), b'NDAR\0\1'+bytes(80)):
            with self.assertRaises(ValueError): checks.validate_bank(payload)
        banks = checks.runtime_artwork(ROOT)
        self.assertGreaterEqual(len(banks), 15)
        for payload in banks.values(): checks.validate_bank(payload)

    def test_complete_sdk_survives_zip_extraction(self):
        entries = package.sdk_entries()
        with tempfile.TemporaryDirectory() as tmp:
            archive = Path(tmp)/'SDK.zip'
            def write(selected): archive.write_bytes(package.zip_bytes(selected))
            write(entries)
            self.assertEqual(checks.check_sdk_zip(archive), len(package.SDK_FILES))
            for required in sorted(checks.ART_EXAMPLE):
                omitted = 'NovaDock-PluginSDK/'+required
                write([(name, data) for name, data in entries if name != omitted])
                with self.assertRaisesRegex(ValueError, 'missing'): checks.check_sdk_zip(archive)
            write([(name, b'changed' if name.endswith('Artwork.ndart') else data) for name, data in entries])
            with self.assertRaisesRegex(ValueError, 'differs'): checks.check_sdk_zip(archive)
            write(entries+[('../unexpected', b'bad')])
            with self.assertRaisesRegex(ValueError, 'unexpected'): checks.check_sdk_zip(archive)

    def test_inventory_cannot_silently_drop_the_artwork_example(self):
        original = package.SDK_FILES
        try:
            package.SDK_FILES = tuple(pair for pair in original if pair[0] != 'tools/pack_plugin_art.py')
            with self.assertRaisesRegex(ValueError, 'omits'): checks.sdk_inventory()
        finally:
            package.SDK_FILES = original


if __name__ == '__main__': unittest.main()
