#!/usr/bin/env python3
"""Exercise production Tiny metric rendering at every supported small geometry."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_draw.c').read_text()
a=s.index('static int parse_ram_pair(',s.index('static void draw_eyes(struct NDDock *dock,int index,int x,int y,int tw,int th)\n{'))
b=s.index('\n}',a)+2
render=s[s.index('struct MetricPaint {'):s.index('static void draw_ram_tile(',s.index('struct MetricPaint {'))]
preview=(ROOT/'prefs/novadockappearanceprefs.c').read_text()
preview=preview[preview.index('struct PreviewMetric {'):preview.index('static void preview_normal_sample')]
code=r"""
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_metric_art.h"
#define ND_METRIC_COLORS 0
#define ND_TEXT 30
struct RastPort {int TxBaseline,pen;};struct Window {struct RastPort *RPort;};
struct NDDock {struct NDConfig conf;struct Window *window;struct {int clock_analog;} opts;};
#define DOCK_OPTS(d) (&(d)->opts)
static unsigned char pixels[140][320];static int bound_x,bound_y,bound_w,bound_h,painted;
static void fill(struct NDDock *d,int color,int x,int y,int w,int h)
{int a,b;(void)d;if(w<1||h<1)return;assert(x>=bound_x&&y>=bound_y&&x+w<=bound_x+bound_w&&y+h<=bound_y+bound_h);assert(color>=0&&color<31);for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)pixels[b][a]=(unsigned char)(color+1);painted+=w*h;}
static int nd_icon_art_top(int y,int th,int ih,int tiny){return tiny ? y+(th-ih)/2:y+(th-24-ih)/2;}
static void centered_text_tight(struct NDDock *d,int color,const char *v,int x,int y,int w){(void)color;(void)v;(void)x;(void)y;(void)w;assert(d->conf.item_size!=ND_ITEM_SIZE_TINY);}
static void caption_text_normal(struct NDDock *d,int color,const char *v,int x,int y,int w){(void)color;(void)v;(void)x;(void)y;(void)w;assert(d->conf.item_size!=ND_ITEM_SIZE_TINY);}
"""+s[a:b]+render+r"""
#define A_METRIC_COLORS ND_METRIC_COLORS
struct AppearancePrefs {struct NDConfig *cfg;struct Window *win;int compact;};
static int preview_pen(struct AppearancePrefs *p,int ink){(void)p;return ink;}
static void SetAPen(struct RastPort *rp,int pen){rp->pen=pen;}
static void RectFill(struct RastPort *rp,int x,int y,int right,int bottom){fill(NULL,rp->pen,x,y,right-x+1,bottom-y+1);}
static void preview_centered_text(struct AppearancePrefs *p,int x,int y,int w,int ink,const char *value){(void)x;(void)y;(void)w;(void)ink;(void)value;assert(!p->compact&&p->cfg->item_size!=ND_ITEM_SIZE_TINY);}
"""+preview+r"""
static void tile(struct NDDock *d,int kind,int x,int y,int w,int h,int online)
{struct NDItem *it=&d->conf.items[0];memset(it,0,sizeof(*it));it->kind=ND_ITEM_PLUGIN;it->online=online;it->meter=100;strcpy(it->id,kind==1 ? "CPU":kind==2 ? "RAM":kind==3 ? "CLOCK":"NET");strcpy(it->value,kind==1 ? "100%":kind==2 ? "C:2.0/2.0 F:8.0/16.0":kind==3 ? "19:44:33":"4294967295/999 p/s");if(kind==2){unsigned int chip=0,fast=0;assert(parse_ram_pair(it->value,&chip,&fast)&&chip==100&&fast==50);}
 bound_x=x;bound_y=y;bound_w=w;bound_h=h;painted=0;assert(metric_kind(d,it)==kind);draw_metric_tile(d,0,kind,x,y,w,h);assert(painted>100);}
