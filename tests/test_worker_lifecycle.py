#!/usr/bin/env python3
"""Exercise production worker ownership/retirement with an Exec queue adapter."""
from pathlib import Path
import os,subprocess,tempfile
R=Path(__file__).resolve().parents[1]
def function(s,name):
 a=s.index(name);b=s.index('{',a);n=1;i=b+1
 while n:
  n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
task=(R/'plugin_src/common/nd_task.c').read_text();monitor=(R/'plugin_src/monitor/monitor.c').read_text()
base=(R/'tests/test_reply.c').read_text().split('int main(void)')[0]
base=base.replace('typedef long LONG;','typedef char *STRPTR;typedef unsigned long ULONG;typedef int BPTR;typedef long LONG;')
base=base.replace('struct Node {','struct Node {char *ln_Name;').replace('struct Message {struct MsgPort *mn_ReplyPort;};','struct Message {unsigned int mn_Length;struct MsgPort *mn_ReplyPort;};')
base=base.replace('static LONG AllocSignal','static unsigned long now;static unsigned long serial;static struct MsgPort *ports[32];\nstatic LONG AllocSignal')
base+=r'''
#define MODE_OLDFILE 0
#define MODE_NEWFILE 1
#define SYS_Input 1
#define SYS_Output 2
#define SYS_Asynch 3
#define NP_Priority 4
#define NP_StackSize 5
#define TRUE 1
#define TAG_DONE 0
#define TASK_MAGIC 0x4e44544bUL
static int launch_ok=1;
static struct MsgPort *CreateMsgPort(void){return nd_reply_port();}
static void DeleteMsgPort(struct MsgPort *p){nd_reply_delete(p);}
static void AddPort(struct MsgPort *p){int i;for(i=0;i<32;++i)if(!ports[i]){ports[i]=p;return;}assert(0);}
static void RemPort(struct MsgPort *p){int i;for(i=0;i<32;++i)if(ports[i]==p){ports[i]=NULL;return;}assert(0);}
static struct MsgPort *FindPort(const char *name){int i;for(i=0;i<32;++i)if(ports[i]&&!strcmp(ports[i]->mp_Node.ln_Name,name))return ports[i];return NULL;}
static void PutMsg(struct MsgPort *p,struct Message *m){assert(!p->queued);p->queued=m;}
static BPTR Open(const char *p,int mode){(void)p;(void)mode;return 1;}
static void Close(BPTR f){(void)f;}
static long SystemTags(const char *command,...){(void)command;return launch_ok ? 0:-1;}
static unsigned long task_ticks(void){return now;}
static unsigned long ticks(void){return now;}
#include "nd_task.h"
#include "nd_interaction.h"
'''
base+='\n'.join(function(task,x) for x in ('static int send_packet(', 'void nd_task_close(', 'void nd_task_cancel(', 'int nd_task_start(', 'struct NDTaskPacket *nd_task_poll('))+'\n'
# Compile the actual LOGS resident completion path, including generation checks.
base+='#define MONITOR_ID \"LOGS\"\n#define ND_MONITOR_LOGS\n#include "nd_monitor.h"\n'+monitor[monitor.index('#define MONITOR_MAGIC'):monitor.index('static unsigned long ticks(void)')]
base+='\n'+function(monitor,'static void close_job(')+'\n'+function(monitor,'static unsigned int receive_job(')
base+=r'''
static struct NDTaskPacket *hello(struct NDTask *j,struct MsgPort **reply){struct NDTaskPacket *m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);*reply=nd_reply_port();m->message.mn_Length=sizeof(*m);m->message.mn_ReplyPort=*reply;m->magic=TASK_MAGIC;m->kind=1;PutMsg(j->port,(struct Message *)m);return m;}
static void clean(void){assert(!allocations&&!signals&&!forbid);}
int main(void){struct NDTask j;struct NDTaskPacket *m,*answer;struct MsgPort *reply,*guard;struct MonitorMessage *old;char data[]="new-target";int i;memset(&j,0,sizeof(j));
 for(i=0;i<1000;++i){now=10;assert(nd_task_start(&j,"C:True","QA.Guard",7,data,sizeof(data)));assert(!nd_task_start(&j,"C:True","QA.Guard",7,data,sizeof(data)));m=hello(&j,&reply);assert(!nd_task_poll(&j));assert(GetMsg(reply)==(struct Message *)m&&m->action==7&&!strcmp((char *)m->data,data));nd_reply_delete(reply);m->message.mn_ReplyPort=NULL;
  if(i%3==0){nd_task_close(&j);m->kind=2;assert(!send_packet(j.name,m));FreeVec(m);}
  else{if(i%3==1)nd_task_cancel(&j);m->kind=2;m->result=1;PutMsg(j.port,(struct Message *)m);answer=nd_task_poll(&j);if(i%3==1)assert(!answer);else{assert(answer==m);FreeVec(answer);}}
  clean();
 }
 assert(nd_task_start(&j,"C:True","QA.Guard",7,data,sizeof(data)));m=hello(&j,&reply);nd_task_cancel(&j);assert(!nd_task_poll(&j)&&!j.port);assert(GetMsg(reply)==(struct Message *)m&&m->kind==-1);FreeVec(m);nd_reply_delete(reply);clean();
 assert(nd_task_start(&j,"C:True","QA.Guard",7,data,sizeof(data)));now=1000;assert(!nd_task_poll(&j)&&!j.port);clean();
 now=10;assert(nd_task_start(&j,"C:True","QA.Guard",7,data,sizeof(data)));guard=CreateMsgPort();guard->mp_Node.ln_Name="QA.Guard";AddPort(guard);now=1000;assert(!nd_task_poll(&j)&&j.port&&j.busy);nd_task_close(&j);RemPort(guard);DeleteMsgPort(guard);clean();
 launch_ok=0;assert(!nd_task_start(&j,"C:True","QA.Guard",7,data,sizeof(data)));clean();launch_ok=1;
 /* A settings/path change rejects the old request before it can read a path. */
 job_port=CreateMsgPort();job_port->mp_Node.ln_Name="Monitor.QA";AddPort(job_port);job_generation=1;generation=2;old=AllocVec(sizeof(*old),MEMF_PUBLIC|MEMF_CLEAR);reply=nd_reply_port();old->message.mn_Length=sizeof(*old);old->message.mn_ReplyPort=reply;old->magic=MONITOR_MAGIC;old->kind=1;PutMsg(job_port,(struct Message *)old);assert(!receive_job()&&!job_port);assert(GetMsg(reply)==(struct Message *)old&&old->kind==-1);FreeVec(old);nd_reply_delete(reply);clean();
 /* A read already in flight may return, but cannot contaminate the new target. */
 job_port=CreateMsgPort();job_port->mp_Node.ln_Name="Monitor.QA";AddPort(job_port);old=AllocVec(sizeof(*old),MEMF_PUBLIC|MEMF_CLEAR);old->message.mn_Length=sizeof(*old);old->magic=MONITOR_MAGIC;old->kind=2;old->action=JOB_SCAN;old->result=1;strcpy((char *)old->data,"ERROR old target\n");old->length=17;PutMsg(job_port,(struct Message *)old);assert(!receive_job()&&!job_port&&!history.count);clean();
 puts("Production worker lifecycle: 1000 completed/cancelled/late exchanges, failed launches, stuck guards and old-target replies passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'check.c').write_text(base)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-Wno-unused-function','-I'+str(R/'include'),'-I'+str(R/'plugin_src/common'),'-I'+str(R/'plugin_src/monitor'),str(p/'check.c'),str(R/'src/interaction.c'),str(R/'plugin_src/monitor/nd_monitor.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-g','-O1','-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)
