#!/usr/bin/env python3
"""Mark one exact ILBM palette colour as transparent.

ppmtoilbm writes compact palette ILBMs but has no transparent-colour option.
This maintainer helper updates the BMHD masking and transparentColor fields
after conversion. It does not alter the palette or BODY data.
"""
import argparse
import struct
from pathlib import Path


def chunks(data):
    if data[:4] != b"FORM" or data[8:12] != b"ILBM":
        raise ValueError("not an IFF ILBM file")
    limit = min(len(data), 8 + struct.unpack(">I", data[4:8])[0])
    pos = 12
    while pos + 8 <= limit:
        size = struct.unpack(">I", data[pos + 4:pos + 8])[0]
        yield data[pos:pos + 4], pos + 8, size
        pos += 8 + size + (size & 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path)
    parser.add_argument("red", type=int)
    parser.add_argument("green", type=int)
    parser.add_argument("blue", type=int)
    args = parser.parse_args()
    wanted = bytes((args.red, args.green, args.blue))
    data = bytearray(args.path.read_bytes())
    bmhd = None
    transparent = None
    for kind, start, size in chunks(data):
        if kind == b"BMHD":
            if size < 20:
                raise ValueError("short BMHD chunk")
            bmhd = start
        elif kind == b"CMAP":
            for index in range(size // 3):
                if data[start + index * 3:start + index * 3 + 3] == wanted:
                    transparent = index
                    break
    if bmhd is None or transparent is None:
        raise ValueError("BMHD or requested palette colour not found")
    data[bmhd + 9] = 2
    data[bmhd + 12:bmhd + 14] = struct.pack(">H", transparent)
    args.path.write_bytes(data)


if __name__ == "__main__":
    main()
