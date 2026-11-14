#!/usr/bin/env python3
"""Execute dock polling and redraw paths with counted operating-system calls."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def extract(source, signature):
    start = source.index(signature+'\n{')
    return source[start:source.index('\n}', start)+2]


class PerformanceTests(unittest.TestCase):
    def test_command_snapshot_capacity_and_overflow_fallback(self):
        source=(ROOT/'src/dock_draw.c').read_text()
        start=source.index('#define ND_CLI_SNAPSHOT')
        end=source.index('\nstatic void snapshot_cli_commands',start)
        harness=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "nd_interaction.h"
struct Task {int id;};
static struct Task fallback;
static int fallbacks;
static struct Task *find_cli_command(const char *wanted)
{(void)wanted;++fallbacks;return &fallback;}
'''+source[start:end]
        harness+=extract(source,'static struct Task *snapshot_cli_find(const struct NDCliSnapshot *snap,\n                                      const char *wanted)')
        harness+=r'''
int main(void)
{
 struct NDCliSnapshot s;struct Task tasks[64];int i;
 assert(ND_CLI_SNAPSHOT>=64);memset(&s,0,sizeof(s));
 s.count=64;s.complete=1;
 for(i=0;i<64;++i){sprintf(s.name[i],"SYS:Tools/App%d",i);s.task[i]=&tasks[i];}
 assert(snapshot_cli_find(&s,"app39")==&tasks[39]);
 assert(snapshot_cli_find(&s,"App63")==&tasks[63]);
 assert(!snapshot_cli_find(&s,"missing")&&!fallbacks);
 s.complete=0;assert(snapshot_cli_find(&s,"missing")==&fallback&&fallbacks==1);
 assert(!snapshot_cli_find(&s,""));return 0;
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp);(path/'snapshot.c').write_text(harness)
            subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'include'),str(path/'snapshot.c'),str(ROOT/'src/interaction.c'),'-o',str(path/'snapshot')],check=True)
            subprocess.run([str(path/'snapshot')],check=True)

    def test_running_match_stops_after_a_positive_task_match(self):
        source=(ROOT/'src/dock_draw.c').read_text()
        harness=r'''
#include <assert.h>
#include <stddef.h>
struct Task {int unused;};struct NDItem {int unused;};
struct NDDock {int unused;};struct NDCliSnapshot {int unused;};
static struct Task task;
static int task_found,port_found,window_found,ports,windows;
static struct Task *launcher_task_using(const struct NDItem *i,const struct NDCliSnapshot *s)
{(void)i;(void)s;return task_found?&task:NULL;}
static struct Task *launcher_port_task(const struct NDItem *i,int *present)
{(void)i;++ports;*present=port_found;return port_found?&task:NULL;}
static struct Task *launcher_window(struct NDDock *d,const struct NDItem *i,struct Task *t,struct Task *p)
{(void)d;(void)i;(void)t;(void)p;++windows;return window_found?&task:NULL;}
'''
        harness+=extract(source,'static int launcher_running_using(struct NDDock *dock,const struct NDItem *item,\n                                  const struct NDCliSnapshot *snap)')
        harness+=r'''
int main(void)
{
 struct NDDock d={0};struct NDItem i={0};int n;
 task_found=1;for(n=0;n<60;++n)assert(launcher_running_using(&d,&i,NULL));
 assert(!ports&&!windows);
 task_found=0;port_found=1;assert(launcher_running_using(&d,&i,NULL));
 assert(ports==1&&!windows);
 port_found=0;window_found=1;assert(launcher_running_using(&d,&i,NULL));
 assert(ports==2&&windows==1);
 window_found=0;assert(!launcher_running_using(&d,&i,NULL));
 assert(ports==3&&windows==2);return 0;
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp);(path/'running.c').write_text(harness)
            subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',str(path/'running.c'),'-o',str(path/'running')],check=True)
            subprocess.run([str(path/'running')],check=True)

    def test_volume_polling_and_running_indicator_coalescing(self):
        source = (ROOT/'src/dock_draw.c').read_text()
        harness = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#define ND_NO_METER 65535U
static const char identity_texts[]="\0Custom";
#define ITEM_TEXT(item,key) (identity_texts+(item)->text[key])
#define ACCESS_READ 0
/* The production paths below need only these fields of the OS types. */
typedef void *APTR;
typedef unsigned long BPTR;
typedef unsigned short UWORD;
typedef char *STRPTR;
struct InfoData {long id_NumBlocks,id_NumBlocksUsed;};
struct NDCliSnapshot {int unused;};
struct NDDock {
    struct NDConfig conf;
    int hidden,covered,box_pending;
    void *window;
    unsigned char running_items[ND_MAX_ITEMS],dirty_tiles[ND_MAX_ITEMS],graph_step[ND_MAX_ITEMS];
};
static int visible_first,visible_end=4,locks,infos,unlocks,requesters,marks,scans,checks,paints;
static int is_running=1;
static APTR quiet_requesters(void) {++requesters;return NULL;}
static void restore_requesters(APTR p) {(void)p;}
static int item_visible(struct NDDock *d,int i) {(void)d;return i>=visible_first && i<visible_end;}
int nd_item_supports_space_meter(const struct NDItem *item) {return item->kind==ND_ITEM_DRAWER;}
unsigned int nd_used_percent(unsigned long used,unsigned long total) {return (unsigned int)(used*100/total);}
static BPTR Lock(STRPTR name,int mode) {(void)name;(void)mode;++locks;return 1;}
static int Info(BPTR lock,struct InfoData *info) {(void)lock;++infos;info->id_NumBlocks=100;info->id_NumBlocksUsed=25;return 1;}
static void UnLock(BPTR lock) {(void)lock;++unlocks;}
static void mark_tile_dirty(struct NDDock *d,int i) {++marks;d->dirty_tiles[i]=1;}
static void snapshot_cli_commands(struct NDCliSnapshot *s) {(void)s;++scans;}
static int launcher_running_using(struct NDDock *d,const struct NDItem *i,const struct NDCliSnapshot *s)
{(void)d;(void)i;(void)s;++checks;return is_running;}
static void draw_tile(struct NDDock *d,int i) {(void)d;(void)i;++paints;}
static void draw_graph_step(struct NDDock *d,int i) {(void)d;(void)i;++paints;}
'''
        for signature in ['static int same_running_identity(const struct NDItem *a,const struct NDItem *b)',
                          'void refresh_volume_meters(struct NDDock *dock)',
                          'void refresh_running_items(struct NDDock *dock)',
                          'void flush_dirty_tiles(struct NDDock *dock)']:
            harness += '\n'+extract(source, signature)
        harness += r'''
int main(void)
{
    static struct NDDock dock;
    int i,before;
    dock.conf.count=12;dock.window=&dock;
    for(i=0;i<12;++i) {
        dock.conf.items[i].kind=ND_ITEM_DRAWER;
        dock.conf.items[i].show_volume_meter=1;
        dock.conf.items[i].meter=ND_NO_METER;
    }
    refresh_volume_meters(&dock);
    assert(locks==4 && infos==4 && unlocks==4 && marks==4);
    assert(dock.conf.items[4].meter==ND_NO_METER);
    marks=0;refresh_volume_meters(&dock);assert(marks==0);
    before=locks;dock.hidden=1;refresh_volume_meters(&dock);assert(locks==before);
    dock.hidden=0;dock.covered=1;refresh_volume_meters(&dock);assert(locks==before);
    dock.covered=0;dock.box_pending=1;refresh_volume_meters(&dock);assert(locks==before);
    dock.box_pending=0;visible_first=4;visible_end=8;marks=0;
    refresh_volume_meters(&dock);assert(locks==before+4 && marks==4);
    dock.conf.items[4].show_volume_meter=0;before=locks;
    refresh_volume_meters(&dock);assert(locks==before+3);
    assert(dock.conf.items[4].meter==ND_NO_METER);
    /* Startup still initialises every enabled meter before opening the window. */
    before=locks;dock.window=NULL;refresh_volume_meters(&dock);assert(locks==before+11);
    dock.window=&dock;dock.conf.running_apps=1;
    visible_first=0;visible_end=4;marks=0;
    memset(dock.dirty_tiles,0,sizeof(dock.dirty_tiles));
    for(i=0;i<12;++i){dock.conf.items[i].kind=ND_ITEM_LAUNCH;sprintf(dock.conf.items[i].command,"App%d",i);}
    refresh_running_items(&dock);
    assert(scans==1 && checks==4 && marks==4 && paints==0);
    refresh_running_items(&dock);assert(marks==4 && paints==0);
    strcpy(dock.conf.items[1].command,dock.conf.items[0].command);
    before=checks;refresh_running_items(&dock);assert(checks==before+3);
    assert(dock.running_items[0]==dock.running_items[1]);
    /* Identity differences must not share a cached result. */
    dock.conf.items[1].launch_mode=ND_LAUNCH_AREXX;
    before=checks;refresh_running_items(&dock);assert(checks==before+4);
    dock.conf.items[1].launch_mode=dock.conf.items[0].launch_mode;
    for(i=ND_TEXT_MATCH_TASK;i<=ND_TEXT_MATCH_PORT;++i) {
        dock.conf.items[1].text[i]=1;
        before=checks;refresh_running_items(&dock);assert(checks==before+4);
        dock.conf.items[1].text[i]=0;
    }
    is_running=0;before=checks;refresh_running_items(&dock);
    assert(checks==before+3 && !dock.running_items[0] && !dock.running_items[1]);
    is_running=1;refresh_running_items(&dock);

    flush_dirty_tiles(&dock);assert(paints==4);
    flush_dirty_tiles(&dock);assert(paints==4);
    before=scans;dock.hidden=1;refresh_running_items(&dock);assert(scans==before);
    puts("12-meter fixture: 4 visible filesystem queries, 0 hidden; 4 indicator changes coalesced into 4 paints.");
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path/'performance.c').write_text(harness)
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c89', '-Wall', '-Wextra',
                            '-I'+str(ROOT/'include'), str(path/'performance.c'),
                            '-o', str(path/'performance')], check=True)
            subprocess.run([str(path/'performance')], check=True)

    def test_idle_hit_testing_and_visible_eyes(self):
        window_source = (ROOT/'src/dock_window.c').read_text()
        draw_source = (ROOT/'src/dock_draw.c').read_text()
        timer_source = (ROOT/'src/dock_timers.c').read_text()
        harness = r'''
#include <assert.h>
#include <string.h>
#include "nd_config.h"
#define ND_TIMER_IDLE_QUANTA 5
#define ND_TIMER_MICROSECONDS 200000UL
#define TR_ADDREQUEST 1
typedef unsigned long ULONG;
struct Window {int Width,Height;};
struct Screen {int MouseX,MouseY;};
struct IORequest {int unused;};
struct TimerRequest {struct {int io_Command;} tr_node;struct {ULONG tv_secs,tv_micro;} tr_time;};
struct NDDock {
    struct NDConfig conf;
    struct Window *window;
    struct Screen *screen;
    struct TimerRequest *timer_req;
    int overflow,view_start,visible_count,timer_open,timer_pending,screen_closed;
    int hidden,covered,box_pending,activate_popup_pending,press_item,is_drag,icons_pending,timer_quanta;
    void *popup,*calendar,*folder_popup,*tooltip;
    struct {int left,top,width,height;} full_box;
};
static int coordinate_calls,dimension_calls,sends,rows=1;
static void tile_coords(struct NDDock *d,int i,int *x,int *y)
{(void)d;++coordinate_calls;*x=5+(rows==1 ? i:i%7)*90;*y=5+(rows==1 ? 0:i/7)*80;}
static void tile_dimensions(struct NDDock *d,int i,int *w,int *h)
{(void)d;(void)i;++dimension_calls;*w=80;*h=70;}
static void SendIO(struct IORequest *request) {(void)request;++sends;}
'''
        for source, signature in [
            (draw_source, 'int item_visible(const struct NDDock *dock, int index)'),
            (draw_source, 'int eyes_index(const struct NDDock *dock)'),
            (window_source, 'int hovered_tile(struct NDDock *dock, int mx, int my)'),
            (timer_source, 'static void arm_timer(struct NDDock *dock)'),
        ]:
            harness += '\n'+extract(source, signature)
        harness += r'''
int main(void)
{
    static struct NDDock dock;
    struct Window window;
    struct Screen screen;
    struct TimerRequest timer;
    int i;
    window.Width=1300;window.Height=100;dock.window=&window;dock.conf.count=13;
    assert(hovered_tile(NULL,0,0)==-1);
    assert(hovered_tile(&dock,-1,10)==-1);
    assert(hovered_tile(&dock,10,-1)==-1);
    assert(hovered_tile(&dock,1300,10)==-1);
    assert(hovered_tile(&dock,10,100)==-1);
    assert(coordinate_calls==0 && dimension_calls==0);
    for(i=0;i<13;++i)assert(hovered_tile(&dock,10+i*90,10)==i);
    assert(hovered_tile(&dock,85,10)==-1);
    assert(hovered_tile(&dock,10,75)==-1);
    rows=2;window.Width=650;window.Height=180;
    assert(hovered_tile(&dock,280,90)==10);
    dock.overflow=1;dock.view_start=5;dock.visible_count=3;
    assert(hovered_tile(&dock,280,90)==-1);
    assert(hovered_tile(&dock,550,10)==6);
    dock.conf.items[0].kind=dock.conf.items[6].kind=ND_ITEM_PLUGIN;
    strcpy(dock.conf.items[0].id,"EYES");strcpy(dock.conf.items[6].id,"EYES");
    dock.conf.items[0].online=1;
    assert(eyes_index(&dock)==-1);
    dock.conf.items[6].online=1;assert(eyes_index(&dock)==6);
    dock.timer_open=1;dock.timer_req=&timer;dock.screen=&screen;dock.press_item=-1;
    dock.full_box.left=100;dock.full_box.top=100;dock.full_box.width=300;dock.full_box.height=100;
    screen.MouseX=screen.MouseY=0;
    arm_timer(&dock);assert(dock.timer_quanta==1 && sends==1);
    dock.timer_pending=0;dock.covered=1;
    arm_timer(&dock);assert(dock.timer_quanta==5 && timer.tr_time.tv_secs==1);
    dock.timer_pending=0;dock.covered=0;dock.conf.items[6].online=0;
    arm_timer(&dock);assert(dock.timer_quanta==5);
    dock.timer_pending=0;dock.hidden=1;
    arm_timer(&dock);assert(dock.timer_quanta==1);
    dock.timer_pending=0;dock.hidden=0;screen.MouseX=150;screen.MouseY=150;
    arm_timer(&dock);assert(dock.timer_quanta==1);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path/'idle.c').write_text(harness)
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c89', '-Wall', '-Wextra',
                            '-I'+str(ROOT/'include'), str(path/'idle.c'),
                            '-o', str(path/'idle')], check=True)
            subprocess.run([str(path/'idle')], check=True)


if __name__ == '__main__':
    unittest.main()
