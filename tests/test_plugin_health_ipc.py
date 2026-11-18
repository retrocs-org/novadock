#!/usr/bin/env python3
"""Exercise the production read-only health branch for every dock item slot."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_reload.c').read_text()
a=s.index('else if(m->action==ND_CONTROL_PLUGIN_HEALTH');b=s.index('            else if (m->action == ND_CONTROL_QUIT)',a)
branch=s[a:b].replace('else if','if',1)
code=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned short UWORD;
#define ND_CONTROL_PLUGIN_HEALTH 13
#define ND_CONTROL_INVALID -1
#define ND_CONTROL_OK 0
#define ND_ITEM_PLUGIN 4
struct NDControlMessage{int action,result;struct{unsigned int mn_Length;}msg;};
struct NDControlPluginHealthMessage{struct NDControlMessage control;char id[20],value[32];UWORD online,stale;};
struct NDItem{int kind,enabled,online;char id[20],value[32];};
struct Dock{struct{int count;struct NDItem items[48];}conf;};
static void query(struct Dock *dock,struct NDControlMessage *m){m->result=ND_CONTROL_INVALID;
'''+branch+r'''
}
int main(void){struct Dock dock,before;struct NDControlPluginHealthMessage q;int i;
 memset(&dock,0,sizeof(dock));dock.conf.count=48;
 for(i=0;i<48;++i){struct NDItem *p=&dock.conf.items[i];p->kind=ND_ITEM_PLUGIN;p->enabled=1;p->online=1;sprintf(p->id,"HASS_%d",i);strcpy(p->value,"off");}
 strcpy(dock.conf.items[39].value,"NOT RESPONDING");before=dock;
 for(i=0;i<48;++i){memset(&q,0,sizeof(q));q.control.action=ND_CONTROL_PLUGIN_HEALTH;q.control.msg.mn_Length=sizeof(q);sprintf(q.id,"HASS_%d",i);query(&dock,&q.control);
  assert(q.control.result==ND_CONTROL_OK&&q.online);assert(q.stale==(i==39));assert(!strcmp(q.value,i==39 ? "NOT RESPONDING":"off"));assert(!memcmp(&before,&dock,sizeof(dock)));}
 memset(&q,0,sizeof(q));q.control.action=ND_CONTROL_PLUGIN_HEALTH;q.control.msg.mn_Length=sizeof(q)-1;strcpy(q.value,"sentinel");query(&dock,&q.control);assert(q.control.result==ND_CONTROL_INVALID&&!strcmp(q.value,"sentinel"));
 q.control.msg.mn_Length=sizeof(q);strcpy(q.id,"missing");query(&dock,&q.control);assert(q.control.result==ND_CONTROL_INVALID);
 strcpy(q.id,"HASS_4");dock.conf.items[4].enabled=0;query(&dock,&q.control);assert(q.control.result==ND_CONTROL_INVALID);
 puts("Plugin health IPC: all 48 slots, exact instance selection, short requests and disabled/missing items passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'health.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',str(p/'health.c'),'-o',str(p/'health')],check=True)
 subprocess.run([str(p/'health')],check=True)
