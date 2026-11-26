#!/usr/bin/env python3
"""Keep real artwork-bank mappings stable while other windows reuse free pens."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_icons.c').read_text()
def function(signature):
 a=s.index(signature);b=s.index('{',a);i=b+1;depth=1
 while depth:
  depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
code=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_artwork.h"
typedef unsigned long ULONG;typedef long LONG;
#define ND_MAX_ITEMS 4
#define ND_ITEM_PLUGIN 3
struct Pen {ULONG red,green,blue;int refs;};
struct Map {struct Pen pens[32];};
struct Screen {struct {struct Map *ColorMap;} ViewPort;};
struct NDItem {int kind,media_capabilities;char id[20],command[520],caption[32];};
struct NDDock {struct {struct NDItem items[4];} conf;struct Screen *screen;char directory[520];ULONG plugin_art_key[4];struct NDArtwork *plugin_artwork[4];};
static int allocations,releases,exhausted;
static LONG ObtainBestPenA(struct Map *map,ULONG r,ULONG g,ULONG b,void *tags)
{int i;(void)tags;if(exhausted)return -1;
 for(i=2;i<32;++i)if(map->pens[i].refs && map->pens[i].red==r && map->pens[i].green==g && map->pens[i].blue==b){++map->pens[i].refs;++allocations;return i;}
 for(i=2;i<32;++i)if(!map->pens[i].refs){map->pens[i].red=r;map->pens[i].green=g;map->pens[i].blue=b;map->pens[i].refs=1;++allocations;return i;}return -1;}
static LONG FindColor(struct Map *map,ULONG r,ULONG g,ULONG b,int max)
{(void)map;(void)r;(void)g;(void)b;(void)max;return 1;}
static void ReleasePen(struct Map *map,ULONG n){assert(n<32 && map->pens[n].refs>0);--map->pens[n].refs;++releases;}
'''+function('static void map_plugin_palette(')+'\n'+function('void map_plugin_artwork(')+'\n'+function('void unmap_plugin_artwork(')+'\n'+function('static void release_plugin_artwork(')+'\n'+function('void load_plugin_artwork(')+r'''
int main(int argc,char **argv){struct NDDock dock;struct Screen screen;struct Map map;struct NDArtwork *bank;int i,held,before;
 assert(argc==2);memset(&dock,0,sizeof(dock));memset(&map,0,sizeof(map));screen.ViewPort.ColorMap=&map;dock.screen=&screen;
 for(i=0;i<2;++i){dock.conf.items[i].kind=ND_ITEM_PLUGIN;strcpy(dock.conf.items[i].command,argv[1]);strcpy(dock.conf.items[i].caption,"ART:ChristmasLights:1");}
 dock.screen=NULL;load_plugin_artwork(&dock,0);bank=dock.plugin_artwork[0];assert(bank&&bank->data&&!bank->palette_ready&&!allocations);load_plugin_artwork(&dock,1);assert(dock.plugin_artwork[1]==bank);dock.screen=&screen;map_plugin_artwork(&dock);assert(bank->palette_ready);held=allocations;assert(held>0);
 load_plugin_artwork(&dock,1);assert(dock.plugin_artwork[1]==bank && allocations==held);
 /* Model another application repainting every unreserved screen pen grey. */
 for(i=0;i<32;++i)if(!map.pens[i].refs)map.pens[i].red=map.pens[i].green=map.pens[i].blue=0x99999999UL;
 for(i=1;i<16;++i){struct Pen *pen=&map.pens[bank->mapped[i]];assert(bank->owned_pens[i] && pen->refs>0);
  assert(pen->red==(ULONG)bank->data[8+i*3]*0x01010101UL && pen->green==(ULONG)bank->data[9+i*3]*0x01010101UL && pen->blue==(ULONG)bank->data[10+i*3]*0x01010101UL);}
 unmap_plugin_artwork(&dock);assert(!bank->palette_ready&&releases==held);unmap_plugin_artwork(&dock);assert(releases==held);map_plugin_artwork(&dock);assert(bank->palette_ready&&allocations==held*2);held*=2;
 release_plugin_artwork(&dock);assert(releases==held && !dock.plugin_artwork[0] && !dock.plugin_artwork[1]);
 for(i=0;i<32;++i)assert(!map.pens[i].refs);
 before=releases;load_plugin_artwork(&dock,0);release_plugin_artwork(&dock);assert(releases-before==held/2);
 exhausted=1;before=releases;load_plugin_artwork(&dock,0);bank=dock.plugin_artwork[0];assert(bank && bank->data);
 for(i=1;i<16;++i)assert(!bank->owned_pens[i] && bank->mapped[i]==1);
 release_plugin_artwork(&dock);assert(releases==before);
 puts("Plugin palette ownership: other windows cannot recolour held pens; shared banks release once; exhausted palettes fall back safely.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'palette.c').write_text(code)
 cmd=[os.environ.get('HOST_CC','cc'),'-std=c99','-Wall','-Wextra','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'palette.c'),'-o',str(p/'palette')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True)
 subprocess.run([str(p/'palette'),str(ROOT/'Plugins/HASS')],check=True)
