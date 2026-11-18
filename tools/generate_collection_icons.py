#!/usr/bin/env python3
"""Create NovaDock's 32-pixel collection artwork as transparent ILBM and PNG."""
import argparse
import struct
import zlib
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
NAMES = ('WHDLoad', 'Demos', 'Apps', 'Arcade', 'Music', 'Graphics', 'Code', 'Movies', 'Books', 'Disks', 'Network', 'Utilities', 'Favourites', 'Aminet', 'Downloads', 'Search', 'Radio', 'Feeds', 'Printer', 'Backup', 'Calendar', 'Terminal', 'DiskSpace', 'Mail', 'Camera', 'Home', 'Thermometer', 'Sensor', 'Door', 'Garage', 'FrontDoor', 'Bulb', 'LightStrip', 'AC', 'Heater', 'Fan', 'TV', 'Scene', 'LightGroup', 'ChristmasLights', 'Fridge', 'Freezer', 'BeerKeg', 'Dishwasher', 'Washer', 'Dryer', 'SousVide', 'CeilingLight', 'FloorLamp', 'ShelfLamp', 'Monitor', 'AppleTV', 'Receiver', 'HomePod', 'Moisture', 'Leak', 'WindowSensor', 'DoorSensor', 'RobotVacuum', 'RobotMower', 'FireAlarm', 'SecurityCamera', 'Firewall', 'Router', 'NetworkSwitch', 'WiFiAP', 'HueBridge', 'HueSyncBox', 'TheaterPC', 'Thermostat')
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
    if name=='LightGroup':
        for x,y in ((2,10),(12,3),(22,10)):
            b(x+2,y,5,2,3);b(x,y+2,9,8,6);b(x+2,y+3,4,5,3);b(x+3,y+10,4,4,2)
        l(6,27,26,27,4);l(16,20,16,27,4)
    elif name=='ChristmasLights':
        for i in range(29):l(1+i,8+(i%10)//4,2+i,8+((i+1)%10)//4,2)
        for x,y,col in ((3,10,7),(10,11,5),(17,10,6),(24,11,4)):
            b(x+2,y,2,4,1);b(x,y+4,6,7,col);b(x+1,y+5,2,3,3)
        l(5,24,7,26,6);l(25,24,23,26,6)
    elif name=='Fridge':
        f(6,2,20,28,3);b(8,4,16,9,4);b(8,15,16,13,3);b(9,6,2,5,2);b(9,17,2,7,2)
        l(8,13,23,13,1);b(8,30,3,1,1);b(22,30,3,1,1)
    elif name=='Freezer':
        f(3,13,26,16,3);b(5,16,22,3,4);b(14,17,5,2,1);l(3,12,10,3,2);l(10,3,30,6,3);l(30,6,28,12,2);l(4,12,28,12,3)
        l(14,21,20,27,2);l(20,21,14,27,2);l(17,20,17,28,4)
    elif name=='BeerKeg':
        b(8,3,17,2,2);f(6,5,21,23,2);b(8,8,17,4,3);b(8,21,17,3,3);b(13,13,7,5,6)
        l(23,8,30,8,1);b(28,9,3,5,2);b(2,19,4,10,6);b(1,17,6,3,3)
    elif name in ('Washer','Dryer'):
        f(5,2,23,28,3);b(7,5,19,4,2);b(8,6,7,2,4);b(22,5,3,3,6)
        b(11,12,12,2,2);b(8,15,18,10,2);b(11,26,12,2,2)
        b(11,14,12,12,1);b(13,15,8,9,4 if name=='Washer' else 6)
        if name=='Washer':l(12,22,21,18,3);l(12,24,22,20,3)
        else:l(14,23,17,17,7);l(18,23,21,17,7)
    elif name=='Dishwasher':
        f(5,2,23,28,3);b(7,4,19,5,2);b(9,5,6,2,5);b(23,5,2,2,6);b(8,12,17,15,4)
        for x in (11,16,21):l(x,16,x,25,3);l(x+1,16,x+1,25,2)
        b(11,12,12,2,1)
    elif name=='SousVide':
        f(3,15,25,14,2);b(5,18,21,9,4);b(0,18,3,4,3);b(28,18,3,4,3)
        f(18,2,9,21,3);b(20,5,5,5,1);b(21,6,3,2,5);b(21,13,3,9,2)
        for x in (7,12):l(x,17,x+3,17,3)
    elif name=='CeilingLight':
        b(15,1,2,9,2);b(9,10,14,3,1);b(5,13,23,4,2);b(3,17,27,3,3);b(9,20,14,2,6)
        for x,y in ((7,24),(15,25),(24,24)):l(x,y,x,y+4,6)
    elif name=='FloorLamp':
        b(13,10,4,19,2);b(5,29,21,2,3);b(10,2,13,3,3)
        for y in range(5,16):b(10-(y-5)//3,y,13+(y-5)//3*2,1,6)
        b(6,16,21,2,1);b(9,17,15,1,3)
    elif name=='ShelfLamp':
        f(2,24,28,5,2);b(6,22,12,2,3);b(11,11,3,11,2);l(12,11,20,5,3);l(13,12,21,6,2)
        b(19,4,8,3,1);b(17,7,13,5,6);l(20,14,17,20,6);l(25,14,24,20,6)
    elif name in ('Monitor','TheaterPC'):
        f(2,3,28,20,2);b(5,6,22,14,1)
        if name=='Monitor':
            b(7,8,8,5,4);b(17,8,8,2,5);b(17,12,8,2,3);b(7,15,18,2,2);b(14,23,4,4,2);b(8,28,17,2,3)
        else:
            for i in range(7):b(11+i,8+i//2,1,9-i,4)
            f(2,25,28,5,3);b(5,27,12,1,1);b(26,27,2,2,5)
    elif name=='AppleTV':
        b(3,10,22,17,1);b(5,8,18,2,2);b(6,12,16,9,2);b(11,15,7,5,3);b(17,14,2,2,5);b(21,24,2,1,3)
        f(27,3,3,23,3);b(28,5,1,4,1);b(28,14,1,2,1)
    elif name=='Receiver':
        f(1,9,29,19,2);b(4,12,15,6,1);b(6,14,11,2,4);b(23,12,5,6,3);b(24,13,3,4,1)
        for x in (5,9,13,17):b(x,22,2,2,3)
        b(3,29,4,2,1);b(25,29,4,2,1)
    elif name=='HomePod':
        b(10,3,12,2,3);b(7,6,18,21,3);b(9,28,14,2,2);b(9,4,14,3,4);b(12,5,8,1,7)
        for y in range(10,26,3):
            for x in range(9,24,3):b(x,y,1,1,2)
    elif name in ('Moisture','Leak'):
        if name=='Moisture':
            f(8,3,17,17,3);b(11,7,10,5,4);b(13,9,6,1,5);b(11,20,3,9,2);b(20,20,3,9,2);l(7,28,26,28,6)
        else:
            f(3,19,26,10,3);b(6,22,20,4,2);b(23,23,2,1,5)
            for x,y in ((9,2),(19,7)):l(x+2,y,x,y+5,4);l(x,y+5,x+4,y+5,4);l(x+4,y+5,x+2,y,3)
    elif name=='WindowSensor':
        f(2,3,22,26,3);b(5,6,16,20,4);b(12,6,2,20,3);b(5,15,16,2,3)
        f(25,8,5,14,2);b(26,10,3,3,5)
    elif name=='DoorSensor':
        f(3,2,20,28,3);b(6,5,14,23,2);b(17,18,2,2,6);f(18,6,5,9,3);f(25,6,5,9,3);b(19,8,2,2,5)
    elif name in ('RobotVacuum','RobotMower'):
        b(8,5,16,2,2);b(4,9,24,15,1);b(7,6,18,21,2);b(5,10,22,13,4 if name=='RobotVacuum' else 5)
        b(11,7,10,3,3);b(12,13,8,5,1);b(14,14,4,2,3)
        if name=='RobotVacuum':l(5,24,1,28,2);l(25,24,30,28,2)
        else:
            for x in (2,27):b(x,12,3,11,1)
            for x in (5,12,19,26):l(x,29,x+2,26,5)
    elif name=='FireAlarm':
        b(9,3,14,2,3);b(5,7,22,17,3);b(9,25,14,2,2);b(10,9,12,12,2);b(14,12,4,5,7)
        for x in (7,11,15,19,23):b(x,22,1,2,1)
        l(2,10,0,8,7);l(29,10,31,8,7)
    elif name=='SecurityCamera':
        f(4,5,22,12,3);b(21,7,5,7,1);b(6,7,10,2,2);l(15,18,20,23,2);b(21,22,8,5,2);b(28,19,2,11,3)
        l(2,4,27,4,2)
    elif name=='Firewall':
        f(3,3,26,27,2)
        for y in range(6,27,6):
            for x in range(6 if y%12 else 10,26,9):b(x,y,7,4,7)
        b(13,9,9,14,1);l(14,10,20,10,3);l(14,10,14,18,3);l(20,10,20,18,3);l(14,18,17,21,3);l(20,18,17,21,3)
    elif name=='Router':
        f(2,18,28,11,3);b(4,21,24,5,2);b(6,22,2,2,5);b(11,22,2,2,4);b(16,22,2,2,4)
        l(5,17,3,3,2);l(26,17,28,3,2);b(14,6,5,2,4);l(12,10,10,12,4);l(21,10,23,12,4)
    elif name=='NetworkSwitch':
        f(1,11,29,17,2);b(3,13,25,3,3)
        for x in (4,10,16,22):b(x,19,5,5,1);b(x+1,20,3,2,4);b(x+1,26,2,1,5)
        l(5,6,25,6,2);l(15,6,15,10,3)
    elif name=='WiFiAP':
        b(10,11,12,2,2);b(6,15,20,11,3);b(9,27,14,2,2);b(12,18,8,2,4);b(14,23,4,1,5)
        for x,y,x2,y2 in ((8,7,5,10),(24,7,27,10),(12,4,20,4)):l(x,y,x2,y2,4)
    elif name=='HueBridge':
        f(4,6,24,23,3);b(11,10,10,11,2);b(13,12,6,7,4)
        for x in (9,15,21):b(x,25,2,1,4)
        b(12,3,9,3,2)
    elif name=='HueSyncBox':
        f(2,12,28,15,1);b(4,14,24,9,2)
        for x,col in ((6,7),(12,5),(18,4),(24,6)):b(x,18,2,3,col)
        l(3,6,28,6,4);l(3,6,3,10,7);l(28,6,28,10,6)
    elif name=='Thermostat':
        f(4,3,25,26,3);b(7,7,19,14,1);b(9,10,6,7,4);b(17,10,6,7,4);b(10,12,3,3,1);b(18,12,3,3,1)
        b(11,24,3,1,2);b(21,23,1,3,2);b(20,24,3,1,2)
    elif name=='Home':
        l(2,15,16,3,1);l(16,3,30,15,1);l(3,15,16,4,3);l(16,4,29,15,3)
        f(6,13,20,16,2);b(14,20,6,9,1);b(15,21,4,8,4);b(9,17,4,4,3);b(22,8,3,7,2)
    elif name=='Thermometer':
        f(12,2,8,23,3);b(15,6,2,16,7);b(10,21,12,9,1);b(11,21,10,8,3);b(13,23,6,4,7)
        for y in (6,11,16):l(21,y,25,y,2)
    elif name=='Sensor':
        f(8,7,16,20,3);b(11,10,10,11,4);b(15,13,3,3,5);b(13,24,6,1,2)
        l(5,5,2,8,2);l(2,8,2,22,2);l(27,5,30,8,2);l(30,8,30,22,2)
    elif name in ('Door','FrontDoor'):
        f(6,2,21,28,3);b(9,5,15,24,2);b(11,7,11,9,4);b(11,18,11,8,1);b(20,19,2,2,6)
        if name=='FrontDoor':b(1,13,4,9,1);b(2,14,2,3,6);b(2,19,2,1,3)
    elif name=='Garage':
        l(1,11,16,2,1);l(16,2,30,11,1);l(2,11,16,3,3);l(16,3,29,11,3);f(4,11,25,19,2)
        b(7,14,19,14,3)
        for y in (17,21,25):l(8,y,24,y,2)
        b(14,26,5,1,1)
    elif name=='Bulb':
        b(10,4,12,2,1);b(7,7,18,13,1);b(9,5,14,16,6);b(11,6,9,13,3)
        l(12,17,14,22,2);l(20,17,18,22,2);f(12,22,8,6,2);b(14,28,4,2,1)
        for x,y,x2,y2 in ((3,8,1,6),(28,8,30,6),(16,1,16,0),(2,17,0,17),(29,17,31,17)):l(x,y,x2,y2,6)
    elif name=='LightStrip':
        f(2,10,28,12,3);b(4,12,24,8,2)
        for x,col in ((5,6),(11,7),(17,5),(23,4)):b(x,14,3,4,col)
        l(29,16,31,16,2);b(1,24,15,2,4)
    elif name=='AC':
        f(2,5,28,14,3);b(5,14,22,3,2);b(24,8,3,2,5)
        for x in (8,15,22):l(x,21,x-2,25,4);l(x-2,25,x,29,4)
    elif name=='Heater':
        f(4,5,24,22,3)
        for x in (8,13,18,23):b(x,8,2,16,2)
        b(6,27,3,3,1);b(23,27,3,3,1);b(24,8,2,3,7)
        for x in (9,17):l(x,3,x-1,1,7)
    elif name=='Fan':
        f(4,2,24,24,2);b(7,5,18,18,1);b(14,12,5,5,3)
        for x,y,w,h in ((15,5,6,7),(7,13,7,6),(18,17,6,6)):b(x,y,w,h,4)
        b(15,14,3,3,6);b(14,26,4,3,2);b(7,29,18,2,3)
    elif name=='TV':
        f(2,4,28,22,2);b(5,7,22,15,1);b(7,9,18,11,4);l(8,19,22,10,3)
        b(8,27,3,3,1);b(22,27,3,3,1);b(25,24,2,1,5)
    elif name=='Scene':
        f(3,4,25,24,2);b(5,6,21,18,1)
        for x in range(8):b(8+x,10+x//2,1,10-x,5)
        b(20,9,2,2,6);b(22,14,2,2,6);b(18,19,2,2,6)
    elif name=='WHDLoad':
        f(3,18,25,11);b(5,20,21,6,2);b(8,22,5,2,3);b(10,20,1,6,3)
        b(21,21,3,3,7);b(14,7,4,13,1);b(15,7,2,13,3)
        b(12,3,8,6,1);b(13,2,6,7,7);b(14,3,2,2,3)
        l(15,24,16,25,5);l(16,25,17,24,5);l(17,24,18,25,5);l(18,25,19,24,5)
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
