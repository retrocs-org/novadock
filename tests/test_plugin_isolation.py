#!/usr/bin/env python3
"""Production watchdog and IPC batching with stalled/flooding fake clients."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/dock_plugins.c').read_text()
def function(sig):
 a=s.index(sig);b=s.index('{',a);i=b+1;n=1
 while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
h=r'''
#include <assert.h>
#include <string.h>
#include "nd_watchdog.h"
typedef unsigned long ULONG;typedef unsigned short UWORD;
#define ND_MAX_ITEMS 4
#define ND_ITEM_PLUGIN 3
#define ND_NO_METER 65535
#define ND_PLUGIN_IPC_BATCH 32U
struct Task {int live;};struct Node {struct Node *ln_Succ;};
struct MsgPort {struct {struct Node *lh_Head;}mp_MsgList;int mp_SigBit;};
struct NDMessage {int unused;};
struct NDItem {int kind,online,meter,clicks,media_event;char id[20],command[64],value[64],badge[8];};
struct NDDock {struct {int count;struct NDItem items[4];}conf;int watchdog_timer_ticks;ULONG diagnostic_plugin_updates[4],diagnostic_plugin_last[4],watchdog_retry_after[4];struct Task *plugin_task[4];int plugin_wake_port[4],plugin_event2[4];struct MsgPort *ipc;};
static ULONG now=2000;static int launches,paints,syncs,forbidden,signals,messages;static struct Task self;static struct NDMessage packet;
static ULONG diagnostic_ticks(void){return now;}
static int find_wake_port(struct NDDock *d,int i,struct Task *t){(void)d;(void)i;return t->live;}
static int task_exists(struct Task *t){return t->live;}
static void sync_plugin_twins(struct NDDock *d,int i){(void)d;(void)i;++syncs;}
static void mark_tile_dirty(struct NDDock *d,int i){(void)d;(void)i;++paints;}
static int launch_plugin(struct NDDock *d,int i,int restarted){(void)d;(void)i;assert(restarted);++launches;return 1;}
static void Forbid(void){++forbidden;}static void Permit(void){assert(forbidden);--forbidden;}
static struct Task *FindTask(void *p){assert(!p);return &self;}
static void Signal(struct Task *t,ULONG bits){assert(forbidden&&t==&self&&bits==8);++signals;}
static struct NDMessage *GetMsg(struct MsgPort *p){assert(p&&!forbidden);++messages;return &packet;}
'''+function('void plugin_watchdog_tick(')+'\n'+function('static struct NDMessage *next_plugin_packet(')+r'''
int main(void){struct NDDock d;struct Task t;struct MsgPort port;struct Node head,tail;unsigned int count;int i;
 memset(&d,0,sizeof(d));d.conf.count=2;for(i=0;i<2;++i){d.conf.items[i].kind=ND_ITEM_PLUGIN;strcpy(d.conf.items[i].id,"TEST");strcpy(d.conf.items[i].command,"Plugins/Test");}
 d.conf.items[0].online=1;strcpy(d.conf.items[0].value,"WORKING");d.conf.items[0].clicks=20;d.conf.items[0].media_event=7;d.plugin_event2[0]=8;
 d.diagnostic_plugin_updates[0]=1;d.diagnostic_plugin_last[0]=1;t.live=1;d.plugin_task[0]=&t;d.plugin_wake_port[0]=1;
 plugin_watchdog_tick(&d,25);assert(!launches&&!d.conf.items[0].online&&!strcmp(d.conf.items[0].value,"NOT RESPONDING"));assert(!d.conf.items[0].clicks&&!d.conf.items[0].media_event&&!d.plugin_event2[0]);
 plugin_watchdog_tick(&d,25);assert(!launches);
 /* A valid late heartbeat restores health without a replacement process. */
 d.conf.items[0].online=1;d.diagnostic_plugin_last[0]=now;strcpy(d.conf.items[0].value,"READY");plugin_watchdog_tick(&d,25);assert(!launches&&d.conf.items[0].online);
 now+=1001;t.live=0;plugin_watchdog_tick(&d,25);assert(launches==1&&!strcmp(d.conf.items[0].value,"OFFLINE")); /* twins do not double launch */
 launches=0;d.watchdog_retry_after[0]=now+100;plugin_watchdog_tick(&d,25);assert(!launches);
 d.ipc=&port;port.mp_SigBit=3;port.mp_MsgList.lh_Head=&head;head.ln_Succ=&tail;tail.ln_Succ=0;count=0;
 while(next_plugin_packet(&d,&count)){}assert(messages==32&&signals==1&&!forbidden);
 count=0;head.ln_Succ=0;for(i=0;i<32;++i)assert(next_plugin_packet(&d,&count));assert(!next_plugin_packet(&d,&count)&&signals==1);
 return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'check.c').write_text(h)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(root/'include'),str(p/'check.c'),str(root/'src/watchdog.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)
print('Plugin isolation: live stalls never duplicate, late heartbeats recover, dead clients retry with backoff, message floods yield after 32 packets.')
