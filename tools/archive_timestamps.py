#!/usr/bin/env python3
"""Use the packaging clock for staged files and DOS-resolution archive dates."""
import argparse
import os
from pathlib import Path
import time


def current_epoch():
    # ZIP and level-1 LHA dates have two-second resolution.
    epoch = int(time.time()) // 2 * 2
    zip_datetime(epoch)  # Reject clocks outside the archive formats' range.
    return epoch


def zip_datetime(epoch):
    value = time.localtime(epoch)
    if not 1980 <= value.tm_year <= 2107:
        raise ValueError('System clock is outside the supported archive date range.')
    return tuple(value[:5]) + (value.tm_sec // 2 * 2,)


def paths_in(roots):
    paths = set()
    for root in map(Path, roots):
        if root.is_symlink() or not root.exists():
            raise ValueError('Expected a regular staging path: '+str(root))
        paths.add(root)
        if root.is_dir():
            paths.update(root.rglob('*'))
    for path in paths:
        if path.is_symlink() or not (path.is_file() or path.is_dir()):
            raise ValueError('Unsupported staging entry: '+str(path))
    return sorted(paths, key=lambda p: (len(p.parts), str(p)), reverse=True)


def stamp_tree(roots, epoch=None):
    epoch = current_epoch() if epoch is None else epoch
    zip_datetime(epoch)
    for path in paths_in(roots):
        os.utime(path, (epoch, epoch))
    return epoch


def check_extracted(roots, epoch):
    # Extractors may create children after restoring a directory's own date;
    # check file dates, while staging assigns dates to directories as well.
    for path in paths_in(roots):
        if path.is_file() and abs(path.stat().st_mtime-epoch) > 2:
            raise ValueError('Extracted archive date differs from packaging time: '+str(path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', type=int, metavar='EPOCH', help='Check extracted file timestamps')
    parser.add_argument('paths', type=Path, nargs='+')
    args = parser.parse_args()
    if args.check is not None:
        check_extracted(args.paths, args.check)
        print('Extracted file dates match the packaging clock.')
    else:
        print(stamp_tree(args.paths))


if __name__ == '__main__':
    main()
