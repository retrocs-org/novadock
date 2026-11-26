#!/usr/bin/env python3
"""Create NovaDock's "Tilt" icon set: 32-pixel ILBM and PNG artwork.

Original artwork in a classic late-90s desktop style: objects seen from
slightly above in a tilted three-quarter view, a bold dark outline, a light
top face and darker sides, and saturated colours.  Every icon is drawn
here from simple shapes (no external images), shares one 16-colour palette
so the Preferences picker needs few pens, and uses colour 0 as the
transparent colour like the rest of the collection.
  python3 tools/generate_tilt_icons.py [--preview sheet.png]
"""
import argparse
import math
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]

# 0 transparent, 1 outline, then light/mid/dark ramps.
PALETTE = (
    (255, 0, 255),    # 0 transparent
    (24, 24, 32),     # 1 outline
    (255, 255, 255),  # 2 white
    (208, 208, 218),  # 3 light grey
    (150, 152, 166),  # 4 grey
    (88, 90, 106),    # 5 dark grey
    (255, 222, 80),   # 6 yellow
    (234, 150, 34),   # 7 orange
    (150, 86, 34),    # 8 brown
    (230, 62, 48),    # 9 red
    (242, 196, 150),  # 10 skin / tan
    (138, 186, 255),  # 11 light blue
    (54, 102, 222),   # 12 blue
    (26, 48, 142),    # 13 dark blue
    (98, 202, 82),    # 14 green
    (30, 122, 48),    # 15 dark green
)
O, W, LG, G, DG, Y, OR, BR, R, SK, LB, B, DB, GR, DGR = range(1, 16)

NAMES = ('Folder', 'HardDisk', 'Floppy', 'CD', 'Trash', 'Settings', 'Clock',
         'User', 'Chat', 'Notes', 'Paint', 'Picture', 'Speaker', 'Keyboard',
         'Joystick', 'Mouse', 'Globe', 'Server', 'Database', 'Lock', 'Key',
         'Archive', 'Calculator', 'Clipboard', 'Battery', 'Power', 'Cloud',
         'Weather', 'Rocket', 'Chip', 'Bug', 'Help', 'Info', 'Headphones',
         'Puzzle', 'Trophy', 'Coffee', 'Bell', 'Pencil', 'Script')


SPAN = range(-4, 36)   # drawing area; finish() centres the result in 32 x 32


