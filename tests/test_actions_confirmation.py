#!/usr/bin/env python3
"""Run actual resident loop with scripted input; no real action can run."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/actions/actions.c').read_text();a=s.index('struct Settings');body=s[a:].replace('int main(int argc,char **argv)','static int plugin_main(int argc,char **argv)')
h=r'''#include <assert.h>
#include <string.h>
#include <stddef.h>
#include "nd_actions.h"
#define ND_EVENT_QUIT 1
#define ND_EVENT_SETTINGS 2
#define ND_EVENT_CLICK 4
#define ND_NO_METER 65535
struct NDClient {int v;};struct NDPanel {int v;};struct Library {int v;};
struct NDPanelField {const char *label,*value;unsigned int limit;};
struct NDOptionString {const char *key,*fallback;char *out;unsigned int limit;};
static struct Library library;static void *IntuitionBase,*RexxSysBase;
static struct NDPanel panel_object;static int mode,update,button,calls,last_action,unavailable;
static int nd_client_init(struct NDClient *c,const char *id){(void)c;assert(!strcmp(id,"ACTIONS_2"));return 1;}
static void *OpenLibrary(const char *n,int v){(void)n;(void)v;return &library;}
static void CloseLibrary(struct Library *p){assert(p);}
static void nd_plugin_option_strings(const char *id,struct NDOptionString *f,int n){int i;assert(!strcmp(id,"ACTIONS_2"));for(i=0;i<n;++i)strcpy(f[i].out,!strcmp(f[i].key,"ACTION") ? "Restart Amiga":!strcmp(f[i].key,"CONFIRM") ? mode==1 ? "NO":"YES":f[i].fallback);}
static unsigned int nd_client_update_quiet(struct NDClient *c,const char *a,const char *v,unsigned int m,unsigned int elapsed,unsigned int heartbeat){(void)c;(void)a;(void)v;(void)m;(void)elapsed;assert(heartbeat==500);++update;if(update==1)return mode==0 ? ND_EVENT_QUIT:ND_EVENT_CLICK;if(mode==5&&update==2)return ND_EVENT_SETTINGS;return ND_EVENT_QUIT;}
static void nd_client_close(struct NDClient *c){(void)c;}
static unsigned int nd_client_wait_signals(struct NDClient *c,unsigned int t,unsigned long sig){(void)c;(void)sig;assert(t==500);return 500;}
static struct NDPanel *nd_panel_open(const char *title,const struct NDPanelField *fields,unsigned int n,const char **buttons,unsigned int count){assert(title&&n==2&&count==2&&strstr(fields[1].value,"Unsaved"));assert(!strcmp(buttons[0],"Run action")&&!strcmp(buttons[1],"Cancel"));return unavailable ? NULL:&panel_object;}
static void nd_panel_close(struct NDPanel *p){(void)p;}
static void nd_panel_raise(struct NDPanel *p){assert(p);}
static unsigned long nd_panel_signals(struct NDPanel *p){return p ? 1:0;}
static int nd_panel_event(struct NDPanel *p){return p ? button:0;}
static const char *run_action(int i){++calls;last_action=i;return NULL;}
'''+body+r'''
static void run(int m,int b,int fail,int expected){char *args[]={"Actions","ACTIONS_2",NULL};mode=m;button=b;unavailable=fail;update=calls=0;assert(plugin_main(2,args)==0);assert(calls==expected);if(calls)assert(last_action==ND_ACTION_REBOOT);}
int main(void){
 run(0,100,0,0); /* startup never runs even with a spurious button */
 run(1,0,0,1); /* explicitly disabled confirmation */
 run(2,100,0,1); /* confirmed */
 run(2,101,0,0);run(2,-1,0,0); /* Cancel and close/Escape */
 run(2,100,1,0); /* window failure refuses action */
 run(5,0,0,0); /* settings change cancels a pending action */
 return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'check.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'include'),str(p/'check.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)
print('ACTIONS: startup/settings never execute; confirmation, cancellation and failed requester gating passed.')
