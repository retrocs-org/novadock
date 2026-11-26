#!/usr/bin/env python3
"""Production backdrop stretching and overlay anchors, including allocation fallback."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(path,signature):
 s=(ROOT/path).read_text();a=s.index(signature+'\n{');return s[a:s.index('\n}',a)+2]
h=r'''
#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nd_config.h"
#include "nd_layout.h"
#include "nd_autohide.h"
#define ND_BACKDROP_GROWTH 128
#define ND_ACCENT2 3
#define ND_BG 0
#define JAM1 0
typedef char *STRPTR;
struct BitMap {int p[200][200];void *Planes[1];};
struct RastPort {int TxHeight,TxBaseline;struct BitMap *bitmap;};
struct NDBackdropPart {struct BitMap *bitmap,*mask,*growth,*growth_mask;int length;};
struct NDBackdrop {struct NDBackdropPart parts[3];int vertical,thickness;};
struct Window {struct RastPort *RPort;};
struct NDDock {struct NDConfig conf;struct Window *window;int hover_headroom,hover_plan_ready,painting_shelf,painting_wave,running_items[ND_MAX_ITEMS];struct NDHoverLayoutItem hover_layout[ND_MAX_ITEMS];struct NDRect hover_rects[ND_MAX_ITEMS];};
static void copy(struct BitMap*b,int sx,int sy,struct RastPort*r,int dx,int dy,int w,int h,struct BitMap*m)
{int x,y;assert(sx>=0&&sy>=0&&sx+w<=200&&sy+h<=200);assert(dx>=0&&dy>=0&&dx+w<=200&&dy+h<=200);for(y=0;y<h;++y)for(x=0;x<w;++x)if(!m||m->p[sy+y][sx+x])r->bitmap->p[dy+y][dx+x]=b->p[sy+y][sx+x];}
static void BltBitMapRastPort(struct BitMap*b,int sx,int sy,struct RastPort*r,int dx,int dy,int w,int h,int mode){(void)mode;copy(b,sx,sy,r,dx,dy,w,h,NULL);}
static void BltMaskBitMapRastPort(struct BitMap*b,int sx,int sy,struct RastPort*r,int dx,int dy,int w,int h,int mode,void*m){(void)mode;copy(b,sx,sy,r,dx,dy,w,h,m);}
static int rect[4],dots[4],fills,text_length;static char text_value[8];
static void fill(struct NDDock*d,int ink,int x,int y,int w,int h){(void)d;(void)ink;rect[0]=x;rect[1]=y;rect[2]=w;rect[3]=h;if(!fills)memcpy(dots,rect,sizeof(rect));++fills;}
static int TextLength(struct RastPort*r,STRPTR t,int n){(void)r;(void)t;return n*8;}
static int pen(struct NDDock*d,int ink){(void)d;return ink;}
static void SetAPen(struct RastPort*r,int ink){(void)r;(void)ink;}
static void SetDrMd(struct RastPort*r,int mode){(void)r;(void)mode;}
static void Move(struct RastPort*r,int x,int y){(void)r;(void)x;(void)y;}
static void Text(struct RastPort*r,STRPTR s,int n){(void)r;text_length=n;memcpy(text_value,s,n);text_value[n]=0;}
'''
h+=r'''
static void tile_coords(struct NDDock*d,int i,int*x,int*y){(void)d;(void)i;*x=3;*y=4;}
static void tile_dimensions(struct NDDock*d,int i,int*w,int*h){(void)d;(void)i;*w=86;*h=76;}
static int hover_art_size(struct NDDock*d,int i,int tw,int th,int*w,int*h){(void)d;(void)i;(void)tw;(void)th;*w=40;*h=40;return 1;}
static int nd_icon_art_top(int y,int th,int h,int tiny){(void)tiny;return y+(th-h-20)/2;}
'''
s=(ROOT/'src/nd_backdrop.c').read_text();h+=s[s.index('int nd_backdrop_paint_sized('):]
for signature in ('static int indicator_box(struct NDDock *dock,int index,int *x,int *y,int *w)',
 'static void running_dot(struct NDDock *dock,int index,int x,int y,int w)',
 'static void draw_running_indicator(struct NDDock *dock,int index,\n                                   int x,int y,int w)',
 'static int draw_plugin_badge(struct NDDock *dock,const struct NDItem *item,\n                             int x,int y,int tw)'):
 h+='\n'+function('src/dock_draw.c',signature)
h+=r'''
static void backdrop(void)
{
 struct NDBackdrop bg;struct RastPort rp;int v,cache,extra,clip,i,a,c,x,y,old,which,offset,expected,total;
 memset(&bg,0,sizeof(bg));rp.bitmap=calloc(1,sizeof(*rp.bitmap));assert(rp.bitmap);bg.thickness=12;
 for(i=0;i<3;++i){struct NDBackdropPart*p=&bg.parts[i];p->length=i?5:8;p->bitmap=calloc(1,sizeof(*p->bitmap));p->mask=calloc(1,sizeof(*p->mask));assert(p->bitmap&&p->mask);p->mask->Planes[0]=p->mask;}
 for(v=0;v<2;++v)for(cache=0;cache<2;++cache){
  bg.vertical=v;
  for(i=0;i<3;++i){struct NDBackdropPart*p=&bg.parts[i];
   for(c=0;c<12;++c)for(a=0;a<p->length;++a){x=v?c:a;y=v?a:c;p->bitmap->p[y][x]=1+i*1000+c*20+a;p->mask->p[y][x]=!(i&&a<2&&(c<2||c>9));}
   if(cache){p->growth=calloc(1,sizeof(*p->growth));p->growth_mask=calloc(1,sizeof(*p->growth_mask));assert(p->growth&&p->growth_mask);p->growth_mask->Planes[0]=p->growth_mask;
    for(c=0;c<128;++c)for(a=0;a<p->length;++a){x=v?c:a;y=v?a:c;p->growth->p[y][x]=p->bitmap->p[v?a:6][v?6:a];p->growth_mask->p[y][x]=p->mask->p[v?a:6][v?6:a];}}
  }
  for(extra=0;extra<=96;extra+=24)for(clip=0;clip<2;++clip){int lo=clip?3:0,hi=clip?37:40,c0=clip?1:0;total=12+extra;
   memset(rp.bitmap->p,0,sizeof(rp.bitmap->p));
   assert(nd_backdrop_paint_sized(&bg,&rp,2,3,40,total,v?c0:lo,v?lo:c0,v?total-c0:hi-lo,v?hi-lo:total-c0));
   for(c=c0;c<total;++c)for(a=lo;a<hi;++a){struct NDBackdropPart*p;
    old=c<6?c:c<6+extra?6:c-extra;which=a<5?1:a>=35?2:0;offset=which==1?a:which==2?a-35:(a-5)%8;p=&bg.parts[which];
    expected=p->mask->p[v?offset:old][v?old:offset]?p->bitmap->p[v?offset:old][v?old:offset]:0;
    assert(rp.bitmap->p[3+(v?a:c)][2+(v?c:a)]==expected);
   }
  }
  for(i=0;i<3;++i){free(bg.parts[i].growth);free(bg.parts[i].growth_mask);bg.parts[i].growth=bg.parts[i].growth_mask=NULL;}
 }
 for(i=0;i<3;++i){free(bg.parts[i].bitmap);free(bg.parts[i].mask);}free(rp.bitmap);
}
static void overlays(void)
{
 static struct NDDock d;struct Window w;struct RastPort rp;int mode,i,x,y,width;
 memset(&d,0,sizeof(d));rp.TxHeight=8;rp.TxBaseline=6;w.RPort=&rp;d.window=&w;d.conf.count=2;d.conf.running_apps=1;d.running_items[0]=1;
 d.hover_headroom=96;d.hover_plan_ready=1;d.hover_layout[0].kind=d.hover_layout[1].kind=ND_HOVER_ART;strcpy(d.conf.items[1].badge,"129");
 for(i=0;i<4;++i){d.hover_rects[0].left=20+i*7;d.hover_rects[0].top=30+i*5;d.hover_rects[0].width=80+i*8;d.hover_rects[0].height=70+i*6;d.hover_rects[1]=d.hover_rects[0];
  for(mode=0;mode<3;++mode){d.painting_shelf=mode==0;d.painting_wave=mode==1;fills=0;
   draw_running_indicator(&d,0,3,4,86);draw_plugin_badge(&d,&d.conf.items[1],3,4,86);
   if(mode==0){assert(!fills);continue;}assert(fills==2);
   assert(dots[0]==(mode==1 ? d.hover_rects[0].left+(d.hover_rects[0].width-3)/2:44));
   assert(dots[1]==(mode==1 ? d.hover_rects[0].top+d.hover_rects[0].height+2:54)&&dots[2]==3&&dots[3]==2);
   width=mode==1 && d.hover_rects[1].width>86?d.hover_rects[1].width:86;x=mode==1?d.hover_rects[1].left+(d.hover_rects[1].width-width)/2:3;y=mode==1?d.hover_rects[1].top:4;
   assert(text_length==3&&!strcmp(text_value,"129"));
   assert(rect[0]==x+width-33&&rect[1]==y+3&&rect[2]==30&&rect[3]==10);
  }
 }
 d.painting_shelf=d.painting_wave=0;d.hover_headroom=0;fills=0;
 draw_running_indicator(&d,0,3,4,86);assert(fills==1&&dots[0]==44&&dots[1]==54&&dots[2]==3&&dots[3]==2);
 d.hover_headroom=96;d.hover_layout[0].kind=ND_HOVER_WIDGET;d.painting_shelf=1;d.painting_wave=0;x=3;y=4;width=86;assert(indicator_box(&d,0,&x,&y,&width));assert(x==3&&y==4&&width==86);
}
int main(void){backdrop();overlays();puts("Hover overlays: moving badge/dot anchors, source suppression, stable masked artwork stretching, both orientations, clipping and allocation fallback passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
