#!/usr/bin/env python3
"""Run actual Christmas window events and cleanup with native calls stubbed."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/saver/nd_saver_window.h').read_text()
a=s.index('static void saver_close(');b=s.index('static void saver_rect(',a)
c=s.index('static void saver_events(')
h=r'''#include <assert.h>
#include <string.h>
typedef unsigned long ULONG;typedef unsigned short UWORD;
#define IDCMP_MOUSEBUTTONS 1
#define IDCMP_RAWKEY 2
#define IDCMP_REFRESHWINDOW 4
#define IDCMP_ACTIVEWINDOW 5
#define IDCMP_INACTIVEWINDOW 6
#define IECODE_UP_PREFIX 128
#define TRUE 1
struct Window {void *UserPort;};struct Message {int unused;};
struct IntuiMessage {ULONG Class;UWORD Code;};
struct NDSaverWindow {void *screen;struct Window *window;void *bitmap;unsigned long ticks;};
static int freed,closed,replied,queued,refreshes;static struct IntuiMessage message;
static struct Message *GetMsg(void *port){assert(port);if(!queued)return 0;queued=0;return (struct Message *)&message;}
static void ReplyMsg(struct Message *m){assert(m==(struct Message *)&message);++replied;}
static void CloseWindow(struct Window *w){assert(w&&replied);++closed;}
static void FreeBitMap(void *b){assert(b);++freed;}
static void UnlockPubScreen(void *n,void *s){assert(!n&&s);++freed;}
static void BeginRefresh(struct Window *w){assert(w);}
static void saver_blit(struct NDSaverWindow *s){assert(s->window);++refreshes;}
static void EndRefresh(struct Window *w,int complete){assert(w&&complete);}
'''+s[a:b]+s[c:]+r'''
static void event(struct NDSaverWindow *s,ULONG kind,UWORD code){message.Class=kind;message.Code=code;queued=1;saver_events(s);}
int main(void){struct NDSaverWindow s;struct Window win;int key,mouse;memset(&s,0,sizeof(s));win.UserPort=&win;
 s.window=&win;s.bitmap=s.screen=&win;
 event(&s,IDCMP_REFRESHWINDOW,0);assert(refreshes==1&&s.window);
 s.ticks=20;event(&s,IDCMP_INACTIVEWINDOW,0);assert(!s.ticks&&s.window);
 event(&s,IDCMP_MOUSEBUTTONS,0xe8);assert(s.window); /* release of opening click is ignored */
 event(&s,IDCMP_MOUSEBUTTONS,0x68);assert(!s.window&&!s.bitmap&&!s.screen&&closed==1&&freed==2);
 saver_close(&s);assert(closed==1&&freed==2);
 for(key=0;key<256;++key){s.window=&win;event(&s,IDCMP_RAWKEY,(UWORD)key);assert(!s.window);}
 for(mouse=0x68;mouse<=0x6a;++mouse){s.window=&win;event(&s,IDCMP_MOUSEBUTTONS,(UWORD)mouse);assert(!s.window);}
 return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp);(p/'check.c').write_text(h)
    subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',str(p/'check.c'),'-o',str(p/'check')],check=True)
    subprocess.run([str(p/'check')],check=True)
print('Fullscreen SAVER: every raw key and mouse button closes safely; refresh/focus and opening-click release are handled.')

assert "WA_Borderless,TRUE" in s and "WA_Width,s->screen->Width" in s
assert "CreateGadget" not in s and "Play/Pause" not in s