int main(int argc,char **argv)
{static struct NDDock d;struct RastPort rp;struct Window win;char time[8];int w,h,kind,online;unsigned long cases=0;static const unsigned char colors[20][3]={{16,46,34},{36,100,70},{164,220,176},{57,222,120},{118,228,156},{16,46,34},{33,108,58},{164,220,154},{249,196,73},{61,200,126},{50,30,15},{203,112,32},{255,240,174},{255,206,82},{105,210,187},{20,43,64},{40,115,166},{190,232,255},{70,216,168},{249,189,66}};
 memset(&d,0,sizeof(d));rp.TxBaseline=6;win.RPort=&rp;d.window=&win;d.conf.item_size=ND_ITEM_SIZE_TINY;
 for(w=34;w<=86;w+=2)for(h=30;h<=76;h+=2)for(kind=1;kind<=4;++kind)for(online=0;online<2;++online){tile(&d,kind,10,10,w,h,online);++cases;}
 {struct AppearancePrefs p;static unsigned char expected[140][320];struct NDItem *it=&d.conf.items[0];int compact;
 p.cfg=&d.conf;p.win=&win;
 for(compact=0;compact<2;++compact)for(w=34;w<=86;w+=2)for(h=30;h<=76;h+=2)for(kind=1;kind<=4;++kind){
  p.compact=compact;d.conf.item_size=ND_ITEM_SIZE_TINY;memset(pixels,0,sizeof(pixels));
  tile(&d,kind,10,10,w,h,1);it->meter=37;
  strcpy(it->value,kind==1 ? "37%":kind==2 ? "C:2.8/10.0 F:6.1/10.0":kind==3 ? "12:34:00":"42/12 p/s");
  memset(pixels,0,sizeof(pixels));draw_metric_tile(&d,0,kind,10,10,w,h);memcpy(expected,pixels,sizeof(expected));memset(pixels,0,sizeof(pixels));
  /* Compact preferences use the Tiny renderer even for a Normal config. */
  if(compact)d.conf.item_size=ND_ITEM_SIZE_NORMAL;
  preview_metric(&p,kind,10,10,w,h);assert(!memcmp(expected,pixels,sizeof(expected)));
 }
 d.conf.item_size=ND_ITEM_SIZE_TINY;puts("Appearance preview: 5184 Tiny/compact geometry cases match live dock pixels.");}
 nd_metric_tiny_time("19:44:33",time);assert(!strcmp(time,"19:44"));nd_metric_tiny_time("12:44:33 PM",time);assert(!strcmp(time,"12:44P")&&nd_metric_text_width(time)==21);nd_metric_tiny_time("7:44 AM",time);assert(!strcmp(time,"7:44A"));nd_metric_tiny_time("",time);assert(!strcmp(time,"--"));nd_metric_tiny_time("1:",time);assert(!strcmp(time,"--"));
 if(argc>1){FILE *f;int x,y;memset(pixels,0,sizeof(pixels));for(online=0;online<2;++online)for(kind=1;kind<=4;++kind)tile(&d,kind,12+(kind-1)*62,10+online*54,43,38,1-online);f=fopen(argv[1],"wb");assert(f);fprintf(f,"P6\n260 112\n255\n");for(y=0;y<112;++y)for(x=0;x<260;++x){unsigned int ink=pixels[y][x];static const unsigned char bg[]={55,65,81};assert(fwrite(ink ? colors[ink-1]:bg,1,3,f)==3);}assert(!fclose(f));}
 printf("Tiny CPU/RAM/clock/network: %lu online/offline geometry cases, clipped-free artwork, compact live digits, large traffic rates and 12/24-hour clock readings passed.\n",cases);return 0;}
"""
with tempfile.TemporaryDirectory(prefix='nd-tiny-metrics-') as tmp:
 p=Path(tmp);(p/'check.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'check.c'),'-o',str(p/'check')]
 subprocess.run(cmd,check=True)
 args=[str(p/'check')]
 if os.environ.get('ND_TINY_PREVIEW'):args+=[os.environ['ND_TINY_PREVIEW']]
 subprocess.run(args,check=True)
