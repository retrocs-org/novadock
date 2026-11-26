#!/usr/bin/env python3
"""Production solid shelf clipping and portable metric artwork bounds."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_draw.c').read_text();a=s.index('static void solid_shelf(');b=s.index('\n}',a)+2
mk=s.index('static int metric_kind(struct NDDock *dock,const struct NDItem *item)\n{');me=s.index('\n}',mk)+2
h=r'''
#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0

#include <string.h>
#include <stdio.h>
#include "nd_config.h"
#include "nd_metric_art.h"
#define ND_BG 0
struct NDDock {struct NDConfig conf;struct {int dock_style;} theme_spec;struct {int valid,clock_analog;} opts;};
#define DOCK_OPTS(d) ((d)->opts.valid=1,(d)->opts.clock_analog=0,&(d)->opts)
static int vertical(struct NDDock*d){(void)d;return 0;}
static unsigned char pixels[160][180];
static void fill(struct NDDock*d,int ink,int x,int y,int w,int h)
{int a,b;(void)d;(void)ink;assert(x>=0&&y>=0&&x+w<=180&&y+h<=160&&w>0&&h>0);for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)pixels[b][a]=1;}
static void paint_shelf_span(struct NDDock*d,int x,int y,int w,int h){fill(d,0,x,y,w,h);}
static void box(void *ctx,int ink,int x,int y,int w,int h)
{int a,b;(void)ctx;assert(ink>=0&&ink<5&&x>=0&&y>=0&&w>0&&h>0&&x+w<=48&&y+h<=48);for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)pixels[b][a]=(unsigned char)(ink+1);}
'''+s[a:b]+r'''
const char *nd_option_get(const struct NDConfig*c,const char*i,const char*k){(void)c;(void)i;(void)k;return "DIGITAL";}
'''+s[mk:me]+r'''
int main(void)
{
 struct NDDock d;int shape,x,y,i,n,clip;unsigned char expected[160][180];
 memset(&d,0,sizeof(d));
 {struct NDItem item;memset(&item,0,sizeof(item));item.kind=ND_ITEM_PLUGIN;strcpy(item.id,"NET");
  assert(metric_kind(&d,&item)==4);item.graph=1;assert(!metric_kind(&d,&item));
  item.graph=0;d.conf.item_size=ND_ITEM_SIZE_TINY;assert(metric_kind(&d,&item)==4);item.graph=1;assert(metric_kind(&d,&item)==4);item.graph=0;
  d.conf.item_size=ND_ITEM_SIZE_NORMAL;strcpy(item.id,"ROADSHOW");assert(metric_kind(&d,&item)==4);
  item.kind=ND_ITEM_LAUNCH;assert(!metric_kind(&d,&item));
 }
 for(shape=0;shape<4;++shape)for(i=0;i<2;++i){
  int sx=13,sy=17,sw=i?70:130,sh=i?120:60;
  d.conf.shape=shape;memset(pixels,0,sizeof(pixels));solid_shelf(&d,sx,sy,sw,sh,sx,sy,sw,sh);memcpy(expected,pixels,sizeof(pixels));
  assert(pixels[sy+sh/2][sx+sw/2]);
  assert((pixels[sy][sx]!=0)==(shape==ND_SHAPE_SQUARE));
  for(clip=1;clip<sw;clip+=13){
   memset(pixels,0,sizeof(pixels));solid_shelf(&d,sx,sy,sw,sh,sx,sy,clip,sh);solid_shelf(&d,sx,sy,sw,sh,sx+clip,sy,sw-clip,sh);
   assert(!memcmp(pixels,expected,sizeof(pixels)));
  }
 }
 for(i=1;i<=4;++i){
  memset(pixels,0,sizeof(pixels));nd_metric_art(NULL,box,i);n=0;
  for(y=0;y<48;++y)for(x=0;x<48;++x)n+=pixels[y][x]!=0;
  assert(n>500&&!pixels[0][0]&&!pixels[47][47]);
 }
 puts("Solid shelves: rounded/chamfered transparent corners and clipped growth match; metric artwork retains transparent bounds and theme roles.");return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
