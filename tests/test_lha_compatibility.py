#!/usr/bin/env python3
"""Regress OLHa path compatibility without requiring an Amiga emulator."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import check_lha_compatibility as lha_check


def entry(name=b'Install', directory=b'NovaDock\xff', payload=b'payload\xffbytes', level=1):
    extension = b'\x02'+directory+b'\0\0'
    header = bytearray(27+len(name))
    header[0] = len(header)-2
    header[2:7] = b'-lh5-'
    header[7:11] = (len(extension)+len(payload)).to_bytes(4, 'little')
    header[11:15] = len(payload).to_bytes(4, 'little')
    header[20] = level
    header[21] = len(name)
    header[22:22+len(name)] = name
    header[-3] = ord('U')
    header[-2:] = len(extension).to_bytes(2, 'little')
    header[1] = sum(header[2:]) & 255
    return bytes(header)+extension+payload


class LhaCompatibility(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.archive = self.root/'fixture.lha'

    def check(self, data):
        self.archive.write_bytes(data)
        return lha_check.check_archive(self.archive)

    def test_level_one_directory_separators_and_payload_are_preserved(self):
        data = entry(directory=b'NovaDock\xffPlugins\xffExample.assets\xff')+b'\0'
        self.assertEqual(self.check(data), 1)
        self.assertEqual(self.archive.read_bytes(), data)

    def test_level_zero_is_rejected_instead_of_rewriting_archive_bytes(self):
        with self.assertRaisesRegex(ValueError, 'level-1'):
            self.check(entry(name=b'NovaDock\xffInstall', level=0)+b'\0')

    def test_incompatible_or_unsafe_names_are_rejected(self):
        for name in (b'NovaDock\xffInstall', b'NovaDock/Install', b'NovaDock\\Install', b'..'):
            with self.subTest(name=name), self.assertRaises(ValueError):
                self.check(entry(name=name)+b'\0')
        for directory in (b'NovaDock/Plugins/', b'NovaDock\xff..\xff', b'NovaDock\xff\xff'):
            with self.subTest(directory=directory), self.assertRaises(ValueError):
                self.check(entry(directory=directory)+b'\0')

    def test_corrupt_and_truncated_headers_are_rejected(self):
        data = entry()+b'\0'
        corrupt = bytearray(data);corrupt[1] ^= 1
        for broken in (b'', bytes(corrupt), data[:12], data[:-4], data[:-1], data+b'junk'):
            with self.subTest(broken=broken), self.assertRaises(ValueError):
                self.check(broken)
        corrupt = bytearray(data)
        base_end = corrupt[0]+2
        corrupt[base_end-2:base_end] = b'\xff\x7f'
        corrupt[1] = sum(corrupt[2:base_end]) & 255
        with self.assertRaisesRegex(ValueError, 'extension size'):
            self.check(corrupt)

    def test_real_archiver_profile_and_release_gate(self):
        lha = os.environ.get('LHA') or shutil.which('lha')
        if not lha:
            self.skipTest('LHa for UNIX unavailable')
        probe = subprocess.run([lha, '--version'], capture_output=True, text=True)
        if 'LHa for UNIX' not in probe.stdout+probe.stderr:
            self.skipTest('LHa for UNIX required')
        nested = self.root/'NovaDock/Plugins/Example.assets'
        nested.mkdir(parents=True)
        (nested/'Artwork.ndart').write_bytes(b'\xff\0\xffpayload')
        (self.root/'NovaDock/Install').write_text('installer')
        subprocess.run([lha, '-a1o5', str(self.archive), 'NovaDock'], cwd=self.root,
                       check=True, stdout=subprocess.DEVNULL)
        self.assertEqual(lha_check.check_archive(self.archive), 2)
        script = (ROOT/'scripts/make-release.sh').read_text()
        gate = script.index('tools/check_lha_compatibility.py')
        self.assertLess(script.index('"$lha" -a1o5'), gate)
        self.assertLess(gate, script.index('mv -f "$archive"'))


if __name__ == '__main__':
    unittest.main()
