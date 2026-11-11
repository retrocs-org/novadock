#!/usr/bin/env python3
"""Aminet preparation, metadata and transport tests. Never contact a server."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('aminet_release', ROOT/'tools/aminet_release.py')
am = importlib.util.module_from_spec(spec); spec.loader.exec_module(am)
README = '''Short:        Extensible application dock
Author:       Unit test
Uploader:     maintainer@unit-test.invalid
Type:         util/wb
Version:      0.1
Architecture: m68k-amigaos >= 3.0.0
Distribution: Aminet

A local test submission. No server is contacted.
'''


class AminetTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='aminet test ')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.archive = self.root/'NovaDock-0.1.lha'
        self.archive.write_bytes(b'unit-test-archive')
        self.readme = self.archive.with_suffix('.readme'); self.readme.write_text(README)
        Path(str(self.archive)+'.sha256').write_text(am.sha256(self.archive)+'  '+self.archive.name+'\n')
        self.output = self.root/'aminet'
        self.crc = patch.object(am, 'archive_crc'); self.crc.start(); self.addCleanup(self.crc.stop)

    def test_prepare_stable_names_and_offline_check(self):
        with patch.object(am.ftplib, 'FTP', side_effect=AssertionError('network used')):
            manifest, fields = am.prepare(self.archive, self.output, 'lha')
            self.assertEqual(set(manifest['files']), {'novadock.lha', 'novadock.readme'})
            self.assertEqual((self.output/'novadock.lha').read_bytes(), self.archive.read_bytes())
            self.assertEqual(manifest['version'], '0.1')
            am.check(self.output, 'lha')
            with contextlib.redirect_stdout(io.StringIO()) as result:
                am.report(self.output, manifest, fields)
            self.assertIn('no files uploaded', result.getvalue())
            self.assertIn('ftp://main.aminet.net/new/', result.getvalue())
        self.assertTrue(self.archive.exists())

    def test_missing_contact_and_bad_checksums_refused(self):
        self.readme.write_text(README.replace('maintainer@unit-test.invalid', '(set UPLOADER before uploading)'))
        with self.assertRaisesRegex(ValueError, 'real email'):
            am.prepare(self.archive, self.output, 'lha')
        self.assertFalse(self.output.exists())
        self.readme.write_text(README)
        self.archive.write_bytes(b'changed archive')
        with self.assertRaisesRegex(ValueError, 'checksum'):
            am.prepare(self.archive, self.output, 'lha')

    def test_readme_rules(self):
        for wrong in [README.replace('Short:', 'Title:'), README.replace('0.1', ''),
                      README.replace('application dock', 'x'*60), README.replace('\n', '\r\n'),
                      README.replace('util/wb', '../wb'), README.replace('A local test', 'x'*79),
                      README.replace('Version:      0.1', 'Version:      0.1\nVersion:      0.2')]:
            self.readme.write_bytes(wrong.encode('ascii'))
            with self.assertRaises(ValueError): am.readme_fields(self.readme)
        self.readme.write_text(README)
        am.format_readme(self.readme, 'util/wb/NovaDock-0.1.lha')
        self.assertEqual(am.readme_fields(self.readme)['Replaces'], 'util/wb/NovaDock-0.1.lha')
        previous = self.readme.read_bytes()
        with self.assertRaises(ValueError): am.format_readme(self.readme, '../other.lha')
        self.assertEqual(self.readme.read_bytes(), previous)

    def test_release_mode_rejects_missing_or_placeholder_contact_before_build(self):
        import os
        for contact in (None, '(set UPLOADER before uploading)'):
            env = dict(os.environ)
            if contact is None: env.pop('UPLOADER', None)
            else: env['UPLOADER'] = contact
            target = self.root/'not-created'
            result = subprocess.run(['sh', str(ROOT/'scripts/make-release.sh'), str(target), '--aminet'],
                                    env=env, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('real email', result.stdout+result.stderr)
            self.assertFalse(target.exists())

    def test_long_description_wraps(self):
        self.readme.write_text(README+'\n'+'word '*100+'\n\n- '+'bullet '*100+'\n')
        am.format_readme(self.readme)
        self.assertTrue(all(len(line)<=78 for line in self.readme.read_text().splitlines()))
        am.readme_fields(self.readme)

    def test_changed_files_and_destination_refused(self):
        am.prepare(self.archive, self.output, 'lha')
        (self.output/'novadock.readme').write_text(README+'edited\n')
        with self.assertRaisesRegex(ValueError, 'changed since preparation'):
            am.check(self.output, 'lha')
        am.prepare(self.archive, self.output, 'lha')
        p=self.output/'submission.json'; manifest=json.loads(p.read_text()); manifest['host']='elsewhere.invalid'
        p.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'destination'):
            am.check(self.output, 'lha')

    def test_malformed_manifest(self):
        am.prepare(self.archive, self.output, 'lha')
        for wrong in ([], {'files': []}, {'host': am.HOST, 'directory': am.DIRECTORY}):
            (self.output/'submission.json').write_text(json.dumps(wrong))
            with self.assertRaisesRegex(ValueError, 'Malformed'):
                am.check(self.output, 'lha')

    def test_crc_failure(self):
        with patch.object(am, 'archive_crc', side_effect=ValueError('CRC verification failed')):
            with self.assertRaisesRegex(ValueError, 'CRC'):
                am.prepare(self.archive, self.output, 'lha')
        self.assertFalse(self.output.exists())

    def test_explicit_upload_pair_order_and_first_failure(self):
        manifest, fields=am.prepare(self.archive, self.output, 'lha')
        events=[]
        class FTP:
            def __init__(self, timeout): events.append(('timeout', timeout))
            def __enter__(self): return self
            def __exit__(self, *args): events.append(('closed',))
            def connect(self, *args): events.append(('connect',)+args)
            def login(self, *args): events.append(('login',)+args)
            def cwd(self, path): events.append(('cwd', path))
            def set_pasv(self, value): events.append(('passive', value))
            def storbinary(self, command, data): events.append((command, data.read()))
        am.upload(self.output, manifest, fields, FTP)
        self.assertIn(('connect', 'main.aminet.net', 21), events)
        self.assertIn(('login', 'anonymous', 'maintainer@unit-test.invalid'), events)
        self.assertIn(('cwd', '/new'), events)
        transfers=[event for event in events if event[0].startswith('STOR ')]
        self.assertEqual([event[0] for event in transfers], ['STOR novadock.lha', 'STOR novadock.readme'])
        self.assertEqual(transfers[0][1], self.archive.read_bytes())
        # A completed receipt makes another call a no-op.
        events.clear()
        am.upload(self.output, manifest, fields, FTP)
        self.assertFalse(events)
        # Use a distinct prepared submission to exercise a failed transfer.
        self.output = self.root/'second-submission'
        manifest, fields = am.prepare(self.archive, self.output, 'lha')
        class FailFTP(FTP):
            def storbinary(self, command, data):
                events.append((command,)); raise am.ftplib.error_temp('forced failure')
        with self.assertRaises(am.ftplib.error_temp): am.upload(self.output, manifest, fields, FailFTP)
        self.assertNotIn(('STOR novadock.readme',), events)
        self.assertEqual(events[-1], ('closed',))
        with self.assertRaisesRegex(ValueError, 'Partial'):
            am.upload(self.output, manifest, fields, FTP)
        with self.assertRaisesRegex(ValueError, 'receipt'):
            am.prepare(self.archive, self.output, 'lha')

    def test_changed_archive_never_connects(self):
        manifest, fields=am.prepare(self.archive, self.output, 'lha')
        (self.output/'novadock.lha').write_bytes(b'rebuilt')
        with self.assertRaisesRegex(ValueError, 'nothing was sent'):
            am.upload(self.output, manifest, fields, lambda **kwargs: self.fail('network used'))


if __name__ == '__main__': unittest.main()