class Canvas:
    def __init__(self):
        self.p = {}

    def put(self, x, y, c):
        if x in SPAN and y in SPAN:
            if c:
                self.p[(x, y)] = c
            else:
                self.p.pop((x, y), None)

    # --- masks: sets of (x, y) pixels whose centres lie inside a shape ---
    @staticmethod
    def poly_mask(points):
        mask = set()
        n = len(points)
        for y in SPAN:
            py = y + .5
            for x in SPAN:
                px = x + .5
                inside = False
                for i in range(n):
                    x1, y1 = points[i]
                    x2, y2 = points[(i + 1) % n]
                    if (y1 > py) != (y2 > py) and px < x1 + (py - y1) * (x2 - x1) / (y2 - y1):
                        inside = not inside
                if inside:
                    mask.add((x, y))
        return mask

    @staticmethod
    def ellipse_mask(cx, cy, rx, ry):
        return {(x, y) for y in SPAN for x in SPAN
                if ((x + .5 - cx) / rx) ** 2 + ((y + .5 - cy) / ry) ** 2 <= 1}

    @staticmethod
    def rect_mask(x, y, w, h):
        return {(xx, yy) for yy in range(y, y + h) for xx in range(x, x + w)}

    def fill(self, mask, c):
        for x, y in mask:
            self.put(x, y, c)

    def poly(self, points, c):
        self.fill(self.poly_mask(points), c)

    def ellipse(self, cx, cy, rx, ry, c):
        self.fill(self.ellipse_mask(cx, cy, rx, ry), c)

    def rect(self, x, y, w, h, c):
        self.fill(self.rect_mask(x, y, w, h), c)

    def line(self, x, y, x2, y2, c):
        x, y, x2, y2 = int(round(x)), int(round(y)), int(round(x2)), int(round(y2))
        dx, dy = abs(x2 - x), -abs(y2 - y)
        sx = 1 if x < x2 else -1
        sy = 1 if y < y2 else -1
        e = dx + dy
        while True:
            self.put(x, y, c)
            if x == x2 and y == y2:
                break
            e2 = 2 * e
            if e2 >= dy:
                e += dy
                x += sx
            if e2 <= dx:
                e += dx
                y += sy

    def edge(self, mask, c):
        """Colour the pixels of MASK that touch its own outside (an inner rim)."""
        for x, y in mask:
            if any((x + dx, y + dy) not in mask for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                self.put(x, y, c)

    def extrude(self, mask, depth, side, top, rim=None):
        """A flat shape lifted DEPTH pixels: darker sides below, light top."""
        for d in range(depth, 0, -1):
            self.fill({(x, y + d) for x, y in mask}, side)
        self.fill(mask, top)
        if rim:
            self.edge(mask, rim)

    def finish(self):
        """Centre the artwork in 32 x 32 and add the bold outline around it."""
        xs = [x for x, _ in self.p]
        ys = [y for _, y in self.p]
        w, h = max(xs) - min(xs) + 1, max(ys) - min(ys) + 1
        assert w <= 30 and h <= 30, 'artwork is %d x %d; at most 30 x 30 fits the outline' % (w, h)
        dx, dy = (32 - w) // 2 - min(xs), (32 - h) // 2 - min(ys)
        out = [[0] * 32 for _ in range(32)]
        for (x, y), c in self.p.items():
            out[y + dy][x + dx] = c
        border = [(x, y) for y in range(32) for x in range(32) if not out[y][x] and any(
            0 <= x + ex < 32 and 0 <= y + ey < 32 and out[y + ey][x + ex] not in (0, O)
            for ex, ey in ((1, 0), (-1, 0), (0, 1), (0, -1)))]
        for x, y in border:
            out[y][x] = O
        return out


def iso(ox, oy, x, y, z):
    """Tilted three-quarter projection: X runs right-down, Y left-down."""
    return (ox + x - y, oy + (x + y) / 2 - z)


def iso_box(c, ox, oy, w, d, h, top, left, right, lines=True):
    P = lambda x, y, z: iso(ox, oy, x, y, z)
    c.poly([P(0, d, 0), P(w, d, 0), P(w, d, h), P(0, d, h)], left)
    c.poly([P(w, 0, 0), P(w, d, 0), P(w, d, h), P(w, 0, h)], right)
    c.poly([P(0, 0, h), P(w, 0, h), P(w, d, h), P(0, d, h)], top)
    if lines:
        a, b = P(w, d, h), P(w, d, 0)
        c.line(a[0], a[1], b[0], b[1] - 1, O)
    return P


def gear_mask(cx, cy, r_out, r_in, teeth, squash=1.0):
    pts = []
    for i in range(teeth * 4):
        a = i * math.pi * 2 / (teeth * 4)
        r = r_out if (i % 4) in (0, 1) else r_in
        pts.append((cx + math.cos(a) * r, cy + math.sin(a) * r * squash))
    return Canvas.poly_mask(pts)


def artwork(name):
    c = Canvas()
    if name == 'Folder':
        c.poly([(4, 10), (7, 5), (14, 5), (16, 9), (28, 9), (28, 25), (4, 25)], OR)
        c.line(7, 6, 13, 6, Y)
        c.rect(7, 11, 19, 4, W)
        c.line(7, 12, 22, 12, LG)
        c.poly([(6, 13), (30, 13), (27, 27), (3, 27)], Y)
        c.poly([(3, 27), (27, 27), (27.5, 25), (3.4, 25)], OR)
        c.line(6, 13, 29, 13, 2)
        c.line(29, 14, 27, 24, OR)
    elif name == 'HardDisk':
        P = iso_box(c, 16, 5, 16, 12, 7, LG, G, DG)
        for t in (5, 9):
            a, b = P(2, 12, 3 + t / 10), P(t + 3, 12, 3)
        a, b = P(3, 12, 4), P(11, 12, 4)
        c.line(a[0], a[1], b[0], b[1], DG)
        x, y = P(14, 12, 3)
        c.rect(int(x) - 1, int(y) - 1, 2, 2, GR)
        x, y = P(8, 6, 7)
        c.ellipse(x, y + .5, 3.5, 2, G)
        c.ellipse(x, y + .5, 1.2, .8, DG)
    elif name == 'Floppy':
        P = iso_box(c, 16, 7, 15, 14, 2, B, DB, DB)
        c.poly([P(3, 0, 2), P(11, 0, 2), P(11, 6, 2), P(3, 6, 2)], LG)
        c.poly([P(7, 1, 2), P(9, 1, 2), P(9, 4, 2), P(7, 4, 2)], DG)
        c.poly([P(2, 8, 2), P(13, 8, 2), P(13, 14, 2), P(2, 14, 2)], W)
        for k in (10, 12):
            a, b = P(4, k, 2), P(11, k, 2)
            c.line(a[0], a[1], b[0], b[1], LB)
    elif name == 'CD':
        c.ellipse(16, 18, 14, 9, G)
        c.ellipse(16, 16.5, 14, 9, LG)
        c.ellipse(16, 16.5, 10, 6.5, LB)
        c.ellipse(16, 16.5, 7, 4.5, LG)
        c.line(7, 12, 12, 10, W)
        c.line(21, 22, 25, 20, 2)
        c.ellipse(16, 16.5, 3.5, 2.2, G)
        c.ellipse(16, 16.5, 1.8, 1.2, 0)
    elif name == 'Trash':
        c.rect(7, 9, 18, 19, G)
        c.ellipse(16, 27.5, 9, 2.5, G)
        c.rect(7, 9, 5, 19, LG)
        for x in (11, 15, 19, 23):
            c.line(x, 11, x, 27, DG)
        c.ellipse(16, 8, 10, 3.5, LG)
        c.ellipse(16, 8, 8, 2.2, DG)
        c.rect(13, 3, 6, 2, G)
        c.line(13, 3, 18, 3, LG)
    elif name == 'Settings':
        m = gear_mask(15, 14, 13, 9.5, 8, .82)
        c.extrude(m, 3, DG, LG)
        c.ellipse(15, 14, 4.5, 3.7, DG)
        c.ellipse(15, 14, 2.6, 2.1, 0)
        c.line(8, 9, 12, 6, W)
    elif name == 'Clock':
        m = c.ellipse_mask(16, 14.5, 13, 12)
        c.extrude(m, 3, DB, B)
        c.ellipse(16, 14.5, 10.5, 9.5, W)
        for a in range(12):
            x = 16 + math.cos(a * math.pi / 6) * 8.5
            y = 14.5 + math.sin(a * math.pi / 6) * 7.7
            c.put(int(x), int(y), G)
        c.line(16, 14, 16, 8, O)
        c.line(16, 14, 21, 16, O)
        c.put(16, 14, R)
    elif name == 'User':
        c.ellipse(16, 26, 12, 7, DB)
        c.ellipse(16, 25, 11, 6, B)
        c.rect(5, 25, 22, 3, B)
        c.rect(5, 27, 22, 1, DB)
        c.poly([(13, 19), (19, 19), (16, 24)], W)
        c.ellipse(16, 11, 7, 8, SK)
        c.ellipse(16, 6, 7, 3, BR)
        c.rect(9, 5, 2, 6, BR)
        c.rect(21, 5, 2, 6, BR)
        c.put(13, 11, O)
        c.put(19, 11, O)
        c.line(14, 15, 18, 15, BR)
    elif name == 'Chat':
        m = c.ellipse_mask(15, 12, 13, 9) | c.poly_mask([(8, 18), (14, 19), (5, 26)])
        c.extrude(m, 2, G, W)
        for x in (9, 15, 21):
            c.rect(x, 11, 2, 2, B)
        m2 = c.ellipse_mask(24, 23, 6, 4.5)
        c.extrude(m2, 2, DB, LB)
        c.rect(22, 22, 1, 1, W)
        c.rect(25, 22, 1, 1, W)
    elif name == 'Notes':
        P = iso_box(c, 14, 5, 12, 16, 2, Y, OR, OR)
        for k in range(5, 15, 3):
            a, b = P(2, k, 2), P(10, k, 2)
            c.line(a[0], a[1], b[0], b[1], OR)
        c.poly([P(0, 0, 2), P(12, 0, 2), P(12, 3, 2), P(0, 3, 2)], R)
        c.poly([P(12, 0, 0), P(12, 3, 0), P(12, 3, 2), P(12, 0, 2)], BR)
        a = P(8, 12, 2)
        c.line(a[0] + 2, a[1] - 9, a[0] + 9, a[1] - 2, O)
    elif name == 'Paint':
        m = c.ellipse_mask(14, 17, 12.5, 9) - c.ellipse_mask(20, 22, 3.5, 2.5)
        c.extrude(m, 2, BR, SK)
        for (x, y, col) in ((7, 15, R), (11, 11, Y), (17, 11, B), (22, 15, GR), (9, 20, W)):
            c.ellipse(x, y, 2.4, 1.8, col)
        c.line(28, 2, 18, 17, BR)
        c.line(29, 3, 19, 18, OR)
        c.poly([(17, 16), (20, 18), (17, 21), (15, 19)], DG)
        c.put(16, 20, R)
    elif name == 'Picture':
        c.poly([(4, 5), (28, 3), (28, 27), (4, 25)], OR)
        c.poly([(28, 3), (30, 4), (30, 28), (28, 27)], BR)
        c.poly([(4, 25), (28, 27), (30, 28), (6, 26)], BR)
        c.poly([(7, 8), (25, 6.5), (25, 23.5), (7, 22)], LB)
        c.poly([(7, 22), (7, 16), (12, 11), (16, 15), (19, 12), (25, 18), (25, 23.5)], GR)
        c.poly([(7, 22), (7, 18), (12, 14), (17, 19), (17, 22.7)], DGR)
        c.ellipse(20, 10, 2, 2, Y)
        c.line(5, 5, 27, 3, Y)
    elif name == 'Speaker':
        c.poly([(4, 12), (10, 12), (17, 5), (17, 27), (10, 20), (4, 20)], DG)
        c.poly([(4, 12), (10, 12), (10, 20), (4, 20)], G)
        c.poly([(10, 12), (17, 5), (17, 9), (11, 14)], LG)
        for r in (5, 9):
            for a in range(-45, 50, 6):
                x = 18 + math.cos(math.radians(a)) * r
                y = 16 + math.sin(math.radians(a)) * r * 1.1
                c.put(int(x), int(y), B)
                c.put(int(x) + 1, int(y), LB)
    elif name == 'Keyboard':
        P = iso_box(c, 12, 10, 20, 9, 3, LG, G, DG)
        for row in range(3):
            for col in range(6):
                x, y = P(1.5 + col * 3, 1.5 + row * 2.4, 3)
                c.rect(int(x), int(y), 2, 1, W)
        a, b = P(5, 7.8, 3), P(14, 7.8, 3)
        c.line(a[0], a[1], b[0], b[1], W)
    elif name == 'Joystick':
        P = iso_box(c, 16, 14, 14, 14, 5, DG, O, O)
        top = P(7, 7, 5)
        c.rect(int(top[0]) - 1, 7, 3, int(top[1]) - 7, G)
        c.line(int(top[0]) - 1, 8, int(top[0]) - 1, int(top[1]) - 1, LG)
        c.ellipse(top[0], 6, 5, 4.5, R)
        c.ellipse(top[0] - 1.5, 4.5, 1.6, 1.3, W)
        x, y = P(11, 3, 5)
        c.ellipse(x, y, 2, 1.2, R)
        ex, ey = P(7, 7, 5)
        c.ellipse(ex, ey, 3, 1.5, O)
    elif name == 'Mouse':
        m = c.ellipse_mask(16, 15, 10, 12.5)
        c.extrude(m, 3, G, LG)
        c.line(16, 3, 16, 12, G)
        c.line(7, 12, 25, 12, G)
        c.line(9, 6, 11, 4, W)
        c.line(16, 3, 19, 0, 0)
        c.line(16, 1, 16, 2, DG)
    elif name == 'Globe':
        c.ellipse(16, 16, 13.5, 13.5, B)
        c.poly([(9, 6), (14, 5), (16, 9), (13, 12), (14, 17), (10, 20), (7, 15), (6, 10)], GR)
        c.poly([(18, 14), (24, 12), (27, 17), (24, 24), (19, 22)], GR)
        c.poly([(14, 22), (17, 24), (15, 27), (12, 26)], GR)
        c.poly([(10, 20), (7, 15), (6, 10), (6, 16)], DGR)
        c.ellipse(11, 9, 3, 2, LB)
        m = c.ellipse_mask(16, 16, 13.5, 13.5) - c.ellipse_mask(14.5, 14.5, 13, 13)
        c.fill(m, DB)
    elif name == 'Server':
        P = iso_box(c, 18, 26, 11, 8, 20, LG, G, DG)
        for z in (4, 10, 16):
            a, b = P(0.5, 8, z), P(10.5, 8, z)
            c.line(a[0], a[1], b[0], b[1], DG)
            a, b = P(1.5, 8, z + 3), P(7, 8, z + 3)
            c.line(a[0], a[1], b[0], b[1], 2)
            x, y = P(9.5, 8, z + 3)
            c.put(int(x), int(y), GR)
    elif name == 'Database':
        for k, (top, side) in enumerate(((B, DB), (B, DB), (LB, B))):
            y = 24 - k * 7
            c.rect(5, y - 6, 22, 6, side)
            c.ellipse(16, y, 11, 4, side)
            c.ellipse(16, y - 6, 11, 4, top)
            c.line(6, y - 6, 6, y - 1, LB if k == 2 else B)
        c.ellipse(16, 10, 8, 2.5, B)
        c.ellipse(14, 9.5, 4, 1, LB)
    elif name == 'Lock':
        sh = c.ellipse_mask(16, 11, 8, 8) - c.ellipse_mask(16, 11, 5, 5.5)
        sh = {(x, y) for x, y in sh if y < 15}
        c.fill(sh, G)
        c.edge(sh, DG)
        m = c.rect_mask(6, 14, 20, 13)
        c.extrude(m, 2, OR, Y)
        c.line(6, 14, 25, 14, W)
        c.ellipse(16, 19.5, 2, 2, O)
        c.rect(15, 20, 2, 4, O)
    elif name == 'Key':
        m = (c.ellipse_mask(9, 12, 7, 7) - c.ellipse_mask(8, 11, 2.5, 2.5)) | \
            c.poly_mask([(13, 13), (28, 13), (28, 17), (13, 17)]) | \
            c.rect_mask(21, 17, 3, 5) | c.rect_mask(25, 17, 3, 4)
        c.extrude(m, 2, OR, Y)
        c.line(4, 9, 6, 7, W)
        c.line(15, 14, 27, 14, W)
    elif name == 'Archive':
        P = iso_box(c, 15, 6, 15, 13, 12, SK, OR, BR)
        a, b = P(7.5, 0, 12), P(7.5, 13, 12)
        c.line(a[0], a[1], b[0], b[1], OR)
        c.poly([P(6, 0, 12), P(9, 0, 12), P(9, 13, 12), P(6, 13, 12)], LG)
        c.poly([P(6, 13, 12), P(9, 13, 12), P(9, 13, 6), P(6, 13, 6)], G)
        x, y = P(12, 13, 5)
        c.rect(int(x) - 3, int(y) - 1, 4, 2, W)
    elif name == 'Calculator':
        m = c.rect_mask(7, 3, 18, 25)
        c.extrude(m, 2, DG, G)
        c.rect(9, 5, 14, 5, GR)
        c.rect(9, 5, 14, 1, DGR)
        c.line(15, 8, 21, 8, DGR)
        for row in range(4):
            for col in range(3):
                c.rect(9 + col * 5, 12 + row * 4, 4, 3, LG if col < 2 or row < 2 else OR)
                c.line(9 + col * 5, 14 + row * 4, 12 + col * 5, 14 + row * 4, DG)
    elif name == 'Clipboard':
        m = c.rect_mask(6, 4, 20, 26)
        c.extrude(m, 1, BR, OR)
        c.rect(8, 7, 16, 21, W)
        for y in (13, 16, 19, 22):
            c.line(10, y, 21, y, G)
        c.line(10, 10, 16, 10, B)
        c.rect(12, 2, 8, 6, G)
        c.rect(12, 2, 8, 1, LG)
        c.rect(14, 3, 4, 1, DG)
    elif name == 'Battery':
        c.rect(9, 7, 14, 20, GR)
        c.ellipse(16, 27, 7, 2.5, DGR)
        c.rect(18, 7, 5, 20, DGR)
        c.ellipse(16, 7, 7, 2.5, G)
        c.rect(9, 7, 14, 3, G)
        c.rect(18, 7, 5, 3, DG)
        c.ellipse(16, 6, 7, 2.5, LG)
        c.rect(13, 2, 6, 4, G)
        c.ellipse(16, 2.5, 3, 1, LG)
        c.line(10, 11, 10, 25, 2)
        c.poly([(17, 11), (12, 19), (16, 19), (14, 25), (20, 16), (16, 16), (18, 11)], Y)
    elif name == 'Power':
        m = c.ellipse_mask(16, 15, 13.5, 12.5)
        c.extrude(m, 3, 8, R)
        c.ellipse(13, 9, 5, 2.5, 7)
        ring = c.ellipse_mask(16, 16, 7.5, 7) - c.ellipse_mask(16, 16, 5, 4.6)
        ring = {(x, y) for x, y in ring if not (13 <= x <= 18 and y < 13)}
        c.fill(ring, W)
        c.rect(15, 6, 2, 9, W)
    elif name == 'Cloud':
        m = c.ellipse_mask(10, 17, 7, 6) | c.ellipse_mask(17, 12, 8, 7.5) | \
            c.ellipse_mask(24, 17, 6, 5.5) | c.rect_mask(9, 17, 16, 6)
        c.extrude(m, 3, G, W)
        c.edge(m, LG)
        c.line(12, 8, 15, 6, W)
    elif name == 'Weather':
        sun = c.ellipse_mask(11, 11, 6.5, 6.5)
        for a in range(0, 360, 45):
            x = 11 + math.cos(math.radians(a)) * 9.5
            y = 11 + math.sin(math.radians(a)) * 9.5
            c.ellipse(x, y, 1.4, 1.4, OR)
        c.fill(sun, Y)
        c.edge(sun, OR)
        m = c.ellipse_mask(14, 22, 5.5, 4.5) | c.ellipse_mask(20, 18, 6.5, 6) | \
            c.ellipse_mask(26, 22, 4, 4) | c.rect_mask(14, 22, 12, 5)
        c.extrude(m, 2, G, W)
        c.edge(m, LG)
    elif name == 'Rocket':
        body = c.poly_mask([(22, 2), (27, 5), (14, 22), (9, 17)])
        c.poly([(9, 17), (5, 18), (3, 24), (11, 19)], R)
        c.poly([(14, 22), (13, 26), (7, 28), (12, 20)], R)
        c.fill(body, LG)
        c.poly([(22, 2), (27, 5), (14, 22), (19, 8)], W)
        c.poly([(22, 2), (29, 1), (27, 5)], R)
        c.ellipse(19, 10, 2.6, 2.6, DB)
        c.ellipse(19, 10, 1.6, 1.6, LB)
        c.poly([(9, 22), (6, 25), (3, 30), (7, 28), (10, 25)], OR)
        c.poly([(8, 24), (6, 27), (8, 26)], Y)
    elif name == 'Chip':
        P = lambda x, y, z: iso(16, 6, x, y, z)
        for k in range(2, 12, 3):
            for (x0, y0, x1, y1) in ((k, 12, k, 15), (12, k, 15, k)):
                a, b = P(x0, y0, 0), P(x1, y1, -1)
                c.line(a[0], a[1], b[0], b[1], LG)
        iso_box(c, 16, 6, 13, 13, 3, DG, O, O)
        a, b, d = P(4, 4, 3), P(9, 4, 3), P(9, 9, 3)
        c.poly([P(4, 4, 3), P(9, 4, 3), P(9, 9, 3), P(4, 9, 3)], G)
        c.line(a[0], a[1], b[0], b[1], LG)
        x, y = P(3, 3, 3)
        c.put(int(x), int(y), W)
        del d
    elif name == 'Bug':
        for dy in (-4, 0, 4):
            c.line(16, 18 + dy, 5, 14 + dy * 1.6, O)
            c.line(16, 18 + dy, 27, 14 + dy * 1.6, O)
        c.line(13, 6, 9, 2, O)
        c.line(19, 6, 23, 2, O)
        c.ellipse(16, 8, 5, 4, DGR)
        m = c.ellipse_mask(16, 19, 8.5, 9.5)
        c.fill(m, GR)
        c.line(16, 11, 16, 28, DGR)
        c.ellipse(12, 15, 1.8, 1.8, DGR)
        c.ellipse(20, 21, 1.8, 1.8, DGR)
        c.ellipse(12.5, 23, 1.5, 1.5, DGR)
        c.line(11, 12, 13, 11, W)
        c.put(14, 7, W)
        c.put(18, 7, W)
    elif name == 'Help':
        ring = c.ellipse_mask(16, 15, 13.5, 12) - c.ellipse_mask(16, 15, 6, 5.2)
        for d in (3, 2, 1):
            c.fill({(x, y + d) for x, y in ring}, 8)
        c.fill(ring, W)
        for a0 in (20, 200):
            seg = {(x, y) for x, y in ring
                   if a0 <= (math.degrees(math.atan2(y + .5 - 15, x + .5 - 16)) % 360) < a0 + 50}
            seg |= {(x, y) for x, y in ring
                    if (a0 + 90) % 360 <= (math.degrees(math.atan2(y + .5 - 15, x + .5 - 16)) % 360) < (a0 + 140) % 360}
            c.fill(seg, R)
        c.edge(c.ellipse_mask(16, 15, 6.8, 6), G)
    elif name == 'Info':
        m = c.ellipse_mask(16, 15, 13.5, 12.5)
        c.extrude(m, 3, DB, B)
        c.ellipse(12, 9, 5, 2.5, LB)
        c.ellipse(16, 8.5, 2, 2, W)
        c.rect(14, 12, 4, 10, W)
        c.rect(12, 12, 3, 2, W)
        c.rect(12, 21, 8, 2, W)
    elif name == 'Headphones':
        band = c.ellipse_mask(16, 16, 12.5, 12.5) - c.ellipse_mask(16, 16, 9.5, 9.8)
        band = {(x, y) for x, y in band if y < 17}
        c.fill(band, DG)
        c.edge(band, G)
        for x in (3, 22):
            m = c.rect_mask(x, 15, 7, 12)
            c.extrude(m, 1, DB, B)
            c.rect(x + 1, 16, 2, 9, LB)
        c.rect(5, 13, 3, 3, G)
        c.rect(24, 13, 3, 3, G)
    elif name == 'Puzzle':
        m = c.rect_mask(5, 9, 19, 16) | c.ellipse_mask(14.5, 7.5, 3.5, 3.5) | \
            c.ellipse_mask(25.5, 17, 3.5, 3.5)
        m -= c.ellipse_mask(5, 17, 3, 3)
        c.extrude(m, 3, DGR, GR)
        c.line(6, 10, 11, 10, W)
        c.line(14, 5, 15, 5, W)
    elif name == 'Trophy':
        handles = (c.ellipse_mask(7, 10, 5, 5) - c.ellipse_mask(7, 10, 3, 3)) | \
                  (c.ellipse_mask(25, 10, 5, 5) - c.ellipse_mask(25, 10, 3, 3))
        c.fill(handles, OR)
        cup = c.poly_mask([(7, 3), (25, 3), (23, 12), (19, 17), (13, 17), (9, 12)])
        c.fill(cup, Y)
        c.poly([(19, 3), (25, 3), (23, 12), (19, 17), (17, 17), (20, 11)], OR)
        c.rect(14, 17, 4, 5, OR)
        P = iso_box(c, 16, 21, 8, 8, 4, BR, 8, 8, lines=False)
        c.poly([(10, 22), (22, 22), (22, 27), (10, 27)], BR)
        c.rect(12, 23, 8, 2, Y)
        c.rect(7, 3, 18, 1, W)
        c.line(10, 5, 12, 12, W)
    elif name == 'Coffee':
        handle = c.ellipse_mask(24, 17, 5, 5) - c.ellipse_mask(24, 17, 2.6, 2.6)
        c.fill(handle, LG)
        body = c.rect_mask(6, 9, 17, 17) | c.ellipse_mask(14.5, 26, 8.5, 3)
        c.fill(body, W)
        c.rect(17, 9, 6, 18, LG)
        c.ellipse(19.5, 26, 3.5, 2.5, LG)
        c.ellipse(14.5, 9, 8.5, 3, LG)
        c.ellipse(14.5, 9.5, 7, 2, BR)
        c.rect(6, 15, 17, 3, R)
        for x in (11, 17):
            c.line(x, 6, x - 1, 4, LG)
            c.line(x - 1, 4, x, 2, LG)
    elif name == 'Bell':
        m = c.poly_mask([(16, 4), (21, 6), (23, 12), (24, 20), (28, 24), (4, 24), (8, 20), (9, 12), (11, 6)])
        c.extrude(m, 2, OR, Y)
        c.poly([(16, 4), (21, 6), (23, 12), (24, 20), (28, 24), (19, 24), (19, 8)], OR)
        c.ellipse(16, 27, 3, 2.5, BR)
        c.rect(15, 1, 3, 3, BR)
        c.line(11, 9, 10, 17, W)
    elif name == 'Pencil':
        body = c.poly_mask([(23, 3), (29, 9), (11, 27), (5, 21)])
        c.fill(body, Y)
        c.poly([(26, 6), (29, 9), (11, 27), (8, 24)], OR)
        c.poly([(23, 3), (26, 0.5), (31.5, 6), (29, 9)], R)
        c.poly([(21, 5), (23, 3), (29, 9), (27, 11)], LG)
        c.poly([(5, 21), (11, 27), (3, 29)], SK)
        c.poly([(3.5, 26.5), (5.5, 28.5), (2, 30)], DG)
        c.line(7, 21, 23, 5, W)
    elif name == 'Script':
        c.ellipse(26, 7, 3.5, 4.5, SK)
        c.ellipse(26, 7, 1.5, 2.2, BR)
        paper = c.poly_mask([(6, 4), (26, 3), (26, 26), (8, 27)])
        c.fill(paper, W)
        c.poly([(19, 4), (26, 3), (26, 26), (19, 27)], SK)
        for y in (9, 12, 15, 18, 21):
            c.line(9, y, 20, y - 1, G)
        c.line(9, 9, 13, 9, B)
        c.ellipse(7, 25, 3.5, 4.5, SK)
        c.ellipse(7, 25, 1.5, 2.2, BR)
        c.rect(7, 28, 18, 1, BR)
    else:
        raise ValueError(name)
    return c.finish()


def chunk(kind, data):
    return kind + struct.pack('>I', len(data)) + data + (b'\0' if len(data) % 2 else b'')


def ilbm(p):
    bmhd = struct.pack('>HHhhBBBBHBBhh', 32, 32, 0, 0, 4, 2, 0, 0, 0, 1, 1, 32, 32)
    body = bytearray()
    for row in p:
        for plane in range(4):
            body.extend(sum(((row[x + i] >> plane) & 1) << (7 - i) for i in range(8)) for x in range(0, 32, 8))
    data = b'ILBM' + chunk(b'BMHD', bmhd) + chunk(b'CMAP', bytes(v for rgb in PALETTE for v in rgb)) + chunk(b'BODY', bytes(body))
    return b'FORM' + struct.pack('>I', len(data)) + data


def png(p):
    def ch(k, d):
        return struct.pack('>I', len(d)) + k + d + struct.pack('>I', zlib.crc32(k + d) & 0xffffffff)
    data = b''.join(b'\0' + bytes(v for n in row for v in (*PALETTE[n], 255 if n else 0)) for row in p)
    return b'\x89PNG\r\n\x1a\n' + ch(b'IHDR', struct.pack('>IIBBBBB', 32, 32, 8, 6, 0, 0, 0)) + ch(b'IDAT', zlib.compress(data)) + ch(b'IEND', b'')


def preview(path):
    from PIL import Image, ImageDraw
    cols = 8
    rows = (len(NAMES) + cols - 1) // cols
    sheet = Image.new('RGB', (cols * 112, rows * 120), (60, 66, 82))
    d = ImageDraw.Draw(sheet)
    for i, name in enumerate(NAMES):
        x, y = (i % cols) * 112, (i // cols) * 120
        icon = Image.open(ROOT / 'Icons' / f'{name}.png').resize((96, 96), Image.Resampling.NEAREST)
        sheet.paste(icon, (x + 8, y + 4), icon)
        d.text((x + 56, y + 104), name, fill=(235, 238, 245), anchor='mt')
    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--preview', type=Path)
    args = ap.parse_args()
    for name in NAMES:
        pixels = artwork(name)
        (ROOT / 'Icons' / f'{name}.iff').write_bytes(ilbm(pixels))
        (ROOT / 'Icons' / f'{name}.png').write_bytes(png(pixels))
    if args.preview:
        preview(args.preview)
    print('Generated', len(NAMES), 'Tilt icons (32px ILBM and PNG).')


if __name__ == '__main__':
    main()
