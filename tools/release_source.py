#!/usr/bin/env python3
"""Explicit source branch publication, separate from compiled release uploads."""
from pathlib import Path
import subprocess

import build_provenance as provenance
import release_credentials as credentials


def run(root, *args, capture=False):
    result = subprocess.run(args, cwd=root, check=True, text=True,
                            stdout=subprocess.PIPE if capture else None,
                            stderr=subprocess.PIPE if capture else None)
    return result.stdout.strip() if capture else None


def private_file(name):
    path = Path(name)
    return (path.parts[0] in ('work', 'outputs', 'dist', 'private-review')
            or path.parts[:2] == ('build', 'provenance')
            or any(p == '.env' or p.startswith('.env.') or p.endswith('.local.json') for p in path.parts)
            or path.name == 'release-targets.local.json'
            or (path.name == 'release-config.json' and path.parts[0] != 'examples'))


def validate_tracked(root):
    tracked = run(root, 'git', 'ls-files', '-z', capture=True).split('\0')
    for name in tracked:
        if name and private_file(name):
            raise ValueError('Private/local file is tracked; remove it from Git before publishing: '+name)
    inputs = provenance.source_inputs(root)
    provenance.tracked_inputs(inputs, root)
    for name in inputs:
        payload = (root/name).read_bytes()
        if any(secret.encode() in payload for secret in credentials.known()):
            raise ValueError('Credential found in a source input; remove it before publishing.')


def initial_checks(root, cfg, targets):
    if not (root/'.gitignore').is_file():
        raise ValueError('Review/create .gitignore before initializing source.')
    detected = subprocess.run(['git', 'rev-parse', '--show-toplevel'], cwd=root, capture_output=True, text=True)
    if detected.returncode == 0:
        if Path(detected.stdout.strip()).resolve() != root.resolve():
            raise ValueError('Refusing to initialize inside another Git checkout.')
        head = subprocess.run(['git', 'rev-parse', '--verify', 'HEAD'], cwd=root, capture_output=True)
        if head.returncode == 0:
            return False
        if run(root, 'git', 'ls-files', '--stage', capture=True):
            raise ValueError('Unfinished repository already has staged files; review its index before initialization.')
    elif (root/'.git').exists() or (root/'.git').is_symlink():
        raise ValueError('Existing .git metadata is invalid; inspect it instead of replacing it.')
    branch = cfg.get('source_branch') or 'main'
    run(root, 'git', 'check-ref-format', '--branch', branch, capture=True)
    for service, target in targets.items():
        if run(root, 'git', 'ls-remote', '--refs', target['push_url'], capture=True):
            raise ValueError(service+' already has history; clone it and import source instead. Nothing initialized.')
    for name in ('GIT_AUTHOR_IDENT', 'GIT_COMMITTER_IDENT'):
        run(root, 'git', 'var', name, capture=True)
    return True


def initialize(root, cfg, targets, message=None):
    branch = cfg.get('source_branch') or 'main'
    inputs = provenance.source_inputs(root)
    for name in inputs:
        if private_file(name):
            raise ValueError('Private file selected for source initialization: '+name)
        if any(secret.encode() in (root/name).read_bytes() for secret in credentials.known()):
            raise ValueError('Credential found in source; nothing staged.')
    if not (root/'.git').exists():
        run(root, 'git', 'init', '--initial-branch', branch)
    # Stage only the reviewed source inventory, never unrelated local files.
    run(root, 'git', 'add', '--', *sorted(inputs))
    validate_tracked(root)
    run(root, 'git', 'commit', '-m', message or 'Import NovaDock source')
    print('Created initial source commit from the selected public source inventory.')


def push(root, cfg, selected, confirmed, dry_run, clean_commit, initialize_requested=False, message=None):
    if not selected or set(selected)-{'gitlab', 'github'} or len(selected) != len(set(selected)):
        raise ValueError('Source push supports distinct gitlab and github destinations only.')
    if not dry_run and not confirmed:
        raise ValueError('Use source --dry-run first, then --confirm-push to upload committed source.')
    needs_init = initial_checks(root, cfg, selected) if initialize_requested else False
    if needs_init and dry_run:
        # Unlike publication, a dry run must not create metadata or a commit.
        print('Initial source plan: '+str(len(provenance.source_inputs(root)))+' selected files; branch '+(cfg.get('source_branch') or 'main'))
        print('Selected remotes are empty; no local Git metadata, commit or upload created.')
        return
    if needs_init:
        initialize(root, cfg, selected, message)
    sha = clean_commit()
    validate_tracked(root)
    branch = run(root, 'git', 'symbolic-ref', '--quiet', '--short', 'HEAD', capture=True)
    target_branch = cfg.get('source_branch') or branch
    run(root, 'git', 'check-ref-format', '--branch', target_branch, capture=True)
    refspec = sha+':refs/heads/'+target_branch
    for service, target in selected.items():
        print('Checking source push: '+service+' ('+branch+' -> '+target_branch+')')
        run(root, 'git', 'push', '--dry-run', '--no-follow-tags', target['push_url'], refspec)
    if dry_run:
        print('Source preflight passed; no upload performed.')
        return
    for service, target in selected.items():
        if clean_commit() != sha:
            raise ValueError('Source changed before branch publication.')
        run(root, 'git', 'push', '--no-follow-tags', target['push_url'], refspec)
        remote = run(root, 'git', 'ls-remote', '--heads', target['push_url'], 'refs/heads/'+target_branch, capture=True)
        if not remote or remote.split()[0] != sha:
            raise ValueError('Remote source branch verification failed: '+service)
        print(service+': source branch verified')
    print('Source uploaded; no compilation, release tags or binary uploads performed.')
