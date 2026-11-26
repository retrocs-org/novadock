#!/usr/bin/env python3
"""Check NovaDock's level-1/LH5 release profile before publishing an LHA.

Level-0 filenames containing 0xff are not portable to OLHa 0.4.4.
Level-1 directory extensions do use 0xff separators; those must be retained.
This checks headers only. The packager separately tests and extracts payloads.
"""
import argparse
from pathlib import Path


def component(name):
    if not name or name in (b'.', b'..') or any(c in name for c in (b'/', b'\\', b'\xff', b'\0', b':')):
        raise ValueError('invalid filename component: '+repr(name))


def check_archive(path):
    data = Path(path).read_bytes()
    offset = 0
    files = set()
    entries = set()
    while offset < len(data) and data[offset]:
        start = offset
        end = start + data[start] + 2
        if end > len(data) or end-start < 27:
            raise ValueError('truncated or undersized LHA header at '+str(start))
        header = data[start:end]
        if sum(header[2:]) & 255 != header[1]:
            raise ValueError('invalid LHA header checksum at '+str(start))
        if header[20] != 1:
            raise ValueError('NovaDock requires level-1 headers for classic extractor compatibility')
        method = header[2:7]
        if method not in (b'-lh5-', b'-lh0-', b'-lhd-'):
            raise ValueError('NovaDock requires LH5, stored files or directory entries')
        name_end = 22 + header[21]
        if name_end+5 != len(header):
            raise ValueError('invalid level-1 filename length')
        name = header[22:name_end]
        packed = int.from_bytes(header[7:11], 'little')
        entry_end = end + packed
        if entry_end > len(data):
            raise ValueError('truncated LHA entry at '+str(start))
        size = int.from_bytes(header[-2:], 'little')
        cursor = end
        directory = b''
        seen = set()
        while size:
            if size < 3 or cursor+size > entry_end:
                raise ValueError('invalid level-1 extension size')
            extension = data[cursor:cursor+size]
            kind, value = extension[0], extension[1:-2]
            if kind in (1, 2):
                if kind in seen:
                    raise ValueError('duplicate filename/directory extension')
                seen.add(kind)
                if kind == 1:
                    name = value
                else:
                    directory = value
            cursor += size
            size = int.from_bytes(extension[-2:], 'little')
        if directory:
            if not directory.endswith(b'\xff'):
                raise ValueError('level-1 directory extension must end with 0xff')
            for part in directory[:-1].split(b'\xff'):
                component(part)
        if name:
            component(name)
        elif method != b'-lhd-':
            raise ValueError('file entry has no basename')
        full_path = directory.replace(b'\xff', b'/') + name
        if method == b'-lhd-':
            if not directory or name or cursor != entry_end or int.from_bytes(header[11:15], 'little'):
                raise ValueError('invalid directory entry')
        else:
            files.add(full_path)
        if full_path in entries:
            raise ValueError('duplicate archive path: '+repr(full_path))
        entries.add(full_path)
        offset = entry_end
    if not files:
        raise ValueError('archive contains no files')
    if offset == len(data) or any(data[offset:]):
        raise ValueError('missing LHA terminator or unexpected trailing data')
    return len(files)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    args = parser.parse_args()
    try:
        count = check_archive(args.archive)
    except (OSError, ValueError) as error:
        parser.exit(1, 'LHA compatibility check failed: '+str(error)+'\n')
    print('NovaDock: verified level-1/LH5-or-stored paths for %d files.' % count)


if __name__ == '__main__':
    main()
