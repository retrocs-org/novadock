#!/usr/bin/env python3
"""Exercise incoming-drag state and rendered edges using production functions."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

def function(path, signature):
    source=(ROOT/path).read_text()
    start=source.index(signature+'\n{')
    return source[start:source.index('\n}',start)+2]

class DropPreviewTests(unittest.TestCase):
    def test_workbench_hook_restores_pixels_and_stays_inside_its_zone(self):
        harness=r"""
#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) view_x
#define dock_view_y(d) view_y

#include <string.h>
#define __reg(x)
typedef unsigned long ULONG;
typedef void *APTR;
#define ADZMACTION_Enter 0
#define ADZMACTION_Leave 1
#define COMPLEMENT 2
struct IBox {int Left,Top,Width,Height;};
struct RastPort {int DrawMode,Area;};
struct Hook {void *h_Data;};
struct AppWindowDropZoneMsg {
 struct RastPort *adzm_RastPort;struct IBox adzm_DropZoneBox;
 ULONG adzm_UserData;int adzm_Action;
};
struct NDDock {int edit_mode;};
static int orientation,fills,pixels[100][120],view_x,view_y;
static int item_visible(struct NDDock *d,int i) {(void)d;return i==0;}
static void tile_coords(struct NDDock *d,int i,int *x,int *y)
{(void)d;(void)i;*x=10;*y=10;}
static void tile_dimensions(struct NDDock *d,int i,int *w,int *h)
{(void)d;(void)i;*w=100;*h=80;}
static int hovered_grip(struct NDDock*d,int x,int y){(void)d;(void)x;(void)y;return 0;}
static int vertical(struct NDDock *d) {(void)d;return orientation;}
static void SetDrMd(struct RastPort *r,int m) {r->DrawMode=m;}
static void SetAfPt(struct RastPort *r,void *p,int n) {(void)p;(void)n;r->Area=0;}
static void RectFill(struct RastPort *r,int l,int t,int rr,int b)
{int x,y;assert(r->DrawMode==COMPLEMENT);++fills;
 for(y=t;y<=b;++y)for(x=l;x<=rr;++x){assert(x>=0&&x<120&&y>=0&&y<100);pixels[y][x]^=1;}}
"""
        source=(ROOT/'src/dock_input.c').read_text()
        start=source.index('static ULONG drop_zone_hook(')
        harness+=source[start:source.index('\n}',start)+2]
        harness+=r"""
