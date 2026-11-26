#!/usr/bin/env python3
"""Production targeting, caption fitting, state policy and optical envelopes."""
from pathlib import Path
import os, subprocess, tempfile, sys
ROOT=Path(__file__).resolve().parents[1]
def fn(path,sig):
 s=(ROOT/path).read_text();a=s.index(sig+'\n{');return s[a:s.index('\n}',a)+2]
code=r'''#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_config.h"
#include "nd_layout.h"
#include "nd_autohide.h"
#include "nd_artwork.h"
#include "nd_tile_state.h"
#include "nd_hover_policy.h"
#define ND_HOVER_ART 1
#define ND_ID_LEN 20
#define ND_NO_METER 65535U
#define dock_left(d) 0
#define dock_top(d) 0
#define dock_view_x(d) 0
#define dock_view_y(d) 0
typedef char *STRPTR;
struct RastPort {int TxHeight;};struct Window {int Width,Height;struct RastPort *RPort;};
struct NDDock {unsigned long watchdog_retry_after[ND_MAX_ITEMS],watchdog_restarts[ND_MAX_ITEMS],watchdog_total_restarts;unsigned short watchdog_failures[ND_MAX_ITEMS];void *plugin_task[ND_MAX_ITEMS];int plugin_woken[ND_MAX_ITEMS],plugin_wake_port[ND_MAX_ITEMS];struct NDConfig conf;struct Window *window;int painting_shelf,painting_mask,painting_item;int gap,hover_headroom,hover_plan_ready,is_drag,hover,hover_pick_valid,hover_pick_index,hover_pick_x,hover_pick_y;struct NDRect hover_rects[ND_MAX_ITEMS];struct {int kind,row;}hover_layout[ND_MAX_ITEMS];};
static int vertical(struct NDDock*d){return d->conf.side==ND_SIDE_LEFT||d->conf.side==ND_SIDE_RIGHT;}
static int item_visible(struct NDDock*d,int i){return i>=0&&i<d->conf.count;}
static int hovered_grip(struct NDDock*d,int x,int y){(void)d;(void)x;(void)y;return 0;}
static int hovered_tile(struct NDDock*d,int x,int y){(void)d;(void)x;(void)y;return -1;}
static int TextLength(struct RastPort*r,STRPTR s,int n){(void)r;(void)s;return n*8;}
static char shown[ND_LABEL_LEN+4];
#define JAM1 0
static int metric_kind(struct NDDock*d,const struct NDItem*i){(void)d;(void)i;return 0;}
static int pen(struct NDDock*d,int c){(void)d;return c;}
static void SetAPen(struct RastPort*r,int c){(void)r;(void)c;}
static void SetDrMd(struct RastPort*r,int m){(void)r;(void)m;}
static void Move(struct RastPort*r,int x,int y){(void)r;(void)x;(void)y;}
static void Text(struct RastPort*r,STRPTR t,int n){(void)r;memcpy(shown,t,n);shown[n]=0;}

'''+r'''
typedef unsigned long ULONG;
static int resolve_ok=1,quote_ok=1,launch_ok=1,launches;
static ULONG diagnostic_ticks(void){return 50;}
static ULONG nd_watchdog_backoff_ticks(unsigned int n){(void)n;return 100;}
static void fire_hook(struct NDDock*d,const char*a,const char*b){(void)d;(void)a;(void)b;}
static int resolve_plugin_executable(struct NDDock*d,const char*c,char*p,unsigned int n){(void)d;(void)c;(void)n;strcpy(p,"Plugins/Test");return resolve_ok;}
static int nd_quote_path(char*out,unsigned int n,const char*p){(void)n;strcpy(out,p);return quote_ok;}
static int launch_command(const char*c){assert(strstr(c,"Plugins/Test"));++launches;return launch_ok;}
static void sync_plugin_twins(struct NDDock*d,int i){(void)d;(void)i;}
static void mark_tile_dirty(struct NDDock*d,int i){(void)d;(void)i;}
'''+fn('src/dock_plugins.c','static int plugin_launch_feedback(struct NDDock *dock,int index,const char *status,int result)')+'\n'+fn('src/dock_plugins.c','static int launch_plugin(struct NDDock *dock,int index,int restarted)')+'\n'+fn('src/dock_window.c','int pointer_hover_target(struct NDDock *dock,int mx,int my)')+'\n'+fn('src/dock_draw.c','static void centered_text_tight(struct NDDock *dock,int color,\n                                const char *value,int x,int baseline,int w)')+'\n'+fn('src/dock_draw.c','static void label_text(struct NDDock *dock,int color,const char *name,int x,int baseline,int w)')+r'''
static void state(const char*id,int online,const char*v,int want){assert(nd_tile_state(id,online,v)==want);}
int main(int argc,char**argv){struct NDDock d;struct Window win;struct RastPort rp;int i;struct NDArtwork a;unsigned int n,flags;const unsigned char*r,*bounds;int x1,y1,x2,y2;
 memset(&d,0,sizeof(d));rp.TxHeight=8;win.RPort=&rp;win.Width=300;win.Height=180;d.window=&win;d.conf.count=4;d.gap=5;d.hover_headroom=96;d.hover_plan_ready=1;d.hover=0;
 for(i=0;i<4;++i){d.hover_layout[i].kind=ND_HOVER_ART;d.hover_layout[i].row=i/2;d.hover_rects[i].left=10+(i%2)*53;d.hover_rects[i].top=10+(i/2)*81;d.hover_rects[i].width=d.hover_rects[i].height=48;}
 assert(pointer_hover_target(&d,57,30)==0);assert(pointer_hover_target(&d,60,30)==0);
 d.hover=pointer_hover_target(&d,61,30);assert(d.hover==1);
 assert(pointer_hover_target(&d,60,30)==1);assert(pointer_hover_target(&d,58,30)==0);assert(pointer_hover_target(&d,56,30)==0);
 d.hover=1;pointer_hover_target(&d,80,30);d.hover_rects[1].left=120;d.hover_rects[0].left=60;
 assert(pointer_hover_target(&d,80,30)==1);assert(pointer_hover_target(&d,81,30)==0); /* movement resumes selection */
 d.hover=0;d.hover_pick_valid=0;assert(pointer_hover_target(&d,81,80)==0); /* caption footer */
 assert(pointer_hover_target(&d,28,100)==2);assert(pointer_hover_target(&d,299,179)==-1);assert(pointer_hover_target(&d,-1,30)==-1);
 /* Sparse artwork must keep a live wave across the full inter-tile gap. */
 d.hover_pick_valid=0;d.hover=0;d.conf.count=2;
 d.hover_rects[0].left=10;d.hover_rects[0].top=10;d.hover_rects[0].width=48;
 d.hover_rects[1].left=200;d.hover_rects[1].top=10;d.hover_rects[1].width=48;
 for(i=34;i<=224;++i){d.hover=pointer_hover_target(&d,i,30);assert(d.hover>=0);if(i>133)assert(d.hover==1);}
 d.hover_pick_valid=0;assert(pointer_hover_target(&d,120,150)==-1);
 d.conf.items[1].kind=ND_ITEM_SPACER;assert(pointer_hover_target(&d,120,30)==-1);d.conf.items[1].kind=ND_ITEM_PLUGIN;
 d.conf.side=ND_SIDE_LEFT;d.hover_pick_valid=0;d.hover=0;
 d.hover_rects[1].left=10;d.hover_rects[1].top=110;
 for(i=34;i<=134;++i){d.hover=pointer_hover_target(&d,30,i);assert(d.hover>=0);}
 d.conf.side=ND_SIDE_BOTTOM;d.conf.count=4;
 assert(nd_hover_axis(39,9)==9&&nd_hover_axis(40,9)==9&&nd_hover_axis(41,9)==10&&nd_hover_axis(39,10)==10);
 centered_text_tight(&d,0,"14:45",0,0,40);assert(!strcmp(shown,"14:45"));
 label_text(&d,0,"Preferences",0,0,80);assert(!strcmp(shown,"Prefs"));label_text(&d,0,"MusicPlayer",0,0,80);assert(!strcmp(shown,"Music"));
 label_text(&d,0,"Custom very long drawer name",0,0,80);assert(strstr(shown,"...")&&strlen(shown)<=9);
 label_text(&d,0,"Shell",0,0,80);assert(!strcmp(shown,"Shell"));
 state("CPU",1,"0%",0);state("RAM",1,"C:0.0/2.0 F:0.0/16.0",0);state("ARCHIVE",1,"Idle",0);state("HASS",1,"OFF",0);state("LOGS",1,"0 ERRORS",0);state("LOGS",1,"ERRORS: 0",0);state("MUSICLIB",1,"NO ITEMS",0);
 state("CPU",0,"STARTING",1);state("WHDLOAD",1,"SCANNING 12",1);state("NET",1,"STOPPING",1);state("NET",1,"WAIT STACK",1);state("NET",1,"ACTION FAILED",4);state("NET",1,"NO ROADSHOW",3);
 state("NET",1,"OFFLINE",2);state("MUSIC",1,"OFFLINE",2);state("CPU",0,"37%",2);
 state("WEATHER",1,"SET LOCATION",3);state("HASS",1,"unavailable",3);state("SENSORS",1,"NO SENSORS",3);
 state("CPU",0,"MISSING PLUGIN",4);state("CPU",0,"FAILED TO START",4);state("CPU",0,"NOT RESPONDING",4);state("MUSIC",1,"NO AREXX",4);
 d.conf.items[0].kind=ND_ITEM_PLUGIN;strcpy(d.conf.items[0].command,"Plugins/Test");strcpy(d.conf.items[0].id,"TEST");
 resolve_ok=0;assert(!launch_plugin(&d,0,0)&&!strcmp(d.conf.items[0].value,"MISSING PLUGIN")&&!launches);
 resolve_ok=1;quote_ok=0;assert(!launch_plugin(&d,0,0)&&!strcmp(d.conf.items[0].value,"FAILED TO START")&&!launches);
 quote_ok=1;launch_ok=0;assert(!launch_plugin(&d,0,0)&&!strcmp(d.conf.items[0].value,"FAILED TO START")&&launches==1);
 launch_ok=1;assert(launch_plugin(&d,0,0)&&!strcmp(d.conf.items[0].value,"STARTING")&&launches==2&&!d.conf.items[0].online&&d.watchdog_retry_after[0]==150);
 strcpy(d.conf.items[0].command,"@passive");assert(!launch_plugin(&d,0,0)&&launches==2);
 assert(argc==2);memset(&a,0,sizeof(a));assert(nd_artwork_load(&a,argv[1]));
 r=nd_artwork_key_frame(&a,nd_artwork_key("test"),0,&n,&flags);assert(r&&n);bounds=nd_artwork_bounds(&a,r);assert(bounds[0]==3&&bounds[1]==4&&bounds[2]==25&&bounds[3]==24);
 nd_artwork_scaled_rect(bounds,r,48,&x1,&y1,&x2,&y2);assert(x1>=0&&y1>=0&&x2<=48&&y2<=48);
 r=nd_artwork_key_frame(&a,nd_artwork_key("test"),1,&n,&flags);assert(!memcmp(bounds,nd_artwork_bounds(&a,r),4));nd_artwork_close(&a);
 puts("Dock polish: boundary hysteresis, stationary reflow, row/footer targeting, ellipsis, state recovery semantics and stable artwork envelopes pass.");return 0;}
'''
sys.path.insert(0,str(ROOT/'tools'))
from pack_plugin_art import build_bank
with tempfile.TemporaryDirectory() as d:
 p=Path(d);frames=[]
 for state,(x,y) in enumerate(((3,4),(25,25))):
  pixels=[[0]*32 for _ in range(32)]
  for yy in range(y,y+3):
   for xx in range(x,x+3):pixels[yy][xx]=1
  frames.append(dict(name='test',state=state,pixels=pixels))
 (p/'art.ndart').write_bytes(build_bank(frames,[(0,0,0)]*16))
 (p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-unused-function','-Wno-deprecated-declarations','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test'),str(p/'art.ndart')],check=True)
