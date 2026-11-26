/* Bounded persistent output. The producer only queues; SupportLog does disk I/O. */
#include "nd_support_log.h"
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
static const char *const component_names[]={"dock","prefs","dopus4"};
/* Only our fixed files in four owned session slots may be removed. A folder
 * with unknown content is left intact, even if that reduces retention space. */
static int session_info(const char *directory,const char *component,ULONG *stamp,int *closed)
{
    char path[240],name[24];FILE *f;unsigned long value;int clean,ok;
    sprintf(path,"%s/session.txt",directory);f=fopen(path,"r");if(!f)return 0;
    ok=fscanf(f,"NovaDock session 1 component=%23s stamp=%lu closed=%d",name,&value,&clean)==3 && !strcmp(name,component) && (clean==0||clean==1);
    fclose(f);if(ok){*stamp=value;*closed=clean;}return ok;
}
static int write_session_info(const char *directory,const char *component,ULONG stamp,int closed)
{
    char path[240];FILE *f;int ok;sprintf(path,"%s/session.txt",directory);f=fopen(path,"w");if(!f)return 0;
    ok=fprintf(f,"NovaDock session 1 component=%s stamp=%lu closed=%d\n",component,stamp,closed)>0;
    if(fclose(f))ok=0;return ok;
}
static int current_info(const char *directory,const char *component,ULONG *stamp,int *closed,int write)
{
    char path[200];FILE *f;unsigned long value;int clean,ok;
    sprintf(path,"%s/%s.session",directory,component);f=fopen(path,write?"w":"r");if(!f)return 0;
    if(write)ok=fprintf(f,"NovaDock current 1 stamp=%lu closed=%d\n",*stamp,*closed)>0;
    else {ok=fscanf(f,"NovaDock current 1 stamp=%lu closed=%d",&value,&clean)==2 && (clean==0||clean==1);if(ok){*stamp=value;*closed=clean;}}
    if(fclose(f))ok=0;return ok;
}
static void archive_session(const char *directory,const char *component)
{
    char root[200],folder[220],source[220],destination[240];BPTR lock;ULONG stamp=0,stamps[4],newest=0,oldest=~0UL;
    int i,closed=0,clean[4],owned[4],present[4],slot=-1,protected=-1,g,have=0;ULONG serial=0;int slot_count=!strcmp(directory,ND_SUPPORT_RAM)?2:4;
    (void)current_info(directory,component,&serial,&closed,0);
    for(g=0;g<2;++g){sprintf(source,"%s/%s.log%s",directory,component,g?".previous":"");lock=Lock(source,ACCESS_READ);if(lock){UnLock(lock);have=1;}}
    if(!have)goto marker;
    sprintf(root,"%s/Sessions",directory);lock=CreateDir(root);if(lock)UnLock(lock);
    for(i=0;i<slot_count;++i){
        sprintf(folder,"%s/%s-%d",root,component,i+1);lock=Lock(folder,ACCESS_READ);present[i]=lock!=0;if(lock)UnLock(lock);
        owned[i]=session_info(folder,component,&stamps[i],&clean[i]);
        if(owned[i]&&!clean[i] && (protected<0||stamps[i]>=newest)){newest=stamps[i];protected=i;}
        if(!present[i] && slot<0)slot=i;
    }
    if(slot<0)for(i=0;i<slot_count;++i)if(owned[i]&&i!=protected&&stamps[i]<=oldest){oldest=stamps[i];slot=i;}
    if(slot<0)goto marker;
    sprintf(folder,"%s/%s-%d",root,component,slot+1);
    if(present[slot]){
        /* Do not delete anything when somebody added an unfamiliar file. */
        struct FileInfoBlock *fib;int unknown=0;
        lock=Lock(folder,ACCESS_READ);fib=AllocDosObject(DOS_FIB,NULL);
        if(!lock||!fib){if(lock)UnLock(lock);if(fib)FreeDosObject(DOS_FIB,fib);goto marker;}
        if(Examine(lock,fib))while(ExNext(lock,fib)){
            char log[40],previous[48];sprintf(log,"%s.log",component);sprintf(previous,"%s.log.previous",component);
            if(strcmp(fib->fib_FileName,"session.txt")&&strcmp(fib->fib_FileName,log)&&strcmp(fib->fib_FileName,previous))unknown=1;
        }else unknown=1;
        if(IoErr()!=ERROR_NO_MORE_ENTRIES)unknown=1;
        FreeDosObject(DOS_FIB,fib);UnLock(lock);if(unknown)goto marker;
        for(g=0;g<2;++g){sprintf(destination,"%s/%s.log%s",folder,component,g?".previous":"");DeleteFile(destination);}
        sprintf(destination,"%s/session.txt",folder);DeleteFile(destination);
        if(!DeleteFile(folder))goto marker;
    }
    lock=CreateDir(folder);if(!lock)goto marker;UnLock(lock);
    (void)current_info(directory,component,&stamp,&closed,0);
    /* A legacy log has serial zero, older than numbered captures. */
    if(!write_session_info(folder,component,stamp,closed))goto marker;
    for(g=0;g<2;++g){
        sprintf(source,"%s/%s.log%s",directory,component,g?".previous":"");
        sprintf(destination,"%s/%s.log%s",folder,component,g?".previous":"");
        /* A failed rename leaves its source in place; never erase it. */
        (void)Rename(source,destination);
    }
 marker:
    stamp=serial+1UL;closed=0;(void)current_info(directory,component,&stamp,&closed,1);
}
int nd_support_append(const char *directory,unsigned int component,const char *data,unsigned int bytes)
{
    BPTR f,lock;LONG size,written;char path[160],previous[168];int ok;ULONG limit;
    if(!directory||!data||component>=3||!bytes||bytes>ND_SUPPORT_RECORD_MAX||strlen(directory)>120)return 0;
    limit=!strcmp(directory,ND_SUPPORT_RAM)?65536UL:ND_SUPPORT_LOG_LIMIT;
    lock=CreateDir((STRPTR)directory);if(lock)UnLock(lock);
    if(bytes>=16 && !memcmp(data,"NovaDock trace ",15))archive_session(directory,component_names[component]);
    sprintf(path,"%s/%s.log",directory,component_names[component]);sprintf(previous,"%s.previous",path);
    f=Open(path,MODE_READWRITE);if(!f)return 0;
    if(Seek(f,0,OFFSET_END)<0){Close(f);return 0;}size=Seek(f,0,OFFSET_CURRENT);
    if(size<0){Close(f);return 0;}
    if((ULONG)size+bytes>limit){
        if(!Close(f))return 0;
        lock=Lock(previous,ACCESS_READ);if(lock){UnLock(lock);if(!DeleteFile(previous))return 0;}
        if(!Rename(path,previous))return 0;
        f=Open(path,MODE_NEWFILE);if(!f)return 0;
    }
    written=Write(f,(APTR)data,(LONG)bytes);ok=written==(LONG)bytes;
    if(!Close(f))ok=0;
    if(ok && bytes>=13 && !memcmp(data,"capture=stop ",13)){
        ULONG stamp=0;int closed=1;
        (void)current_info(directory,component_names[component],&stamp,&closed,0);closed=1;
        (void)current_info(directory,component_names[component],&stamp,&closed,1);
    }
    return ok;
}
void nd_support_log_reply(struct NDSupportLogPacket *m)
{
    struct NDSupportLogPort *p=(struct NDSupportLogPort *)m->message.mn_ReplyPort;
    Forbid();
    if(p->abandoned){FreeVec(m);if(!--p->pending)FreeVec(p);}
    else ReplyMsg((struct Message *)m);
    Permit();
}
static struct NDSupportLogPort *reply_port;
static struct MsgPort *outbox;
static char outbox_name[48];
static struct NDSupportLogPacket *queue[ND_SUPPORT_PENDING];
static unsigned char sent[ND_SUPPORT_PENDING];
static ULONG dropped,errors,last_start;
static int attempted;
static ULONG support_ticks(void)
{struct DateStamp d;DateStamp(&d);return ((ULONG)d.ds_Days*1440UL+(ULONG)d.ds_Minute)*3000UL+(ULONG)d.ds_Tick;}
static void init_port_list(struct MsgPort *p)
{
    p->mp_Node.ln_Type=NT_MSGPORT;
    p->mp_MsgList.lh_Head=(struct Node *)&p->mp_MsgList.lh_Tail;
    p->mp_MsgList.lh_TailPred=(struct Node *)&p->mp_MsgList.lh_Head;p->mp_MsgList.lh_Type=NT_MESSAGE;
}
static int make_port(void)
{
    LONG bit;int ok=0;
    if(reply_port)return 1;
    bit=AllocSignal(-1L);if(bit<0)return 0;
    reply_port=AllocVec(sizeof(*reply_port),MEMF_PUBLIC|MEMF_CLEAR);
    outbox=AllocVec(sizeof(*outbox),MEMF_PUBLIC|MEMF_CLEAR);
    if(!reply_port||!outbox)goto failed;
    init_port_list(&reply_port->port);reply_port->port.mp_Flags=PA_SIGNAL;
    reply_port->port.mp_SigBit=(UBYTE)bit;reply_port->port.mp_SigTask=FindTask(NULL);
    init_port_list(outbox);outbox->mp_Flags=PA_IGNORE;
    sprintf(outbox_name,ND_SUPPORT_OUTBOX "%08lx",(ULONG)FindTask(NULL));outbox->mp_Node.ln_Name=outbox_name;
    Forbid();if(!FindPort(outbox_name)){AddPort(outbox);ok=1;}Permit();
    if(ok)return 1;
failed:
    if(reply_port)FreeVec(reply_port);if(outbox)FreeVec(outbox);
    reply_port=NULL;outbox=NULL;FreeSignal(bit);return 0;
}
void nd_support_sink_poll(void)
{
    struct NDSupportLogPacket *m;struct MsgPort *worker;int i,waiting=0;ULONG now;
    BPTR in,out;LONG rc;
    if(!reply_port)return;
    while((m=(struct NDSupportLogPacket *)GetMsg(&reply_port->port))!=NULL){
        for(i=0;i<ND_SUPPORT_PENDING;++i)if(queue[i]==m){queue[i]=NULL;sent[i]=0;break;}
        if(!m->result)++errors;--reply_port->pending;FreeVec(m);
    }
    Forbid();worker=FindPort(ND_SUPPORT_LOG_PORT);
    if(worker&&outbox)while((m=(struct NDSupportLogPacket *)GetMsg(outbox))!=NULL)PutMsg(worker,(struct Message *)m);
    if(outbox)waiting=outbox->mp_MsgList.lh_Head->ln_Succ!=NULL;
    Permit();
    if(!waiting)return;
    now=support_ticks();if(attempted&&now>=last_start&&now-last_start<500UL)return;
    attempted=1;last_start=now;
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    if(!in||!out){if(in)Close(in);if(out)Close(out);++errors;return;}
    rc=SystemTags("NovaDock:SupportLog",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_Priority,-2L,NP_StackSize,32768UL,TAG_DONE);
    if(rc<0){Close(in);Close(out);++errors;}
}
void nd_support_sink_write(unsigned int component,const char *data,unsigned int bytes)
{
    struct NDSupportLogPacket *m;int i;
    nd_support_sink_poll();
    if(component>=3||!bytes||bytes>ND_SUPPORT_RECORD_MAX)return;
    for(i=0;i<ND_SUPPORT_PENDING&&queue[i];++i);
    if(i==ND_SUPPORT_PENDING){++dropped;return;}
    if(!make_port()){++errors;return;}
    m=AllocVec((ULONG)offsetof(struct NDSupportLogPacket,data)+bytes,MEMF_PUBLIC|MEMF_CLEAR);
    if(!m){++errors;return;}
    m->message.mn_ReplyPort=&reply_port->port;m->message.mn_Length=(UWORD)(offsetof(struct NDSupportLogPacket,data)+bytes);
    m->magic=ND_SUPPORT_LOG_MAGIC;m->component=(UWORD)component;m->bytes=(UWORD)bytes;
    memcpy(m->data,data,bytes);queue[i]=m;sent[i]=1;
    /* A public PA_IGNORE outbox lets a starting worker collect the first
     * records even if this process immediately blocks in the traced call. */
    Forbid();++reply_port->pending;PutMsg(outbox,(struct Message *)m);Permit();nd_support_sink_poll();
}
void nd_support_sink_close(void)
{
    int i;if(!reply_port)return;nd_support_sink_poll();Forbid();
    if(outbox){
        struct NDSupportLogPacket *m;RemPort(outbox);
        while((m=(struct NDSupportLogPacket *)GetMsg(outbox))!=NULL){
            for(i=0;i<ND_SUPPORT_PENDING;++i)if(queue[i]==m){queue[i]=NULL;sent[i]=0;break;}
            --reply_port->pending;FreeVec(m);++dropped;
        }
        FreeVec(outbox);outbox=NULL;
    }
    reply_port->abandoned=1;reply_port->port.mp_Flags=PA_IGNORE;reply_port->port.mp_SigTask=NULL;
    FreeSignal((LONG)reply_port->port.mp_SigBit);
    /* Completion cannot race this drain: worker retirement also holds Forbid. */
    {struct NDSupportLogPacket *m;while((m=(struct NDSupportLogPacket *)GetMsg(&reply_port->port))!=NULL){
        for(i=0;i<ND_SUPPORT_PENDING;++i)if(queue[i]==m){queue[i]=NULL;sent[i]=0;break;}
        --reply_port->pending;FreeVec(m);
    }}
    for(i=0;i<ND_SUPPORT_PENDING;++i){if(queue[i]&&!sent[i]){FreeVec(queue[i]);++dropped;}queue[i]=NULL;sent[i]=0;}
    if(!reply_port->pending)FreeVec(reply_port);reply_port=NULL;Permit();
}
void nd_support_sink_status(ULONG *queued,ULONG *lost,ULONG *failed)
{int i;*queued=0;for(i=0;i<ND_SUPPORT_PENDING;++i)if(queue[i])++*queued;*lost=dropped;*failed=errors;}
