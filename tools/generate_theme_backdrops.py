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
    "Copper", "Ocean", "Emerald", "NeXTSTEP", "BeOS", "Nord", "Vapor", "Christmas", "MacModern", "MacGlass",
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
        "edge": rgb(values["ACCENT"]),
        "style": values.get("DOCK_STYLE", "FLAT"),
    }


def blank(width, height=SKIN_HEIGHT):
    return [[KEY for _ in range(width)] for _ in range(height)]


def blend(base, accent, percent):
    return tuple((a * (100 - percent) + b * percent + 50) // 100
                 for a, b in zip(base, accent))


def shelf_color(colors, y):
    base = colors["base"]
    if colors.get("style") == "GLASS":
        # Eight restrained reflection bands keep the ILBM within 16 colours.
        band = min(7, y * 8 // SKIN_HEIGHT)
        return blend(base, (255,255,255), (68,52,34,18,8,16,26,46)[band])
    if y in (0, SKIN_HEIGHT - 1):
        return blend(base, colors["edge"], 30)
    return base


def centre(colors):
    # Every row is continuous, so tile boundaries cannot produce a pattern.
    width = 64
    return [[shelf_color(colors, y)] * width for y in range(SKIN_HEIGHT)]


def cap(colors, right=False):
    width = 32 if colors.get("style")=="GLASS" else 20
    image = blank(width)
    last = SKIN_HEIGHT - 1
    curve = (7, 5, 4, 3, 2, 1, 1, 0)
    insets = [curve[min(y, last - y, len(curve) - 1)] for y in range(SKIN_HEIGHT)]
    if colors.get("style")=="GLASS":insets=[20*(last-y)//last for y in range(SKIN_HEIGHT)]
    edge = shelf_color(colors, 0)
    for y, inset in enumerate(insets):
        image[y][inset:] = [shelf_color(colors, y)] * (width - inset)
        if 0 < y < last:
            # Include every opaque pixel bordering a transparent one.
            # The same contour colour runs around all four corners.
            end = max(inset + 1, insets[y - 1], insets[y + 1])
            image[y][inset:end] = [edge] * (end - inset)
        if right:
            image[y].reverse()
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


def ilbm_chunk(name, data):
    return name + struct.pack(">I", len(data)) + data + (b"\0" if len(data) % 2 else b"")


def write_ilbm(output, pixels):
    """Encode the bounded four-plane indexed brushes used by the dock."""
    width, height = len(pixels[0]), len(pixels)
    palette = sorted({color for row in pixels for color in row})
    if len(palette) > 16 or not (1 <= width <= 96 and 1 <= height <= 96):
        raise ValueError("Artwork exceeds the dock ILBM limits")
    indices = {color: index for index, color in enumerate(palette)}
    planes = max(1, (len(palette) - 1).bit_length())
    row_bytes = 2 * ((width + 15) // 16)
    body = bytearray()
    for row in pixels:
        for plane in range(planes):
            planar_row = bytearray(row_bytes)
            for x, color in enumerate(row):
                if indices[color] & (1 << plane):
                    planar_row[x // 8] |= 0x80 >> (x % 8)
            body.extend(planar_row)
    header = struct.pack(">HHhhBBBBHBBhh", width, height, 0, 0, planes,
                         2 if KEY in indices else 0, 0, 0, indices.get(KEY, 0),
                         1, 1, width, height)
    palette_bytes = bytes(channel for color in palette for channel in color)
    data = (b"ILBM" + ilbm_chunk(b"BMHD", header) + ilbm_chunk(b"CMAP", palette_bytes)
            + ilbm_chunk(b"BODY", bytes(body)))
    output.write_bytes(b"FORM" + struct.pack(">I", len(data)) + data)


def build_part(ppmtoilbm, output, pixels, temporary):
    if ppmtoilbm is None:
        write_ilbm(output, pixels)
        return
    ppm = temporary / (output.stem + ".ppm")
    write_ppm(ppm, pixels)
    with output.open("wb") as target:
        subprocess.run(
            (ppmtoilbm, "-maxplanes", "4", "-compress", str(ppm)),
            check=True, stdout=target)
    if any(KEY in row for row in pixels):
        mark_transparent(output)


def build_skin(ppmtoilbm, output_dir, source, colors, temporary):
    output_dir.mkdir(parents=True, exist_ok=True)
    parts = {"Backdrop.iff": centre(colors), "BackdropLeft.iff": cap(colors),
             "BackdropRight.iff": cap(colors, right=True)}
    for name, pixels in parts.items():
        build_part(ppmtoilbm, output_dir / name, pixels, temporary)
    left, middle, right = (parts[name] for name in
                           ("BackdropLeft.iff", "Backdrop.iff", "BackdropRight.iff"))
    write_png(source, [a + b * 6 + c for a, b, c in zip(left, middle, right)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--ppmtoilbm", help="Optional external encoder for ByteRun1 compression")
    parser.add_argument("--theme", action="append", choices=THEMES,
                        help="Generate only the selected theme, repeatable")
    args = parser.parse_args()
    root = args.root.resolve()
    with tempfile.TemporaryDirectory(prefix="novadock-skins-") as name:
        temporary = Path(name)
        for theme in args.theme or THEMES:
            directory = root / "Themes" / theme
            build_skin(args.ppmtoilbm, directory / "Images",
                       directory / "Images" / "Source.png",
                       read_theme(directory / "theme.cfg"), temporary)
        if args.theme:
            return
        samples = {
            "ClassicGold": {
                "base": (170, 122, 38), "edge": (242, 194, 76),
            },
            "ClassicBlue": {
                "base": (35, 78, 130), "edge": (96, 181, 231),
            },
        }
        for name, colors in samples.items():
            build_skin(args.ppmtoilbm, root / "Backgrounds",
                       root / "Backgrounds" / "Source" / (name + ".png"),
                       colors, temporary)


if __name__ == "__main__":
    main()
