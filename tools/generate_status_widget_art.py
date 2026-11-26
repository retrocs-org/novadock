#!/usr/bin/env python3
"""State banks for widgets that previously displayed only text in Tiny mode."""
import argparse
from pathlib import Path
from pack_plugin_art import build_bank
from generate_collection_icons import artwork, PALETTE
from artifact_compare import same_artifact
ROOT=Path(__file__).resolve().parents[1]
WIDGETS={'Command':'Terminal','Script':'Code','Mixer':'Radio','Download':'Downloads','System':'DiskSpace'}
def generated():
 for program,icon in WIDGETS.items():
  original=artwork(icon);frames=[]
  for state in range(3):
   pixels=[row[:] for row in original]
   if state:
    # Distinct busy/muted indicator and a readable sad face for failures.
    def box(x,y,w,h,c):
     for yy in range(y,y+h):
      for xx in range(x,x+w):pixels[yy][xx]=c
    box(18,18,14,14,1);box(19,19,12,12,6 if state==1 else 7)
    if state==1:box(22,22,2,6,1);box(26,22,2,6,1)
    else:
     box(21,21,2,2,1);box(27,21,2,2,1)
     box(23,25,4,1,1);box(21,26,2,2,1);box(27,26,2,2,1)
   # A live heading remains visible in Normal, with a value in both sizes.
   frames.append(dict(name=program.lower(),state=state,flags=9,pixels=pixels))
  yield program,build_bank(frames,[list(c) for c in PALETTE],[0,1,2,3,4,5]+[0]*10)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--check',action='store_true');a=p.parse_args()
 for program,data in generated():
  target=ROOT/'Plugins'/(program+'.assets')/'Artwork.ndart'
  if a.check:assert target.is_file() and same_artifact(target.read_bytes(),data),str(target)+' needs regeneration'
  else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
 print('Five text/status widget banks: normal, busy/muted and sad/error artwork is current.')
if __name__=='__main__':main()
