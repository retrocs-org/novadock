#!/usr/bin/env python3
"""Source selection and native Amiga build provenance for release automation."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    if path.is_symlink() or not path.is_file():
        raise ValueError('Expected a regular, non-symlink file: '+str(path))
    with path.open('rb') as stream:
        result = hashlib.sha256()
        for chunk in iter(lambda: stream.read(1024*1024), b''):
            result.update(chunk)
    return result.hexdigest()


def source_inputs(root=ROOT):
    # Reuse the explicit source archive selection; downloads and native binaries
    # are outputs, never inputs to their own fingerprint.
    import package_release as package
    previous = package.ROOT
    package.ROOT = root
    try:
        files = list(package.source_files())
    finally:
        package.ROOT = previous
    files += [p for p in (root/'public-web').rglob('*')
              if p.is_file() and 'downloads' not in p.relative_to(root/'public-web').parts
              and package.kept(p.relative_to(root))]
    result = {}
    for path in sorted(set(files)):
        name = path.relative_to(root).as_posix()
        if name.startswith('build/provenance/'):
            continue
        current = root
        for part in path.relative_to(root).parts:
            current = current/part
            if current.is_symlink():
                raise ValueError('Symlinked source input: '+name)
        result[name] = digest(path)
    return result


def fingerprint(inputs):
    return hashlib.sha256(json.dumps(inputs, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def native_programs(root=ROOT):
    match = re.search(r'^all:\s*(.+)$', (root/'Makefile').read_text(), re.M)
    if not match:
        raise ValueError('Makefile has no native target inventory.')
    return match.group(1).split()


def binaries(root=ROOT):
    result = {}
    for name in native_programs(root):
        path = root/name
        if path.read_bytes()[:4] != b'\0\0\x03\xf3':
            raise ValueError('Not an Amiga Hunk executable: '+name)
        result[name] = digest(path)
    return result


def tracked_inputs(inputs, root=ROOT):
    output = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root)
    tracked = set(output.decode().split('\0'))
    missing = set(inputs) - tracked
    if missing:
        raise ValueError('Release inputs must be tracked: '+', '.join(sorted(missing)))
