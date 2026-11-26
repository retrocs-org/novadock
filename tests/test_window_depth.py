#!/usr/bin/env python3
"""Overlapping application windows suppress backdrop-dock hover, not its popups."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_timers.c').read_text();a=s.index('static int dock_pointer_visible(');b=s.index('\n}',a)+2
window=(ROOT/'src/dock_window.c').read_text()
assert re.search(r'WA_Backdrop,\s*!dock->conf.always_on_top',window)
h=r'''#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0

#include <stdio.h>
typedef unsigned long ULONG;
#define WFLG_BACKDROP 1
struct Window {int LeftEdge,TopEdge,Width,Height,Flags;struct Window*NextWindow;};
struct Screen {int MouseX,MouseY;struct Window*FirstWindow;};
struct NDDock {int canvas_view,press_item;struct Window*window,*tooltip,*caption_popup;struct Screen*screen;};
#define IEQUALIFIER_LEFTBUTTON 1
static void *InputBase;static int dragging;
static struct {struct Window *ActiveWindow;} intuition;
#define IntuitionBase (&intuition)
static int PeekQualifier(void){return dragging;}
static int locked;
static ULONG LockIBase(int zero){assert(!zero&&!locked);locked=1;return 1;}
static void UnlockIBase(ULONG key){assert(key==1&&locked);locked=0;}
'''+s[a:b]+r'''
int main(void){struct Window dock={0,0,500,100,1,0},app={100,10,200,80,0,0},popup={120,20,100,50,0,0},desk={0,0,640,256,1,0};struct Screen screen={130,30,&app};struct NDDock d={0,-1,&dock,&popup,0,&screen};
 app.NextWindow=&popup;popup.NextWindow=&dock;dock.NextWindow=&desk;
 assert(!dock_pointer_visible(&d)&&!locked);
 app.LeftEdge=400;assert(dock_pointer_visible(&d));
 d.tooltip=0;d.caption_popup=&popup;assert(dock_pointer_visible(&d));
 d.caption_popup=0;assert(!dock_pointer_visible(&d));
 screen.MouseX=220;assert(dock_pointer_visible(&d)); /* right edge is exclusive */
 app.Flags=WFLG_BACKDROP;screen.MouseX=450;assert(dock_pointer_visible(&d));
 app.Flags=0;assert(!dock_pointer_visible(&d));
 dock.Flags=0;assert(dock_pointer_visible(&d)); /* explicit Always on top */
 InputBase=(void*)1;dragging=1;intuition.ActiveWindow=&app;assert(!dock_pointer_visible(&d));d.press_item=0;assert(dock_pointer_visible(&d));InputBase=0;
 puts("Window depth: application occlusion, exclusive edges, backdrop peers, caption/tooltip exceptions and explicit top mode passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
