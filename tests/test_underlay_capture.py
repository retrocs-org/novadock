#!/usr/bin/env python3
"""Exercise the actual screen-to-snapshot capture before viewport growth."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/dock_window.c').read_text();a=s.index('static void capture_view_underlay(');b=s.index('\n}',a)+2
h=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_underlay.h"
struct BitMap {int width,height;unsigned char pixels[80*70];};
struct Window {int LeftEdge,TopEdge,Width,Height;};
struct Screen {struct {struct BitMap *BitMap;} RastPort;int LayerInfo;};
struct NDDock {struct BitMap *hover_desktop;struct Screen *screen;struct Window *window;struct NDRect full_box;int width,height,transparency,hover_prev_valid,desktop_refresh_pending,desktop_ready_ms,desktop_probe_ms;};
static int captures,locks,closes,refreshes,rebuilds,pending,probes,refresh_ok=1;
static int desktop_repaint_pending(struct Screen *s,const struct NDRect *r){assert(!s->LayerInfo);assert(r->left>=20&&r->top>=10&&r->left+r->width<=60&&r->top+r->height<=46);++probes;return pending;}
static void close_caption_popup(struct NDDock *d){(void)d;++closes;}
static void close_tooltip(struct NDDock *d){(void)d;++closes;}
static void WaitBlit(void){}
static void LockLayerInfo(int *i){assert(!*i);*i=1;++locks;}
static void UnlockLayerInfo(int *i){assert(*i==1);*i=0;}
static void BltBitMap(struct BitMap *s,int sx,int sy,struct BitMap *d,int dx,int dy,int w,int h,int op,int mask,void *unused)
{int x,y;assert(sx>=0&&sy>=0&&sx+w<=s->width&&sy+h<=s->height&&dx>=0&&dy>=0&&dx+w<=d->width&&dy+h<=d->height&&op==0xc0&&mask==0xff&&!unused);++captures;for(y=0;y<h;++y)for(x=0;x<w;++x)d->pixels[(dy+y)*d->width+dx+x]=s->pixels[(sy+y)*s->width+sx+x];}
static int nd_transparency_refresh(int *t,struct BitMap *b,int left,int top,int width,int height){(void)t;(void)b;assert(left>=0&&width>0&&left+width<=40&&top>=0&&height>0&&top+height<=36);++refreshes;return refresh_ok;}
static void prepare_transparency(struct NDDock *d){(void)d;++rebuilds;}
'''+s[a:b]+r'''
static int in(const struct NDRect *r,int x,int y){return x>=r->left&&x<r->left+r->width&&y>=r->top&&y<r->top+r->height;}
int main(void)
{
 struct NDDock d;struct Screen screen;struct Window window={30,22,10,10};struct BitMap live,snapshot;
 struct NDRect old={10,12,10,10},fresh={8,7,18,20},shrink={12,14,4,4};int x,y,before;
 memset(&d,0,sizeof(d));memset(&screen,0,sizeof(screen));live.width=80;live.height=70;snapshot.width=40;snapshot.height=36;
 for(y=0;y<70;++y)for(x=0;x<80;++x)live.pixels[y*80+x]=(unsigned char)((x*3+y*7)%250);
 for(y=22;y<32;++y)for(x=30;x<40;++x)live.pixels[y*80+x]=255; /* dock artwork */
 memset(snapshot.pixels,5,sizeof(snapshot.pixels));screen.RastPort.BitMap=&live;
 d.screen=&screen;d.window=&window;d.hover_desktop=&snapshot;d.width=40;d.height=36;d.full_box.left=20;d.full_box.top=10;d.hover_prev_valid=1;
 pending=1;d.desktop_ready_ms=9;d.desktop_probe_ms=10;
 capture_view_underlay(&d,&fresh);assert(!captures&&!locks&&!refreshes&&probes==1&&d.hover_prev_valid&&d.desktop_refresh_pending==2&&!d.desktop_ready_ms&&!d.desktop_probe_ms);
 for(y=0;y<36;++y)for(x=0;x<40;++x)assert(snapshot.pixels[y*40+x]==5);
 pending=0;closes=0;
 capture_view_underlay(&d,&fresh);assert(captures==4&&locks==1&&closes==2&&refreshes==4&&!d.hover_prev_valid&&!screen.LayerInfo);
 for(y=0;y<36;++y)for(x=0;x<40;++x){unsigned char expected=5;if(in(&fresh,x,y)&&!in(&old,x,y))expected=(unsigned char)(((x+20)*3+(y+10)*7)%250);assert(snapshot.pixels[y*40+x]==expected);}
 before=captures;d.hover_prev_valid=1;capture_view_underlay(&d,&old);capture_view_underlay(&d,&shrink);assert(captures==before&&closes==2&&d.hover_prev_valid);
 d.hover_desktop=NULL;capture_view_underlay(&d,&fresh);assert(captures==before);d.hover_desktop=&snapshot;
 refresh_ok=0;capture_view_underlay(&d,&fresh);assert(rebuilds==1);
 puts("Viewport capture: fresh foreign-window pixels, no self-capture, no growth/no work, bounds, delayed repaint deferral without animation waits, cache invalidation and tint failure recovery passed.");return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(root/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
