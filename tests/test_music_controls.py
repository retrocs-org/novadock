#!/usr/bin/env python3
"""Exercise the production music hit tests and held controls through scaling."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/"src/dock_draw.c").read_text()
def f(sig):
 a=s.index(sig+"\n{");return s[a:s.index("\n}",a)+2]
code=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_layout.h"
#include "nd_autohide.h"
#include "nd_input_policy.h"
#define ND_MEDIA_VOLUME_UNKNOWN 65535U
#define ND_EVENT_MEDIA_PREVIOUS 8U
#define ND_EVENT_MEDIA_PLAY_PAUSE 16U
#define ND_EVENT_MEDIA_NEXT 32U
#define ND_EVENT_MEDIA_VOLUME 64U
#define ND_MEDIA_CAP_PREVIOUS 1U
#define ND_MEDIA_CAP_PLAY_PAUSE 2U
#define ND_MEDIA_CAP_NEXT 4U
#define ND_MEDIA_CAP_VOLUME 8U
#define ND_MEDIA_STATE_PLAYING 1U
struct RastPort {int TxHeight;};struct Window {struct RastPort *RPort;};
struct NDDock {struct Window *window;struct NDConfig conf;int edit_mode,hover_headroom,hover_plan_ready,press_item;unsigned int music_press_action,music_hover_action;int hover_shelf_valid;struct NDHoverLayoutItem hover_layout[1];struct NDRect hover_rects[1];};
static int draws,wakes,tw=177,th=76;
static void tile_coords(struct NDDock*d,int i,int*x,int*y){(void)d;assert(i==0);*x=100;*y=200;}
static void tile_dimensions(struct NDDock*d,int i,int*w,int*h){(void)d;assert(i==0);*w=tw;*h=th;}
static void wake_pending_plugins(struct NDDock*d){assert(d->conf.items[0].media_event);++wakes;}
static void draw_tile(struct NDDock*d,int i){(void)d;assert(i==0);++draws;}
'''
for sig in ["static unsigned int music_control_hit(struct NDDock *dock,int index,\n                                      int mx,int my,unsigned int *volume)","static int set_music_control(struct NDDock *dock,int index,unsigned int action,unsigned int volume)","int queue_music_control(struct NDDock *dock,int index,int mx,int my)","int update_music_control_hover(struct NDDock *dock,int index,int mx,int my)","int begin_music_control(struct NDDock *dock,int index,int mx,int my)","void drag_music_control(struct NDDock *dock,int mx)","void finish_music_control(struct NDDock *dock,int mx,int my)"]:
 code+="\n"+f(sig)
code+=r'''
static void point(struct NDDock*d,int sx,int sy,int*x,int*y){*x=100+sx;*y=200+sy;if(d->hover_headroom){struct NDRect*b=&d->hover_rects[0];*x=b->left+(sx*b->width+tw/2)/tw;*y=b->top+(sy*b->height+th/2)/th;}}
int main(void){static struct NDDock d;struct NDItem*it=&d.conf.items[0];int tiny,scaled,b,x,y,v,content;struct RastPort rp={8};struct Window win={&rp};unsigned int action,volume;
 for(tiny=0;tiny<2;++tiny)for(scaled=0;scaled<3;++scaled){memset(&d,0,sizeof(d));d.window=&win;d.conf.count=1;d.conf.item_size=tiny;d.press_item=0;it->kind=ND_ITEM_PLUGIN;it->online=1;it->media_capabilities=15;strcpy(it->id,"MUSIC");tw=tiny?86:177;th=tiny?42:76;content=tiny?th:th-16;d.hover_headroom=scaled?96:0;d.hover_plan_ready=1;d.hover_layout[0].kind=ND_HOVER_WIDGET;d.hover_rects[0].left=340;d.hover_rects[0].top=60;d.hover_rects[0].width=scaled==2?256:tw;d.hover_rects[0].height=scaled==2?141:th;
  for(b=0;b<(tiny?1:3);++b){int sx=tiny?tw/2:6+b*((tw-16)/3+2)+(tw-16)/6;int sy=tiny?9:content>=60?38:28;action=tiny||b==1?ND_EVENT_MEDIA_PLAY_PAUSE:b==0?ND_EVENT_MEDIA_PREVIOUS:ND_EVENT_MEDIA_NEXT;point(&d,sx,sy,&x,&y);it->media_event=0;
   assert(music_control_hit(&d,0,x,y,&volume)==action);d.music_hover_action=0;d.hover_shelf_valid=1;assert(update_music_control_hover(&d,0,x,y)&&d.music_hover_action==action&&!d.hover_shelf_valid);assert(!update_music_control_hover(&d,0,x,y));assert(begin_music_control(&d,0,x,y)&&d.music_press_action==action&&!it->media_event);finish_music_control(&d,x,y);assert(!d.music_press_action&&it->media_event==action);
  }
  for(v=0;v<=100;v+=50){int start=tiny?5:8,width=tw-(tiny?10:16);point(&d,start+(width-1)*v/100,content-8,&x,&y);it->media_event=0;assert(begin_music_control(&d,0,x,y)&&d.music_press_action==ND_EVENT_MEDIA_VOLUME);assert(it->media_event==ND_EVENT_MEDIA_VOLUME);if(!(it->media_volume>=v-2&&it->media_volume<=v+2)){fprintf(stderr,"volume tiny%d scale%d requested%d got%u at%d,%d\n",tiny,scaled,v,it->media_volume,x,y);assert(0);}}
  drag_music_control(&d,-1000);assert(it->media_volume==0&&it->media_volume_request==0);drag_music_control(&d,2000);assert(it->media_volume==100&&it->media_volume_request==100);finish_music_control(&d,-1000,-1000);assert(it->media_volume==0&&!d.music_press_action);
  point(&d,tw/2,tiny?9:38,&x,&y);it->media_event=0;assert(begin_music_control(&d,0,x,y));finish_music_control(&d,-1000,-1000);assert(!it->media_event&&!d.music_press_action);
  d.edit_mode=1;assert(!begin_music_control(&d,0,x,y));d.edit_mode=0;it->online=0;assert(!begin_music_control(&d,0,x,y));it->online=1;it->media_capabilities=0;assert(!begin_music_control(&d,0,x,y));
 }
 /* Several actions before the next plugin exchange accumulate; a double
  * Play/Pause cancels out in both the queued event and the tile state. */
 {struct NDItem *m=&d.conf.items[0];unsigned int state;d.edit_mode=0;m->media_event=0;state=m->media_state;
  assert(set_music_control(&d,0,ND_EVENT_MEDIA_NEXT,ND_MEDIA_VOLUME_UNKNOWN));
  assert(set_music_control(&d,0,ND_EVENT_MEDIA_PLAY_PAUSE,ND_MEDIA_VOLUME_UNKNOWN));
  assert(m->media_event==(ND_EVENT_MEDIA_NEXT|ND_EVENT_MEDIA_PLAY_PAUSE)&&m->media_state!=state);
  assert(set_music_control(&d,0,ND_EVENT_MEDIA_PLAY_PAUSE,ND_MEDIA_VOLUME_UNKNOWN));
  assert(m->media_event==ND_EVENT_MEDIA_NEXT&&m->media_state==state);
  assert(set_music_control(&d,0,ND_EVENT_MEDIA_VOLUME,40)&&m->media_event==(ND_EVENT_MEDIA_NEXT|ND_EVENT_MEDIA_VOLUME)&&m->media_volume_request==40);}
 assert(draws>0&&wakes>0);puts("Music controls: queued actions accumulate, double Play/Pause cancels, translated/scaled transport hits, deferred button release, cancellation, live slider drags, endpoint clamping, tiny mode and offline/edit guards passed.");return 0;}
'''
with tempfile.TemporaryDirectory(prefix="nd-music-controls-") as t:
 p=Path(t);(p/"test.c").write_text(code)
 subprocess.run([os.environ.get("CC","cc"),"-std=c89","-Wall","-Wextra","-Werror","-fsanitize=address,undefined","-I"+str(root/"include"),str(p/"test.c"),"-o",str(p/"test")],check=True)
 subprocess.run([str(p/"test")],check=True)
