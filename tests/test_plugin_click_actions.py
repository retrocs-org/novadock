#!/usr/bin/env python3
"""Run actual tile click routing, including latched Shift and stale graph flags."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_events.c').read_text();a=s.index('static void click_tile(struct NDDock *dock,int index,int mx,int my,\n');b=s.index('\n/* ---- Keyboard control',a)
h=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
typedef unsigned short UWORD;
#define IEQUALIFIER_LSHIFT 1
#define IEQUALIFIER_RSHIFT 2
struct NDDock {struct NDConfig conf;void *screen;};
static int toggles,draws;
static int queue_music_control(struct NDDock *d,int i,int x,int y){(void)d;(void)i;(void)x;(void)y;return 0;}
static int launch_default_music_application(struct NDDock *d){(void)d;return 1;}
static void open_trash(struct NDDock *d){(void)d;}
static void open_calendar(struct NDDock *d,int i){(void)d;(void)i;}
static void request_dopus_action(struct NDDock *d,int i){(void)d;(void)i;}
static int launch_plugin_helper(struct NDDock *d,int i,const char *v,const char *p){(void)d;(void)i;(void)v;(void)p;return 1;}
static void show_feedback(struct NDDock *d,const char *v,int error){(void)d;(void)v;(void)error;}
static int toggle_monitor_view(struct NDDock *d,int i){(void)d;(void)i;++toggles;return 1;}
static void DisplayBeep(void *s){(void)s;}
static void draw_tile(struct NDDock *d,int i){(void)d;(void)i;++draws;}
static void open_popup(struct NDDock *d,int i,int x,int y){(void)d;(void)i;(void)x;(void)y;}
static void launch_item(struct NDDock *d,int i){(void)d;(void)i;}
'''+s[a:b]+r'''
int main(void){static struct NDDock dock;const char *ids[]={"SCREENSHOT","SCREENMODE","SCREENSWITCHER"};int i,shift;
dock.conf.items[0].kind=ND_ITEM_PLUGIN;
for(i=0;i<3;++i)for(shift=0;shift<4;++shift){strcpy(dock.conf.items[0].id,ids[i]);dock.conf.items[0].graph=1;dock.conf.items[0].clicks=0;click_tile(&dock,0,0,0,(UWORD)shift);assert(dock.conf.items[0].clicks==1&&!dock.conf.items[0].graph&&!toggles);}
dock.conf.items[0].clicks=3;click_tile(&dock,0,0,0,0);assert(dock.conf.items[0].clicks==3);
strcpy(dock.conf.items[0].id,"CPU");click_tile(&dock,0,0,0,0);assert(toggles==1);
strcpy(dock.conf.items[0].id,"OTHER");dock.conf.items[0].graph=0;dock.conf.items[0].clicks=0;click_tile(&dock,0,0,0,1);assert(dock.conf.items[0].graph&&!dock.conf.items[0].clicks);
puts("Screenshot, Mode and Screen Switcher clicks queue actions with any Shift state; monitor toggles remain separate.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'include'),str(p/'check.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)
