#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_task.h"
#include "nd_reply.h"
#include "nd_interaction.h"
#define TASK_MAGIC 0x4e44544bUL
static unsigned long serial;
static unsigned long task_ticks(void)
{struct DateStamp d;DateStamp(&d);return ((unsigned long)d.ds_Days*1440UL+(unsigned long)d.ds_Minute)*3000UL+(unsigned long)d.ds_Tick;}
static int send_packet(const char *name,struct NDTaskPacket *m)
{struct MsgPort *p;Forbid();p=FindPort((STRPTR)name);if(p)PutMsg(p,(struct Message *)m);Permit();return p!=NULL;}
void nd_task_close(struct NDTask *j)
{struct NDTaskPacket *m;if(j->port){Forbid();RemPort(j->port);Permit();while((m=(struct NDTaskPacket *)GetMsg(j->port))!=NULL){if(m->kind==1 && m->message.mn_ReplyPort){m->kind=-1;nd_reply_finish((struct Message *)m);}else FreeVec(m);}DeleteMsgPort(j->port);}if(j->request)FreeVec(j->request);memset(j,0,sizeof(*j));}
void nd_task_stage(const char *parent,unsigned int stage)
{struct NDTaskProgress *m;if(!parent)return;m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!m)return;m->magic=TASK_MAGIC;m->kind=3;m->stage=stage;m->message.mn_Length=sizeof(*m);if(!send_packet(parent,(struct NDTaskPacket *)m))FreeVec(m);}
void nd_task_cancel(struct NDTask *j){j->cancelled=1;}
unsigned long nd_task_signals(const struct NDTask *j){return j->port ? 1UL<<j->port->mp_SigBit:0;}
int nd_task_start(struct NDTask *j,const char *program,const char *guard,unsigned int action,const void *data,unsigned int bytes)
{
 struct MsgPort *p;char self[530],command[680];BPTR in,out;LONG rc;
 if(j->port || bytes>ND_TASK_DATA || !guard || strlen(guard)>63 || !nd_quote_path(self,sizeof(self),program))return 0;
 Forbid();p=FindPort((STRPTR)guard);Permit();if(p)return 0;
 j->port=CreateMsgPort();j->request=AllocVec(sizeof(*j->request),MEMF_PUBLIC|MEMF_CLEAR);if(!j->port || !j->request){nd_task_close(j);return 0;}
 j->started=task_ticks();strcpy(j->guard,guard);j->cancelled=j->busy=0;
 sprintf(j->name,"NovaDock.Task.%08lx.%08lx.%08lx",(ULONG)FindTask(NULL),j->started,++serial);j->port->mp_Node.ln_Name=j->name;
 Forbid();if(FindPort(j->name)){Permit();DeleteMsgPort(j->port);j->port=NULL;nd_task_close(j);return 0;}AddPort(j->port);Permit();
 j->request->action=action;j->request->bytes=bytes;strcpy(j->request->guard,guard);if(bytes)memcpy(j->request->data,data,bytes);
 sprintf(command,"%s TASK %s",self,j->name);in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(!in || !out){if(in)Close(in);if(out)Close(out);nd_task_close(j);return 0;}
 rc=SystemTags(command,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_Priority,-1L,NP_StackSize,65536UL,TAG_DONE);if(rc==-1){Close(in);Close(out);nd_task_close(j);return 0;}return 1;
}
struct NDTaskPacket *nd_task_poll(struct NDTask *j)
{
 struct NDTaskPacket *m;struct MsgPort *guard;
 if(!j->port)return NULL;
 while((m=(struct NDTaskPacket *)GetMsg(j->port))!=NULL){
  if(m->message.mn_Length==sizeof(struct NDTaskProgress)&&m->magic==TASK_MAGIC&&m->kind==3){j->stage=((struct NDTaskProgress *)m)->stage;FreeVec(m);continue;}
  if(m->message.mn_Length!=sizeof(*m) || m->magic!=TASK_MAGIC){if(m->message.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;}
  if(m->kind==1){if(j->cancelled)m->kind=-1;else{m->action=j->request->action;m->bytes=j->request->bytes;strcpy(m->guard,j->guard);memcpy(m->data,j->request->data,m->bytes);}nd_reply_finish((struct Message *)m);if(j->cancelled){nd_task_close(j);return NULL;}}
  else{int cancelled=j->cancelled;nd_task_close(j);if(cancelled){FreeVec(m);return NULL;}return m;}
 }
 if(task_ticks()-j->started>750UL){Forbid();guard=FindPort(j->guard);Permit();j->busy=1;if(!guard)nd_task_close(j);}
 return NULL;
}
int nd_task_worker(const char *parent,NDTaskAction action)
{
 struct NDTaskPacket *m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);struct MsgPort *reply=nd_reply_port(),*guard=NULL;int replied=0,i,owner=0;
 ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 if(!m || !reply){if(m)FreeVec(m);if(reply)nd_reply_delete(reply);return 20;}
 m->magic=TASK_MAGIC;m->kind=1;m->message.mn_Length=sizeof(*m);m->message.mn_ReplyPort=reply;
 if(!send_packet(parent,m)){FreeVec(m);nd_reply_delete(reply);return 10;}
 for(i=0;i<150 && !replied;i+=2){if(GetMsg(reply))replied=1;else Delay(2);}
 /* Atomic with the sender: do not abandon an already delivered reply. */
 Forbid();if(!replied && GetMsg(reply))replied=1;if(!replied)nd_reply_abandon(reply);Permit();if(!replied)return 10;
 nd_reply_delete(reply);m->message.mn_ReplyPort=NULL;if(m->kind!=1 || m->bytes>ND_TASK_DATA){FreeVec(m);return 10;}
 guard=CreateMsgPort();if(guard){guard->mp_Node.ln_Name=m->guard;Forbid();if(!FindPort(m->guard)){AddPort(guard);owner=1;}Permit();}
 m->result=owner ? action(m->action,m->data,m->bytes,parent):-3;
 /* Remove the guard before freeing the packet which carries its name. */
 if(guard){if(owner){Forbid();RemPort(guard);Permit();}DeleteMsgPort(guard);}
 m->kind=2;if(!send_packet(parent,m))FreeVec(m);return 0;
}
