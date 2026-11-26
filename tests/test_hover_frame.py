#!/usr/bin/env python3
"""A hover update exposes one completed frame, reuses storage and fails safely."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/dock_draw.c').read_text();a=s.index('static void hover_tile_extent(');b=s.index('\n}',s.index('void draw_hover('))+2
code=r'''#include <assert.h>
#include "nd_hover_policy.h"
#include "nd_trace.h"
#include "nd_render_policy.h"
typedef unsigned long ULONG;
static void CurrentTime(ULONG *s,ULONG *m){static ULONG clock;*s=0;*m=clock;clock+=100;}
#include "nd_hover_dirty.h"
#define ND_MAX_ITEMS 4
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define BMF_CLEAR 0
#define BMF_DISPLAYABLE 2
#define dock_left(d) 10
#define dock_top(d) 20
#define dock_view_x(d) ((d)->vx)
#define dock_view_y(d) ((d)->vy)
struct BitMap {int Depth;};
struct RastPort {void *Layer;struct BitMap *BitMap;int TxHeight;};
struct Window {int Width,Height;struct RastPort *RPort;};
struct Screen {int MouseX,MouseY;struct RastPort RastPort;};
struct NDDock {struct Window *window;struct Screen *screen;struct BitMap *hover_frame;int hidden,box_pending,hover_shelf_valid,hover_pointer,width,height,hover_frame_width,hover_frame_height,caption_dirty,vx,vy;
 unsigned long frame_average_us,frame_peak_us,frame_samples;
 unsigned long hover_shelf_serial,diagnostic_hover_full,diagnostic_hover_partial,diagnostic_hover_skipped;int hover,music_hover_action,music_press_action,press_item,is_drag,edit_mode,overflow,view_start;struct {int count,drag_bar;} conf;
 int hover_box_paint,hover_prev_valid,hover_anim_advance,hover_animating;long hover_prev_key[12];struct NDRect hover_rects[ND_MAX_ITEMS],hover_prev_rects[ND_MAX_ITEMS],hover_prev_shelf,hover_shelf_rect;unsigned char hover_tile_changed[ND_MAX_ITEMS];
 struct NDRect hover_ext[ND_MAX_ITEMS],hover_prev_ext[ND_MAX_ITEMS];int hover_prev_index,wave_span_active,wave_span_lo,wave_span_hi;unsigned char wave_draw[ND_MAX_ITEMS],hover_visible[ND_MAX_ITEMS];};
static int item_visible(const struct NDDock*d,int i){(void)d;(void)i;return 1;}
static void tile_coords(struct NDDock*d,int i,int*x,int*y){(void)d;*x=100+i*60;*y=50;}
static void tile_dimensions(struct NDDock*d,int i,int*w,int*h){(void)d;(void)i;*w=48;*h=48;}
static int span_x=-1,span_w=-1,grip_present;
static struct Window *visible;static int allocations,frees,fail,paints,presents,pending,phase,direct;
static int vertical(struct NDDock*d){(void)d;return 0;}
static void draw_all(struct NDDock*d){(void)d;assert(0);}
static void prepare_hover_layout(struct NDDock*d){assert(d->hover_anim_advance==!d->hover_box_paint);}
static void kick_animation_timer(struct NDDock*d){(void)d;assert(0);}
static int sync_dock_viewport(struct NDDock*d){(void)d;return !pending;}
static void *LayersBase=(void*)1;static int scrollx,scrolly,layer_locked;
static void LockLayer(int zero,void *p){assert(!zero&&p&&!layer_locked);layer_locked=1;}
static void UnlockLayer(void *p){assert(p&&layer_locked&&!scrollx&&!scrolly);layer_locked=0;}
static void ScrollLayer(int zero,void *p,int x,int y){assert(!zero&&p&&layer_locked);scrollx+=x;scrolly+=y;}
static void WaitBlit(void){}
static void FreeBitMap(struct BitMap*b){++frees;free(b);}
static struct BitMap *AllocBitMap(int w,int h,int depth,int flags,struct BitMap*friend)
{struct BitMap*b;(void)flags;(void)friend;assert(w>0&&h>0&&depth==8);if(fail)return NULL;++allocations;b=malloc(sizeof(*b));b->Depth=depth;return b;}
static void paint_background(struct NDDock*d,int x,int y,int w,int h)
{assert(!phase&&y==0&&h==d->height);if(span_x<0)assert(x==0&&w==d->width);else assert(x==span_x&&w==span_w);phase=1;++paints;if(!fail)assert(d->window!=visible&&d->window->RPort->Layer==NULL&&d->window->RPort->BitMap==d->hover_frame);else{assert(d->window==visible&&layer_locked&&scrollx==23&&scrolly==19);++direct;}}
static void paint_hover_wave(struct NDDock*d){(void)d;assert(phase==1);phase=2;}
static void BltBitMapRastPort(struct BitMap*b,int sx,int sy,struct RastPort*rp,int dx,int dy,int w,int h,int mt)
{assert(b&&phase==2&&rp==visible->RPort&&sy==19&&dx==sx-23&&dy==sy-19&&h==visible->Height&&mt==0xc0);
 if(span_x<0)assert(sx==23&&w==visible->Width);else assert(sx==(grip_present ? 23:(span_x>23 ? span_x:23))&&sx+w==span_x+span_w);phase=0;++presents;}
'''+s[a:b]+r'''
int main(void){struct NDDock d;struct Window win;struct RastPort rp;struct Screen screen;struct BitMap bitmap;int i;
 memset(&d,0,sizeof(d));bitmap.Depth=8;screen.RastPort.BitMap=&bitmap;rp.Layer=&win;rp.BitMap=&bitmap;win.RPort=&rp;win.Width=700;win.Height=100;visible=&win;d.window=&win;d.screen=&screen;d.width=800;d.height=200;d.vx=23;d.vy=19;d.hover_shelf_valid=1;
 d.conf.count=3;for(i=0;i<3;++i){d.hover_rects[i].left=100+i*60;d.hover_rects[i].top=50;d.hover_rects[i].width=d.hover_rects[i].height=48;}
 d.hover_shelf_rect.left=90;d.hover_shelf_rect.width=200;d.hover_shelf_rect.height=60;
 /* First frame in full; an unchanged frame is neither recomposed nor presented. */
 for(i=0;i<20;++i){draw_hover(&d);assert(d.window==&win&&d.caption_dirty);}
 assert(allocations==1&&presents==1&&paints==1&&d.diagnostic_hover_skipped==19);
 /* A tile that grows recomposes only its old and new span. */
 d.hover_rects[1].left=150;d.hover_rects[1].width=64;span_x=148;span_w=68; /* artwork, tile area and label extent, old and new */
 draw_hover(&d);assert(paints==2&&presents==2&&d.diagnostic_hover_partial==1);
 /* A repainted tile (content update) is recomposed even if it did not move. */
 d.hover_tile_changed[2]=1;span_x=218;span_w=52;draw_hover(&d);assert(paints==3&&!d.hover_tile_changed[2]);
 /* Moving the hover from tile 0 to tile 2 recomposes just those two tiles. */
 span_x=98;span_w=172;d.hover=2;draw_hover(&d);assert(paints==4&&d.diagnostic_hover_partial==3);
 /* A grip update must present the cleared leading pixels as well as the tile span. */
 d.conf.drag_bar=1;grip_present=1;d.hover_tile_changed[2]=1;span_x=218;span_w=52;draw_hover(&d);assert(paints==5&&presents==5);d.conf.drag_bar=0;grip_present=0;
 /* No tile hovered at all changes every label: a full frame, as does new shelf content. */
 span_x=-1;d.hover=-1;draw_hover(&d);assert(paints==6&&d.diagnostic_hover_full==2);
 ++d.hover_shelf_serial;draw_hover(&d);assert(paints==7);
 d.hover_prev_valid=0;draw_hover(&d);assert(paints==8&&presents==8);   /* damage */
 pending=1;draw_hover(&d);assert(paints==8&&presents==8);pending=0;
 d.width+=4;draw_hover(&d);assert(allocations==2&&frees==1&&presents==9);
 d.hover_box_paint=1;d.hover_prev_valid=0;span_x=-1;draw_hover(&d);assert(d.hover_box_paint);d.hover_box_paint=0;
 FreeBitMap(d.hover_frame);d.hover_frame=NULL;fail=1;draw_hover(&d);assert(d.window==&win&&direct==1&&presents==10&&!d.hover_prev_valid);
 assert(!scrollx&&!scrolly&&!layer_locked);
 puts("Hover frames: full first frame, unchanged frames skipped, changed spans only, hover moves as spans, full recompose on state/damage, warm reuse, resize invalidation, pending geometry and low-memory fallback pass.");return 0;}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(root/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
