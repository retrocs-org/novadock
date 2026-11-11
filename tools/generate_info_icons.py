#!/usr/bin/env python3
"""Generate NovaDock's palette-safe classic Workbench WBTOOL icons.

The output is the original planar DiskObject format understood by icon.library
on AmigaOS 3.x.  It deliberately avoids OS 3.5 colour-icon extensions so the
same artwork is used on both old and new Workbench installations.
"""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
WIDTH = 54
HEIGHT = 22


class Canvas:
    def __init__(self, width=WIDTH, height=HEIGHT):
        self.width = width
        self.height = height
        self.pixels = [[0 for _ in range(width)] for _ in range(height)]

    def pixel(self, x, y, colour):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y][x] = colour

    def rect(self, x1, y1, x2, y2, colour):
        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                self.pixel(x, y, colour)

    def line(self, x1, y1, x2, y2, colour):
        dx = abs(x2 - x1)
        sx = 1 if x1 < x2 else -1
        dy = -abs(y2 - y1)
        sy = 1 if y1 < y2 else -1
        error = dx + dy
        while True:
            self.pixel(x1, y1, colour)
            if x1 == x2 and y1 == y2:
                return
            twice = 2 * error
            if twice >= dy:
                error += dy
                x1 += sx
            if twice <= dx:
                error += dx
                y1 += sy


def dock_art():
    c = Canvas()
    # Chamfered dock body and a bright lower accent.
    c.rect(2, 5, 51, 19, 1)
    c.rect(4, 6, 49, 17, 2)
    for x, y in ((2, 5), (3, 5), (50, 5), (51, 5),
                 (2, 19), (3, 19), (50, 19), (51, 19)):
        c.pixel(x, y, 0)
    c.line(4, 18, 49, 18, 3)
    # Four launch tiles; the N-shaped centre pair forms a small brand cue.
    for x1 in (7, 18, 29, 40):
        c.rect(x1, 8, x1 + 7, 15, 1)
        c.rect(x1 + 1, 9, x1 + 6, 14, 3)
    c.line(20, 13, 20, 10, 2)
    c.line(20, 10, 25, 14, 2)
    c.line(25, 14, 25, 10, 2)
    return c


def prefs_art():
    c = Canvas()
    # A compact preferences window with three clearly different sliders.
    c.rect(3, 2, 50, 20, 1)
    c.rect(5, 4, 48, 18, 3)
    c.rect(5, 4, 48, 7, 2)
    c.rect(44, 5, 47, 6, 1)
    for y in (10, 14, 18):
        c.line(9, y, 44, y, 1)
    c.rect(15, 8, 19, 12, 2)
    c.rect(34, 12, 38, 16, 2)
    c.rect(24, 16, 28, 19, 2)
    return c


