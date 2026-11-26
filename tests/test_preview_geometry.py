#!/usr/bin/env python3
"""Exercise real preview geometry, cache resizing and size-switch clearing."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'prefs/novadockappearanceprefs.c').read_text()
geometry=s[s.index('struct PreviewGeometry {'):s.index('static void appearance_sections(')]
draw=s[s.index('static void draw_preview('):s.index('static void set_gadgets(')]
code=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_prefs_layout.h"
typedef unsigned long ULONG;typedef char *STRPTR;
#define BACKGROUNDPEN 0
#define JAM1 1
struct RastPort {int pen,TxHeight,TxBaseline;};
struct Screen {int unused;};struct Window {struct RastPort *RPort;};
struct DrawInfo {int dri_Pens[1];};
struct Backdrop {int thickness;struct {int bitmap;} parts[1];};
struct AppearancePrefs {struct NDConfig *cfg;int compact;struct NDPrefsLayout layout;char preview_theme[64],theme_root[256];struct Backdrop background;struct Screen *screen;struct Window *win;};
static int loads,frees,artwork=1;static unsigned char pixels[160][710];
static struct DrawInfo info={{1}};
const char *nd_config_theme_id(const struct NDConfig *c){(void)c;return "Nord";}
static void nd_backdrop_free(struct Backdrop *b,struct Screen *s){(void)s;++frees;memset(b,0,sizeof(*b));}
static void obtain_pens(struct AppearancePrefs *p){(void)p;}
static struct DrawInfo *GetScreenDrawInfo(struct Screen *s){(void)s;return &info;}
static void FreeScreenDrawInfo(struct Screen *s,struct DrawInfo *i){(void)s;(void)i;}
static void nd_backdrop_load(struct Backdrop *b,struct Screen *s,const char *r,const char *t,int h,int v,ULONG matte,void *d){(void)s;(void)r;(void)t;(void)v;(void)d;assert(matte==1);++loads;b->thickness=h;b->parts[0].bitmap=1;}
static int preview_pen(struct AppearancePrefs *p,int n){(void)p;return n+2;}
static void SetAPen(struct RastPort *rp,int pen){rp->pen=pen;}
static void RectFill(struct RastPort *rp,int x,int y,int right,int bottom){int a,b;assert(x>=0&&y>=0&&right<710&&bottom<160);for(b=y;b<=bottom;++b)for(a=x;a<=right;++a)pixels[b][a]=rp->pen;}
static int nd_backdrop_paint(struct Backdrop *b,struct RastPort *rp,int x,int y,int w,int sx,int sy,int sw,int sh){(void)b;(void)sx;(void)sy;assert(w==sw);if(!artwork)return 0;SetAPen(rp,3);RectFill(rp,x,y,x+sw-1,y+sh-1);return 1;}
static void SetDrMd(struct RastPort *r,int mode){(void)r;(void)mode;}
static void appearance_sections(struct AppearancePrefs *p){(void)p;}
static void Move(struct RastPort *r,int x,int y){(void)r;(void)x;(void)y;}
static void Draw(struct RastPort *r,int x,int y){(void)r;(void)x;(void)y;}
static void preview_tile(struct AppearancePrefs *p,int x,int y,int w,int h,int c){(void)p;(void)x;(void)y;(void)w;(void)h;(void)c;}
static void preview_tiny_sample(struct AppearancePrefs *p,int k,int x,int y,int w,int h){(void)p;(void)k;(void)x;(void)y;(void)w;(void)h;}
static void preview_normal_sample(struct AppearancePrefs *p,int k,int x,int y,int w,int h){(void)p;(void)k;(void)x;(void)y;(void)w;(void)h;}
static void preview_centered_text(struct AppearancePrefs *p,int x,int y,int w,int c,const char *t){(void)p;(void)x;(void)y;(void)w;(void)c;(void)t;}
''' +geometry+draw+r'''
static void contains(struct PreviewGeometry *g){int i;assert(g->left>=140&&g->top>=46);for(i=0;i<5;++i){int x=g->row_x+i*(g->tile_width+3);assert(x>g->left&&x+g->tile_width<g->left+g->width);assert(g->row_y>g->top&&g->row_y+g->tile_height<g->top+g->height);}}
int main(void){static struct NDConfig cfg;struct AppearancePrefs p;struct PreviewGeometry normal,tiny;struct RastPort rp;struct Window win;int fonts[]={4,8,9,12,16,24,32,64};int fx,fy,compact,grip,cases=0,n_h,t_h,x,y,a;memset(&p,0,sizeof(p));p.cfg=&cfg;
 for(fx=0;fx<8;++fx)for(fy=0;fy<8;++fy)for(compact=0;compact<2;++compact)for(grip=0;grip<2;++grip){
  p.compact=compact;cfg.drag_bar=grip;cfg.theme_artwork=1;cfg.item_size=ND_ITEM_SIZE_NORMAL;memset(&p.background,0,sizeof(p.background));strcpy(p.preview_theme,"Nord");loads=frees=0;
  nd_prefs_layout_init(&p.layout,fonts[fx],fonts[fy],10000,10000,10,compact?620:700,480);
  preview_geometry(&p,&normal);contains(&normal);n_h=preview_thickness(&p);prepare_preview(&p);assert(loads==1&&p.background.thickness==n_h);prepare_preview(&p);assert(loads==1);
  cfg.item_size=ND_ITEM_SIZE_TINY;preview_geometry(&p,&tiny);contains(&tiny);t_h=preview_thickness(&p);assert(tiny.width<normal.width&&t_h<n_h);assert(tiny.left+tiny.width<=normal.left+normal.width&&tiny.top+tiny.height<=normal.top+normal.height);
  prepare_preview(&p);assert(loads==2&&p.background.thickness==t_h);prepare_preview(&p);assert(loads==2);
  cfg.item_size=ND_ITEM_SIZE_NORMAL;prepare_preview(&p);assert(loads==3&&p.background.thickness==n_h);
  cfg.theme_artwork=0;prepare_preview(&p);assert(!p.background.thickness);cfg.theme_artwork=1;prepare_preview(&p);assert(loads==4);++cases;
 }
 nd_prefs_layout_init(&p.layout,8,8,1024,600,10,700,468);p.compact=0;cfg.drag_bar=0;cfg.hover_magnification=100;cfg.label_mode=ND_LABEL_HIDDEN;memset(&rp,0,sizeof(rp));win.RPort=&rp;p.win=&win;
 for(a=0;a<2;++a){artwork=a;cfg.item_size=ND_ITEM_SIZE_NORMAL;draw_preview(&p);assert(pixels[47][141]!=1);
 cfg.item_size=ND_ITEM_SIZE_TINY;preview_geometry(&p,&tiny);draw_preview(&p);
 for(y=46;y<=138;++y)for(x=140;x<=688;++x)if(x<tiny.left||x>=tiny.left+tiny.width||y<tiny.top||y>=tiny.top+tiny.height)assert(pixels[y][x]==1);
 cfg.item_size=ND_ITEM_SIZE_NORMAL;draw_preview(&p);assert(pixels[47][141]!=1);
 }
 printf("Appearance preview: %d font/layout/grip cases resize the shelf and artwork cache; Normal/Tiny/Normal clears old pixels for solid and artwork backgrounds.\n",cases);return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root/'include'),str(p/'test.c'),str(root/'src/prefs_layout.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
