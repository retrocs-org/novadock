#!/usr/bin/env python3
"""Exercise focus-independent pointer updates and Commodities coalescing."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(path,signature):
 s=(ROOT/path).read_text();a=s.index(signature+'\n{');return s[a:s.index('\n}',a)+2]
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
#include "nd_config.h"
#define ND_CX_POINTER_ID 30003
#define ND_CX_RBUTTON_ID 30000
#define ND_CX_DISK_ID 30001
#define ND_CX_FOCUS_ID 30002
#define ND_MEDIA_EVENT_TICKS 3
#define CXM_IEVENT 1
#define CXM_COMMAND 2
#define CXCMD_KILL 1
#define CXCMD_APPEAR 2
#define CXCMD_UNIQUE 3
#define CXCMD_DISAPPEAR 4
#define CXCMD_DISABLE 5
#define CXCMD_ENABLE 6
#define WFLG_WINDOWACTIVE 1
#define ITEM_TEXT(item,key) ""
typedef unsigned long ULONG;typedef long LONG;
struct Message {int unused;};
struct InputEvent {unsigned int ie_Class,ie_Code;};
typedef struct CxMsg {ULONG type;LONG id;} CxMsg;
struct Window {int LeftEdge,TopEdge,Flags;};
struct Screen {int MouseX,MouseY;};
struct NDDock {int grip_drag;
 struct NDConfig conf;struct Window *window;struct Screen *screen;
 struct {int left,top,width,height;} full_box;
 int hidden,screen_closed,covered,box_pending,popup,calendar,folder_popup,is_drag,keyboard;int music_press_action;
 int keyboard_mouse_x,keyboard_mouse_y,hover,hover_pointer,hover_ticks,hover_headroom;
 int action_count,activate_popup_pending,media_scan_ticks,running;
 void *hotkey_port,*hotkey_broker;struct NDItem *action_items;
};
static int frames,replies,queued,position,activated,occluded;
static int dock_pointer_visible(struct NDDock*d){(void)d;return !occluded;}
static CxMsg messages[64];
static int vertical(struct NDDock*d){return d->conf.side==ND_SIDE_LEFT||d->conf.side==ND_SIDE_RIGHT;}
static int hovered_tile(struct NDDock*d,int x,int y){int axis=vertical(d)?y:x;if(x<0||y<0||x>=600||y>=600)return -1;return axis/100<d->conf.count?axis/100:-1;}
static int update_music_control_hover(struct NDDock*d,int i,int x,int y){(void)d;(void)i;(void)x;(void)y;return 0;}
static int pointer_hover_target(struct NDDock*d,int x,int y){return hovered_tile(d,x,y);}
static void close_tooltip(struct NDDock*d){(void)d;}
static void draw_hover(struct NDDock*d){(void)d;++frames;}
static void redraw_changed_tiles(struct NDDock*d,int a,int b,int c){(void)d;(void)a;(void)b;(void)c;++frames;}
static CxMsg *GetMsg(void *p){(void)p;return position<queued?&messages[position++]:0;}
static ULONG CxMsgType(CxMsg*m){return m->type;}
static LONG CxMsgID(CxMsg*m){return m->id;}
static void *CxMsgData(CxMsg*m){(void)m;return 0;}
static void ReplyMsg(struct Message*m){(void)m;++replies;}
static int nd_right_button_down(unsigned int a,unsigned int b){(void)a;(void)b;return 0;}
static int inside_rect(void*r,int x,int y){(void)r;(void)x;(void)y;return 0;}
static void ActivateWindow(struct Window*w){(void)w;++activated;}
static void keyboard_begin(struct NDDock*d){(void)d;}
static int launch_action(struct NDDock*d,struct NDItem*i){(void)d;(void)i;return 1;}
static void show_feedback(struct NDDock*d,const char*s,int e){(void)d;(void)s;(void)e;}
static void show_dock(struct NDDock*d){(void)d;}
static void hide_dock(struct NDDock*d){(void)d;}
static void ActivateCxObj(void*p,long n){(void)p;(void)n;}
'''+function('src/dock_timers.c','void update_hover_pointer(struct NDDock *dock)')+'\n'+function('src/dock_input.c','void process_hotkeys(struct NDDock *dock)')+r'''
static void batch(struct NDDock*d){int i;queued=64;position=0;for(i=0;i<64;++i){messages[i].type=CXM_IEVENT;messages[i].id=ND_CX_POINTER_ID;}process_hotkeys(d);}
int main(void)
{
 struct NDDock d;struct Window w;struct Screen s;int before;
 memset(&d,0,sizeof(d));memset(&w,0,sizeof(w));d.window=&w;d.screen=&s;
 d.conf.count=6;d.conf.hover_magnification=100;d.hover_headroom=96;d.hotkey_port=&d;
 d.hover=0;d.hover_pointer=12;w.LeftEdge=100;w.TopEdge=200;
 s.MouseX=162;s.MouseY=240;batch(&d);assert(frames==1&&replies==64&&!activated);
 batch(&d);assert(frames==1&&replies==128); /* stationary duplicate source */
 w.Flags=WFLG_WINDOWACTIVE;s.MouseX+=8;batch(&d);assert(frames==2&&!activated);
 w.Flags=0;s.MouseX+=8;batch(&d);assert(frames==3&&!activated);
 d.conf.side=ND_SIDE_RIGHT;s.MouseY=320;batch(&d);assert(frames==4&&d.hover==1);
 before=frames;d.covered=1;s.MouseY+=8;batch(&d);assert(frames==before);
 d.covered=0;d.popup=1;batch(&d);assert(frames==before);d.popup=0;
 d.keyboard=1;d.keyboard_mouse_x=s.MouseX;d.keyboard_mouse_y=s.MouseY;
 batch(&d);assert(frames==before&&d.keyboard);s.MouseY+=8;batch(&d);assert(frames==before+1&&!d.keyboard);
 s.MouseX=0;batch(&d);assert(d.hover==-1&&frames==before+2);
 occluded=1;batch(&d);assert(d.hover==-1);occluded=0;
 {int old=d.hover;before=frames;d.music_press_action=64;s.MouseX=160;s.MouseY+=8;batch(&d);assert(frames==before&&d.hover==old);d.music_press_action=0;batch(&d);assert(frames==before+1);}
 before=frames;d.conf.hover_magnification=0;s.MouseX=160;batch(&d);assert(frames==before);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='novadock-hover-pointer-') as t:
 p=Path(t);(p/'pointer.c').write_text(h)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'include'),str(p/'pointer.c'),'-o',str(p/'pointer')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'pointer')],check=True)
print('Pointer events: inactive/active parity, 64 messages coalesced, stationary suppression, vertical movement, keyboard and covered/modal guards pass without activation.')