def music_art():
    """Launcher-sized retro face wearing headphones in four Workbench pens."""
    c = Canvas(48, 38)
    # Headphone band behind the head. Blocky stepped curves deliberately echo
    # classic late-1980s Amiga icon artwork rather than modern antialiasing.
    c.rect(9, 13, 11, 22, 1)
    c.rect(11, 8, 13, 14, 1)
    c.rect(14, 5, 17, 9, 1)
    c.rect(18, 3, 29, 5, 1)
    c.rect(30, 5, 33, 9, 1)
    c.rect(34, 8, 36, 14, 1)
    c.rect(36, 13, 38, 22, 1)
    c.rect(12, 9, 13, 15, 2)
    c.rect(15, 6, 18, 7, 2)
    c.rect(19, 4, 28, 4, 2)
    c.rect(29, 6, 32, 7, 2)
    c.rect(34, 9, 35, 15, 2)

    # Rounded pixel head: dark outline, warm mid-tone hair and a light face.
    c.rect(18, 6, 29, 6, 1)
    c.rect(15, 7, 32, 10, 1)
    c.rect(13, 11, 34, 22, 1)
    c.rect(15, 23, 32, 28, 1)
    c.rect(18, 29, 29, 31, 1)
    c.rect(17, 8, 30, 11, 2)
    c.rect(15, 12, 32, 14, 2)
    c.rect(14, 15, 33, 21, 3)
    c.rect(16, 22, 31, 26, 3)
    c.rect(18, 27, 29, 29, 3)
    # Asymmetric fringe gives the face a little character at low resolution.
    c.rect(15, 11, 30, 13, 2)
    c.rect(15, 13, 19, 16, 2)
    c.rect(28, 12, 32, 14, 2)

    # Ear cups sit in front of the head and include a bright bevel.
    c.rect(6, 15, 13, 27, 1)
    c.rect(8, 17, 12, 25, 2)
    c.rect(9, 18, 11, 22, 3)
    c.rect(34, 15, 41, 27, 1)
    c.rect(35, 17, 39, 25, 2)
    c.rect(36, 18, 38, 22, 3)

    # Eyes, nose and a friendly two-step smile.
    c.rect(20, 17, 21, 18, 1)
    c.rect(27, 17, 28, 18, 1)
    c.rect(24, 19, 25, 22, 2)
    c.pixel(23, 22, 1)
    c.line(20, 24, 22, 26, 1)
    c.line(22, 26, 27, 26, 1)
    c.line(27, 26, 29, 24, 1)

    # Small shoulders make the glyph read as a character rather than a mask.
    c.rect(14, 32, 33, 33, 1)
    c.rect(11, 34, 36, 36, 1)
    c.rect(15, 33, 32, 34, 2)
    c.rect(13, 35, 34, 35, 2)
    c.rect(20, 31, 27, 33, 3)
    return c


def running_apps_art():
    """Launcher-style 2x2 application grid in classic Workbench colours."""
    c = Canvas(48, 38)
    c.rect(5, 4, 42, 33, 1)
    c.rect(7, 6, 40, 31, 2)
    c.rect(8, 7, 39, 30, 3)
    for index, (x, y) in enumerate(((10, 9), (25, 9), (10, 20), (25, 20))):
        c.rect(x, y, x + 11, y + 8, 1)
        c.rect(x + 1, y + 1, x + 10, y + 7, 3)
        c.rect(x + 1, y + 1, x + 10, y + 2,
               2 if index % 2 == 0 else 1)
        c.rect(x + 2, y + 4, x + 4, y + 6, 2)
        c.rect(x + 6, y + 4, x + 9, y + 4, 2)
        c.rect(x + 6, y + 6, x + 9, y + 6, 2)
    c.line(8, 31, 39, 31, 3)
    return c


def installer_art():
    c = Canvas()
    # A package box with a downward arrow: "install NovaDock".
    c.rect(14, 9, 39, 20, 1)
    c.rect(16, 11, 37, 19, 2)
    c.line(14, 9, 19, 5, 1)
    c.line(39, 9, 34, 5, 1)
    c.line(19, 5, 34, 5, 1)
    c.rect(20, 6, 33, 8, 3)
    c.rect(25, 0, 28, 10, 3)
    for i in range(5):
        c.line(22 + i, 10 + i, 31 - i, 10 + i, 3)
    c.line(18, 15, 35, 15, 1)
    return c


def drawer_art():
    c = Canvas()
    # A classic drawer front with a handle and a small dock strip on top.
    c.rect(8, 6, 45, 20, 1)
    c.rect(10, 8, 43, 18, 2)
    c.rect(23, 11, 30, 13, 1)
    c.rect(12, 2, 41, 5, 1)
    for x1 in (14, 21, 28, 35):
        c.rect(x1, 3, x1 + 4, 4, 3)
    return c


def guide_art():
    c = Canvas()
    # An open book: two pages, a spine and lines of text.
    c.rect(9, 3, 26, 19, 1)
    c.rect(27, 3, 44, 19, 1)
    c.rect(10, 4, 25, 18, 2)
    c.rect(28, 4, 43, 18, 2)
    c.line(26, 3, 26, 20, 3)
    c.line(27, 3, 27, 20, 3)
    for y in (7, 10, 13, 16):
        c.line(12, y, 23, y, 1)
        c.line(30, y, 41, y, 1)
    c.rect(30, 6, 35, 8, 3)
    return c


