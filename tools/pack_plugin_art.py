#!/usr/bin/env python3
"""Build a portable 32x32 plugin artwork bank from indexed JSON pixel rows."""
import argparse,json,struct
from pathlib import Path

def rectangles(pixels):
    result=[]
    for y,row in enumerate(pixels):
        x=0
        while x<32:
            color=row[x];end=x+1
            while end<32 and row[end]==color:end+=1
            if color:result.append((x,y,end-x,1,color))
            x=end
    return result

def build_bank(frames,palette,palette_roles=None):
    if len(palette)!=16 or any(len(c)!=3 or any(type(v)!=int or not 0<=v<=255 for v in c) for c in palette):raise ValueError('Palette must contain sixteen RGB triples')
    if not 1<=len(frames)<=256:raise ValueError('One to 256 frames are supported')
    modern=palette_roles is not None or any(frame.get('flags',0) for frame in frames)
    roles=palette_roles if palette_roles is not None else [0]*16
    if len(roles)!=16 or roles[0]!=0 or any(type(v)!=int or not 0<=v<=5 for v in roles):raise ValueError('Sixteen palette roles must be 0 (RGB) or 1-5 (theme), with transparent index zero')
    table=bytearray();body=bytearray();seen=set();keys={};offset=(72 if modern else 56)+32*len(frames)
    for frame in frames:
        name=frame['name'];state=frame['state'];pixels=frame['pixels']
        if not isinstance(name,str) or not 1<=len(name)<=23 or any(c not in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-' for c in name):raise ValueError('Artwork names use 1-23 ASCII letters, digits, _ or -')
        if type(state)!=int or not 0<=state<=255 or (name,state) in seen:raise ValueError('Duplicate/invalid artwork state')
        key=2166136261
        for char in name.encode():key=((key^char)*16777619)&0xffffffff
        key=key or 1
        if key in keys and keys[key]!=name:raise ValueError('Artwork names have a conflicting wire key')
        keys[key]=name;seen.add((name,state))
        flags=frame.get('flags',0)
        if type(flags)!=int or flags<0 or flags&~15:raise ValueError('Unknown artwork presentation flags')
        if flags&8 and not flags&1:raise ValueError('A live heading requires a value row')
        if len(pixels)!=32 or any(len(row)!=32 or any(type(v)!=int or not 0<=v<=15 for v in row) for row in pixels):raise ValueError('Every frame must be 32x32 palette indexes')
        runs=rectangles(pixels)
        if not runs:raise ValueError('Frames cannot be fully transparent')
        table+=struct.pack('>24sBBHI',name.encode(),state,flags,len(runs),offset+len(body));body+=bytes(v for r in runs for v in r)
    data=b'NDAR\0'+bytes([2 if modern else 1])+struct.pack('>H',len(frames))+bytes(v for c in palette for v in c)+(bytes(roles) if modern else b'')+table+body
    if len(data)>262144:raise ValueError('Artwork bank exceeds 256 KiB')
    return data

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('input',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args()
    doc=json.loads(args.input.read_text());frames=[]
    for frame in doc['frames']:
        frame=dict(frame);frame['pixels']=[[int(v,16) for v in row] if isinstance(row,str) else row for row in frame['pixels']];frames.append(frame)
    data=build_bank(frames,doc['palette'],doc.get('palette_roles'));args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data);print(f'Wrote {args.output} ({len(data)} bytes, {len(frames)} frames)')
if __name__=='__main__':main()
