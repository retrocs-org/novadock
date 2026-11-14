#!/usr/bin/env python3
"""Independently decode every pixel from the production 24-plane ILBM writer."""
from pathlib import Path
import os,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
harness=r'''#include <assert.h>
#include "nd_shotfile.h"
int main(void){struct NDShotWriter w;unsigned char rgb[45*3];unsigned int x,y;
assert(nd_shot_begin(&w,"rgb.iff",45,21,24,NULL,0,0));
for(y=0;y<21;++y){for(x=0;x<45;++x){rgb[x*3]=(unsigned char)(x*7+y);rgb[x*3+1]=(unsigned char)(x+y*11);rgb[x*3+2]=(unsigned char)(x*13+y*3);}assert(nd_shot_row(&w,rgb));}
assert(nd_shot_end(&w,"rgb.iff"));assert(!nd_shot_begin(&w,"bad.iff",45,21,32,NULL,0,0));return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(harness)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations','-I'+str(ROOT/'plugin_src/screenshot'),str(p/'check.c'),str(ROOT/'plugin_src/screenshot/nd_shotfile.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],cwd=p,check=True)
 data=(p/'rgb.iff').read_bytes();assert data[:4]==b'FORM' and data[8:12]==b'ILBM'
 assert struct.unpack('>I',data[4:8])[0]==len(data)-8
 chunks={};at=12
 while at<len(data):
  tag=data[at:at+4];size=struct.unpack('>I',data[at+4:at+8])[0];chunks[tag]=data[at+8:at+8+size];at+=8+size+(size&1)
 assert chunks[b'BMHD'][8]==24 and b'CMAP' not in chunks
 body=chunks[b'BODY'];at=0
 for y in range(21):
  planes=[]
  for plane in range(24):
   row=bytearray()
   while len(row)<6:
    control=body[at];at+=1
    if control<128:
     row+=body[at:at+control+1];at+=control+1
    elif control>128:
     row+=bytes([body[at]])*(257-control);at+=1
   assert len(row)==6;planes.append(row)
  for x in range(45):
   rgb=[0,0,0]
   for plane in range(24):
    if planes[plane][x//8]&(128>>(x%8)):rgb[plane//8]|=1<<(plane%8)
   assert rgb==[(x*7+y)&255,(x+y*11)&255,(x*13+y*3)&255],(x,y,rgb)
 assert at==len(body)
print('True-colour ILBM: all RGB bits, odd row widths, chunk bounds and compressed pixel round trips passed.')
