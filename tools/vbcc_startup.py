#!/usr/bin/env python3
"""Patch vbcc's m68k-amigaos startup.o exit race and check linked programs.

vbcc 0.9's ___exit restores the entry stack pointer and only then reads the
two words its startup pushed there (the Workbench message and the argument
block):

    move.l  __savedsp(a4),sp
    subq.l  #8,sp
    move.l  (sp)+,a2
    move.l  (sp)+,a5
    move.l  4(a5),d1
    beq.s   .noclose

Between the first two instructions both words lie below the stack pointer.
Exec saves a pre-empted task's registers on its user stack, so a task switch
at that boundary overwrites them with the task's a5/a6. Exit then closes a
garbage handle and frees SysBase (alerts 0100000F, then 81000005). NovaDock's
helpers and plugins hit this when many of them exit together.

The patch keeps every later instruction, relocation and branch target in
place and never lets the words fall below the stack pointer:

    movea.l __savedsp(a4),a5
    lea     -8(a5),sp
    move.l  (sp)+,a2
    move.l  (sp)+,a5
    move.l  4(a5),d1

The dropped "beq.s .noclose" only skipped Close(0), which dos.library V36+
treats as a no-op (checked on AmigaOS 3.2.3). NovaDock requires OS 3.0.

  vbcc_startup.py patch SOURCE DEST    write a fixed copy of startup.o
  vbcc_startup.py check PROGRAM...     fail if a program still has the race
"""
import argparse
import re
import sys
from pathlib import Path

# move.l d16(a4),sp ; subq.l #8,sp ; move.l (sp)+,a2 ; move.l (sp)+,a5 ;
# move.l 4(a5),d1 ; beq.s +8
UNSAFE_OBJECT = re.compile(rb'\x2e\x6c(..)\x51\x8f\x24\x5f\x2a\x5f\x22\x2d\x00\x04\x67\x08', re.S)
# The same exit sequence as it appears in a linked program.
UNSAFE_PROGRAM = re.compile(rb'\x2e\x6c..\x51\x8f\x24\x5f\x2a\x5f', re.S)
FIXED_PROGRAM = re.compile(rb'\x2a\x6c..\x4f\xed\xff\xf8\x24\x5f\x2a\x5f\x22\x2d\x00\x04', re.S)


def patched(data):
    matches = list(UNSAFE_OBJECT.finditer(data))
    if len(matches) != 1:
        if not matches and FIXED_PROGRAM.search(data):
            return data
        raise ValueError('expected exactly one vbcc ___exit sequence, found %d' % len(matches))
    match = matches[0]
    displacement = match.group(1)  # relocated small-data offset; kept in place
    fixed = (b'\x2a\x6c' + displacement +   # movea.l __savedsp(a4),a5
             b'\x4f\xed\xff\xf8' +           # lea -8(a5),sp
             b'\x24\x5f\x2a\x5f' +           # move.l (sp)+,a2 ; move.l (sp)+,a5
             b'\x22\x2d\x00\x04')            # move.l 4(a5),d1
    assert len(fixed) == match.end() - match.start()
    return data[:match.start()] + fixed + data[match.end():]


def check(paths):
    bad = []
    for path in paths:
        path = Path(path)
        if not path.is_file():
            continue
        data = path.read_bytes()
        if data[:4] != b'\x00\x00\x03\xf3':
            continue
        if UNSAFE_PROGRAM.search(data):
            bad.append(str(path))
    return bad


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    p = sub.add_parser('patch')
    p.add_argument('source', type=Path)
    p.add_argument('dest', type=Path)
    c = sub.add_parser('check')
    c.add_argument('programs', type=Path, nargs='+')
    args = parser.parse_args()
    if args.command == 'patch':
        data = patched(args.source.read_bytes())
        args.dest.parent.mkdir(parents=True, exist_ok=True)
        temporary = args.dest.with_name(args.dest.name + '.tmp')
        temporary.write_bytes(data)
        temporary.replace(args.dest)
        return 0
    bad = check(args.programs)
    for path in bad:
        print('unsafe vbcc exit sequence (relink with tools/vbcc_startup.py patch):', path, file=sys.stderr)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