def amiga_string(text):
    raw = text.encode('latin-1') + b'\0'
    return struct.pack('>I', len(raw)) + raw


def drawer_data():
    # struct OldDrawerData: the drawer window (a NewWindow) and its scroll
    # position; Workbench sizes the window itself on first open.
    return struct.pack('>hhhhBBIIIIIIIhhHHHii',
                       40, 30, 420, 160,      # LeftEdge, TopEdge, Width, Height
                       0xFF, 0xFF,            # DetailPen, BlockPen
                       0, 0, 0, 0, 0, 0, 0,   # IDCMP, Flags, pointers (unused)
                       90, 40, 0xFFFF, 0xFFFF,  # min/max size
                       1,                     # WBENCHSCREEN
                       0, 0)                  # dd_CurrentX, dd_CurrentY


def disk_object(canvas, kind=3, default_tool=None, tool_types=()):
    """KIND 3 is WBTOOL, 4 WBPROJECT, 2 WBDRAWER.  A project names the
    program that opens it (DEFAULT_TOOL) and may carry TOOL_TYPES; both are
    stored as length-prefixed strings after the image data.  A drawer
    carries its window (OldDrawerData) right after the DiskObject and the
    OS 2.x view settings (flags, view modes) at the very end."""
    # Serialized DiskObject, followed by one planar Image and its two planes.
    data = bytearray(78)
    struct.pack_into('>HH', data, 0, 0xE310, 1)  # magic, version
    struct.pack_into('>I', data, 4, 0)           # NextGadget
    struct.pack_into('>hhhh', data, 8, 0, 0, canvas.width, canvas.height)
    struct.pack_into('>HHH', data, 16, 0x0004, 0x0001, 0x0001)
    struct.pack_into('>I', data, 22, 1)           # GadgetRender is present
    struct.pack_into('>I', data, 26, 0)           # algorithmic selected image
    struct.pack_into('>I', data, 44, 1)           # disk revision marker
    data[48] = kind                               # WBTOOL / WBPROJECT
    if default_tool:
        struct.pack_into('>I', data, 50, 1)       # do_DefaultTool present
    if tool_types:
        struct.pack_into('>I', data, 54, 1)       # do_ToolTypes present
    struct.pack_into('>II', data, 58, 0x80000000, 0x80000000)
    struct.pack_into('>I', data, 74, 16384)
    if kind == 2:
        struct.pack_into('>I', data, 66, 1)       # do_DrawerData present
        struct.pack_into('>I', data, 74, 0)       # drawers have no stack
        data.extend(drawer_data())
    data.extend(struct.pack('>hhhhhIBBI', 0, 0, canvas.width, canvas.height, 2,
                            1, 3, 0, 0))
    row_words = (canvas.width + 15) // 16
    for plane in range(2):
        for y in range(canvas.height):
            for word_index in range(row_words):
                word = 0
                for bit in range(16):
                    x = word_index * 16 + bit
                    if x < canvas.width and canvas.pixels[y][x] & (1 << plane):
                        word |= 1 << (15 - bit)
                data.extend(struct.pack('>H', word))
    if default_tool:
        data.extend(amiga_string(default_tool))
    if tool_types:
        data.extend(struct.pack('>I', (len(tool_types) + 1) * 4))
        for entry in tool_types:
            data.extend(amiga_string(entry))
    if kind == 2:
        data.extend(struct.pack('>IH', 0, 0))    # DDFLAGS_SHOWDEFAULT, DDVM_BYDEFAULT
    return bytes(data)


