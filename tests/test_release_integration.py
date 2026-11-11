#!/usr/bin/env python3
"""Shared configuration, secret providers and source pushes; all tests are local."""
import argparse
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('release', ROOT/'tools/release.py')
r = importlib.util.module_from_spec(spec); spec.loader.exec_module(r)
c = r.credentials
s = r.release_source


class CredentialTests(unittest.TestCase):
    def setUp(self):
        cache = patch.object(c, 'SECRETS', set()); cache.start(); self.addCleanup(cache.stop)

    def test_environment_reference_and_build_isolation(self):
        cfg = {'credentials': {'github': {'provider': 'env', 'variable': 'CUSTOM_RELEASE_TOKEN'}},
               'env': {'PHP': '/configured/php'}}
        with patch.dict(os.environ, {'CUSTOM_RELEASE_TOKEN': 'sensitive-value', 'GITLAB_TOKEN': 'other-value',
                                    'OP_SESSION_example': 'session', 'OP_SERVICE_ACCOUNT_TOKEN': 'service'}):
            self.assertEqual(c.token('github', cfg), 'sensitive-value')
            env = c.build_environment(cfg)
            for key in ('CUSTOM_RELEASE_TOKEN', 'GITLAB_TOKEN', 'OP_SESSION_example', 'OP_SERVICE_ACCOUNT_TOKEN'):
                self.assertNotIn(key, env)
            self.assertEqual(env['PHP'], '/configured/php')
            self.assertNotIn('sensitive-value', c.redact('sensitive-value'))

    def test_secret_reference_stays_out_of_environment_and_output(self):
        cfg = {'credentials': {'github': {'provider': '1password', 'reference': 'op://vault/item/token'}}}
        response = subprocess.CompletedProcess([], 0, 'sensitive-value', '')
        with patch.object(c.subprocess, 'run', return_value=response) as command, contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(c.token('github', cfg), 'sensitive-value')
        self.assertEqual(command.call_args.args[0], ['op', 'read', '--no-newline', 'op://vault/item/token'])
        self.assertEqual(output.getvalue(), '')
        self.assertNotEqual(os.environ.get('GITHUB_TOKEN'), 'sensitive-value')

    def test_1password_field_lookup_matches_exact_label(self):
        cfg = {'credentials': {'gitlab': {'provider': '1password', 'vault': 'Employee', 'item': 'server', 'field': 'org/project-rw', 'account': 'account-id'}}}
        response = subprocess.CompletedProcess([], 0, json.dumps([{'label': 'org/project-rw', 'value': 'sensitive-value'}]), '')
        with patch.object(c.subprocess, 'run', return_value=response) as command:
            self.assertEqual(c.token('gitlab', cfg), 'sensitive-value')
        self.assertIn('label=org/project-rw', command.call_args.args[0])
        self.assertEqual(command.call_args.args[0][-2:], ['--account', 'account-id'])

    def test_1password_failure_and_ambiguous_fields_never_expose_output(self):
        cfg = {'credentials': {'gitlab': {'provider': '1password', 'vault': 'Employee', 'item': 'server', 'field': 'token'}}}
        for response in (subprocess.CompletedProcess([], 1, 'secret-output', 'private-error'),
                         subprocess.CompletedProcess([], 0, json.dumps([{'label': 'token', 'value': 'first'}, {'label': 'token', 'value': 'second'}]), '')):
            with patch.object(c.subprocess, 'run', return_value=response):
                with self.assertRaises(ValueError) as error:
                    c.token('gitlab', cfg)
                for private in ('secret-output', 'private-error', 'first', 'second'):
                    self.assertNotIn(private, str(error.exception))

    def test_invalid_providers_and_unresolved_tokens_fail_offline(self):
        for cfg in ({'github': {'provider': 'file', 'value': 'secret'}},
                    {'github': {'provider': '1password', 'reference': 'https://other'}},
                    {'github': {'provider': 'env', 'variable': 'INVALID-NAME'}}):
            with self.assertRaises(ValueError): c.validate(cfg)
        with patch.dict(os.environ, {'GITHUB_TOKEN': 'op://unresolved/value'}):
            with self.assertRaises(ValueError): c.token('github', {})

    def test_shared_default_config_and_explicit_override(self):
        with patch.object(r.Path, 'home', return_value=Path('/configured/home')), patch.dict(os.environ, {}, clear=True):
            self.assertEqual(r.default_config(), Path('/configured/home/Development/tools/git-release/projects/novadock.json'))
        with patch.dict(os.environ, {'NOVADOCK_RELEASE_CONFIG': '/custom/novadock.json'}):
            self.assertEqual(r.default_config(), Path('/custom/novadock.json'))

    def test_plan_never_reads_1password_and_uses_configured_python(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'config.json'
            path.write_text(json.dumps({'env': {'NOVADOCK_RELEASE_PYTHON': '/configured/python'},
                                       'credentials': {'github': {'provider': '1password', 'reference': 'op://vault/item/token'}}}))
            args = argparse.Namespace(config=path, output=Path(tmp)/'release')
            with patch.object(c, 'token', side_effect=AssertionError('secret lookup')), patch.object(r, 'run', side_effect=AssertionError('build')), contextlib.redirect_stdout(io.StringIO()) as output:
                r.plan(args)
            self.assertIn('/configured/python tools/package_release.py', output.getvalue())

    def test_release_requires_confirmation_before_build_or_prepare(self):
        args = argparse.Namespace(confirm_publish=False, execute=False)
        with patch.object(r, 'prepare', side_effect=AssertionError('build')):
            with self.assertRaisesRegex(ValueError, 'confirm-publish'): r.release(args)


class SourceTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)/'source'; self.root.mkdir()
        for name in ('README.md', 'VERSION', '.gitignore'):
            (self.root/name).write_text('source input')
        self.inputs = {name: 'hash' for name in ('README.md', 'VERSION', '.gitignore')}
        patcher = patch.object(s.provenance, 'source_inputs', return_value=self.inputs)
        patcher.start(); self.addCleanup(patcher.stop)
        cache = patch.object(c, 'SECRETS', set()); cache.start(); self.addCleanup(cache.stop)
        identity = patch.dict(os.environ, {'GIT_AUTHOR_NAME': 'Release Test', 'GIT_AUTHOR_EMAIL': 'test@example.org',
                                          'GIT_COMMITTER_NAME': 'Release Test', 'GIT_COMMITTER_EMAIL': 'test@example.org'})
        identity.start(); self.addCleanup(identity.stop)
        self.targets = {}
        for name in ('github', 'gitlab'):
            remote = Path(self.tmp.name)/name
            subprocess.run(['git', 'init', '--bare', '-q', str(remote)], check=True)
            self.targets[name] = {'push_url': str(remote)}

    def git(self, *args):
        return subprocess.check_output(['git', *args], cwd=self.root, stderr=subprocess.DEVNULL).decode().strip()

    def clean(self):
        if self.git('status', '--porcelain', '--untracked-files=all'):
            raise ValueError('Dirty test source')
        return self.git('rev-parse', 'HEAD')

    def test_init_dry_run_creates_neither_metadata_nor_remote_refs(self):
        s.push(self.root, {}, self.targets, False, True, self.clean, True)
        self.assertFalse((self.root/'.git').exists())
        for item in self.targets.values():
            self.assertEqual(subprocess.check_output(['git', 'ls-remote', item['push_url']]), b'')

    def test_source_initialization_pushes_only_selected_branch_and_files(self):
        # A private, ignored file is never staged by source initialization.
        (self.root/'.gitignore').write_text('private.txt\n')
        (self.root/'private.txt').write_text('local data')
        s.push(self.root, {'source_branch': 'main'}, self.targets, True, False, self.clean, True, 'Initial source')
        sha = self.clean()
        self.assertNotIn('private.txt', self.git('ls-files').splitlines())
        for item in self.targets.values():
            refs = subprocess.check_output(['git', 'ls-remote', item['push_url']]).decode()
            self.assertIn(sha+'\trefs/heads/main', refs)
            self.assertNotIn('refs/tags/', refs)

    def test_source_requires_confirmation_without_mutating_local_git(self):
        with self.assertRaisesRegex(ValueError, 'confirm-push'):
            s.push(self.root, {}, self.targets, False, False, self.clean, True)
        self.assertFalse((self.root/'.git').exists())

    def test_nonempty_remote_blocks_initialization(self):
        self.git('init', '-q'); self.git('add', '.'); self.git('commit', '-qm', 'initial')
        self.git('push', self.targets['github']['push_url'], 'HEAD:refs/heads/main')
        fresh = Path(self.tmp.name)/'fresh'; fresh.mkdir(); (fresh/'.gitignore').write_text('')
        with self.assertRaisesRegex(ValueError, 'already has history'):
            s.initial_checks(fresh, {}, self.targets)
        self.assertFalse((fresh/'.git').exists())

    def test_tracked_private_configuration_blocks_source_push(self):
        self.git('init', '-q'); self.git('add', '.'); self.git('commit', '-qm', 'initial')
        (self.root/'work').mkdir(); (self.root/'work/release-config.json').write_text('{}')
        self.git('add', '.'); self.git('commit', '-qm', 'private fixture')
        with self.assertRaisesRegex(ValueError, 'Private/local file is tracked'):
            s.push(self.root, {}, self.targets, True, False, self.clean)
        for item in self.targets.values():
            self.assertEqual(subprocess.check_output(['git', 'ls-remote', item['push_url']]), b'')

    def test_second_preflight_failure_prevents_first_real_branch_push(self):
        self.git('init', '-q'); self.git('add', '.'); self.git('commit', '-qm', 'initial')
        original = s.run
        def fail(root, *args, **kwargs):
            if args[:3] == ('git', 'push', '--dry-run') and self.targets['gitlab']['push_url'] in args:
                raise ValueError('Second destination rejects source')
            return original(root, *args, **kwargs)
        with patch.object(s, 'run', side_effect=fail):
            with self.assertRaisesRegex(ValueError, 'Second destination'):
                s.push(self.root, {}, self.targets, True, False, self.clean)
        for item in self.targets.values():
            self.assertEqual(subprocess.check_output(['git', 'ls-remote', item['push_url']]), b'')

    def test_unfinished_index_is_not_silently_committed(self):
        self.git('init', '-q')
        (self.root/'unrelated.txt').write_text('not selected')
        self.git('add', 'unrelated.txt')
        with self.assertRaisesRegex(ValueError, 'already has staged files'):
            s.push(self.root, {}, self.targets, True, False, self.clean, True)
        self.assertEqual(self.git('ls-files'), 'unrelated.txt')
        for item in self.targets.values():
            self.assertEqual(subprocess.check_output(['git', 'ls-remote', item['push_url']]), b'')

    def test_source_push_is_pinned_even_if_branch_moves_after_check(self):
        self.git('init', '--initial-branch=main', '-q'); self.git('add', '.'); self.git('commit', '-qm', 'initial')
        sha = self.clean()
        tree = self.git('rev-parse', 'HEAD^{tree}')
        later = subprocess.check_output(['git', 'commit-tree', tree, '-p', sha], cwd=self.root, input=b'later commit\n').decode().strip()
        original = s.run
        def move_branch(root, *args, **kwargs):
            if args[:2] == ('git', 'push') and '--dry-run' not in args:
                self.git('update-ref', 'refs/heads/main', later)
            return original(root, *args, **kwargs)
        selected = {'github': self.targets['github']}
        with patch.object(s, 'run', side_effect=move_branch):
            s.push(self.root, {}, selected, True, False, self.clean)
        refs = subprocess.check_output(['git', 'ls-remote', self.targets['github']['push_url'], 'refs/heads/main']).decode()
        self.assertEqual(refs.split()[0], sha)
        self.assertNotEqual(sha, self.clean())


if __name__ == '__main__': unittest.main()
