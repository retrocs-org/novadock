#!/usr/bin/env python3
"""Build bundled plugin banks from their own portable authoring shapes."""
import argparse,json,os,shlex,subprocess,tempfile
from pathlib import Path
from pack_plugin_art import build_bank
from plugin_art_colours import colourful_palette
from artifact_compare import same_artifact
ROOT=Path(__file__).resolve().parents[1]
# Shape evaluation is a host-side build step; the dock loads only bounded banks.
C=r'''
#include <stdio.h>
#include <string.h>
#include "plugin_src/archive/nd_tile_art.h"
#include "plugin_src/notes/nd_tile_art.h"
#include "plugin_src/trash/nd_tile_art.h"
#include "plugin_src/timer/nd_tile_art.h"
#include "plugin_src/weather/nd_weather_art.h"
#include "plugin_src/common/nd_screen_art.h"
#include "plugin_src/common/nd_places_saver_art.h"
#include "plugin_src/aquarium/nd_aquarium_art.h"
#include "plugin_src/christmas/nd_christmas_art.h"
#include "plugin_src/pet/nd_pet_art.h"
static unsigned char pixels[32][32];static int shift;
static void box(void *ctx,int c,int x,int y,int w,int h){int a,b;(void)ctx;for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)if(a>=0 && a<32 && b>=0 && b<32)pixels[b][a]=(unsigned char)(c+shift);}
static void begin(void){memset(pixels,0,sizeof(pixels));}
static void emit(const char *program,const char *name,int state,int flags){int x,y;printf("%s %s %d %d\n",program,name,state,flags);for(y=0;y<32;++y){for(x=0;x<32;++x)printf("%x",pixels[y][x]);putchar('\n');}}
int main(void){int i,p,l;
 shift=0;for(i=0;i<4;++i){begin();nd_archive_tile_art(NULL,box,i);emit("ArchiveExtractor","archive",i,0);}
 begin();nd_notes_tile_art(NULL,box,0);emit("Notes","notes",0,0);
 for(i=0;i<2;++i){begin();nd_trash_tile_art(NULL,box,i);emit("Trash","trash",i,0);}
 for(i=0;i<3;++i){begin();nd_timer_tile_art(NULL,box,i);emit("Timer","timer",i,5);}
 shift=1;for(i=0;i<11;++i){begin();nd_weather_art(NULL,box,i);emit("Weather","weather",i,1);}
 for(i=0;i<6;++i){begin();nd_camera_art(NULL,box,i);emit("Screenshot","shot",i,0);}
 begin();nd_modes_art(NULL,box);emit("ScreenMode","modes",0,0);
 begin();nd_places_art(NULL,box);emit("Places","places",0,0);
 begin();nd_saver_art(NULL,box);emit("Saver","saver",0,0);
 shift=0;for(i=0;i<70;++i){begin();nd_aquarium_tile(NULL,box,(unsigned long)i);emit("Aquarium","fish",i,2);}
 shift=1;for(p=0;p<3;++p)for(l=0;l<2;++l)for(i=0;i<8;++i){begin();nd_christmas_art(NULL,box,i,p,l);emit("Christmas","tree",p*16+l*8+i,2);}
 shift=0;for(i=0;i<ND_PET_STATES*ND_PET_CHARACTERS;++i){begin();nd_pet_art(NULL,box,i);emit("Pet","pet",i,2);}
 return 0;}
'''
AQUARIUM=[0x092b44,0x134a62,0x236c78,0x4299a1,0xd2b57b,0xf5d27c,0xe68d57,0xba5e55,0x63985c,0x397e57,0xb8e4dd,0xf1f4d9,0x111f31,0x5b6f87,0xa78bad,0x719bc4]
PET=[(0,0,0),(52,40,40),(226,138,62),(248,198,132),(244,242,234),(238,146,168),(98,186,96),(24,24,24),
     (72,98,168),(122,152,212),(220,58,72),(206,222,255),(92,72,62),(196,52,48),(232,104,92),(150,154,162)]   # nd_pet_rgb
TREE=[(65,63,56),(51,132,78),(129,179,118),(234,197,102),(198,101,109),(124,177,211),(126,98,79),(241,241,220)]
def generated():
 with tempfile.TemporaryDirectory(prefix='novadock-art-author-') as tmp:
  p=Path(tmp);(p/'art.c').write_text(C);subprocess.run(shlex.split(os.environ.get('HOST_CC','cc'))+['-std=c99','-I'+str(ROOT),str(p/'art.c'),'-o',str(p/'art')],check=True)
  lines=subprocess.check_output([str(p/'art')],text=True).splitlines()
 banks={}
 for n in range(0,len(lines),33):
  program,name,state,flags=lines[n].split();frames=banks.setdefault(program,[]);frames.append({'name':name,'state':int(state),'flags':int(flags),'pixels':[[int(v,16) for v in row] for row in lines[n+1:n+33]]})
 for program,frames in banks.items():
  if program=='Aquarium':palette=[[(v>>16)&255,(v>>8)&255,v&255] for v in AQUARIUM];roles=[0]*16
  elif program=='Pet':palette=[list(c) for c in PET];roles=[0]*16
  elif program=='Christmas':palette=[(0,0,0)]+TREE+[(0,0,0)]*7;roles=[0]*16
  else:palette=colourful_palette(program);roles=[0,1,2,3,4,5]+[0]*10
  yield program,build_bank(frames,palette,roles)
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args();count=0
 for program,data in generated():
  target=ROOT/'Plugins'/(program+'.assets')/'Artwork.ndart'
  if args.check:
   if not target.is_file() or not same_artifact(target.read_bytes(),data):raise SystemExit(str(target)+' needs regeneration')
  else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
  count+=1
 print(f'{count} bundled plugin artwork banks are current.')
if __name__=='__main__':main()
