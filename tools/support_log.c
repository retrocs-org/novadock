/* Low-priority disk writer; never reads or writes user configuration. */
#include "nd_support_log.h"
#include "nd_volume_worker.h"
#include "nd_async_io.h"
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stddef.h>
#include "nd_wait.h"
long __stack=32768;
/* Claim startup records without waiting for the producer to run again.
 * Port lifetime and each transfer are protected by the scheduler lock; no
 * allocation, file access or semaphore wait occurs in this short walk. */
static void collect_outboxes(struct MsgPort *destination)
{
    struct Node *node;struct Message *m;
    Forbid();
    for(node=SysBase->PortList.lh_Head;node->ln_Succ;node=node->ln_Succ)
        if(node->ln_Name&&!strncmp(node->ln_Name,ND_SUPPORT_OUTBOX,sizeof(ND_SUPPORT_OUTBOX)-1))
            while((m=GetMsg((struct MsgPort *)node))!=NULL)PutMsg(destination,m);
    Permit();
}
int main(int argc,char **argv)
{
    struct MsgPort *p;struct NDSupportLogPacket *m;struct NDWaitTimer timer;
    int owner=0,idle=0;
    if(argc==3 && !strcmp(argv[1],"TASK"))return nd_background_worker(argv[2]);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    p=CreateMsgPort();if(!p)return 20;p->mp_Node.ln_Name=ND_SUPPORT_LOG_PORT;
    Forbid();if(!FindPort(ND_SUPPORT_LOG_PORT)){AddPort(p);owner=1;}Permit();
    if(!owner){DeleteMsgPort(p);return 0;}memset(&timer,0,sizeof(timer));
    for(;;){
        collect_outboxes(p);
        if(nd_wait_port(&timer,p,50UL))idle=0;else ++idle;
        while((m=(struct NDSupportLogPacket *)GetMsg(p))!=NULL){
            if(m->message.mn_Length>=offsetof(struct NDSupportLogPacket,data)&&
               m->magic==ND_SUPPORT_LOG_MAGIC&&m->component<3&&m->bytes>0&&
               m->bytes<=ND_SUPPORT_RECORD_MAX&&m->message.mn_Length==offsetof(struct NDSupportLogPacket,data)+m->bytes){
                m->result=nd_support_append(ND_SUPPORT_DISK,m->component,m->data,m->bytes);
                nd_support_log_reply(m);
            }else if(m->message.mn_ReplyPort)ReplyMsg((struct Message *)m);
        }
        if(idle>=15){
            /* Atomically stop accepting packets before deciding to exit. */
            Forbid();if(!p->mp_MsgList.lh_Head->ln_Succ){RemPort(p);owner=0;}Permit();
            if(!owner)break;idle=0;
        }
    }
    nd_wait_timer_close(&timer);DeleteMsgPort(p);return 0;
}
