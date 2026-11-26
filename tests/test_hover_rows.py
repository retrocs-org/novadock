#!/usr/bin/env python3
"""Run the production row planner across strengths, edges and screen budgets."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_draw.c').read_text();a=s.index('static int hover_label_width(');b=s.index('static void prepare_hover_layout(struct NDDock *dock)\n{');production=s[a:s.index('\n}',b)+2]
wa=s.index('static int wave_widget(const struct NDItem *item)\n{');wb=s.index('}',wa)+1
widget=s[wa:wb]
h=r'''
#include <assert.h>
#include "nd_hover_policy.h"
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0

#include <string.h>
#include <stdio.h>
#include "nd_config.h"
#include "nd_layout.h"
#include "nd_autohide.h"
#include "nd_hover_ease.h"
#define ND_BORDER 7
#define ND_DRAG_BAR_SIZE 16
#define ND_HOVER_WIDGET_LIMIT 256
typedef char *STRPTR;
struct RastPort {int TxHeight;void *Font;};
struct NDLabelWidth {const void *font;int width;char text[ND_LABEL_LEN];};
struct Window {int LeftEdge,TopEdge;struct RastPort *RPort;};
struct Screen {int MouseX,MouseY;};
struct NDDock {int grip_drag;
 struct NDConfig conf;struct Window *window;struct Screen *screen;
 int width,height,is_drag,hover,hover_headroom,hover_pad_x,hover_pad_y,hover_axis_padding;int music_press_action,music_hover_action;
 int tile_width,tile_height,gap,layout_rows,hover_plan_ready,hover_pointer;
 int hover_anim_valid,hover_anim_advance,hover_animating,hover_anim_row,hover_anim_pointer;short hover_anim_w[ND_MAX_ITEMS],hover_anim_h[ND_MAX_ITEMS];
 struct NDHoverLayoutItem hover_layout[ND_MAX_ITEMS];
 struct NDRect hover_rects[ND_MAX_ITEMS],hover_shelf_rect;
 struct NDLabelWidth hover_label_width[ND_MAX_ITEMS];
};
static int vertical(struct NDDock*d){return d->conf.side==ND_SIDE_LEFT||d->conf.side==ND_SIDE_RIGHT;}
static void tile_coords(struct NDDock*d,int i,int*x,int*y){int cols=(d->conf.count+d->layout_rows-1)/d->layout_rows,row=i/cols,used=d->conf.count-row*cols;if(vertical(d)){*x=d->hover_pad_x+7;*y=d->hover_pad_y+7+i*(d->tile_height+d->gap);return;}if(used>cols)used=cols;*x=d->hover_pad_x+7+(i%cols)*(d->tile_width+d->gap)+(cols-used)*(d->tile_width+d->gap)/2;*y=d->hover_pad_y+7+row*(d->tile_height+d->gap);}
static void tile_dimensions(struct NDDock*d,int i,int*w,int*h){(void)i;*w=d->tile_width;*h=d->tile_height;}
static int metric_kind(struct NDDock*d,const struct NDItem*i){(void)d;(void)i;return 0;}
static int metric_source_box(struct NDDock*d,int i,int*x,int*y,int*w,int*h){(void)d;(void)i;(void)x;(void)y;(void)w;(void)h;return 0;}
'''+widget+r'''
static int hover_art_size(struct NDDock*d,int i,int tw,int th,int*w,int*h){(void)tw;(void)th;*w=*h=16;return d->conf.items[i].kind!=ND_ITEM_PLUGIN||i==3;}
static int text_lengths;
static int TextLength(struct RastPort*r,STRPTR p,int n){(void)r;(void)p;++text_lengths;return n*8;}
static const char *volume_label(struct NDItem*i){return i->label;}
static int nd_icon_art_top(int y,int th,int size,int tiny){return y+(th-size-(tiny?0:20))/2;}
'''+production+r'''
int main(void)
{
 struct NDDock d;struct Window w;struct Screen screen;struct RastPort rp;
 int side,rows,strength,room,focus,i,j,x,y,tw,th;
 for(side=0;side<=1;++side)for(rows=2;rows<=3;++rows)for(room=32;room<=96;room+=64)
 for(strength=0;strength<=100;strength+=25)for(focus=0;focus<9;++focus){
  int cols=(9+rows-1)/rows;
  memset(&d,0,sizeof(d));memset(&w,0,sizeof(w));rp.TxHeight=8;w.RPort=&rp;d.window=&w;d.screen=&screen;
  d.conf.count=9;d.conf.side=side;d.conf.hover_magnification=strength;d.layout_rows=rows;
  d.tile_width=86;d.tile_height=76;d.gap=5;d.hover_headroom=room;d.hover_axis_padding=d.hover_pad_x=64;
  d.hover_pad_y=side==ND_SIDE_BOTTOM ? room:0;d.width=cols*86+(cols-1)*5+14+128;d.height=rows*76+(rows-1)*5+14+room;
  for(i=0;i<9;++i){d.conf.items[i].kind=(i==2||i==3||i==4)?ND_ITEM_PLUGIN:ND_ITEM_LAUNCH;strcpy(d.conf.items[i].label,"Tile");}
  strcpy(d.conf.items[2].id,"EYES");strcpy(d.conf.items[4].id,"UNLISTED");
  d.hover=focus;tile_coords(&d,focus,&x,&y);tile_dimensions(&d,focus,&tw,&th);screen.MouseX=x+tw/2;screen.MouseY=y+th/2;
  d.hover_pointer=(side<2 ? screen.MouseX:screen.MouseY)/4;prepare_hover_layout(&d);assert(d.hover_plan_ready);assert(d.hover_layout[2].kind==ND_HOVER_WIDGET&&d.hover_layout[4].kind==ND_HOVER_WIDGET);
  for(i=0;i<9;++i){struct NDRect*a=&d.hover_rects[i];int bottom=a->top+a->height+(d.hover_layout[i].kind==ND_HOVER_ART ? rp.TxHeight+3:0);
   assert(a->left>=0&&a->top>=0&&a->left+a->width<=d.width&&bottom<=d.height);
   for(j=0;j<9;++j)if(d.hover_layout[j].row==d.hover_layout[i].row+1)assert(bottom+d.gap<=d.hover_rects[j].top);
   if(d.hover_layout[i].row!=d.hover_layout[focus].row)assert(d.hover_layout[i].width==d.hover_layout[i].base_width&&d.hover_layout[i].height==d.hover_layout[i].base_height);
  }
  assert(d.hover_shelf_rect.top>=0&&d.hover_shelf_rect.top+d.hover_shelf_rect.height<=d.height);
  {struct NDRect held[9];memcpy(held,d.hover_rects,sizeof(held));d.music_press_action=64;d.hover=(focus+1)%9;d.hover_pointer+=50;
   prepare_hover_layout(&d);assert(!memcmp(held,d.hover_rects,sizeof(held)));d.music_press_action=0;d.music_hover_action=16;prepare_hover_layout(&d);assert(!memcmp(held,d.hover_rects,sizeof(held)));d.music_hover_action=0;d.hover=focus;d.hover_pointer-=50;}
  d.hover=-1;prepare_hover_layout(&d);for(i=0;i<9;++i)assert(!d.hover_layout[i].cross_shift);
  {int measured=text_lengths;prepare_hover_layout(&d);prepare_hover_layout(&d);assert(text_lengths==measured);} /* widths are cached per label */
  /* With Animate off, entry jumps to the full selected strength. Pointer
     movement within the same tile changes neither size nor packing. */
  {struct NDRect snapped[9];int offset,peak_w,peak_h,max_w,max_h,saved_pointer=d.hover_pointer;
   d.hover=focus;d.conf.hover_animation=0;prepare_hover_layout(&d);
   memcpy(snapped,d.hover_rects,sizeof(snapped));
   assert(!d.hover_animating&&!d.hover_anim_valid);
   peak_w=d.hover_layout[focus].base_width;peak_h=d.hover_layout[focus].base_height;
   max_w=d.hover_layout[focus].kind==ND_HOVER_ART ? ND_HOVER_ICON_LIMIT:ND_HOVER_WIDGET_LIMIT;
   max_h=d.tile_height+d.hover_headroom-(rp.TxHeight+3)-6;
   if(d.hover_layout[focus].kind==ND_HOVER_ART&&max_h>ND_HOVER_ICON_LIMIT)max_h=ND_HOVER_ICON_LIMIT;
   nd_hover_size(0,2*(d.tile_width+d.gap),strength,max_w,max_h,&peak_w,&peak_h);
   assert(snapped[focus].width==peak_w&&snapped[focus].height==peak_h);
   for(i=0;i<9;++i)if(i!=focus)assert(d.hover_layout[i].width==d.hover_layout[i].base_width&&d.hover_layout[i].height==d.hover_layout[i].base_height);
   for(offset=-12;offset<=12;++offset){d.hover_pointer=saved_pointer+offset;d.hover_anim_advance=1;
    prepare_hover_layout(&d);assert(!d.hover_animating&&!memcmp(snapped,d.hover_rects,sizeof(snapped)));}
   d.hover_anim_advance=0;d.hover_pointer=saved_pointer;
   /* Switching off during easing must also snap on the next paint. */
   d.conf.hover_animation=1;d.hover_anim_valid=0;d.hover=-1;prepare_hover_layout(&d);
   d.hover=focus;d.hover_anim_advance=1;prepare_hover_layout(&d);d.hover_anim_advance=0;
   d.conf.hover_animation=0;prepare_hover_layout(&d);
   assert(!d.hover_animating&&!memcmp(snapped,d.hover_rects,sizeof(snapped)));
   d.hover=-1;prepare_hover_layout(&d);
  }
  /* Animated magnification: the first frame starts from the resting size,
     frames ease toward the settled animated wave, redraws without a timer tick do not
     advance, and the animation settles on exactly the static layout. */
  {struct NDRect target[9];int steps,first_w;
   d.conf.hover_animation=1;d.hover_anim_valid=0;
   d.hover=focus;prepare_hover_layout(&d);memcpy(target,d.hover_rects,sizeof(target));
   d.hover=-1;prepare_hover_layout(&d);
   d.conf.hover_animation=1;d.hover_anim_valid=0;prepare_hover_layout(&d);assert(!d.hover_animating);
   d.hover=focus;d.hover_anim_advance=1;prepare_hover_layout(&d);d.hover_anim_advance=0;
   first_w=d.hover_rects[focus].width;
   if(strength){assert(d.hover_animating&&first_w<target[focus].width);
    prepare_hover_layout(&d);assert(d.hover_rects[focus].width==first_w);}
   for(steps=0;d.hover_animating;++steps){assert(steps<12);d.hover_anim_advance=1;prepare_hover_layout(&d);d.hover_anim_advance=0;}
   assert(!memcmp(target,d.hover_rects,sizeof(target)));
   /* Leaving eases back down and then settles at rest. */
   d.hover=-1;for(steps=0;steps<14;++steps){d.hover_anim_advance=1;prepare_hover_layout(&d);d.hover_anim_advance=0;}
   assert(!d.hover_animating);
   d.conf.hover_animation=0;d.hover_anim_valid=0;}
 }
 /* A plugin may switch from a 256-pixel live widget to compact artwork
    while an animation is in progress. The first eased frame must fit the
    smaller icon cache too, even before the next timer advance. */
 d.conf.side=ND_SIDE_BOTTOM;d.conf.hover_animation=1;d.hover_anim_valid=1;d.hover_anim_advance=0;d.hover=0;
 d.hover_anim_w[0]=256;d.hover_anim_h[0]=256;prepare_hover_layout(&d);
 assert(d.hover_layout[0].kind==ND_HOVER_ART);
 assert(d.hover_rects[0].width<=ND_HOVER_ICON_LIMIT&&d.hover_rects[0].height<=ND_HOVER_ICON_LIMIT);
 /* Left/right docks have the same immediate peak and stable packing. */
 for(side=ND_SIDE_LEFT;side<=ND_SIDE_RIGHT;++side)for(strength=0;strength<=100;strength+=25)
 for(focus=0;focus<5;++focus){struct NDRect target[5];int offset,mw,mh,expected_w,expected_h;
  memset(&d,0,sizeof(d));memset(&w,0,sizeof(w));rp.TxHeight=8;w.RPort=&rp;d.window=&w;d.screen=&screen;
  d.conf.count=5;d.conf.side=side;d.conf.hover_magnification=strength;d.tile_width=86;d.tile_height=76;d.gap=5;
  d.hover_headroom=96;d.hover_pad_x=side==ND_SIDE_RIGHT?96:0;d.hover_pad_y=d.hover_axis_padding=64;
  d.width=196;d.height=900;d.layout_rows=1;d.hover=focus;
  for(i=0;i<5;++i){d.conf.items[i].kind=ND_ITEM_LAUNCH;strcpy(d.conf.items[i].label,"Tile");}
  tile_coords(&d,focus,&x,&y);d.hover_pointer=(y+38)/4;prepare_hover_layout(&d);memcpy(target,d.hover_rects,sizeof(target));
  mw=mh=ND_HOVER_ICON_LIMIT;expected_w=expected_h=16;
  nd_hover_size(0,2*(d.tile_height+d.gap),strength,mw,mh,&expected_w,&expected_h);
  assert(target[focus].width==expected_w&&target[focus].height==expected_h&&!d.hover_animating);
  for(offset=-12;offset<=12;++offset){d.hover_pointer+=offset;prepare_hover_layout(&d);assert(!memcmp(target,d.hover_rects,sizeof(target)));}
  d.hover=-1;prepare_hover_layout(&d);for(i=0;i<5;++i)assert(d.hover_rects[i].width==16&&d.hover_rects[i].height==16);
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='novadock-hover-rows-') as t:
 p=Path(t);(p/'rows.c').write_text(h)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'rows.c'),str(ROOT/'src/layout.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),'-o',str(p/'rows')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'rows')],check=True)
print('Production hover planner: Animate-off immediate peaks, fixed per-tile sizes/packing, mid-transition disable and restoration on all four sides; animated waves, rows, bounds and gaps passed.')
