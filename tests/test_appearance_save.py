#!/usr/bin/env python3
"""Use/Save reload only after Preferences has closed; Test remains live."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/"prefs/novadockappearanceprefs.c").read_text()
a=s.index("static int apply_appearance(");b=s.index("\n}",a)+2
cleanup=s[s.index("    if (p->screen) nd_backdrop_free",b):s.index("    if (AslBase)",b)]
code=r'''#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_config.h"
#include "nd_layout.h"
typedef long LONG;typedef char *STRPTR;
#define ND_CONTROL_OK 0
#define ND_CONTROL_NOHOST 1
#define ND_CONTROL_PENDING 2
#define ND_CONTROL_TIMEOUT 3
#define ND_PREFS_MARK_CLEAN() (++clean)
#define ND_PREFS_CLEAR_PREVIEW() (++cleared)
#define ND_PREFS_MARK_PREVIEW() (++preview)
struct Screen {int Width,Height,BarHeight;};
struct AppearancePrefs {struct NDConfig *cfg;struct Screen *screen;void *win,*gadgets,*vi;int background,notify_reload,done,status;};
static int visible,unlocked,reloads,writes,clean,cleared,preview,fail_write,expect_open;
static int collect_numbers(struct AppearancePrefs*p,int validate){(void)p;assert(validate);return 1;}
static int nd_layout_fit_enabled_mock(struct NDConfig*c,int w,int h,int bar,struct NDLayout*l){(void)c;(void)w;(void)h;(void)bar;(void)l;return 1;}
#define nd_layout_fit_enabled nd_layout_fit_enabled_mock
static void create_settings_dir(const char*p){(void)p;}
static int nd_control_write_config(struct NDConfig*c,const char*p){(void)c;assert(visible&&strstr(p,"novadock.cfg"));++writes;return writes!=fail_write;}
static LONG nd_control_reload(void){assert(visible==expect_open);if(!expect_open)assert(unlocked);++reloads;return ND_CONTROL_OK;}
static void SetWindowTitles(void*w,STRPTR text,STRPTR screen){(void)w;(void)text;(void)screen;}
static void DisplayBeep(struct Screen*s){(void)s;assert(0);}
static void nd_backdrop_free(int*b,struct Screen*s){(void)b;(void)s;}
static void free_pens(struct AppearancePrefs*p){(void)p;}
static void CloseWindow(void*w){assert(w&&visible);visible=0;}
static void FreeGadgets(void*g){(void)g;}
static void FreeVisualInfo(void*v){(void)v;}
static void UnlockPubScreen(void*name,struct Screen*s){(void)name;(void)s;unlocked=1;}
'''+s[a:b]+"\nstatic void finish(struct AppearancePrefs*p){\n"+cleanup+"}\n"+r'''
static struct NDConfig cfg;static struct Screen screen;
static void init(struct AppearancePrefs*p){memset(p,0,sizeof(*p));p->cfg=&cfg;p->screen=&screen;p->win=p;visible=1;unlocked=reloads=writes=clean=cleared=preview=fail_write=expect_open=0;}
int main(void){struct AppearancePrefs p;int permanent;
 for(permanent=0;permanent<=1;++permanent){init(&p);assert(apply_appearance(&p,permanent,1));assert(!reloads&&p.done&&p.notify_reload&&visible&&writes==1+permanent&&clean==1&&cleared==1);finish(&p);assert(reloads==1&&!visible&&unlocked);}
 init(&p);expect_open=1;assert(apply_appearance(&p,0,0));assert(reloads==1&&visible&&!p.done&&!p.notify_reload&&preview==1);
 init(&p);fail_write=1;assert(!apply_appearance(&p,1,1));assert(!p.done&&!p.notify_reload&&!reloads&&visible);
 puts("Appearance Use/Save closes and unlocks before reload; Test stays live; failed writes keep the editor open.");return 0;}
'''
with tempfile.TemporaryDirectory(prefix="nd-appearance-save-") as t:
 p=Path(t);(p/"test.c").write_text(code)
 subprocess.run([os.environ.get("CC","cc"),"-std=c89","-Wall","-Wextra","-Werror","-I"+str(root/"include"),str(p/"test.c"),"-o",str(p/"test")],check=True)
 subprocess.run([str(p/"test")],check=True)
