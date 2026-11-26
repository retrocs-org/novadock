#!/usr/bin/env python3
"""Exercise the production resize acknowledgement before any next animation step."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_events.c').read_text()
a=s.index('int complete_window_box(struct NDDock *dock)\n{')
f=s[a:s.index('\n}',a)+2]
h=r'''#include <assert.h>
#include <string.h>
#include <stdio.h>
struct Window {int unused;};struct Screen {int MouseX,MouseY;};
struct NDDock {int viewport_pending,box_pending,hidden,box_target_hidden,box_pending_ticks,box_late_ack,hover,hover_ticks,idle_ticks,hover_box_paint,hover_shelf_valid,deferred_menu;struct Window *window;struct Screen *screen;struct {int count;}deferred_buttons;};
static int hover_draws,all_draws,volume_queries,task_queries,replays,closed,expect_view;
#define dock_left(d) 100
#define dock_top(d) 200
static void apply_dock_viewport(struct NDDock*d){assert(!d->viewport_pending);}
static void refresh_drop_zones(struct NDDock*d){(void)d;}
static void refresh_volume_meters(struct NDDock*d){(void)d;++volume_queries;}
static void refresh_running_items(struct NDDock*d){(void)d;++task_queries;}
static void draw_hover(struct NDDock*d){assert(d->hover_box_paint==expect_view);assert(!d->box_pending);++hover_draws;}
static void draw_all(struct NDDock*d){assert(d->hover_box_paint==expect_view);assert(!d->box_pending);++all_draws;}
static int replay_deferred_buttons(struct NDDock*d){assert(!d->hover_box_paint);++replays;d->deferred_buttons.count=0;return closed;}
static int hovered_tile(struct NDDock*d,int x,int y){(void)d;assert(x==20&&y==30);return 2;}
static void open_popup(struct NDDock*d,int i,int x,int y){(void)d;assert(i==2&&x==20&&y==30);}
static void set_hidden(struct NDDock*d,int h){d->hidden=h;}
'''+f+r'''
static void pending(struct NDDock*d,int viewport){d->box_pending=1;d->viewport_pending=viewport;d->box_pending_ticks=3;d->box_late_ack=1;}
int main(void){struct NDDock d;struct Window w;struct Screen s={120,230};memset(&d,0,sizeof d);d.window=&w;d.screen=&s;d.hover=4;d.hover_ticks=8;d.hover_shelf_valid=1;expect_view=1;
 pending(&d,1);d.deferred_buttons.count=1;d.deferred_menu=1;
 assert(!complete_window_box(&d));assert(hover_draws==1&&!all_draws&&!volume_queries&&!task_queries&&replays==1&&!d.deferred_menu);assert(!d.box_pending&&!d.viewport_pending&&!d.box_pending_ticks&&!d.box_late_ack&&!d.hover_box_paint);assert(d.hover==4&&d.hover_ticks==8);
 /* Allocation/cache fallback still paints the acknowledged frame, without advancing. */
 pending(&d,1);d.hover_shelf_valid=0;assert(!complete_window_box(&d));assert(all_draws==1&&!volume_queries&&!task_queries);
 /* A normal show acknowledgement refreshes content and clears old hover. */
 pending(&d,0);expect_view=0;assert(!complete_window_box(&d));assert(all_draws==2&&volume_queries==1&&task_queries==1&&d.hover==-1&&!d.hover_ticks);
 /* Deferred input may close the dock; no subsequent access is required. */
 pending(&d,1);expect_view=1;d.deferred_buttons.count=1;closed=1;assert(complete_window_box(&d)==1&&replays==2&&!d.hover_box_paint);
 assert(!complete_window_box(&d));d.window=0;pending(&d,1);assert(!complete_window_box(&d));
 puts("Resize acknowledgement: paints before easing, reuses shelf, avoids per-frame disk/task queries and replays deferred input.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
