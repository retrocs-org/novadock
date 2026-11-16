#!/usr/bin/env python3
"""Create NovaDock's 32-pixel collection artwork as transparent ILBM and PNG."""
import argparse
import struct
import zlib
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
NAMES = ('WHDLoad', 'SadWHDLoad', 'SadMusic', 'SadSensor', 'Demos', 'Apps', 'Arcade', 'Music', 'Graphics', 'Code', 'Movies', 'Books', 'Disks', 'Network', 'Utilities', 'Favourites', 'Aminet', 'Downloads', 'Search', 'Radio', 'Feeds', 'Printer', 'Backup', 'Calendar', 'Terminal', 'DiskSpace', 'Mail', 'Camera')
PALETTE = ((255,0,255),(36,44,57),(124,157,186),(232,239,245),(143,194,205),(170,194,140),(220,180,103),(192,113,118),(197,190,166),(144,137,116),(232,225,204),(105,101,87)) + ((0,0,0),)*4
class Canvas:
    def __init__(self): self.p=[[0]*32 for _ in range(32)]
    def box(self,x,y,w,h,c):
        for yy in range(y,y+h):
            for xx in range(x,x+w):
                assert 0<=xx<32 and 0<=yy<32
                self.p[yy][xx]=c
    def line(self,x,y,x2,y2,c):
        dx,dy=abs(x2-x),-abs(y2-y);sx=1 if x<x2 else -1;sy=1 if y<y2 else -1;e=dx+dy
        while True:
            self.box(x,y,1,1,c)
            if x==x2 and y==y2: break
            e2=e*2
            if e2>=dy:e+=dy;x+=sx
            if e2<=dx:e+=dx;y+=sy
    def frame(self,x,y,w,h,c=2):
        self.box(x+1,y+1,w,h,1);self.box(x,y,w,h,1)
        self.box(x+1,y+1,w-2,h-2,c);self.line(x+1,y+1,x+w-2,y+1,3)
    def window(self):
        self.frame(3,4,25,24);self.box(5,6,21,3,4);self.box(5,11,21,14,1)
