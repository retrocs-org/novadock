#!/usr/bin/env python3
"""Offline integrity, manifest and publication recovery tests."""
import argparse
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('release', ROOT/'tools/release.py')
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)
import package_release as package


class IntegrityTests(unittest.TestCase):
    def setUp(self):
        cache = patch.object(r.credentials, "SECRETS", set())
        cache.start(); self.addCleanup(cache.stop)
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)/'source'
        self.root.mkdir()
        self.out = Path(self.tmp.name)/'release'
        self.out.mkdir()
        for name in package.ROOT_FILES:
            path = self.root/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('source input\n')
        for name in package.SOURCE_DIRS:
            (self.root/name).mkdir(exist_ok=True)
        (self.root/'VERSION').write_text('VERSION=0.1\n')
        (self.root/'Makefile').write_text('all: NovaDock Plugins/Clock\n')
        (self.root/'public-web').mkdir()
        (self.root/'public-web/index.php').write_text('website caption')
        (self.root/'docs/guide.md').write_text('documentation')
        for name in ('NovaDock', 'Plugins/Clock'):
            (self.root/name).write_bytes(b'\0\0\x03\xf3native')
        for obj, attr, value in ((r, 'ROOT', self.root), (package, 'ROOT', self.root)):
            p = patch.object(obj, attr, value); p.start(); self.addCleanup(p.stop)
        p = patch.object(r, 'commit', return_value=None); p.start(); self.addCleanup(p.stop)
        inputs = r.provenance.source_inputs(self.root)
        self.info = {'version': '0.1', 'commit': None, 'prebuilt': False,
                     'source_sha256': r.provenance.fingerprint(inputs), 'inputs': inputs,
                     'binaries': r.provenance.binaries(self.root)}
        proof = self.root/'build/provenance/amiga.json'
        proof.parent.mkdir(parents=True)
        r.write_json(proof, self.info)
        for name in r.required_assets('0.1'):
            (self.out/name).write_bytes(b'asset')
        r.write_json(self.out/'BUILD-INFO.json', self.info)
        (self.out/'RELEASE-NOTES.md').write_text('notes')
        # Use real icon bytes; the rest of this source tree is an isolated fixture.
        import shutil
        (self.root/'Icons').mkdir(exist_ok=True)
        shutil.copyfile(ROOT/'Icons/SoftwarePackage.info', self.root/'Icons/SoftwarePackage.info')
        for member, name in package.SDK_FILES:
            path = self.root/name
            path.parent.mkdir(parents=True, exist_ok=True)
            if not path.exists():
                path.write_text('SDK fixture')
        # Adding fixture SDK/icon inputs changes the fingerprint before building.
        inputs = r.provenance.source_inputs(self.root)
        self.info['inputs'] = inputs
        self.info['source_sha256'] = r.provenance.fingerprint(inputs)
        r.write_json(proof, self.info)
        r.write_json(self.out/'BUILD-INFO.json', self.info)
        self.archive_sources()
        self.state = {'assets': r.seal_assets(self.out), 'source_sha256': self.info['source_sha256']}
        r.write_json(self.out/'build-state.json', self.state)
        self.args = argparse.Namespace(output=self.out, config=Path(self.tmp.name)/'config.json', notes=None)
        self.args.config.write_text(json.dumps({'github': 'https://github.com/a/b',
                                              'gitlab': 'https://gitlab.example.org/group/sub/project'}))

    def archive_sources(self):
        # Build dependency order: source/SDK first, then website/full package.
        for suffix in ('source.zip', 'plugin-sdk.zip', 'public-web-updated.zip', 'continued-source-and-site.zip'):
            expected = r.package_contents(self.out)[suffix]
            with ZipFile(self.out/('NovaDock-0.1-'+suffix), 'w') as archive:
                for name, payload in expected.items():
                    archive.writestr(name, payload)

    def test_source_and_documentation_changes_block_check(self):
        r.check_build(self.out)
        for name in ('docs/guide.md', 'public-web/index.php'):
            path = self.root/name
            original = path.read_bytes()
            path.write_bytes(original+b'changed')
            with self.assertRaisesRegex(ValueError, 'Stale source'):
                r.check_build(self.out)
            path.write_bytes(original)

    def test_exact_tag_and_annotation_are_preserved(self):
        self.args.tag = '0.1'
        self.args.tag_message = 'NovaDock 0.1 release'
        r.prepare(self.args)
        manifest = r.bundle(self.out)
        self.assertEqual(manifest['tag'], '0.1')
        self.assertEqual(manifest['tag_message'], 'NovaDock 0.1 release')
        self.assertEqual(manifest['version'], '0.1')
        self.args.tag = None; self.args.tag_message = None
        r.prepare(self.args)
        self.assertEqual(r.bundle(self.out), manifest)

    def test_mismatching_tag_or_annotation_stops_before_credentials(self):
        self.args.tag = '0.1'; self.args.tag_message = 'Reviewed annotation'
        r.prepare(self.args)
        self.args.to = 'github'; self.args.confirm_publish = True
        with patch.object(r.credentials, 'token', side_effect=AssertionError('secret lookup')):
            self.args.tag = 'v0.1'
            with self.assertRaisesRegex(ValueError, 'Requested tag differs'):
                r.publish(self.args)
            self.args.tag = '0.1'; self.args.tag_message = 'Changed annotation'
            with self.assertRaisesRegex(ValueError, 'annotation differs'):
                r.publish(self.args)

    def test_custom_tag_names_are_validated_by_git(self):
        for valid in ('0.1', 'v0.1', 'preview/0.1'):
            self.assertEqual(r.release_tag(valid), valid)
        for invalid in ('', '-option', 'bad tag', 'a..b', 'bad.lock', 'bad?tag'):
            with self.assertRaisesRegex(ValueError, 'Invalid release tag'):
                r.release_tag(invalid)

    def test_invalid_tag_is_rejected_before_building(self):
        self.args.tag = 'bad tag'
        with patch.object(r, 'build', side_effect=AssertionError('build')):
            with self.assertRaisesRegex(ValueError, 'Invalid release tag'):
                r.prepare(self.args)

    def test_incomplete_build_has_actionable_error(self):
        r.write_json(self.out/'build-state.json', {'phase': 'building', 'inputs': {}})
        with self.assertRaisesRegex(ValueError, 'Build did not complete'):
            r.check_build(self.out)

    def test_missing_manifest_has_actionable_error_before_network(self):
        self.args.to = 'github'; self.args.confirm_publish = True
        with patch.object(r.credentials, 'token', side_effect=AssertionError('secret lookup')):
            with self.assertRaisesRegex(ValueError, 'Run prepare successfully before publish'):
                r.publish(self.args)

    def test_dirty_build_stops_before_generators_or_output_creation(self):
        self.args.output = Path(self.tmp.name)/'not-created'
        with patch.object(r, 'commit', side_effect=['a'*40, ValueError('working-tree changes')]), \
             patch.object(r, 'run', side_effect=AssertionError('generator ran')):
            with self.assertRaisesRegex(ValueError, 'working-tree changes'):
                r.build(self.args)
        self.assertFalse(self.args.output.exists())

    def test_publication_only_contains_binary_sdk_and_checksum_sidecars(self):
        r.prepare(self.args)
        manifest = r.bundle(self.out)
        self.assertEqual(set(r.publication_assets(manifest)), {
            'NovaDock-0.1.lha', 'NovaDock-0.1.lha.sha256',
            'NovaDock-0.1-plugin-sdk.zip', 'NovaDock-0.1-plugin-sdk.zip.sha256'})
        sdk = self.out/'NovaDock-0.1-plugin-sdk.zip'
        self.assertEqual((self.out/(sdk.name+'.sha256')).read_text(), r.digest(sdk)+'  '+sdk.name+'\n')
        manifest['publication_assets']['BUILD-INFO.json'] = manifest['assets']['BUILD-INFO.json']
        with self.assertRaisesRegex(ValueError, 'Publication inventory'):
            r.publication_assets(manifest)

    def test_host_debug_bundles_are_not_release_inputs(self):
        path = self.root/'tests/nd_sample_test.dSYM/Contents/Resources/DWARF/nd_sample_test'
        path.parent.mkdir(parents=True)
        path.write_text('host debug data')
        self.assertEqual(r.provenance.source_inputs(self.root), self.info['inputs'])
        self.assertFalse(any('.dSYM' in p.as_posix() for p in package.source_files()))

    def test_changed_native_binary_blocks_check(self):
        (self.root/'NovaDock').write_bytes(b'\0\0\x03\xf3changed')
        with self.assertRaisesRegex(ValueError, 'binaries changed'):
            r.check_build(self.out)

    def test_missing_unexpected_and_symlink_assets(self):
        extra = self.out/'unexpected.zip'; extra.write_text('extra')
        with self.assertRaisesRegex(ValueError, 'Unexpected files'):
            r.check_build(self.out)
        extra.unlink()
        binary = self.out/'NovaDock-0.1.lha'; binary.unlink()
        with self.assertRaisesRegex(ValueError, 'Missing'):
            r.check_build(self.out)
        binary.symlink_to(self.root/'NovaDock')
        with self.assertRaisesRegex(ValueError, 'Symlinked'):
            r.check_build(self.out)

    def test_checksum_inventory_must_cover_exactly_the_files(self):
        path = self.out/'SHA256SUMS.txt'
        path.write_text(path.read_text()+path.read_text().splitlines()[0]+'\n')
        self.state['assets']['SHA256SUMS.txt'] = r.digest(path)
        r.write_json(self.out/'build-state.json', self.state)
        with self.assertRaisesRegex(ValueError, 'Checksum inventory'):
            r.check_build(self.out)

    def test_duplicate_json_keys_rejected(self):
        (self.out/'build-state.json').write_text('{"assets": {}, "assets": {}}')
        with self.assertRaisesRegex(ValueError, 'Duplicate JSON'):
            r.check_build(self.out)

    def test_stale_source_inside_archive_rejected_even_with_new_checksum(self):
        self.archive_sources()
        path = self.out/'NovaDock-0.1-source.zip'
        import warnings
        with warnings.catch_warnings(), ZipFile(path, 'a') as archive:
            warnings.simplefilter('ignore', UserWarning)
            archive.writestr('NovaDock/docs/guide.md', 'old')
        self.state['assets'] = r.seal_assets(self.out)
        r.write_json(self.out/'build-state.json', self.state)
        with self.assertRaisesRegex(ValueError, 'Duplicate source'):
            r.check_build(self.out)

    def test_prepare_is_offline_and_manifest_is_immutable(self):
        with patch.object(r, 'API', side_effect=AssertionError('network')):
            r.prepare(self.args)
            first = (self.out/'manifest.json').read_bytes()
            r.prepare(self.args)
            self.assertEqual(first, (self.out/'manifest.json').read_bytes())
            self.args.config.write_text(json.dumps({'github': 'https://github.com/other/project'}))
            with self.assertRaisesRegex(ValueError, 'manifest differs'):
                r.prepare(self.args)

    def test_notes_are_explicitly_sealed(self):
        notes = Path(self.tmp.name)/'notes.md'; notes.write_text('Reviewed notes')
        self.args.notes = notes
        r.prepare(self.args)
        self.assertEqual(r.bundle(self.out)['notes'], 'Reviewed notes')

    def test_receipt_cannot_be_reused_for_another_manifest(self):
        r.prepare(self.args)
        receipt = r.publication_receipt(self.out)
        r.write_json(self.out/'receipt.json', receipt)
        (self.out/'manifest.json').write_text('{}')
        with self.assertRaisesRegex(ValueError, 'different manifest'):
            r.publication_receipt(self.out)

    def test_publication_requires_explicit_confirmation_before_any_network(self):
        self.args.to = 'github'; self.args.execute = False; self.args.confirm_publish = False
        with patch.object(r, 'API', side_effect=AssertionError('network')):
            with self.assertRaisesRegex(ValueError, 'confirm-publish'):
                r.publish(self.args)

    def test_destination_changes_and_missing_tokens_stop_before_upload(self):
        r.prepare(self.args)
        self.args.to = 'github'; self.args.confirm_publish = True
        with patch.object(r, 'commit', return_value=None):
            self.args.config.write_text(json.dumps({'github': 'https://github.com/other/project'}))
            with self.assertRaisesRegex(ValueError, 'destinations differ'):
                r.publish(self.args)

    def test_duplicate_destinations_are_rejected(self):
        self.args.to = 'github,github'
        with self.assertRaisesRegex(ValueError, 'distinct'):
            r.selected_targets(self.args)

    def test_plan_runs_no_builds_or_network(self):
        with patch.object(r, 'run', side_effect=AssertionError('build')), \
             patch.object(r, 'API', side_effect=AssertionError('network')), contextlib.redirect_stdout(io.StringIO()) as output:
            r.plan(self.args)
        self.assertIn('make-release.sh', output.getvalue())
        self.assertLess(output.getvalue().index('make-release.sh'), output.getvalue().index('package_release.py'))

    def test_configuration_rejects_secret_and_unexpected_environment_fields(self):
        self.args.config.write_text(json.dumps({'env': {'GITHUB_TOKEN': 'secret'}}))
        with self.assertRaisesRegex(ValueError, 'Unsupported'):
            r.config(self.args.config)


