#!/usr/bin/env python3
"""Exercise real dock/preview pen allocation with oversized public ColorMaps."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(source,name):
 a=source.index(name);a=source.rfind('static ',0,source.index('\n{',a));return source[a:source.index('\n}',a)+2]
draw=(ROOT/'src/dock_draw.c').read_text();prefs=(ROOT/'prefs/novadockappearanceprefs.c').read_text()
code=r"""
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef long LONG;typedef unsigned char UBYTE;
#define RGBBYTE(x) (x)
#define A_COLOR_COUNT 33
struct BitMap {int Depth;};struct RastPort {struct BitMap *BitMap;};
struct Screen {struct RastPort RastPort;struct {void *ColorMap;} ViewPort;};
struct NDDock {struct Screen *screen;LONG colors[33];UBYTE color_owned[33];};
struct AppearancePrefs {struct Screen *screen;LONG pens[33];UBYTE pen_owned[33];void *cfg;};
static int obtained,limit,released,found;
static LONG ObtainBestPenA(void *m,int r,int g,int b,void *tag){(void)m;(void)r;(void)g;(void)b;(void)tag;return obtained;}
static void ReleasePen(void *m,LONG p){(void)m;assert(p==obtained);++released;}
static LONG FindColor(void *m,int r,int g,int b,int max){(void)m;(void)r;(void)g;(void)b;assert(max==limit);++found;return limit;}
static const char *nd_config_theme_id(void *c){(void)c;return "Nord";}
static void theme_rgb(struct AppearancePrefs *p,const char *id,UBYTE rgb[33][3]){(void)p;(void)id;memset(rgb,0,99);}
"""+function(draw,'static void color_pair(struct NDDock *dock, int index, int r, int g, int b)\n{')+function(prefs,'static void free_pens(')+function(prefs,'static void obtain_pens(')+r"""
int main(void){struct BitMap b;struct Screen screen;struct NDDock dock;struct AppearancePrefs p;int depths[]={1,2,4,8,16},i,j,values[5],fallback;
 memset(&screen,0,sizeof(screen));memset(&dock,0,sizeof(dock));screen.RastPort.BitMap=&b;dock.screen=&screen;
 for(i=0;i<5;++i){b.Depth=depths[i];limit=b.Depth>=8 ? 255:(1<<b.Depth)-1;
 values[0]=-1;values[1]=0;values[2]=limit;values[3]=limit+1;values[4]=255;
 for(j=0;j<5;++j){obtained=values[j];released=found=0;fallback=obtained<0||obtained>limit;color_pair(&dock,0,100,200,50);
 assert(dock.colors[0]==(fallback ? limit:obtained));assert(dock.color_owned[0]==!fallback);assert(found==fallback);assert(released==(obtained>limit));
 memset(&p,0,sizeof(p));p.screen=&screen;memset(p.pens,255,sizeof(p.pens));released=found=0;obtain_pens(&p);
 assert(found==(fallback ? 33:0));assert(released==(obtained>limit ? 33:0));
 {int k;for(k=0;k<33;++k){assert(p.pens[k]==(fallback ? limit:obtained));assert(p.pen_owned[k]==!fallback);}}
 released=0;free_pens(&p);assert(released==(fallback ? 0:33));
 }}puts("Screen pens: dock and preview stay within bitmap depth and release only owned pens (25 allocation/fallback cases).");return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
