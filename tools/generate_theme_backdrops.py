#!/usr/bin/env python3
"""Generate NovaDock's small palette-mapped, transparent dock skins.

The artwork is intentionally deterministic: a quiet repeatable centre and
two compact rounded caps.  Magenta is used only as the ILBM transparent key.
"""
import argparse
import struct
import subprocess
import tempfile
import zlib
from pathlib import Path


KEY = (255, 0, 255)
SKIN_HEIGHT = 82
THEMES = (
    "Mocha", "Light", "Workbench", "Graphite", "HighContrast",
    "Copper", "Ocean", "Emerald", "NeXTSTEP", "BeOS", "Nord", "Vapor",
)


def rgb(value):
    parts = tuple(int(part) for part in value.split(","))
    if len(parts) != 3 or any(part < 0 or part > 255 for part in parts):
        raise ValueError("invalid RGB value: " + value)
    return parts


def read_theme(path):
    values = {}
    for raw in path.read_text(encoding="ascii").splitlines():
        line = raw.strip()
        if line and not line.startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
    return {
        "base": rgb(values["TILE"]),
        "pattern": rgb(values["HOVER"]),
        "edge": rgb(values["ACCENT"]),
        "shadow": rgb(values["BAR"]),
        "highlight": rgb(values["SUBTLE"]),
    }


def blank(width, height=SKIN_HEIGHT):
    return [[KEY for _ in range(width)] for _ in range(height)]


def centre(colors):
    image = blank(64)
    bottom = len(image) - 3
    for y in range(2, bottom + 1):
        for x in range(64):
            image[y][x] = colors["base"]
    for x in range(64):
        image[2][x] = colors["edge"]
        image[3][x] = colors["highlight"]
        image[bottom - 1][x] = colors["shadow"]
        image[bottom][x] = colors["edge"]
    # Broad, low-contrast bands read cleanly at native and doubled display sizes.
    # Single-pixel stipple looked like conversion noise after palette mapping.
    for y in (20, 40, 60):
        for x in range(0, 64, 16):
            image[y][x:x + 8] = [colors["pattern"]] * 8
    return image


def cap(colors, right=False):
    width = 20
    image = blank(width)
    last = len(image) - 3
    # One-pixel radius steps make a purposeful native-resolution curve.  The
    # old 12/7/3-pixel jumps produced visibly blocky corners on a
    # doubled Workbench display.
    curve = (7, 5, 4, 3, 2, 1, 1, 0)
    for y in range(2, last + 1):
        distance = min(y - 2, last - y)
        inset = curve[distance] if distance < len(curve) else 0
        for x in range(inset, width):
            target = width - 1 - x if right else x
            image[y][target] = colors["base"]
        boundary = width - 1 - inset if right else inset
        image[y][boundary] = colors["edge"]
        if distance == 1:
            for x in range(inset, width):
                target = width - 1 - x if right else x
                image[y][target] = colors["highlight"]
        elif y == last - 1:
            for x in range(inset, width):
                target = width - 1 - x if right else x
                image[y][target] = colors["shadow"]
    return image


def write_ppm(path, pixels):
    height = len(pixels)
    width = len(pixels[0])
    with path.open("wb") as output:
        output.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
        output.write(bytes(component for row in pixels for pixel in row for component in pixel))


def png_chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(
        ">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def write_png(path, pixels):
    height = len(pixels)
    width = len(pixels[0])
    raw = bytearray()
    for row in pixels:
        raw.append(0)
        for red, green, blue in row:
            if (red, green, blue) == KEY:
                raw.extend((0, 0, 0, 0))
            else:
                raw.extend((red, green, blue, 255))
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", header) +
        png_chunk(b"IDAT", zlib.compress(bytes(raw), 9)) +
        png_chunk(b"IEND", b""))


def iff_chunks(data):
    if data[:4] != b"FORM" or data[8:12] != b"ILBM":
        raise ValueError("ppmtoilbm did not create an ILBM")
    position = 12
    limit = min(len(data), 8 + struct.unpack(">I", data[4:8])[0])
    while position + 8 <= limit:
        size = struct.unpack(">I", data[position + 4:position + 8])[0]
        yield data[position:position + 4], position + 8, size
        position += 8 + size + (size & 1)


def mark_transparent(path):
    data = bytearray(path.read_bytes())
    bmhd = None
    transparent = None
    for kind, start, size in iff_chunks(data):
        if kind == b"BMHD":
            bmhd = start
        elif kind == b"CMAP":
            for index in range(size // 3):
                if tuple(data[start + index * 3:start + index * 3 + 3]) == KEY:
                    transparent = index
                    break
    if bmhd is None or transparent is None:
        raise ValueError("transparent key is missing from " + str(path))
    data[bmhd + 9] = 2
    data[bmhd + 12:bmhd + 14] = struct.pack(">H", transparent)
    path.write_bytes(data)


def build_part(ppmtoilbm, output, pixels, temporary):
    ppm = temporary / (output.stem + ".ppm")
    write_ppm(ppm, pixels)
    with output.open("wb") as target:
        subprocess.run(
            (ppmtoilbm, "-maxplanes", "4", "-compress", str(ppm)),
            check=True, stdout=target)
    mark_transparent(output)


def build_skin(ppmtoilbm, output_dir, source, colors, temporary):
    output_dir.mkdir(parents=True, exist_ok=True)
    left = cap(colors)
    middle = centre(colors)
    right = cap(colors, right=True)
    build_part(ppmtoilbm, output_dir / "Backdrop.iff", middle, temporary)
    build_part(ppmtoilbm, output_dir / "BackdropLeft.iff", left, temporary)
    build_part(ppmtoilbm, output_dir / "BackdropRight.iff", right, temporary)
    write_png(source, [a + b * 6 + c for a, b, c in zip(left, middle, right)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--ppmtoilbm", default="ppmtoilbm")
    args = parser.parse_args()
    root = args.root.resolve()
    with tempfile.TemporaryDirectory(prefix="novadock-skins-") as name:
        temporary = Path(name)
        for theme in THEMES:
            directory = root / "Themes" / theme
            build_skin(args.ppmtoilbm, directory / "Images",
                       directory / "Images" / "Source.png",
                       read_theme(directory / "theme.cfg"), temporary)
        samples = {
            "ClassicGold": {
                "base": (170, 122, 38), "pattern": (145, 94, 24),
                "edge": (242, 194, 76), "shadow": (92, 54, 18),
                "highlight": (214, 162, 62),
            },
            "ClassicBlue": {
                "base": (35, 78, 130), "pattern": (25, 59, 105),
                "edge": (96, 181, 231), "shadow": (15, 36, 70),
                "highlight": (73, 135, 181),
            },
        }
        for name, colors in samples.items():
            build_skin(args.ppmtoilbm, root / "Backgrounds",
                       root / "Backgrounds" / "Source" / (name + ".png"),
                       colors, temporary)


if __name__ == "__main__":
    main()