def amirc_art():
    c = Canvas(48, 32)
    # Two overlapping chat bubbles, using the existing four Workbench pens.
    c.rect(15, 9, 44, 25, 1)
    c.rect(17, 11, 42, 23, 2)
    c.rect(35, 25, 39, 28, 1)
    c.rect(3, 3, 34, 20, 1)
    c.rect(5, 5, 32, 18, 3)
    c.rect(8, 20, 12, 24, 1)
    c.line(8, 8, 28, 8, 2)
    c.line(8, 12, 25, 12, 2)
    c.line(8, 15, 20, 15, 2)
    return c


def plugin_icons():
    """Run the original MakeIcon drawing code on the build host."""
    import subprocess
    import tempfile
    source = (ROOT / 'tools/makeicon.c').read_text()
    drawing = source[source.index('#define ICON_W'):source.index('static int make_one')]
    drawing = drawing.replace('struct Library *IconBase = NULL;', '')
    # Snapshot the error before either axis step (Bresenham).
    drawing = drawing.replace('int err = dx + dy;', 'int err = dx + dy; int twice;')
    drawing = drawing.replace('if (2 * err >= dy)', 'twice = 2 * err; if (twice >= dy)')
    drawing = drawing.replace('if (2 * err <= dx)', 'if (twice <= dx)')
    harness = ('#include <stdio.h>\n#include <stddef.h>\n'
               'typedef unsigned short UWORD;\n' + drawing +
               '\nint main(void) { int k,x,y; for(k=6;k<=14;k++) {'
               'draw_icon(k); for(y=0;y<ICON_H;y++) for(x=0;x<ICON_W;x++)'
               'putchar(read_pixel(x,y)); } return 0; }\n')
    with tempfile.TemporaryDirectory() as temp:
        cfile = Path(temp) / 'icons.c'
        exe = Path(temp) / 'icons'
        cfile.write_text(harness)
        subprocess.run(['cc', str(cfile), '-o', str(exe)], check=True)
        pixels = subprocess.check_output([str(exe)])
    names = ('Clock', 'Eyes', 'Activity', 'Memory', 'Network',
             'MusicPlayer', 'DOpus4', 'ArchiveExtractor', 'Command')
    for index, name in enumerate(names):
        if name in ('MusicPlayer', 'Network'):
            continue  # Ships dedicated artwork.
        canvas = Canvas(32, 24)
        for y in range(24):
            for x in range(32):
                canvas.pixel(x, y, pixels[index * 768 + y * 32 + x])
        (ROOT / 'Plugins' / (name + '.info')).write_bytes(disk_object(canvas))


def clipboard_art():
    """Clipboard and layered paper in the Apps icon's 48x38 palette."""
    c = Canvas(48, 38)
    c.rect(11, 4, 36, 34, 1)
    c.rect(13, 6, 34, 32, 2)
    c.rect(15, 8, 32, 30, 3)
    c.rect(19, 2, 28, 8, 1)
    c.rect(21, 3, 26, 6, 2)
    for y in (12, 16, 20, 24):
        c.line(18, y, 29, y, 2)
    c.rect(27, 23, 41, 35, 1)
    c.rect(29, 25, 39, 33, 3)
    c.line(31, 27, 37, 27, 2)
    c.line(31, 30, 36, 30, 2)
    return c


def network_art():
    """Networked displays using the same classic 48x38 Apps palette."""
    c = Canvas(48, 38)
    c.line(24, 14, 24, 24, 1)
    c.line(10, 24, 38, 24, 1)
    c.line(10, 24, 10, 28, 1)
    c.line(38, 24, 38, 28, 1)
    for x, y, w, h in ((14, 2, 20, 15), (2, 26, 16, 11), (30, 26, 16, 11)):
        c.rect(x, y, x+w-1, y+h-1, 1)
        c.rect(x+2, y+2, x+w-3, y+h-3, 2)
        c.rect(x+3, y+3, x+w-4, y+4, 3)
        c.line(x+3, y+h-4, x+w-4, y+h-4, 3)
    return c