class TagTests(unittest.TestCase):
    def test_real_annotated_tag_and_clean_source_checks(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            def git(*args):
                return subprocess.check_output(['git', *args], cwd=root, stderr=subprocess.DEVNULL).decode().strip()
            git('init', '-q'); git('config', 'user.name', 'Release Test')
            git('config', 'user.email', 'test@example.org')
            (root/'source.c').write_text('input')
            (root/'VERSION').write_text('VERSION=0.1\n')
            git('add', '.'); git('commit', '-qm', 'initial')
            with patch.object(r, 'ROOT', root), patch.object(r.provenance, 'source_inputs', return_value={'source.c': 'hash', 'VERSION': 'hash'}):
                sha = r.commit(clean=True)
                r.local_tag('v0.1', sha, create=True)
                self.assertEqual(git('cat-file', '-t', 'v0.1'), 'tag')
                r.local_tag('v0.1', sha)
                git('tag', 'v0.2')
                with self.assertRaisesRegex(ValueError, 'annotated'):
                    r.local_tag('v0.2', sha)
                (root/'ignored.c').write_text('untracked input')
                with self.assertRaisesRegex(ValueError, 'must be tracked'):
                    r.provenance.tracked_inputs({'ignored.c': 'hash'}, root)
                with self.assertRaisesRegex(ValueError, 'working-tree'):
                    r.commit(clean=True)

    def test_tag_push_ignores_follow_tags_setting(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)/'source'; root.mkdir()
            remote = Path(tmp)/'remote'
            subprocess.run(['git', 'init', '--bare', '-q', str(remote)], check=True)
            def git(*args):
                return subprocess.check_output(['git', *args], cwd=root, stderr=subprocess.DEVNULL).decode().strip()
            git('init', '-q'); git('config', 'user.name', 'Release Test'); git('config', 'user.email', 'test@example.org')
            (root/'VERSION').write_text('VERSION=0.1\n')
            git('add', '.'); git('commit', '-qm', 'initial')
            sha = git('rev-parse', 'HEAD')
            git('config', 'push.followTags', 'true')
            git('tag', '-a', 'unrelated', '-m', 'Do not upload')
            with patch.object(r, 'ROOT', root):
                r.local_tag('v0.1', sha, create=True)
                r.push_tag(str(remote), 'v0.1', sha)
            refs = git('ls-remote', '--refs', str(remote)).splitlines()
            self.assertEqual([line.split()[1] for line in refs], ['refs/tags/v0.1'])

    def test_remote_annotated_tag_is_peeled(self):
        with patch.object(r, 'run', return_value='tagobject\trefs/tags/v0.1\ncommit\trefs/tags/v0.1^{}\n'):
            self.assertEqual(r.remote_tag('unused', 'v0.1'), 'commit')


if __name__ == '__main__':
    unittest.main()
