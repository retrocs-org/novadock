#!/usr/bin/env python3
"""Plugin-owned artwork for BENCH, QR, MOON and COLOUR in the dock palette."""
import argparse
import math
from pathlib import Path
from pack_plugin_art import build_bank
from plugin_art_colours import colourful_palette
from artifact_compare import same_artifact
ROOT=Path(__file__).resolve().parents[1]
ROLES=[0,1,2,3,4,5]+[0]*10

def pixels(kind,state):
 p=[[0]*32 for _ in range(32)]
 def box(x,y,w,h,c):
  for yy in range(y,y+h):
   for xx in range(x,x+w):p[yy][xx]=c
 if kind=='moon':
  phase=state/16;threshold=math.cos(2*math.pi*phase)
  for y in range(3,30):
   for x in range(3,30):
    dx=x-16;dy=y-16;r2=dx*dx+dy*dy
    if r2<=169:p[y][x]=1
    if r2<=144:
     r=math.sqrt(max(0,144-dy*dy));lit=(dx>=threshold*r if phase<=0.5 else dx<=-threshold*r) and state!=0
     p[y][x]=3 if lit else 2
     if lit and ((x-12)**2+(y-11)**2<5 or (x-21)**2+(y-19)**2<8):p[y][x]=4
 elif kind=='bench':
  box(3,4,26,22,1);box(5,6,22,17,2);box(7,8,18,13,1);box(14,26,4,3,2);box(9,29,14,2,2)
  for i,h in enumerate((5,11,7) if state else (9,6,12)):box(9+i*5,21-h,3,h,4 if i!=1 else 5)
 elif kind=='agenda':
  box(4,5,24,25,1);box(6,7,20,21,3);box(6,7,20,5,6 if state else 2)
  for x in (9,21):box(x,2,3,8,2)
  for y in (16,21,26):
   box(8,y,3,2,5);box(14,y,10,2,2)
 elif kind=='pictures':
  box(2,5,28,23,1);box(4,7,24,19,2);box(6,9,20,15,3)
  if not state:
   box(20,10,4,4,5)
   for y in range(14,24):
    for x in range(6,26):
     if y>22-abs(x-12)//2:p[y][x]=4
  else:box(10,15,12,3,6)
 elif kind=='qr':
  box(2,2,28,28,1);box(3,3,26,26,3)
  for x,y in ((5,5),(20,5),(5,20)):
   box(x,y,7,7,1);box(x+1,y+1,5,5,3);box(x+2,y+2,3,3,1)
  for y in range(14,27):
   for x in range(14,27):
    if (x*11+y*7+(state*3))%5<2:p[y][x]=1
 else:
  for y in range(4,29):
   for x in range(3,30):
    if ((x-16)/13)**2+((y-16)/12)**2<=1:p[y][x]=1
    if ((x-16)/11)**2+((y-16)/10)**2<=1:p[y][x]=3
  for x,y,c in ((8,9,6),(16,7,5),(23,12,8),(9,20,4)):
   box(x,y,4,4,c)
  box(19,20,6,6,1);box(20,21,4,4,0);box(25,4,3,14,2);box(26,2,2,4,4)
 return p

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args()
 from generate_info_icons import Canvas,disk_object
 for program,kind in [('Bench','bench'),('QR','qr'),('Moon','moon'),('Colour','colour'),('Agenda','agenda'),('Pictures','pictures')]:
  frames=[dict(name=kind,state=i,pixels=pixels(kind,i)) for i in range(16 if kind=='moon' else 2)]
  bank=build_bank(frames,colourful_palette(kind),ROLES);canvas=Canvas(32,32)
  for y,row in enumerate(frames[0]['pixels']):
   for x,c in enumerate(row):canvas.pixel(x,y,min(c,3))
  for target,data in [(ROOT/'Plugins'/(program+'.assets')/'Artwork.ndart',bank),(ROOT/'Plugins'/(program+'.info'),disk_object(canvas))]:
   if args.check:
    if not target.is_file() or not same_artifact(target.read_bytes(),data):raise SystemExit('Regenerate '+str(target))
   else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
 print('0.5 plugin artwork and Workbench icons are current.')
if __name__=='__main__':main()
