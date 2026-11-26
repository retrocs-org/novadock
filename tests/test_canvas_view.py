#!/usr/bin/env python3
"""Production viewport bounds and scrolled logical coordinates across resize/hide."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_window.c').read_text()
def f(sig):
 a=s.index(sig+'\n{');return s[a:s.index('\n}',a)+2]
macros='\n'.join(x for x in (ROOT/'src/dock_internal.h').read_text().splitlines() if x.startswith(('#define dock_left','#define dock_top','#define dock_width','#define dock_height')))
h=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_autohide.h"
#include "nd_input_policy.h"
#define ND_HOVER_ART 1
#define IEQUALIFIER_LEFTBUTTON 0x4000
struct Layer {int Scroll_X,Scroll_Y;};struct RP {int TxHeight;};
struct Window {int LeftEdge,TopEdge,Width,Height;struct Layer*WLayer;struct RP*RPort;};
struct NDDock {int grip_drag;int canvas_view,hidden,hover_plan_ready,width,height,viewport_pending,box_pending,box_target_hidden,press_item,is_drag,box_origin_x,box_origin_y,box_pending_ticks,deferred_menu;unsigned int music_press_action;struct NDDeferredButtons deferred_buttons;struct {int count;} conf;struct Window*window;struct NDRect viewport,full_box,tab_box,window_box_target,hover_shelf_rect,hover_rects[3];struct {int kind;} hover_layout[3];};
static void *LayersBase=(void*)1;static int changes,epochs,qualifier;static void *InputBase=(void*)1;
static int PeekQualifier(void){return qualifier;}
static void note_pointer_epoch_now(struct NDDock*d){(void)d;++epochs;}
static void ScrollLayer(int unused,struct Layer*l,int x,int y){assert(!unused);l->Scroll_X+=x;l->Scroll_Y+=y;}
static void ChangeWindowBox(struct Window*w,int x,int y,int width,int height){(void)w;(void)x;(void)y;(void)width;(void)height;++changes;}
static void capture_view_underlay(struct NDDock*d,const struct NDRect*v){(void)d;(void)v;}
static int item_visible(struct NDDock*d,int i){(void)d;return i!=2;}
'''+macros+'\n'+f('void dock_window_box(struct NDDock *dock,int hidden,struct NDRect *box)')+'\n'+f('void apply_dock_viewport(struct NDDock *dock)')+'\n'+f('int sync_dock_viewport(struct NDDock *dock)')+r'''
int main(void){struct NDDock d;struct Layer l={0,0};struct RP rp={8};struct Window w={220,296,300,100,&l,&rp};struct NDRect b;memset(&d,0,sizeof(d));d.press_item=-1;d.canvas_view=d.hover_plan_ready=1;d.width=600;d.height=220;d.conf.count=3;d.window=&w;
 d.full_box.left=100;d.full_box.top=200;d.full_box.width=600;d.full_box.height=220;d.tab_box.left=10;d.tab_box.top=20;d.tab_box.width=d.tab_box.height=8;
 d.hover_shelf_rect.left=120;d.hover_shelf_rect.top=96;d.hover_shelf_rect.width=300;d.hover_shelf_rect.height=100;d.viewport=d.hover_shelf_rect;
 d.hover_rects[0].left=130;d.hover_rects[0].top=103;d.hover_rects[0].width=d.hover_rects[0].height=48;d.hover_layout[0].kind=ND_HOVER_ART;d.hover_rects[1]=d.hover_rects[0];d.hover_rects[1].left=230;d.hover_rects[2].left=-1000;
 assert(sync_dock_viewport(&d)&&!changes&&!l.Scroll_X&&!l.Scroll_Y&&epochs==0);
 assert(sync_dock_viewport(&d)&&epochs==0); /* no scroll, no new pointer epoch */assert(dock_left(&d)==100&&dock_top(&d)==200&&dock_width(&d)==600);
 d.hover_rects[0].left=100;d.hover_rects[0].top=20;d.hover_rects[0].width=d.hover_rects[0].height=128;
 /* Never resize under a held button or pending button events. */
 d.press_item=0;assert(sync_dock_viewport(&d)&&!changes);d.press_item=-1;
 qualifier=IEQUALIFIER_LEFTBUTTON;assert(sync_dock_viewport(&d)&&!changes);qualifier=0;
 d.music_press_action=1;assert(sync_dock_viewport(&d)&&!changes);d.music_press_action=0;
 d.deferred_buttons.count=1;assert(sync_dock_viewport(&d)&&!changes);d.deferred_buttons.count=0;
 assert(!sync_dock_viewport(&d)&&changes==1&&d.box_pending&&d.viewport_pending&&d.box_origin_x==220&&d.box_origin_y==296&&d.box_pending_ticks==0);b=d.window_box_target;assert(b.left==200&&b.top==220&&b.width==320&&b.height==176);
 w.LeftEdge=b.left;w.TopEdge=b.top;w.Width=b.width;w.Height=b.height;d.box_pending=0;l.Scroll_X=13;l.Scroll_Y=7;apply_dock_viewport(&d);assert(!l.Scroll_X&&!l.Scroll_Y&&epochs==1);assert(sync_dock_viewport(&d)&&changes==1&&dock_left(&d)==100&&dock_top(&d)==200);
 dock_window_box(&d,1,&b);assert(b.left==10&&b.top==20&&b.width==8&&b.height==8);d.hidden=1;apply_dock_viewport(&d);assert(!l.Scroll_X&&!l.Scroll_Y&&dock_width(&d)==w.Width);
 puts("Canvas viewport: held-button geometry freeze, unscrolled Intuition layers, visible bounds, hidden items, async growth, stable logical hit origins, warm geometry and auto-hide coordinates passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
