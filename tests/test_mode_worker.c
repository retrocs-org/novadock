/* Native worker transport with controlled parent replies and command outcomes. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef unsigned long ULONG;typedef long LONG;typedef long BPTR;
typedef void *APTR;typedef char *STRPTR;
#define MEMF_PUBLIC 0
#define MEMF_CLEAR 0
#define PA_IGNORE 0
#define MODE_OLDFILE 1
#define MODE_NEWFILE 2
#define SYS_Input 1
#define SYS_Output 2
#define SYS_Asynch 3
#define NP_StackSize 4
#define TAG_DONE 0
#define TRUE 1
struct Node {char *ln_Name;};
struct Message {struct Node mn_Node;struct MsgPort *mn_ReplyPort;unsigned int mn_Length;struct Message *next;};
struct MsgPort {struct Node mp_Node;int mp_Flags,mp_SigBit;void *mp_SigTask;struct Message *head;};
struct Process {void *pr_WindowPtr;};struct DateStamp {long ds_Days,ds_Minute,ds_Tick;};
static struct Process self;
static struct MsgPort *registry[8],*parent;
static int commands,allocations,disappear,refuse_reply,blocked_second,command_result;
static void Forbid(void){}static void Permit(void){}
static void FreeSignal(int n){(void)n;}
static void Delay(int n){(void)n;}
static void *FindTask(void *name){(void)name;return &self;}
static void DateStamp(struct DateStamp *d){memset(d,0,sizeof(*d));d->ds_Tick=10;}
static void *AllocVec(size_t n,int f){(void)f;++allocations;return calloc(1,n);}
static void FreeVec(void *p){--allocations;free(p);}
static struct MsgPort *CreateMsgPort(void){struct MsgPort *p=calloc(1,sizeof(*p));p->mp_SigTask=&self;p->mp_Flags=1;return p;}
static void DeleteMsgPort(struct MsgPort *p){assert(!p->head);free(p);}
static void AddPort(struct MsgPort *p){int i;for(i=0;i<8;++i)if(!registry[i]){registry[i]=p;return;}assert(0);}
static void RemPort(struct MsgPort *p){int i;for(i=0;i<8;++i)if(registry[i]==p)registry[i]=NULL;}
static struct MsgPort *FindPort(STRPTR name){int i;for(i=0;i<8;++i)if(registry[i] && !strcmp(registry[i]->mp_Node.ln_Name,name))return registry[i];return NULL;}
static void enqueue(struct MsgPort *p,struct Message *m){struct Message **q=&p->head;while(*q)q=&(*q)->next;*q=m;m->next=NULL;}
static struct Message *GetMsg(struct MsgPort *p){struct Message *m=p->head;if(m){p->head=m->next;m->next=NULL;}return m;}
static void ReplyMsg(struct Message *m){enqueue(m->mn_ReplyPort,m);}
static void PutMsg(struct MsgPort *,struct Message *);
static BPTR Open(STRPTR name,int mode){(void)name;(void)mode;return 1;}
static int Close(BPTR n){(void)n;return 1;}
static LONG SystemTags(STRPTR command,...);
static int nd_quote_path(char *out,size_t size,const char *path){if(strlen(path)+3>size)return 0;sprintf(out,"\"%s\"",path);return 1;}
#include "../plugin_src/screenmode/nd_mode_worker.h"
static void PutMsg(struct MsgPort *p,struct Message *message)
{
    struct ModeMessage *m=(struct ModeMessage *)message;
    if(p==parent && m->kind==1 && !refuse_reply){
        strcpy(m->command,"ScreenMode FROM preset USE");ReplyMsg(message);
    }else enqueue(p,message);
}
static LONG SystemTags(STRPTR command,...)
{
    if(strstr(command," WORK "))return 0;
    ++commands;assert(FindPort(MODE_GUARD));
    blocked_second=mode_worker("PARENT")==10;
    if(disappear){RemPort(parent);DeleteMsgPort(parent);parent=NULL;}
    return command_result;
}
static void make_parent(void){parent=CreateMsgPort();parent->mp_Node.ln_Name="PARENT";AddPort(parent);}
int main(void)
{
    struct ModeMessage *m;struct MsgPort *reply;
    make_parent();assert(mode_ticks()==10);
    assert(start_mode_worker("Plugins/ScreenMode","PARENT"));
    assert(mode_worker("PARENT")==0 && commands==1 && blocked_second);
    m=(struct ModeMessage *)GetMsg(parent);assert(m && m->kind==2 && m->result==0);FreeVec(m);
    command_result=20;assert(mode_worker("PARENT")==0);
    m=(struct ModeMessage *)GetMsg(parent);assert(m->result==20);FreeVec(m);
    disappear=1;assert(mode_worker("PARENT")==0 && !parent && allocations==0);
    assert(mode_worker("MISSING")==0 && allocations==0);
    make_parent();refuse_reply=1;
    assert(mode_worker("PARENT")==20);assert(!FindPort(MODE_GUARD));
    /* The parent can reject a late handshake after the worker has timed out. */
    m=(struct ModeMessage *)parent->head;assert(m);reply=m->msg.mn_ReplyPort;
    assert(reply->mp_Flags==PA_IGNORE && !reply->mp_SigTask);
    close_mode_port(parent);parent=NULL;
    assert(GetMsg(reply)==(struct Message *)m && m->kind==-1);
    FreeVec(m);DeleteMsgPort(reply);assert(allocations==0);
    puts("Screen Mode worker: completion, errors, missing parent, single worker and late handshake passed.");
    return 0;
}