def generic_icon(name,kind,default_tool=None,tool_types=(),show_icons=False):
    """Preserve both supplied planar images; adapt Workbench metadata only."""
    source=(ROOT / 'Icons' / name).read_bytes()
    if struct.unpack_from('>HH',source,0)!=(0xE310,1):
        raise ValueError('Not a classic Amiga icon: '+name)
    start=78+(56 if struct.unpack_from('>I',source,66)[0] else 0)
    end=start
    for pointer in (22,26):
        if not struct.unpack_from('>I',source,pointer)[0]:
            continue
        width,height,depth=struct.unpack_from('>hhh',source,end+4)
        if not (0<width<=256 and 0<height<=256 and 0<depth<=8):
            raise ValueError('Invalid planar image: '+name)
        end+=20+((width+15)//16)*2*height*depth
    if end>len(source):
        raise ValueError('Truncated planar image: '+name)
    data=bytearray(source[:78])
    data[48]=kind
    struct.pack_into('>II',data,50,int(bool(default_tool)),int(bool(tool_types)))
    struct.pack_into('>II',data,58,0x80000000,0x80000000)
    struct.pack_into('>I',data,66,int(kind==2))
    struct.pack_into('>I',data,70,0)
    struct.pack_into('>I',data,74,0 if kind==2 else 16384)
    if kind==2:
        data.extend(drawer_data())
    data.extend(source[start:end])
    if default_tool:
        data.extend(amiga_string(default_tool))
    if tool_types:
        data.extend(struct.pack('>I',(len(tool_types)+1)*4))
        for entry in tool_types:
            data.extend(amiga_string(entry))
    if kind==2:
        # OS 2.x drawer extension. Release drawers explicitly override the
        # parent's Show All Files/text settings; other drawer icons inherit.
        struct.pack_into('>I',data,44,1)  # revision enables DrawerData extension
        data.extend(struct.pack('>IH',1 if show_icons else 0,1 if show_icons else 0))
    return bytes(data)


def main():
    # Supply the same complete drawer metadata to both release archives and
    # installations, while keeping the user's two planar images unchanged.
    (ROOT / 'Icons' / 'SoftwarePackage.info').write_bytes(
        generic_icon('SoftwarePackage.info',kind=2))
    plugin_icons()
    outputs = ((ROOT / 'NovaDock.info', dock_art()),
               (ROOT / 'NovaDockPrefs.info', prefs_art()),
               (ROOT / 'Plugins' / 'MusicPlayer.info', music_art()),
               (ROOT / 'Plugins' / 'RunningApps.info', running_apps_art()),
               (ROOT / 'Plugins' / 'AmIRC.info', amirc_art()),
               (ROOT / 'Plugins' / 'Clipboard.info', clipboard_art()),
               (ROOT / 'Plugins' / 'Network.info', network_art()))
    for path, canvas in outputs:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(disk_object(canvas))
        print('Created', path)
    # The GUI installer: a project icon that Workbench opens with Installer.
    # The local launcher selects the supported system Installer and stack.
    installer = ROOT / 'Install-NovaDock.info'
    installer.write_bytes(generic_icon(
        'Install.info', kind=4, default_tool='StartInstaller',
        tool_types=('APPNAME=NovaDock', 'MINUSER=NOVICE',
                    'DEFUSER=NOVICE')))
    print('Created', installer)
    # The AmigaGuide manual, opened with MultiView (AmigaOS 3.0 and later).
    guide = ROOT / 'NovaDock.guide.info'
    guide.write_bytes(disk_object(guide_art(), kind=4,
                                  default_tool='SYS:Utilities/MultiView'))
    print('Created', guide)


if __name__ == '__main__':
    import sys
    if len(sys.argv) == 3 and sys.argv[1] == '--drawer-icon':
        # Release archives: the icon for the unpacked NovaDock drawer.
        Path(sys.argv[2]).write_bytes(generic_icon('SoftwarePackage.info',kind=2,show_icons=True))
        print('Created', sys.argv[2])
    else:
        main()
