#!/usr/bin/env python3
"""Generate plugin-owned notification and shortcut artwork."""
import argparse
from pathlib import Path
from pack_plugin_art import build_bank
from plugin_art_colours import colourful_palette
from artifact_compare import same_artifact
ROOT=Path(__file__).resolve().parents[1]
def bank(name):
 frames=[]
 for state in range(2):
  p=[[0]*32 for _ in range(32)]
  def box(x,y,w,h,c):
   for yy in range(y,y+h):
    for xx in range(x,x+w):p[yy][xx]=c
  if name=='alerts':
   box(6,6,20,19,1);box(7,7,18,16,2);box(9,9,14,12,3);box(12,4,8,3,2);box(5,23,22,3,2);box(13,27,6,2,2)
   if state:box(22,2,8,8,1);box(23,3,6,6,5);box(25,4,2,3,1);box(25,8,2,1,1)
  elif name in ('calc','tooltypes'):
   box(6,3,21,27,1);box(7,4,19,24,2);box(9,6,15,6,1);box(10,7,13,4,4)
   if name=='calc':
    for yy in (15,20,25):
     for xx in (10,15,20):box(xx,yy,3,2,3)
   else:
    for yy in (15,19,23):box(10,yy,12,2,3)
    box(21,16,4,14,1);box(19,18,8,3,5);box(20,17,2,2,5);box(24,17,2,2,5);box(22,22,2,6,2)
  elif name in ('watch','logs'):
   if name=='watch':
    box(2,9,28,19,1);box(3,10,26,16,2);box(4,6,11,5,2);box(4,12,24,13,4)
    box(8,15,16,7,1);box(6,17,20,3,1);box(12,14,8,9,3);box(15,16,3,5,2)
   else:
    box(6,3,21,27,1);box(7,4,19,24,3);box(9,7,14,2,2)
    for yy in (12,17,22):box(10,yy,12,2,2);box(8,yy,1,2,5)
   if state:
    box(20,18,11,12,1);box(22,20,2,2,3);box(27,20,2,2,3);box(24,24,3,1,3);box(22,26,2,1,3);box(27,26,2,1,3)
  else:
   box(3,8,26,21,1);box(4,9,24,18,2);box(5,10,22,5,3);box(5,17,22,4,3);box(5,23,22,3,3)
   box(7,4,7,10,2);box(8,5,5,8,4);box(17,2,7,12,2);box(18,3,5,10,5)
   if state:box(18,17,12,13,1);box(20,19,2,2,2);box(26,19,2,2,2);box(22,23,4,1,2);box(20,25,2,1,2);box(26,25,2,1,2)
  frames.append(dict(name=name,state=state,pixels=p))
 return build_bank(frames,colourful_palette(name),[0,1,2,3,4,5]+[0]*10)
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args()
 for program,name in [('Shelf','shelf'),('Watch','watch'),('Logs','logs'),('Calc','calc'),('ToolTypes','tooltypes')]:
  target=ROOT/'Plugins'/(program+'.assets')/'Artwork.ndart';data=bank(name)
  if args.check:
   if not target.is_file() or not same_artifact(target.read_bytes(),data):raise SystemExit('Regenerate '+str(target))
  else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
 print('Utility plugin artwork is current.')
if __name__=='__main__':main()