int main(void)
{
 struct NDDock d={1};struct Hook h;struct RastPort rp={7,9};
 struct AppWindowDropZoneMsg m;int i,x,y,total;
 h.h_Data=&d;memset(&m,0,sizeof(m));m.adzm_RastPort=&rp;
 for(i=0;i<4;++i){
  orientation=i/2;m.adzm_UserData=i%2;
  view_x=i?5:0;view_y=i?5:0;
  m.adzm_DropZoneBox.Left=10-view_x+(orientation?0:(i%2)*50);
  m.adzm_DropZoneBox.Top=10-view_y+(orientation?(i%2)*40:0);
  m.adzm_DropZoneBox.Width=orientation?100:50;
  m.adzm_DropZoneBox.Height=orientation?40:80;
  m.adzm_Action=ADZMACTION_Enter;assert(drop_zone_hook(&h,0,&m)==0);
  total=0;for(y=0;y<100;++y)for(x=0;x<120;++x)if(pixels[y][x]){
   ++total;assert(x>=m.adzm_DropZoneBox.Left&&x<m.adzm_DropZoneBox.Left+m.adzm_DropZoneBox.Width);
   assert(y>=m.adzm_DropZoneBox.Top&&y<m.adzm_DropZoneBox.Top+m.adzm_DropZoneBox.Height);}
  assert(total==(orientation?84:64)*3);
  assert(rp.DrawMode==7&&rp.Area==9);
  m.adzm_Action=ADZMACTION_Leave;drop_zone_hook(&h,0,&m);
  for(y=0;y<100;++y)for(x=0;x<120;++x)assert(!pixels[y][x]);
 }
 i=fills;m.adzm_Action=99;drop_zone_hook(&h,0,&m);assert(fills==i);
 m.adzm_Action=ADZMACTION_Enter;d.edit_mode=0;drop_zone_hook(&h,0,&m);assert(fills==i);
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as temporary:
            path=Path(temporary);(path/'hook.c').write_text(harness)
            subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',
                            str(path/'hook.c'),'-o',str(path/'hook')],check=True)
            subprocess.run([str(path/'hook')],check=True)

    def test_preview_matches_drop_edges_and_clears_without_idle_redraws(self):
        harness=r"""
#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) view_x
#define dock_view_y(d) view_y
#include <string.h>
#include "nd_config.h"
#include "nd_interaction.h"
#include "nd_layout.h"
#include "nd_autohide.h"
#define ND_BORDER 7
#define IEQUALIFIER_LEFTBUTTON 0x4000
#define ND_ACCENT 1
struct Layer {int Scroll_X,Scroll_Y;};
struct RastPort {int TxHeight;struct Layer *Layer;};
struct Window {int LeftEdge,TopEdge,Width,Height;struct RastPort *RPort;};
struct Screen {int MouseX,MouseY;};
struct NDDock {int grip_drag;
 struct NDConfig conf;struct Window *window;struct Screen *screen;
 int hover_headroom,edit_mode,hidden,covered,box_pending,popup,calendar,folder_popup;
 int hover_plan_ready,hover,hover_pad_x,hover_pad_y,tile_width,tile_height;
 struct NDRect hover_rects[ND_MAX_ITEMS];
 struct NDHoverLayoutItem hover_layout[ND_MAX_ITEMS];
 int press_item,is_drag,drag_to,drag_after;
 int drop_preview,drop_preview_item,drop_preview_after,drop_zones;
 int overflow,view_start,visible_count;
};
static int input,orientation,qualifier,peeks,paints,tips,persistent=6,scroll;
static int first_visible,last_visible=6,view_x,view_y;
static int rect[4],fills;
#define InputBase (&input)
static int PeekQualifier(void) {++peeks;return qualifier;}
static int hovered_grip(struct NDDock*d,int x,int y){(void)d;(void)x;(void)y;return 0;}
static int vertical(struct NDDock *d) {(void)d;return orientation;}
static void tile_coords(struct NDDock *d,int i,int *x,int *y)
{(void)d;*x=10+(i%2)*110;*y=10+(i/2)*90;}
static void tile_dimensions(struct NDDock *d,int i,int *w,int *h)
{(void)d;(void)i;*w=100;*h=80;}
static int item_visible(struct NDDock *d,int i)
{(void)d;return i>=first_visible && i<last_visible;}
static int nd_media_persistent_count(struct NDConfig *c) {(void)c;return persistent;}
static int hovered_scroll(struct NDDock *d,int x,int y) {(void)d;(void)x;(void)y;return scroll;}
static void close_tooltip(struct NDDock *d) {(void)d;++tips;}
static void draw_all(struct NDDock *d) {(void)d;++paints;}
static void fill(struct NDDock *d,int color,int x,int y,int w,int h)
{(void)d;assert(color==ND_ACCENT);++fills;rect[0]=x;rect[1]=y;rect[2]=w;rect[3]=h;}
"""
        for path,signature in [
            ('src/dock_window.c','int hovered_tile(struct NDDock *dock, int mx, int my)'),
            ('src/dock_reload.c','int drop_after_tile(struct NDDock *dock,int index,int mx,int my)'),
            ('src/dock_timers.c','void update_drop_preview(struct NDDock *dock)'),
            ('src/dock_draw.c','static void draw_insertion_marker(struct NDDock *dock)'),
        ]:
            harness+='\n'+function(path,signature)
        harness+=r"""
static void pointer(struct NDDock *d,int x,int y)
{d->screen->MouseX=d->window->LeftEdge+x;d->screen->MouseY=d->window->TopEdge+y;update_drop_preview(d);}
int main(void)
{
 static struct NDDock d;
 struct RastPort rp={8,NULL};struct Layer layer={0,0};
 struct Window w={100,200,250,290,&rp};struct Screen s;
 int before;
 d.window=&w;d.screen=&s;d.conf.count=6;d.press_item=-1;
 pointer(&d,30,30);assert(!d.drop_preview && !paints && !peeks);
 d.edit_mode=1;pointer(&d,30,30);assert(!d.drop_preview && !paints);
 qualifier=IEQUALIFIER_LEFTBUTTON;pointer(&d,30,30);
 assert(d.drop_preview && d.drop_preview_item==0 && !d.drop_preview_after && paints==1);
 draw_insertion_marker(&d);assert(fills==1 && rect[0]==7 && rect[1]==18 && rect[2]==3 && rect[3]==64);
 before=paints;pointer(&d,35,35);assert(paints==before);
 pointer(&d,90,30);assert(d.drop_preview_after && paints==before+1);
 draw_insertion_marker(&d);assert(rect[0]==110);
 pointer(&d,30,120);assert(d.drop_preview_item==2 && !d.drop_preview_after);
 draw_insertion_marker(&d);assert(rect[0]==7 && rect[1]==108);
 orientation=1;pointer(&d,30,75);assert(d.drop_preview_item==0 && d.drop_preview_after);
 draw_insertion_marker(&d);assert(rect[0]==18 && rect[1]==90 && rect[2]==84 && rect[3]==3);
 orientation=0;persistent=4;pointer(&d,30,220);
 assert(d.drop_preview_item==3 && d.drop_preview_after); /* append before transient media */
 pointer(&d,240,120);assert(d.drop_preview_item==3 && d.drop_preview_after); /* background appends */
 d.overflow=1;d.view_start=2;d.visible_count=2;scroll=-1;
 pointer(&d,240,120);assert(d.drop_preview_item==2 && !d.drop_preview_after);
 scroll=1;pointer(&d,240,120);assert(d.drop_preview_item==3 && d.drop_preview_after);
 scroll=0;last_visible=3;pointer(&d,240,120);assert(!d.drop_preview); /* no off-page marker */
 last_visible=6;d.overflow=0;pointer(&d,30,30);assert(d.drop_preview);
 qualifier=0;pointer(&d,30,30);assert(!d.drop_preview);
 before=paints;pointer(&d,30,30);assert(paints==before);
 qualifier=IEQUALIFIER_LEFTBUTTON;pointer(&d,30,30);
 pointer(&d,-1,30);assert(!d.drop_preview); /* pointer leaves */
 pointer(&d,30,30);d.covered=1;pointer(&d,30,30);assert(!d.drop_preview);
 d.covered=0;d.popup=1;pointer(&d,30,30);assert(!d.drop_preview);
 d.popup=0;d.press_item=0;pointer(&d,30,30);assert(!d.drop_preview);
 d.press_item=-1;d.is_drag=1;pointer(&d,30,30);assert(!d.drop_preview);
 d.is_drag=0;pointer(&d,30,30);d.edit_mode=0;pointer(&d,30,30);assert(!d.drop_preview);
 d.edit_mode=1;pointer(&d,30,30);d.hidden=1;pointer(&d,30,30);assert(!d.drop_preview);
 d.hidden=0;pointer(&d,30,30);d.box_pending=1;pointer(&d,30,30);assert(!d.drop_preview);
 d.box_pending=0;persistent=0;pointer(&d,30,30);assert(!d.drop_preview);
 d.drop_zones=1;pointer(&d,30,30);assert(!d.drop_preview);
 d.drop_zones=0;
 /* Existing internal reorder feedback remains independent. */
 d.is_drag=1;d.drag_to=2;d.drag_after=1;draw_insertion_marker(&d);assert(rect[0]==110 && rect[1]==108);
 /* Drop insertion uses the magnified artwork, not the stale base cell. */
 d.hover_headroom=d.hover_plan_ready=1;orientation=0;
 d.hover_rects[0].left=120;d.hover_rects[0].top=60;d.hover_rects[0].width=80;d.hover_rects[0].height=100;
 assert(!drop_after_tile(&d,0,130,70));assert(drop_after_tile(&d,0,185,70));
 d.is_drag=0;d.drop_preview=1;d.drop_preview_item=0;d.drop_preview_after=1;
 draw_insertion_marker(&d);assert(rect[0]==200&&rect[1]==68&&rect[2]==3&&rect[3]==84);
 /* Markers on the real unscrolled window use local coordinates. */
 view_x=12;view_y=20;rp.Layer=&layer;
 draw_insertion_marker(&d);assert(rect[0]==188&&rect[1]==48);
 /* Locked direct-paint fallback is temporarily scrolled; offscreen is logical. */
 layer.Scroll_X=12;layer.Scroll_Y=20;draw_insertion_marker(&d);assert(rect[0]==200&&rect[1]==68);
 rp.Layer=NULL;draw_insertion_marker(&d);assert(rect[0]==200&&rect[1]==68);
 orientation=1;assert(!drop_after_tile(&d,0,130,80));assert(drop_after_tile(&d,0,130,140));
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as temporary:
            path=Path(temporary);(path/'preview.c').write_text(harness)
            subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',
                            '-I'+str(ROOT/'include'),str(path/'preview.c'),
                            str(ROOT/'src/interaction.c'),'-o',str(path/'preview')],check=True)
            subprocess.run([str(path/'preview')],check=True)

if __name__=='__main__':
    unittest.main()
