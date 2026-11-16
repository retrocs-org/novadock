#!/usr/bin/env python3
"""Exercise action dispatch using harmless fake system calls."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/actions/nd_actions_native.h').read_text();s=s[s.index('static const char *run_action('):]
h=r'''#include <assert.h>
#include <string.h>
#include "nd_actions.h"
typedef long BPTR,LONG;
#define MODE_OLDFILE 1
#define MODE_NEWFILE 2
#define SYS_Asynch 1
#define SYS_Input 2
#define SYS_Output 3
#define TAG_DONE 0
#define TRUE 1
static int reboot,closed,opened,resumed,front,back,shell,suspended,fail_suspend,fail_close,fail_open;
static void ColdReboot(void){++reboot;}
static int dock_command(const char *c){if(!strcmp(c,"SUSPEND")){++suspended;return !fail_suspend;}if(!strcmp(c,"RESUME")){++resumed;return 1;}return !strcmp(c,"RESTART");}
static int CloseWorkBench(void){++closed;return !fail_close;}
static int OpenWorkBench(void){++opened;return !fail_open;}
static void WBenchToFront(void){++front;}static void WBenchToBack(void){++back;}
static BPTR Open(const char *p,int mode){assert(!strcmp(p,"NIL:")&&mode);return 1;}
static void Close(BPTR p){assert(p);}
static LONG SystemTags(const char *p,int tag,...){assert(!strcmp(p,"NewShell")&&tag==SYS_Asynch);++shell;return 0;}
'''+s+r'''
int main(void){
 assert(nd_action_index("Open Shell")==ND_ACTION_SHELL);
 assert(run_action(-1)&&run_action(99));assert(!reboot&&!closed&&!shell);
 assert(!run_action(ND_ACTION_WORKBENCH)&&suspended==1&&closed==1&&opened==1&&resumed==1);
 fail_close=1;assert(run_action(ND_ACTION_WORKBENCH)&&resumed==2&&opened==2);fail_close=0;
 fail_open=1;assert(run_action(ND_ACTION_WORKBENCH)&&resumed==3);fail_open=0;
 fail_suspend=1;assert(run_action(ND_ACTION_WORKBENCH)&&closed==3&&resumed==3);
 assert(!run_action(ND_ACTION_DOCK));assert(!run_action(ND_ACTION_SHELL)&&shell==1);
 assert(!run_action(ND_ACTION_FRONT)&&front==1);assert(!run_action(ND_ACTION_BACK)&&back==1);
 assert(run_action(ND_ACTION_REBOOT)&&reboot==1); /* ColdReboot is a counter here. */
 return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('ACTIONS: stubbed system dispatch, safe Workbench recovery, bounded fixed commands and invalid actions passed.')
