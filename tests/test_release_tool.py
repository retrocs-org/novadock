#!/usr/bin/env python3
"""Release lifecycle regressions; all remote calls are mocked, nothing uploaded."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('release', Path(__file__).resolve().parents[1]/'tools/release.py')
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        cache = patch.object(r.credentials, "SECRETS", set())
        cache.start(); self.addCleanup(cache.stop)
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.out = Path(self.tmp.name)
        (self.out/'RELEASE-NOTES.md').write_text('Release notes')
        v = r.version()
        for suffix in ('.lha', '.readme', '.lha.sha256', '-source.zip', '-plugin-sdk.zip',
                       '-public-web-updated.zip', '-continued-source-and-site.zip'):
            (self.out/('NovaDock-'+v+suffix)).write_bytes(b'test asset')
        (self.out/'SHA256SUMS.txt').write_text('')
        (self.out/'BUILD-INFO.json').write_text(json.dumps({'version': v, 'commit': 'a'*40,
                                                          'prebuilt': False}))
        self.m = {'version': r.version(), 'tag': 'v'+r.version(), 'commit': 'a'*40, 'prebuilt': False,
                  'assets': {p.name: r.digest(p) for p in self.out.iterdir()}}
        self.m['assets'] = r.seal_assets(self.out)
        self.save()

    def save(self):
        (self.out/'release.json').write_text(json.dumps(self.m))

    def test_checksum_tampering_stops_before_publication(self):
        r.inventory(self.out, self.m['assets'])
        (self.out/('NovaDock-'+r.version()+'.lha')).write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'modified'):
            r.inventory(self.out, self.m['assets'])

    def test_asset_path_checks(self):
        self.m['assets'].pop(next(iter(self.m['assets'])))
        self.m['assets']['../outside'] = '0'*64
        with self.assertRaisesRegex(ValueError, 'unexpected'):
            r.inventory(self.out, self.m['assets'])

    def test_repository_credentials_and_wrong_host_rejected(self):
        for url in ('http://github.com/a/b', 'https://token@github.com/a/b',
                    'https://other.org/a/b', 'https://github.com/a/b?token=secret'):
            with self.assertRaises(ValueError):
                r.repo_url(url, github=True)

    def github_api(self, draft=True, sha=None):
        calls = []
        assets = [{'name': name, 'digest': 'sha256:'+(sha or digest)}
                  for name, digest in r.publication_assets(self.m).items()]
        class Mock:
            def call(inner, method, path, payload=None, **kwargs):
                calls.append((method, path, payload))
                if path.endswith('?per_page=100') and '/assets?' not in path:
                    return [{'tag_name': self.m['tag'], 'id': 7, 'draft': draft, 'body': 'Release notes'}]
                if method == 'GET' and '/assets?' in path:
                    return assets
                return {}
        return Mock(), calls, assets

    def test_github_preview_never_writes(self):
        api, calls, _ = self.github_api()
        r.publish_github(api, 'a/b', self.out, self.m, True)
        self.assertTrue(all(x[0] == 'GET' for x in calls))

    def test_github_conflicting_asset_stops(self):
        api, _, _ = self.github_api(sha='0'*64)
        with self.assertRaisesRegex(ValueError, 'different/unverifiable'):
            r.publish_github(api, 'a/b', self.out, self.m, False)

    def test_github_completed_draft_resumes_and_publishes_last(self):
        api, calls, _ = self.github_api()
        with patch.dict(r.os.environ, {'GITHUB_TOKEN': 'test'}):
            r.publish_github(api, 'a/b', self.out, self.m, False)
        self.assertEqual(calls[-1], ('PATCH', '/repos/a/b/releases/7', {'draft': False}))
        self.assertFalse(any(x[0] == 'POST' for x in calls))

    def test_github_published_release_missing_asset_is_not_modified(self):
        api, calls, assets = self.github_api(draft=False)
        assets.pop()
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            r.publish_github(api, 'a/b', self.out, self.m, False)
        self.assertTrue(all(x[0] == 'GET' for x in calls))

    def test_gitlab_verifies_before_creating_release_and_retries(self):
        files, calls = {}, []
        class Mock:
            base = 'https://gitlab.example.org/api/v4'
            def file_digest(inner, path, missing=False):
                return hashlib.sha256(files[path]).hexdigest() if path in files else None
            def call(inner, method, path, payload=None, **kwargs):
                calls.append((method, path))
                if method == 'PUT':
                    files[path] = kwargs['upload'].read_bytes(); return {}
                if method == 'POST':
                    self.assertEqual(len(files), len(r.publication_assets(self.m)))
                    return {}
                return files.get(path)
        api = Mock()
        r.publish_gitlab(api, 'group/project', self.out, self.m, True)
        self.assertFalse(files)
        r.publish_gitlab(api, 'group/project', self.out, self.m, False)
        self.assertEqual(calls[-1][0], 'POST')
        calls.clear()
        r.publish_gitlab(api, 'group/project', self.out, self.m, False)
        self.assertFalse(any(x[0] == 'PUT' for x in calls))
        files[next(iter(files))] = b'wrong'
        with self.assertRaisesRegex(ValueError, 'different asset'):
            r.publish_gitlab(api, 'group/project', self.out, self.m, True)

    def test_real_git_clean_checkout_required(self):
        repo = self.out/'repo'
        repo.mkdir()
        import subprocess
        def git(*args):
            return subprocess.run(['git', *args], cwd=repo, check=True,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        git('init', '-q')
        (repo/'source.c').write_text('source')
        git('add', 'source.c')
        git('-c', 'user.name=Release Test', '-c', 'user.email=test@example.org',
            'commit', '-qm', 'initial')
        with patch.object(r, 'ROOT', repo), patch.object(r.provenance, 'source_inputs', return_value={'source.c': 'hash'}):
            self.assertEqual(len(r.commit(clean=True)), 40)
            (repo/'source.c').write_text('changed')
            with self.assertRaisesRegex(ValueError, 'working-tree changes'):
                r.commit(clean=True)

    def test_both_targets_preflight_before_first_push(self):
        cfg = self.out/'targets.json'
        cfg.write_text(json.dumps({'github': {'url': 'https://github.com/a/b'},
                                   'gitlab': {'url': 'https://gitlab.example.org/a/b'}}))
        self.m['destinations'] = r.destinations(r.load_json(cfg))
        (self.out/'manifest.json').write_text(json.dumps(self.m))
        args = argparse.Namespace(output=self.out, config=cfg, target=["github", "gitlab"], execute=True)
        with patch.dict(r.os.environ, {'GITHUB_TOKEN': 'test', 'GITLAB_TOKEN': 'test'}), \
             patch.object(r, 'bundle', return_value=self.m), \
             patch.object(r, 'local_tag'), \
             patch.object(r.provenance, 'tracked_inputs'), \
             patch.object(r, 'commit', return_value='a'*40), \
             patch.object(r, 'run', return_value='') as git, \
             patch.object(r, 'API') as api, \
             patch.object(r, 'publish_github') as gh, \
             patch.object(r, 'publish_gitlab', side_effect=ValueError('no permission')):
            with self.assertRaisesRegex(ValueError, 'no permission'):
                r.publish(args)
            self.assertFalse(any(c.args[:2] == ('git', 'push') for c in git.call_args_list))
            self.assertTrue(gh.call_args.args[-1])

    def test_github_unknown_digest_and_notes_conflict_stop_without_writes(self):
        api, calls, assets = self.github_api()
        assets[0].pop('digest')
        with self.assertRaisesRegex(ValueError, 'unverifiable'):
            r.publish_github(api, 'a/b', self.out, self.m, False)
        self.assertTrue(all(method == 'GET' for method, _, _ in calls))
        api, calls, _ = self.github_api()
        (self.out/'RELEASE-NOTES.md').write_text('changed notes')
        with self.assertRaisesRegex(ValueError, 'notes differ'):
            r.publish_github(api, 'a/b', self.out, self.m, False)

    def test_new_github_draft_only_publishes_after_every_upload_verifies(self):
        calls, stored = [], {}
        class Mock:
            def call(inner, method, path, payload=None, **kwargs):
                calls.append((method, path, payload))
                if method == 'GET' and path.endswith('?per_page=100'):
                    return list(stored.values()) if '/assets?' in path else []
                if method == 'GET': return None
                if method == 'POST':
                    self.assertTrue(payload['draft'])
                    return {'id': 8, 'draft': True}
                if method == 'PATCH':
                    self.assertEqual(len(stored), len(r.publication_assets(self.m)))
                return {}
        class Uploads:
            def call(inner, method, path, upload):
                result = {'name': upload.name, 'digest': 'sha256:'+r.digest(upload)}
                stored[upload.name] = result
                return result
        with patch.object(r, 'API', return_value=Uploads()), patch.dict(r.os.environ, {'GITHUB_TOKEN': 'test'}):
            r.publish_github(Mock(), 'a/b', self.out, self.m, False)
        self.assertEqual(calls[-1][0], 'PATCH')
        self.assertEqual(len(stored), len(r.publication_assets(self.m)))

    def test_github_bad_upload_digest_never_publishes_draft(self):
        api, calls, assets = self.github_api()
        assets.clear()
        class BadUpload:
            def call(inner, *args, **kwargs): return {'digest': None}
        with patch.object(r, 'API', return_value=BadUpload()), patch.dict(r.os.environ, {'GITHUB_TOKEN': 'test'}):
            with self.assertRaisesRegex(ValueError, 'digest mismatch'):
                r.publish_github(api, 'a/b', self.out, self.m, False)
        self.assertFalse(any(method == 'PATCH' for method, _, _ in calls))

    def test_receipt_preserves_first_completed_destination_after_second_failure(self):
        cfg = self.out/'targets.json'
        cfg.write_text(json.dumps({'github': {'url': 'https://github.com/a/b'},
                                   'gitlab': {'url': 'https://gitlab.example.org/a/b'}}))
        self.m['destinations'] = r.destinations(r.load_json(cfg))
        (self.out/'manifest.json').write_text(json.dumps(self.m))
        args = argparse.Namespace(output=self.out, config=cfg, target=['github', 'gitlab'], execute=True)
        def fail_on_upload(api, project, directory, manifest, dry):
            if not dry: raise ValueError('interrupted GitLab upload')
        with patch.dict(r.os.environ, {'GITHUB_TOKEN': 'test', 'GITLAB_TOKEN': 'test'}), \
             patch.object(r, 'bundle', return_value=self.m), patch.object(r, 'local_tag'), patch.object(r, 'push_tag'), \
             patch.object(r.provenance, 'tracked_inputs'), patch.object(r, 'commit', return_value='a'*40), \
             patch.object(r, 'run', return_value=''), patch.object(r, 'API'), \
             patch.object(r, 'remote_tag', return_value='a'*40), \
             patch.object(r, 'publish_github'), patch.object(r, 'publish_gitlab', side_effect=fail_on_upload):
            with self.assertRaisesRegex(ValueError, 'interrupted'):
                r.publish(args)
        receipt = r.load_json(self.out/'receipt.json')
        self.assertEqual(receipt['completed'], ['github'])
        self.assertEqual(receipt['current'], 'gitlab')

    def test_missing_token_stops_before_api_or_push(self):
        cfg = self.out/'targets.json'
        cfg.write_text(json.dumps({'github': {'url': 'https://github.com/a/b'}}))
        self.m['destinations'] = r.destinations(r.load_json(cfg))
        (self.out/'manifest.json').write_text(json.dumps(self.m))
        args = argparse.Namespace(output=self.out, config=cfg, target=['github'], execute=True)
        with patch.dict(r.os.environ, {}, clear=True), patch.object(r, 'bundle', return_value=self.m), \
             patch.object(r, 'commit', return_value='a'*40), patch.object(r, 'local_tag'), \
             patch.object(r.provenance, 'tracked_inputs'), patch.object(r, 'API') as api:
            with self.assertRaisesRegex(ValueError, 'GITHUB_TOKEN'):
                r.publish(args)
            api.assert_not_called()



if __name__ == '__main__':
    unittest.main()
