#!/usr/bin/env python3
"""Theme-role banks for Preferences and the idle MusicPlayer tile."""
import argparse
from pathlib import Path
from generate_info_icons import prefs_art,music_art
from pack_plugin_art import build_bank
from plugin_art_colours import colourful_palette
from artifact_compare import same_artifact
ROOT=Path(__file__).resolve().parents[1]
def generated():
 for program,key,draw in (("Preferences","preferences",prefs_art),("MusicPlayer","music",music_art)):
  c=draw();pixels=[[0]*32 for _ in range(32)];scale=min(30/c.width,30/c.height);w,h=round(c.width*scale),round(c.height*scale)
  for y in range(h):
   for x in range(w):pixels[(32-h)//2+y][(32-w)//2+x]=c.pixels[y*c.height//h][x*c.width//w]
  yield program,build_bank([dict(name=key,state=0,pixels=pixels)],colourful_palette(program),[0,1,2,3]+[0]*12)
def main():
 p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');args=p.parse_args()
 for name,data in generated():
  path=ROOT/'Plugins'/(name+'.assets')/'Artwork.ndart'
  if args.check:
   assert path.is_file() and same_artifact(path.read_bytes(),data),str(path)+' needs regeneration'
  else:path.parent.mkdir(exist_ok=True);path.write_bytes(data)
 print('Preferences and idle MusicPlayer theme-role artwork is current.')
if __name__=='__main__':main()
