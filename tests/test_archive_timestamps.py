#!/usr/bin/env python3
"""Regress old checkout dates and ZIP defaults through real archive creation."""
from io import BytesIO
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from zipfile import ZipFile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import archive_timestamps as dates
import package_release as package


class ArchiveDates(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name);self.epoch=dates.current_epoch()

    def fixture(self):
        source=self.root/'source';source.write_bytes(b'unchanged source payload')
        old=315532800;os.utime(source,(old,old))  # 1980, independent of today's clock.
        drawer=self.root/'NovaDock';nested=drawer/'Plugins/Example.assets';nested.mkdir(parents=True)
        copied=nested/'Artwork.ndart';shutil.copy2(source,copied)
        icon=self.root/'NovaDock.info';shutil.copy2(source,icon)
        return source,drawer,icon,copied,old

    def test_old_file_and_directory_dates_are_replaced_only_in_staging(self):
        source,drawer,icon,copied,old=self.fixture()
        for path in dates.paths_in([drawer,icon]):os.utime(path,(old,old))
        dates.stamp_tree([drawer,icon],self.epoch)
        for path in dates.paths_in([drawer,icon]):self.assertEqual(path.stat().st_mtime,self.epoch)
        self.assertEqual(source.stat().st_mtime,old)
        self.assertEqual(copied.read_bytes(),source.read_bytes())
        dates.check_extracted([drawer,icon],self.epoch)
        os.utime(copied,(old,old))
        with self.assertRaisesRegex(ValueError,'archive date'):dates.check_extracted([drawer,icon],self.epoch)

    def test_zip_members_use_build_time_not_1980_or_fixed_2020(self):
        payload=package.zip_bytes([('NovaDock/Install',b'script'),('NovaDock.info',b'icon')],timestamp=self.epoch)
        with ZipFile(BytesIO(payload)) as archive:
            for entry in archive.infolist():self.assertEqual(entry.date_time,dates.zip_datetime(self.epoch))
        before=dates.current_epoch();payload=package.zip_bytes([('default',b'today')]);after=dates.current_epoch()
        with ZipFile(BytesIO(payload)) as archive:self.assertIn(archive.infolist()[0].date_time,[dates.zip_datetime(t) for t in range(before,after+1,2)])
        explicit=package.zip_bytes([('default',b'today')],timestamp=self.epoch)
        self.assertEqual(explicit,package.zip_bytes([('default',b'today')],timestamp=self.epoch))

    def test_zip_members_are_world_readable_and_scripts_executable(self):
        payload=package.zip_bytes([('NovaDock/scripts/fuzz.sh',b'#!/bin/sh\n'),('NovaDock/tools/a.py',b'#!/usr/bin/env python3\n'),
                                   ('NovaDock/tools/lib.py',b'import os\n'),('NovaDock/README.md',b'x')],timestamp=self.epoch)
        with ZipFile(BytesIO(payload)) as archive:
            modes={e.filename:(e.external_attr>>16)&0o777 for e in archive.infolist()}
        self.assertEqual(modes['NovaDock/scripts/fuzz.sh'],0o755)
        self.assertEqual(modes['NovaDock/tools/a.py'],0o755)
        self.assertEqual(modes['NovaDock/tools/lib.py'],0o644)
        self.assertEqual(modes['NovaDock/README.md'],0o644)

    def test_source_checksums_replace_stale_manifest_and_include_new_files(self):
        entries=[('NovaDock/old.c',b'changed'),('NovaDock/new.c',b'new'),
                 ('NovaDock/SHA256SUMS',b'stale')]
        entries=package.with_source_checksums(entries)
        payload=package.zip_bytes(entries,timestamp=self.epoch)
        with ZipFile(BytesIO(payload)) as archive:
            manifest=archive.read('NovaDock/SHA256SUMS').decode().splitlines()
            rows={name:digest for digest,name in (line.split('  ',1) for line in manifest)}
            self.assertEqual(set(rows),{'old.c','new.c'})
            for name,digest in rows.items():
                self.assertEqual(digest,hashlib.sha256(archive.read('NovaDock/'+name)).hexdigest())
        self.assertEqual(entries,package.with_source_checksums(entries))

    def test_real_level_one_lha_preserves_current_dates(self):
        lha=os.environ.get('LHA') or shutil.which('lha')
        if not lha:self.skipTest('LHa for UNIX unavailable')
        probe=subprocess.run([lha,'--version'],capture_output=True,text=True)
        if 'LHa for UNIX' not in probe.stdout+probe.stderr:self.skipTest('LHa for UNIX required')
        source,drawer,icon,copied,old=self.fixture();dates.stamp_tree([drawer,icon],self.epoch)
        archive=self.root/'dates.lha'
        subprocess.run([lha,'-a1o5',str(archive),'NovaDock','NovaDock.info'],cwd=self.root,check=True,stdout=subprocess.DEVNULL)
        extract=self.root/'extract';extract.mkdir()
        subprocess.run([lha,'-x',str(archive)],cwd=extract,check=True,stdout=subprocess.DEVNULL)
        dates.check_extracted([extract/'NovaDock',extract/'NovaDock.info'],self.epoch)
        self.assertEqual((extract/copied.relative_to(self.root)).read_bytes(),source.read_bytes())
        self.assertEqual(source.stat().st_mtime,old)

    def test_no_partial_stamping_when_tree_contains_a_symlink(self):
        source,drawer,icon,copied,old=self.fixture();(drawer/'linked').symlink_to(source)
        with self.assertRaises(ValueError):dates.stamp_tree([drawer,icon],self.epoch)
        self.assertEqual(copied.stat().st_mtime,old)


if __name__=='__main__':unittest.main()
