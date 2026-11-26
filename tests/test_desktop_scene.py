#!/usr/bin/env python3
"""Production scene invalidation: close/move detection and drag deferral."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_window.c').read_text();a=s.index('void desktop_scene_tick(');b=s.index('\n}',a)+2
h=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_autohide.h"
typedef unsigned long ULONG;
#define WFLG_BACKDROP 1
#define IEQUALIFIER_LEFTBUTTON 1
struct Window {int LeftEdge,TopEdge,Width,Height,Flags;struct Window*NextWindow;};
struct Screen {struct Window*FirstWindow;};
struct NDDock {struct Window*window,*tooltip,*caption_popup,*popup,*calendar,*folder_popup,*about,*feedback;struct Screen*screen;void*hover_desktop;struct NDRect full_box;int screen_closed,desktop_scene_age,desktop_scene_dirty,desktop_scene_valid,desktop_refresh_pending,desktop_probe_ms,desktop_ready_ms;ULONG desktop_scene_stamp;};
static int locks,dragging;static void*InputBase=(void*)1;
static ULONG LockIBase(int z){assert(!z);++locks;return 1;}static void UnlockIBase(ULONG k){assert(k==1);}
static int PeekQualifier(void){return dragging;}
'''+s[a:b]+r'''
int main(void){struct Window app={20,20,100,100,0,0},dock={0,0,200,200,0,0},desktop={0,0,640,480,1,0},foreign={400,400,50,50,0,0};struct Screen screen={&app};struct NDDock d;int n;memset(&d,0,sizeof(d));app.NextWindow=&dock;dock.NextWindow=&desktop;desktop.NextWindow=&foreign;d.screen=&screen;d.window=&dock;d.hover_desktop=&d;d.full_box.width=d.full_box.height=200;d.desktop_scene_age=3000;
 desktop_scene_tick(&d,200);assert(d.desktop_scene_valid&&!d.desktop_scene_dirty&&!d.desktop_refresh_pending);
 for(n=0;n<100;++n)desktop_scene_tick(&d,200);assert(!d.desktop_refresh_pending);foreign.LeftEdge++;desktop_scene_tick(&d,200);assert(!d.desktop_refresh_pending);
 dragging=1;app.LeftEdge++;desktop_scene_tick(&d,200);assert(d.desktop_scene_dirty&&!d.desktop_refresh_pending);dragging=0;desktop_scene_tick(&d,200);assert(d.desktop_refresh_pending==2);
 d.desktop_scene_dirty=d.desktop_refresh_pending=0;screen.FirstWindow=&dock;desktop_scene_tick(&d,200);assert(d.desktop_refresh_pending==2);
 d.desktop_scene_dirty=d.desktop_refresh_pending=0;d.tooltip=&app;screen.FirstWindow=&app;desktop_scene_tick(&d,200);assert(!d.desktop_refresh_pending);/* A late-closing startup backdrop must invalidate an already settled capture. */
 d.desktop_scene_dirty=d.desktop_refresh_pending=0;desktop.NextWindow=0;
 desktop_scene_tick(&d,200);assert(!d.desktop_refresh_pending);
 dock.NextWindow=0;desktop_scene_tick(&d,200);assert(d.desktop_refresh_pending==2);
 d.hover_desktop=0;n=locks;desktop_scene_tick(&d,200);assert(locks==n);
 puts("Desktop scene: nearby close/move invalidation, stable/outside scenes, mouse-drag deferral and own-popup exclusion passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
