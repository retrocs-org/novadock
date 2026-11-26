#!/usr/bin/env python3
"""Plugin-owned artwork and Workbench icon for AMINOTIS.

States: 0 idle bell, 1 unread (red dot), 2 quiet (blue Z), 3 sad face when
the AmiNotis service is not running or cannot serve the dock inbox."""
import argparse
from pathlib import Path
from pack_plugin_art import build_bank
from artifact_compare import same_artifact
ROOT=Path(__file__).resolve().parents[1]
ROLES=[0,1,2,3,4,5]+[0]*10
MATERIAL=(243,181,51)  # the notification amber previously used by ALERTS

def palette():
 # Same layout as plugin_art_colours.colourful_palette: outline, material,
 # highlight, two details, then RGB-only inks.
 dark=tuple(max(14,c//5) for c in MATERIAL)
 return [(0,0,0),dark,MATERIAL,(250,248,237),(48,173,235),(69,214,139),(240,88,97),
         (250,204,62),(126,105,223)]+[(0,0,0)]*7

def pixels(state):
 p=[[0]*32 for _ in range(32)]
 def box(x,y,w,h,c):
  for yy in range(y,y+h):
   for xx in range(x,x+w):p[yy][xx]=c
 # Bell: knob, dome, flared rim and clapper.
 box(15,2,2,3,1)
 box(12,5,8,2,1);box(10,7,12,4,1);box(9,11,14,8,1);box(7,19,18,3,1);box(5,22,22,3,1)
 box(13,6,6,1,2);box(11,7,10,4,2);box(10,11,12,8,2);box(8,19,16,3,2);box(6,23,20,1,2)
 box(12,9,2,10,3)
 box(14,25,4,3,1);box(15,25,2,2,2)
 if state==1:
  box(21,1,10,10,1);box(22,2,8,8,6);box(23,3,3,2,3)
 elif state==2:
  box(19,17,12,13,1);box(20,18,10,11,4)
  box(22,20,6,2,3);box(26,22,2,2,3);box(24,24,2,2,3);box(22,26,6,2,3)
 elif state==3:
  box(18,17,12,13,1);box(20,19,2,2,2);box(26,19,2,2,2);box(22,23,4,1,2);box(20,25,2,1,2);box(26,25,2,1,2)
 return p

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
 from generate_info_icons import Canvas,disk_object
 frames=[dict(name='aminotis',state=i,pixels=pixels(i)) for i in range(4)]
 bank=build_bank(frames,palette(),ROLES);canvas=Canvas(32,32)
 for y,row in enumerate(frames[0]['pixels']):
  for x,c in enumerate(row):canvas.pixel(x,y,min(c,3))
 for target,data in [(ROOT/'Plugins'/'AmiNotis.assets'/'Artwork.ndart',bank),(ROOT/'Plugins'/'AmiNotis.info',disk_object(canvas,stack=32768))]:
  if args.check:
   if not target.is_file() or not same_artifact(target.read_bytes(),data):raise SystemExit('Regenerate '+str(target))
  else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
 print('AMINOTIS artwork and Workbench icon are current.')
if __name__=='__main__':main()
