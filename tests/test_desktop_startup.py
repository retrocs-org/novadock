#!/usr/bin/env python3
"""Startup transparency: real capture/policy functions, no devices or emulator."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_window.c').read_text()
def function(signature):
 a=s.index(signature+'\n{');return s[a:s.index('\n}',a)+2]
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
#include "nd_autohide.h"
#include "nd_trace.h"
#include "nd_input_policy.h"
#define BMF_CLEAR 0
#define NULL_PORT ((void *)1)
#define WINDOW_BACKMOST ((struct Window *)0)
#define WFLG_BACKDROP 0x40
#define STRPTR char *
typedef unsigned long ULONG;
struct BitMap {int Depth,ink;};struct RastPort {struct BitMap *BitMap;};
struct Gadget {struct Gadget *NextGadget;int GadgetType,Flags;};
#define IDCMP_RAWKEY 1024
#define IDCMP_VANILLAKEY 2048
#define GTYP_GTYPEMASK 7
#define GTYP_STRGADGET 4
#define GFLG_SELECTED 128
struct Layer {int Flags;};
#define LAYERREFRESH 128
#define LAYERUPDATING 16
struct Window {struct Layer *WLayer;unsigned long IDCMPFlags;struct Gadget *FirstGadget;int LeftEdge,TopEdge,Width,Height,Flags;struct Window *NextWindow;};
struct Screen {struct RastPort RastPort;struct Window *FirstWindow;int MouseX,MouseY;};
struct NDDock {
 struct {int theme_artwork,background_transparency;} conf;
 struct Window *window;struct Screen *screen;struct BitMap *hover_desktop;int hover_desktop_width,hover_desktop_height;
 void *app_window;struct NDRect full_box;
 int width,height,screen_closed,hover_shelf_valid,hidden,hover_headroom;
 int desktop_scene_dirty,desktop_scene_valid;
 int keyboard,desktop_refresh_pending,desktop_ready_ms,desktop_probe_ms;
 int covered,box_pending,press_item,is_drag,drop_preview,popup,calendar,folder_popup,about,tooltip,feedback,icons_pending;
};
static struct {struct {int lib_Version;} LibNode;struct Window *ActiveWindow;} intuition;
#define IntuitionBase (&intuition)
static struct Screen screen;static struct Window original,replacement,other,backdrop;
static struct BitMap desktop,display;static struct Layer slow_layer;static int slow_ticks,delay_ticks,expected_ink=2;
static int visible,hide_calls,show_calls,closes,creates,attaches,detaches,draws,copies,allocations,port_queries;
static int fail_hide,fail_show,fail_create,fail_alloc,fail_lock,workbench,screen_locked,base_locked,released;
static struct Screen *LockPubScreen(char*n){assert(!strcmp(n,"Workbench"));if(fail_lock)return NULL;assert(!screen_locked);screen_locked=1;return &screen;}
static void UnlockPubScreen(void*n,struct Screen*s){(void)n;assert(s==&screen&&screen_locked);screen_locked=0;}
static int alloc_w,alloc_h;
static struct BitMap *AllocBitMap(int w,int h,int dep,int f,struct BitMap*b){(void)dep;(void)f;(void)b;++allocations;alloc_w=w;alloc_h=h;return fail_alloc?NULL:&desktop;}
static void release_desktop_snapshot(struct NDDock*d){d->hover_desktop=NULL;d->hover_desktop_width=d->hover_desktop_height=0;}


static void close_caption_popup(struct NDDock*d){(void)d;}
static void close_tooltip(struct NDDock*d){(void)d;}
static void detach_appwindow(struct NDDock*d){++detaches;d->app_window=NULL;}
static void CloseWindow(struct Window*w){assert(w==&original);++closes;visible=0;display.ink=2;}
static struct Window *create_dock_window(struct NDDock*d,const struct NDRect*b){(void)d;++creates;if(fail_create)return NULL;replacement=original;assert(b->left==original.LeftEdge&&b->top==original.TopEdge&&b->width==original.Width&&b->height==original.Height);visible=1;display.ink=9;return &replacement;}
static void apply_dock_font(struct NDDock*d){(void)d;}
static void attach_appwindow(struct NDDock*d){++attaches;d->app_window=NULL_PORT;}
static void release_icons(struct NDDock*d){(void)d;++released;}
static void release_dock_font(struct NDDock*d){(void)d;++released;}
static void free_colors(struct NDDock*d){d->hover_desktop=NULL;++released;}
static void Delay(int n){assert((n==3||n==1)&&!visible&&screen_locked);delay_ticks+=n;if(slow_ticks>0){slow_ticks-=n;if(slow_ticks<=0)slow_layer.Flags=0;}}
static void WaitBlit(void){}
static void BltBitMap(struct BitMap*b,int x,int y,struct BitMap*to,int dx,int dy,int w,int h,int m,int mask,void*p){(void)m;(void)mask;(void)p;assert(!visible&&screen_locked);assert(b==&display&&x==10&&y==20&&dx==0&&dy==0&&w==40&&h==30);++copies;to->ink=b->ink;}
static void prepare_transparency(struct NDDock*d){(void)d;}
static void draw_all(struct NDDock*d){assert(d->window&&d->hover_desktop->ink==expected_ink&&!d->hover_shelf_valid&&visible);++draws;}
static ULONG LockIBase(int n){assert(!n&&!base_locked);base_locked=1;return 7;}
static void UnlockIBase(ULONG n){assert(n==7&&base_locked);base_locked=0;}
#define IEQUALIFIER_LEFTBUTTON 1
static void *InputBase;static int PeekQualifier(void){return 0;}
static void Forbid(void){}static void Permit(void){}
static void *FindPort(char*n){assert(!strcmp(n,"WORKBENCH"));++port_queries;return workbench?NULL_PORT:NULL;}
'''
draw=(ROOT/'src/dock_draw.c').read_text()
a=draw.index('int ensure_desktop_snapshot(struct NDDock *dock,struct BitMap *friend_bitmap)\n{');h+='\n'+draw[a:draw.index('\n}',a)+2]
for sig in ('static int desktop_repaint_pending(struct Screen *screen,const struct NDRect *box)',
 'static int recapture_desktop(struct NDDock *dock)',
 'int refresh_dock_desktop(struct NDDock *dock)',
 'static int desktop_region_clear(struct NDDock *dock)',
 'static int desktop_input_busy(struct NDDock *dock)',
 'void desktop_startup_tick(struct NDDock *dock,int milliseconds)'):
 h+='\n'+function(sig)
h+=r'''
static void init(struct NDDock*d)
{
 memset(d,0,sizeof(*d));memset(&screen,0,sizeof(screen));memset(&original,0,sizeof(original));memset(&other,0,sizeof(other));memset(&backdrop,0,sizeof(backdrop));
 hide_calls=show_calls=closes=creates=attaches=detaches=draws=copies=allocations=port_queries=0;slow_layer.Flags=0;slow_ticks=delay_ticks=0;expected_ink=2;
 fail_hide=fail_show=fail_create=fail_alloc=fail_lock=screen_locked=base_locked=released=0;workbench=visible=1;
 original.LeftEdge=10;original.TopEdge=20;original.Width=40;original.Height=30;
 other.LeftEdge=70;other.TopEdge=20;other.Width=10;other.Height=10;backdrop.Width=100;backdrop.Height=100;backdrop.Flags=WFLG_BACKDROP;
 original.NextWindow=&other;other.NextWindow=&backdrop;screen.FirstWindow=&original;screen.RastPort.BitMap=&display;
 intuition.LibNode.lib_Version=47;intuition.ActiveWindow=&other;
 desktop.ink=1;display.ink=9;display.Depth=8;
 d->window=&original;d->screen=&screen;d->hover_desktop=&desktop;d->hover_desktop_width=40;d->hover_desktop_height=30;d->app_window=NULL_PORT;d->width=40;d->height=30;d->full_box.left=10;d->full_box.top=20;d->full_box.width=40;d->full_box.height=30;d->press_item=-1;
 d->desktop_refresh_pending=1;d->hover_headroom=16;d->conf.theme_artwork=1;
}
static void tick(struct NDDock*d,int seconds){int i;for(i=0;i<seconds;++i)desktop_startup_tick(d,1000);}
int main(void)
{
 struct NDDock d;int i,mode;
 init(&d);backdrop.WLayer=&slow_layer;slow_layer.Flags=LAYERREFRESH;slow_ticks=12;
 assert(recapture_desktop(&d)&&delay_ticks>=12&&copies==1&&desktop.ink==2);
 init(&d);backdrop.WLayer=&slow_layer;slow_layer.Flags=LAYERUPDATING;slow_ticks=100;expected_ink=1;
 assert(!recapture_desktop(&d)&&delay_ticks==28&&!copies&&desktop.ink==1&&visible&&draws==1);
 slow_layer.Flags=0;expected_ink=2;d.window=&original;assert(recapture_desktop(&d)&&copies==1);

 for(mode=0;mode<2;++mode){init(&d);assert(recapture_desktop(&d));assert(desktop.ink==2&&copies==1&&draws==1&&!screen_locked&&visible);assert(!released);assert(d.hidden==0);assert(intuition.ActiveWindow==&other);
  assert(!hide_calls&&!show_calls&&closes==1&&creates==1&&attaches==1);}
 init(&d);d.hidden=1;original.Width=original.Height=8;assert(recapture_desktop(&d)&&d.hidden&&replacement.Width==8&&replacement.Height==8);
 init(&d);fail_hide=1;assert(recapture_desktop(&d)&&visible&&closes==1&&creates==1&&copies==1&&!screen_locked);
 init(&d);fail_show=1;assert(recapture_desktop(&d)&&closes==1&&creates==1&&visible);
 init(&d);fail_create=1;assert(!recapture_desktop(&d)&&!d.window&&!d.screen&&d.screen_closed&&released==3&&!screen_locked);
 init(&d);d.hover_desktop=NULL;fail_alloc=1;assert(!recapture_desktop(&d)&&visible&&!hide_calls&&!copies&&!screen_locked);
 init(&d);fail_lock=1;assert(!recapture_desktop(&d)&&visible&&!hide_calls&&!copies);
 /* A snapshot left at an older dock size is replaced before the blit:
  * BltBitMap never clips, so the old bitmap would be overrun. */
 init(&d);d.hover_desktop_width=20;assert(recapture_desktop(&d)&&allocations==1&&alloc_w==40&&alloc_h==30&&d.hover_desktop_width==40&&d.hover_desktop_height==30);
 init(&d);assert(recapture_desktop(&d)&&allocations==0);
 init(&d);workbench=0;tick(&d,20);assert(d.desktop_refresh_pending&&!copies&&!d.desktop_ready_ms);
 workbench=1;other.LeftEdge=15;tick(&d,20);assert(!copies&&!d.desktop_ready_ms);other.LeftEdge=70;
 tick(&d,2);assert(!copies&&d.desktop_ready_ms==2000);tick(&d,1);assert(copies==1&&!d.desktop_refresh_pending);
 i=port_queries;tick(&d,100);assert(copies==1&&port_queries==i); /* no permanent polling */
 init(&d);for(i=0;i<9;++i)desktop_startup_tick(&d,100);assert(!port_queries);desktop_startup_tick(&d,100);assert(port_queries==1);
 init(&d);d.about=1;tick(&d,10);assert(!copies);d.about=0;d.drop_preview=1;tick(&d,10);assert(!copies);d.drop_preview=0;
 screen.MouseX=15;screen.MouseY=25;tick(&d,10);assert(!copies);screen.MouseX=screen.MouseY=0;
 intuition.ActiveWindow=&original;tick(&d,10);assert(!copies);intuition.ActiveWindow=&other;tick(&d,3);assert(copies==1);
 init(&d);fail_lock=1;tick(&d,3);assert(d.desktop_refresh_pending&&!copies);fail_lock=0;tick(&d,3);assert(!d.desktop_refresh_pending&&copies==1);
 init(&d);d.hover_headroom=0;d.conf.theme_artwork=0;tick(&d,1);assert(!d.desktop_refresh_pending&&!copies);
 init(&d);d.hover_headroom=0;tick(&d,3);assert(copies==1&&!d.desktop_refresh_pending);
 /* Runtime repair never closes the dock window under the pointer: a click
  * in that gap would reach Workbench.  It runs as soon as the pointer
  * leaves; a tooltip or an active dock window alone no longer blocks it. */
 init(&d);d.desktop_refresh_pending=2;d.desktop_scene_valid=1;screen.MouseX=15;screen.MouseY=25;d.tooltip=1;intuition.ActiveWindow=&original;
 for(i=0;i<20;++i)desktop_startup_tick(&d,200);assert(!copies&&d.desktop_refresh_pending==2);
 screen.MouseX=5;screen.MouseY=5;desktop_startup_tick(&d,200);assert(copies==1&&!d.desktop_refresh_pending);
 init(&d);d.desktop_refresh_pending=2;d.desktop_scene_valid=1;d.keyboard=1;
 desktop_startup_tick(&d,200);assert(!copies&&d.desktop_refresh_pending);d.keyboard=0;desktop_startup_tick(&d,200);assert(copies==1);
 {struct Gadget field;memset(&field,0,sizeof(field));field.GadgetType=GTYP_STRGADGET;field.Flags=GFLG_SELECTED;
 init(&d);other.FirstGadget=&field;tick(&d,4);assert(!copies);d.desktop_refresh_pending=2;d.desktop_scene_valid=1;
 desktop_startup_tick(&d,200);assert(!copies&&d.desktop_refresh_pending);field.Flags=0;desktop_startup_tick(&d,200);assert(copies==1);}
 init(&d);other.IDCMPFlags=IDCMP_RAWKEY;tick(&d,3);assert(copies==1); /* idle keyboard subscribers do not block repair */
 puts("Desktop startup: delayed Workbench, clear-region settling, modal/input deferral, no self-capture, V47/legacy window paths, hidden docks, failure recovery and zero steady-state polling passed.");return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