def artwork(name):
    c=Canvas();b=c.box;l=c.line;f=c.frame
    if name=='WHDLoad':
        f(3,18,25,11);b(5,20,21,6,2);b(8,22,5,2,3);b(10,20,1,6,3)
        b(21,21,3,3,7);b(14,7,4,13,1);b(15,7,2,13,3)
        b(12,3,8,6,1);b(13,2,6,7,7);b(14,3,2,2,3)
        l(15,24,16,25,5);l(16,25,17,24,5);l(17,24,18,25,5);l(18,25,19,24,5)
    elif name=='SadWHDLoad':
        f(3,18,25,11);b(5,21,20,5,4);b(14,10,4,10,1);b(15,10,2,10,3)
        b(10,1,12,12,1);b(11,2,10,10,2);b(12,3,8,1,3)
        b(13,5,2,2,1);b(18,5,2,2,1);b(15,8,3,1,1);b(14,9,1,1,1);b(18,9,1,1,1)
    elif name=='SadSensor':
        f(7,5,18,22);b(9,8,14,16,4)
        for x in (3,25):
            for y in (9,15,21):b(x,y,4,2,2)
        b(10,11,3,3,1);b(19,11,3,3,1);b(13,18,6,2,1);b(11,20,2,2,1);b(19,20,2,2,1)
    elif name=='Demos':
        # Beige Commodore-style CRT: recessed glass, chunky lower bezel,
        # side vents, small front controls and broad feet instead of a pedestal.
        b(3,4,27,25,1);b(2,3,27,24,9);b(2,3,25,23,8)
        l(3,3,26,3,10);l(2,4,2,24,10);b(27,5,2,21,9)
        for y in (8,11,14,17):b(28,y,1,2,11)
        b(4,5,22,17,9);b(5,6,20,15,1)
        # Copper raster bars and a wireframe pyramid on the CRT.
        for y,col in ((8,7),(9,6),(10,7),(14,2),(15,4),(16,2),(19,5)):
            b(6,y,18,1,col)
        l(15,7,9,19,3);l(15,7,21,19,3);l(9,19,21,19,3)
        l(15,7,15,19,2);l(12,13,18,13,3);l(10,17,20,17,2)
        # A tiny blue/red badge, recessed controls and a green power light.
        b(5,24,4,1,2);b(5,23,1,1,7)
        b(16,23,2,2,11);b(20,23,2,2,11);b(24,24,1,1,5)
        b(4,27,22,2,9);l(5,27,25,27,8)
    elif name=='Apps':
        f(2,3,19,17);f(9,10,20,18);b(11,12,16,3,4)
        for y in (18,23):
            for x in (12,18,24):b(x,y,2,2,3)
    elif name=='Arcade':
        f(7,2,18,24);b(9,4,14,3,4);b(9,9,14,10,1)
        for x,y in ((12,12),(19,12),(10,14),(14,14),(17,14),(21,14),(12,16),(19,16)):b(x,y,2,2,5)
        b(5,22,22,6,1);b(6,23,20,2,2);b(10,22,2,2,3);b(21,23,2,1,7);b(8,28,16,2,2)
    elif name=='Music':
        f(3,5,25,23);b(5,7,21,18,1);b(18,8,3,13,4);b(11,11,3,13,4)
        l(12,10,20,7,3);l(12,12,20,9,4);b(8,21,6,4,3);b(15,18,6,4,3)
    elif name=='SadMusic':
        f(3,5,25,23);b(5,7,21,18,4);b(8,11,3,3,1);b(21,11,3,3,1)
        b(12,18,8,2,1);b(10,20,2,2,1);b(20,20,2,2,1)
        b(18,1,3,7,2);l(19,1,25,0,3);b(15,6,6,3,3)
    elif name=='Graphics':
        f(3,4,25,24,3);b(5,6,21,19,2);b(6,7,5,5,6)
        for i in range(10):l(5+i,24-i,24,24-i,4 if i<5 else 5)
        l(24,3,11,24,1);l(25,3,12,24,6);l(26,4,13,24,3);b(10,25,3,3,7)
    elif name=='Code':
        c.window();l(11,14,7,18,4);l(7,18,11,22,4);l(21,14,25,18,4);l(25,18,21,22,4);l(18,13,15,23,5)
    elif name=='Movies':
        f(2,8,27,20);b(4,10,23,15,1)
        for x in range(4,26,5):b(x,10,2,2,3);b(x,23,2,2,3)
        for i in range(7):b(12+i,14+i//2,1,8-i,4)
        b(2,4,27,4,2)
        for x in (4,12,20):l(x,4,x+3,7,3)
    elif name=='Books':
        for x,y,col in ((3,5,2),(12,3,4),(21,7,5)):
            f(x,y,7,24-y,col);l(x+2,y+3,x+4,y+3,3);l(x+2,21,x+4,21,3)
        b(2,27,28,2,2);l(3,27,28,27,3)
    elif name=='Disks':
        f(8,3,20,24,4);f(3,8,20,21,2);b(6,10,13,7,3);b(14,11,3,5,1)
        b(6,20,14,8,1);b(8,22,10,6,3);l(9,24,16,24,2);b(5,11,1,4,6)
    elif name=='Network':
        for x,y in ((10,2),(1,18),(20,18)):
            f(x,y,10,8);b(x+2,y+2,6,3,4);b(x+4,y+8,2,2,2);b(x+2,y+10,6,1,3)
        l(15,13,15,16,3);l(6,16,25,16,3);l(6,16,6,17,3);l(25,16,25,17,3)
    elif name=='Utilities':
        f(3,11,25,17);b(10,6,11,6,1);b(12,8,7,3,2);b(4,13,23,5,4)
        b(13,16,5,5,1);b(14,17,3,3,6);l(6,23,24,23,3)
    elif name=='Favourites':
        f(2,9,27,19);b(3,5,12,6,1);b(4,6,10,4,4);b(4,12,23,14,2)
        for y,x,w in ((11,15,2),(12,14,4),(13,14,4),(14,9,14),(15,10,12),(16,11,10),(17,12,8),(18,12,8),(19,11,10),(20,11,3),(20,18,3)):
            b(x,y,w,1,6)
        b(15,14,2,3,3)
    elif name=='Aminet':
        f(3,4,25,24);b(5,6,21,17,1)
        for y in (9,14,19):
            b(7,y,3,2,5);b(12,y,11,1,3);b(12,y+2,8,1,2)
        b(11,25,9,2,4);b(14,27,3,3,2);b(9,29,13,1,3)
    elif name=='Downloads':
        f(3,21,25,8);b(5,22,21,4,1);b(23,24,2,1,5)
        b(13,2,6,14,4);l(13,2,17,2,3)
        for i in range(7):b(7+i,13+i,18-i*2,1,4)
        l(8,13,15,20,3)
    elif name=='Search':
        f(4,3,17,18,4);b(6,5,13,14,1);b(8,6,9,2,3);b(7,8,2,8,2)
        for i in range(10):b(18+i,18+i,4,4,1);b(19+i,18+i,2,3,6)
    elif name=='Radio':
        f(2,10,27,19);b(4,12,23,4,1);b(6,13,13,1,5)
        f(5,19,10,7,1);b(7,21,6,3,2);b(22,20,4,4,6)
        l(9,9,23,1,1);l(10,9,24,1,3);b(21,13,4,2,4)
    elif name=='Feeds':
        f(3,4,25,24,2);b(5,6,21,19,1);b(7,21,4,4,6)
        for d in (9,16):
            for a in range(d):
                y=24-a;xx=8+int((d*d-a*a)**.5)
                b(xx,y,2,2,6)
    elif name=='Printer':
        f(7,2,18,13,3);l(10,5,21,5,2);l(10,8,20,8,2)
        f(2,12,28,14);b(4,14,24,4,4);b(24,15,2,2,5)
        f(7,20,18,10,3);l(10,23,21,23,2);l(10,26,20,26,2)
    elif name=='Backup':
        f(3,15,26,14);b(5,17,22,4,4);b(24,24,2,2,5)
        b(12,3,7,14,5)
        for i in range(7):b(6+i,9-i,19-i*2,1,5)
        l(7,8,15,1,3)
    elif name=='Calendar':
        f(3,5,26,24,3);b(5,7,22,5,7)
        for x in (8,21):b(x,2,3,7,1);b(x,3,2,4,2)
        for y in (15,20,25):
            for x in (7,13,19,24):b(x,y,3,2,2)
        b(12,19,5,5,4);b(13,20,3,3,1)
    elif name=='Terminal':
        c.window();l(7,14,11,18,3);l(11,18,7,22,3);b(14,21,8,2,5)
    elif name=='DiskSpace':
        f(3,4,26,24);b(5,6,22,5,1);b(7,7,13,2,3)
        for y,w,col in ((15,16,4),(20,11,5),(25,6,6)):
            b(6,y,20,2,1);b(6,y,w,2,col)
    elif name=='Mail':
        f(2,8,28,20,3);l(4,10,16,20,2);l(16,20,27,10,2)
        l(4,25,11,18,4);l(27,25,21,18,4);b(23,4,6,7,7);b(25,6,2,3,3)
    elif name=='Camera':
        f(2,9,28,19);b(5,11,22,14,4);b(7,5,10,5,1);b(8,6,8,4,3)
        f(10,13,13,12,1);b(12,15,9,8,2);b(14,16,5,5,1);b(14,16,2,2,3)
        b(24,12,3,3,3)
    else: raise ValueError(name)
    return c.p
def chunk(kind,data):return kind+struct.pack('>I',len(data))+data+(b'\0' if len(data)%2 else b'')
def ilbm(p):
    bmhd=struct.pack('>HHhhBBBBHBBhh',32,32,0,0,4,2,0,0,0,1,1,32,32)
    body=bytearray()
    for row in p:
        for plane in range(4):
            body.extend(sum(((row[x+i]>>plane)&1)<<(7-i) for i in range(8)) for x in range(0,32,8))
    data=b'ILBM'+chunk(b'BMHD',bmhd)+chunk(b'CMAP',bytes(v for rgb in PALETTE for v in rgb))+chunk(b'BODY',bytes(body))
    return b'FORM'+struct.pack('>I',len(data))+data
def png(p):
    def ch(k,d):return struct.pack('>I',len(d))+k+d+struct.pack('>I',zlib.crc32(k+d)&0xffffffff)
    data=b''.join(b'\0'+bytes(v for n in row for v in (*PALETTE[n],255 if n else 0)) for row in p)
    return b'\x89PNG\r\n\x1a\n'+ch(b'IHDR',struct.pack('>IIBBBBB',32,32,8,6,0,0,0))+ch(b'IDAT',zlib.compress(data))+ch(b'IEND',b'')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--preview',type=Path);args=ap.parse_args()
    for name in NAMES:
        pixels=artwork(name);(ROOT/'Icons'/f'{name}.iff').write_bytes(ilbm(pixels));(ROOT/'Icons'/f'{name}.png').write_bytes(png(pixels))
    if args.preview:
        from PIL import Image,ImageDraw,ImageFont
        sheet=Image.new('RGB',(720,((len(NAMES)+3)//4)*156+12),(37,44,56));d=ImageDraw.Draw(sheet)
        font=ImageFont.truetype('/System/Library/Fonts/Menlo.ttc',15)
        for i,name in enumerate(NAMES):
            x=30+(i%4)*178;y=20+(i//4)*156
            icon=Image.open(ROOT/'Icons'/f'{name}.png').resize((96,96),Image.Resampling.NEAREST)
            sheet.paste(icon,(x+26,y),icon);d.text((x+74,y+108),name,font=font,fill=(232,239,245),anchor='mt')
        args.preview.parent.mkdir(parents=True,exist_ok=True);sheet.save(args.preview)
    print('Generated',len(NAMES),'collection icons (32px ILBM and PNG).')
if __name__=='__main__':main()
