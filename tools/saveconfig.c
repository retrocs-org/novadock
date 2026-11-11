/* A separate executable keeps C runtime/static parser state and filesystem
 * waits out of the dock process. The dock lends its snapshot for the write. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "nd_config.h"
#include "nd_save.h"
#include "nd_version.h"
const char nd_version[]=ND_VER_STRING("SaveConfig");
long __stack=32768;
static ULONG ticks(void)
{
    struct DateStamp d;DateStamp(&d);
    return ((ULONG)d.ds_Days*1440UL+(ULONG)d.ds_Minute)*3000UL+(ULONG)d.ds_Tick;
}
static int send_to(const char *name,struct NDSaveMessage *m)
{
    struct MsgPort *p;
    Forbid();p=FindPort((STRPTR)name);
    if(p)PutMsg(p,(struct Message *)m);
    Permit();return p!=NULL;
}
int main(int argc,char **argv)
{
    struct NDSaveMessage *m;struct MsgPort *reply;ULONG start,phase;BPTR dir;
    if(argc!=2 || strlen(argv[1])>63)return 20;
    SetTaskPri(FindTask(NULL),-1);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);reply=CreateMsgPort();
    if(!m || !reply){if(m)FreeVec(m);if(reply)DeleteMsgPort(reply);return 20;}
    m->msg.mn_Length=sizeof(*m);m->msg.mn_ReplyPort=reply;
    m->magic=ND_SAVE_MAGIC;m->kind=ND_SAVE_BORROW;m->config_size=sizeof(struct NDConfig);
    if(!send_to(argv[1],m)){DeleteMsgPort(reply);FreeVec(m);return 10;}
    WaitPort(reply);GetMsg(reply);DeleteMsgPort(reply);m->msg.mn_ReplyPort=NULL;
    if(!m->config){FreeVec(m);return 10;}
    start=ticks();
    dir=CreateDir("ENV:NovaDock");if(dir)UnLock(dir);
    dir=CreateDir("ENVARC:NovaDock");if(dir)UnLock(dir);
    phase=ticks();m->env_ok=nd_write_config(m->config,"ENV:NovaDock/novadock.cfg");
    m->env_ticks=ticks()-phase;
    phase=ticks();m->envarc_ok=nd_write_config(m->config,"ENVARC:NovaDock/novadock.cfg");
    m->envarc_ticks=ticks()-phase;m->elapsed=ticks()-start;
    /* The configuration is the dock's: lent for this write, not given. */
    m->config=NULL;m->kind=ND_SAVE_COMPLETE;
    if(!send_to(argv[1],m))FreeVec(m);
    return 0;
}
