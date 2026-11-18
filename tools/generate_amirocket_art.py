#!/usr/bin/env python3
"""Generate original indexed rocket/chat status artwork for the dock plugin."""
import argparse
from pathlib import Path
from pack_plugin_art import build_bank
ROOT=Path(__file__).resolve().parents[1]
def bank():
    frames=[]
    for state in range(4):
        p=[[0]*32 for _ in range(32)]
        def rect(x,y,w,h,c):
            for yy in range(y,y+h):
                for xx in range(x,x+w):
                    if 0<=xx<32 and 0<=yy<32:p[yy][xx]=c
        # Chat bubble with a small upward rocket, in the shared pixel style.
        rect(2,6,27,21,1);rect(3,7,25,18,2);rect(5,9,21,14,1)
        rect(7,25,6,4,2);rect(7,29,3,1,2)
        for y in range(3,21):
            half=min(4,(y-3)//2+1);rect(17-half,y,half*2+1,1,3 if state!=0 else 2)
        rect(14,8,7,8,1);rect(15,9,5,5,4 if state!=0 else 2)
        rect(11,17,3,6,2);rect(21,17,3,6,2)
        rect(15,21,5,2,2)
        if state in (1,2):
            rect(16,23,3,5,5);rect(17,28,1,2,4)
        if state==2:
            rect(24,1,7,7,1);rect(25,2,5,5,5);rect(27,3,1,3,3)
        if state==3:
            rect(23,1,8,8,1);rect(24,2,6,6,3);rect(26,3,1,3,2);rect(27,5,2,1,2)
        if state==0:
            rect(23,20,8,10,1);rect(25,23,1,1,2);rect(29,23,1,1,2);rect(26,26,3,1,2);rect(25,27,1,1,2);rect(29,27,1,1,2)
        frames.append(dict(name='Rocket',state=state,pixels=p))
    return build_bank(frames,[(0,0,0),(36,44,57),(124,157,186),(232,239,245),(143,194,205),(170,194,140)]+[(0,0,0)]*10,[0,1,2,3,4,5]+[0]*10)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    target=ROOT/'Plugins/AmiRocket.assets/Artwork.ndart';data=bank()
    if args.check:
        if not target.is_file() or target.read_bytes()!=data:raise SystemExit('AmiRocket artwork needs regeneration')
    else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    print('AmiRocket: four plugin-owned rocket status frames are current.')
if __name__=='__main__':main()
