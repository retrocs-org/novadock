#!/usr/bin/env python3
"""Exercise the production hover cache: neighbours, invalidation and exhaustion."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/dock_icons.c').read_text()
draw_source=(ROOT/'src/dock_draw.c').read_text()
def function(signature,source=source):
 start=source.index(signature);end=source.index('\n}',start)+2
 return source[start:end]
header=(ROOT/'src/dock_internal.h').read_text()
structs=header[header.index('#define ND_HOVER_CACHE_SLOTS'):header.index('struct NDArtwork;')]
harness=r'''
#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef unsigned long ULONG;
#define ND_MAX_ITEMS 32
#define ND_LABEL_LEN 32
#define ND_HOVER_ICON_LIMIT 128
#define BMF_CLEAR 0
#define BMF_DISPLAYABLE 2
struct BitMap {int Depth;void *Planes[1];};
struct RastPort {struct BitMap *BitMap;};
struct Screen {struct RastPort RastPort;};
struct Window {struct RastPort *RPort;};
struct NDRect {int left,top,width,height;};
struct BitScaleArgs {int bsa_SrcX,bsa_SrcY,bsa_SrcWidth,bsa_XSrcFactor,bsa_SrcHeight,bsa_YSrcFactor,bsa_XDestFactor,bsa_YDestFactor,bsa_DestWidth,bsa_DestHeight;struct BitMap *bsa_SrcBitMap,*bsa_DestBitMap;};
'''+structs+r'''
struct NDDock {
 struct NDHoverScale hover_icons[ND_HOVER_CACHE_SLOTS];
 struct NDHoverBank hover_banks[ND_MAX_ITEMS];
 ULONG hover_cache_clock,hover_cache_budget,hover_cache_evictions;struct {int hover_cache_kb;} conf;
 struct Screen *screen;
 struct Window *window;
 int width,height;
 struct NDRect hover_rects[ND_MAX_ITEMS];
 struct BitMap *hover_shelf,*hover_widget_mask,*hover_frame;
 int hover_frame_width,hover_frame_height;
 int painting_mask;
 struct NDHoverWidget hover_widgets[ND_MAX_ITEMS];
 ULONG hover_shelf_serial;
 struct NDLabelWidth hover_label_width[ND_MAX_ITEMS];
 int hover_prev_valid,hover_anim_valid,hover_animating;
 int hover_shelf_width,hover_shelf_height,hover_shelf_valid;
};
#define MEMF_ANY 0
static unsigned long memory_available=8388608UL;
static unsigned long AvailMem(int flags){(void)flags;return memory_available;}
static int scales,allocations,frees,fail_alloc;
static void *GfxBase=(void *)1;
static void WaitBlit(void){}
static struct BitMap *AllocBitMap(int w,int h,int depth,int flags,struct BitMap *friend_bitmap)
{struct BitMap *b;(void)w;(void)h;(void)flags;(void)friend_bitmap;if(fail_alloc)return NULL;b=calloc(1,sizeof(*b));assert(b);b->Depth=depth;++allocations;return b;}
static void FreeBitMap(struct BitMap *b){assert(b);++frees;free(b);}
static int scale_hover_bitmap(struct BitMap *source,struct BitMap *dest,int w,int h,int zw,int zh)
{assert(source&&dest&&w>0&&h>0&&zw<=128&&zh<=128);++scales;return 1;}
'''+function('static ULONG hover_bitmap_bytes(')+'\n'+function('ULONG hover_cache_bytes(')+'\n'+function('int hover_cache_reserve(')+'\n'+function('int magnify_icon(')+'\n'+function('void release_hover_graphics(')+r'''
static int metric_source_box(struct NDDock*d,int i,int*x,int*y,int*w,int*h){(void)d;(void)i;(void)x;(void)y;(void)w;(void)h;return 0;}
static int widget_scales,widget_blits,widget_width=120,widget_height=100,caption_visible=1,masked;
static void widget_content_bounds(struct NDHoverWidget*c,int w,int h){c->content_left=c->content_top=0;c->content_width=w;c->content_height=h;}
static void mask_widget_caption(struct NDDock*d,struct NDHoverWidget*c,int i,int x,int y,int tw,int th,int w,int h){(void)d;(void)c;(void)i;(void)x;(void)y;(void)tw;(void)th;(void)w;(void)h;++masked;}
static int widget_wave_box(struct NDDock *d,int i,int *w,int *h){(void)d;(void)i;*w=widget_width;*h=widget_height;return 1;}
static int scale_planar_widget(struct BitScaleArgs *a){(void)a;return 0;}
static void BitMapScale(struct BitScaleArgs *a){++widget_scales;a->bsa_DestWidth=a->bsa_XDestFactor;a->bsa_DestHeight=a->bsa_YDestFactor;}
static void BltMaskBitMapRastPort(struct BitMap *b,int x,int y,struct RastPort *rp,int dx,int dy,int w,int h,int minterm,void *mask){(void)mask;(void)b;(void)x;(void)y;(void)rp;(void)dx;(void)dy;(void)w;(void)h;(void)minterm;++widget_blits;}
'''+function('static struct NDHoverWidget *widget_buffer(',draw_source)+'\n'+function('static void draw_widget_wave(',draw_source)+r'''
int main(void)
{
 struct NDDock dock;struct Screen screen;struct BitMap art[12],masks[12],*b,*m;
 int i,w,h,before;
 memset(&dock,0,sizeof(dock));memset(&screen,0,sizeof(screen));dock.screen=&screen;
 for(i=0;i<12;++i){art[i].Depth=8;masks[i].Depth=1;}
 screen.RastPort.BitMap=&art[0];
 /* Neighbours no longer evict each other from a single scratch pair. */
 for(i=0;i<5;++i){b=&art[i];m=&masks[i];w=h=32;assert(magnify_icon(&dock,i,&b,&m,&w,&h,64+i,64+i));}
 assert(scales==10);before=allocations;assert(hover_cache_bytes(&dock)<=dock.hover_cache_budget);
 for(i=0;i<5;++i){b=&art[i];m=&masks[i];w=h=32;assert(magnify_icon(&dock,i,&b,&m,&w,&h,64+i,64+i));assert(w==64+i);}
 assert(scales==10&&allocations==before);
 /* Bank animation/online state changes invalidate a reused bitmap address. */
 ++dock.hover_banks[0].serial;b=&art[0];m=&masks[0];w=h=32;
 assert(magnify_icon(&dock,0,&b,&m,&w,&h,64,64));assert(scales==12);
 b=&art[0];m=&masks[1];w=h=32;
 assert(magnify_icon(&dock,0,&b,&m,&w,&h,64,64));assert(scales==14);
 /* Oversized transition frames must not call BitMapScale on fixed storage. */
 before=scales;b=&art[0];m=&masks[0];w=h=32;
 assert(!magnify_icon(&dock,0,&b,&m,&w,&h,129,64));
 assert(!magnify_icon(&dock,0,&b,&m,&w,&h,64,256));assert(scales==before);
 dock.conf.hover_cache_kb=128;
 memory_available=131072UL;
 assert(hover_cache_reserve(&dock,16384UL,-1,-1));
 assert(dock.hover_cache_evictions>0 && hover_cache_bytes(&dock)+16384UL<=131072UL);
 dock.conf.hover_cache_kb=0;memory_available=8388608UL;
 /* The cache remains bounded after visiting more than eight items. */
 for(i=0;i<12;++i){b=&art[i];m=&masks[i];w=h=32;assert(magnify_icon(&dock,i,&b,&m,&w,&h,80,80));}
 assert(allocations-frees==2*ND_HOVER_CACHE_SLOTS);
 release_hover_graphics(&dock);assert(frees==allocations);
 fail_alloc=1;b=&art[0];m=&masks[0];w=h=32;
 assert(!magnify_icon(&dock,0,&b,&m,&w,&h,64,64));
 assert(b==&art[0]&&m==&masks[0]&&w==32&&h==32);
 release_hover_graphics(&dock);assert(frees==allocations);
 /* Live widgets reuse the enlarged pixels until their source or size changes. */
 fail_alloc=0;dock.width=600;dock.height=180;dock.hover_shelf=&art[0];dock.hover_widget_mask=&masks[0];
 dock.hover_shelf_valid=1;dock.hover_shelf_serial=1;
 {struct Window window;struct RastPort rp;window.RPort=&rp;dock.window=&window;
  draw_widget_wave(&dock,2,100,103,86,76);
  draw_widget_wave(&dock,2,100,103,86,76);
  assert(widget_scales==2&&widget_blits==2);
  ++dock.hover_shelf_serial;draw_widget_wave(&dock,2,100,103,86,76);
  assert(widget_scales==4&&widget_blits==3);
  widget_width=150;draw_widget_wave(&dock,2,100,103,86,76);
  assert(widget_scales==6&&widget_blits==4);
  assert(dock.hover_widgets[2].capacity_width==160 && dock.hover_widgets[2].capacity_height==128);
  caption_visible=0;draw_widget_wave(&dock,2,100,103,86,76);
  assert(widget_scales==6&&widget_blits==5&&masked==3);
  draw_widget_wave(&dock,2,100,103,86,76);assert(widget_scales==6&&masked==3);
  caption_visible=1;draw_widget_wave(&dock,2,100,103,86,76);assert(widget_scales==6&&masked==3);
 }
 /* Wide widgets use the unscaled shelf fallback rather than overflowing a 256px cache. */
 {struct Window window;struct RastPort rp;int previous=widget_scales;
  window.RPort=&rp;dock.window=&window;widget_width=300;draw_widget_wave(&dock,2,100,103,86,76);assert(widget_scales==previous);}
 dock.hover_shelf=NULL;dock.hover_widget_mask=NULL;release_hover_graphics(&dock);assert(frees==allocations);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='novadock-hover-cache-') as tmp:
 p=Path(tmp);(p/'cache.c').write_text(harness)
 command=[os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',str(p/'cache.c'),'-o',str(p/'cache')]
 if os.environ.get('SANITIZE')=='1':command+=['-fsanitize=address,undefined']
 subprocess.run(command,check=True);subprocess.run([str(p/'cache')],check=True)
print('Hover cache: warm neighbouring icons and widgets avoid repeated scaling; state invalidation, bounded memory and allocation failure pass.')
