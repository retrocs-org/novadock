#!/usr/bin/env python3
"""Keep every Makefile target's header prerequisites complete.

vbcc has no -MMD, so the header lists in Makefile and Makefile.cross are
written by hand, and a missing one means an incremental build keeps a
stale program after a header change (for example the dock and a plugin
disagreeing about nd_protocol.h).  This follows the #include "..." tree of
each target's sources and compares it with the prerequisites.

    python3 tools/make_deps.py          check both Makefiles (host tests)
    python3 tools/make_deps.py --fix    append the missing headers
"""
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
MAKEFILES = ('Makefile', 'Makefile.cross')
# The -I directories from CFLAGS in both Makefiles.
INCLUDE_DIRS = ('include', 'plugin_src/common', 'plugin_src/archive')
# Variables Makefile.cross substitutes when compat/ (never published) is absent.
VARIABLES = {
    'ND_DOCK_MAIN': 'src/novadock.c src/dock_save.c src/save_policy.c src/indicator.c',
    'ND_PREFS_MAIN': 'prefs/novadockprefs.c',
}
TARGET = re.compile(r'^([A-Za-z][A-Za-z0-9_/]*):(?!=)(.*)$')
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)


def headers_of(path, seen):
    full = os.path.join(ROOT, path)
    if path in seen or not os.path.isfile(full):
        return
    seen.add(path)
    with open(full, encoding='latin-1') as f:
        text = f.read()
    for name in INCLUDE.findall(text):
        for base in (os.path.dirname(path),) + INCLUDE_DIRS:
            candidate = os.path.normpath(os.path.join(base, name))
            if os.path.isfile(os.path.join(ROOT, candidate)):
                headers_of(candidate, seen)
                break


def missing_headers(prerequisites):
    words = prerequisites.split()
    expanded = []
    for word in words:
        m = re.fullmatch(r'\$\((\w+)\)', word)
        expanded += VARIABLES[m.group(1)].split() if m and m.group(1) in VARIABLES else [word]
    seen = set()
    for source in expanded:
        if source.endswith(('.c', '.asm')):
            headers_of(source, seen)
    listed = set(expanded)
    return sorted(h for h in seen if h.endswith('.h') and h not in listed)


def process(makefile, fix):
    path = os.path.join(ROOT, makefile)
    with open(path) as f:
        lines = f.read().split('\n')
    problems = []
    for i, line in enumerate(lines):
        m = TARGET.match(line)
        if not m or m.group(1) in ('all', 'clean'):
            continue
        missing = missing_headers(m.group(2))
        if missing:
            problems.append((m.group(1), missing))
            if fix:
                lines[i] = line.rstrip() + ' ' + ' '.join(missing)
    if fix and problems:
        with open(path, 'w') as f:
            f.write('\n'.join(lines))
    return problems


def main():
    fix = '--fix' in sys.argv[1:]
    failed = False
    for makefile in MAKEFILES:
        for target, missing in process(makefile, fix):
            if not fix:
                failed = True
                print(f'{makefile}: {target} does not list {" ".join(missing)}')
    if failed:
        print('Run: python3 tools/make_deps.py --fix')
        return 1
    if not fix:
        print('Makefile and Makefile.cross list every header each program includes.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
