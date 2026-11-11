/* Exercise the actual native request/recovery implementation with fake ports. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef unsigned long ULONG;
typedef long LONG;
typedef char *STRPTR;
#define RXFF_RESULT 1
#define RXCOMM 2
#define PA_IGNORE 0
#define MEMF_PUBLIC 0
#define MEMF_CLEAR 0
struct Task {int n;};
struct Node {char *ln_Name;};
struct Message {struct Node mn_Node;struct MsgPort *mn_ReplyPort;unsigned int mn_Length;struct Message *next;};
struct MsgPort {struct Node mp_Node;int mp_Flags,mp_SigBit;struct Task *mp_SigTask;struct Message *head;};
struct RexxMsg {struct Message rm_Node;LONG rm_Result1,rm_Result2;ULONG rm_Action;char *rm_Args[16];};
struct Pending {struct MsgPort *reply,*target;struct Task *owner;struct RexxMsg *message;STRPTR marker;ULONG age;int kind,obsolete;char host[64];};
static struct Task task={1},other={2};
static struct MsgPort *host;
static int allocations,fail_alloc;
static void *RexxSysBase=&task;
static void Forbid(void){}
static void Permit(void){}
static void FreeSignal(int n){(void)n;}
static void *AllocVec(size_t n,int f){(void)f;if(fail_alloc)return NULL;return calloc(1,n);}
static void FreeVec(void *p){free(p);}
static struct MsgPort *CreateMsgPort(void){struct MsgPort *p=calloc(1,sizeof(*p));p->mp_SigTask=&task;p->mp_Flags=1;return p;}
static void DeleteMsgPort(struct MsgPort *p){assert(!p->head);free(p);}
static struct MsgPort *FindPort(STRPTR name){return !strcmp(name,"BITSKIM") ? host : NULL;}
static void PutMsg(struct MsgPort *p,struct Message *m){struct Message **q=&p->head;while(*q)q=&(*q)->next;*q=m;m->next=NULL;}
static struct Message *GetMsg(struct MsgPort *p){struct Message *m=p->head;if(m){p->head=m->next;m->next=NULL;}return m;}
static struct RexxMsg *CreateRexxMsg(struct MsgPort *p,STRPTR ext,STRPTR name)
{struct RexxMsg *m=calloc(1,sizeof(*m));(void)ext;(void)name;m->rm_Node.mn_ReplyPort=p;++allocations;return m;}
static void DeleteRexxMsg(struct RexxMsg *m){--allocations;free(m);}
static STRPTR CreateArgstring(STRPTR s,size_t n){char *p=malloc(n+1);memcpy(p,s,n);p[n]=0;return p;}
static void DeleteArgstring(STRPTR s){free(s);}
#include "../plugin_src/bitskim/nd_bs_request.h"
static void reply_to(struct MsgPort *target)
{struct Message *m=GetMsg(target);assert(m);PutMsg(m->mn_ReplyPort,m);}
int main(void)
{
    struct Pending p;struct RetiredRequest *retired=NULL,*walk;
    struct MsgPort *old,*ports[6];LONG rc;char text[10];int i;
    memset(&p,0,sizeof(p));p.reply=CreateMsgPort();host=CreateMsgPort();old=host;
    assert(send_request(&p,"BITSKIM","STATUS",1,1));
    p.age=999999;assert(recover_request(&p,&retired)==0); /* modal save: never resend */
    host=NULL;fail_alloc=1;assert(recover_request(&p,&retired)==-1 && p.message);fail_alloc=0;
    host=CreateMsgPort();assert(recover_request(&p,&retired)==1 && !p.message);
    assert(send_request(&p,"BITSKIM","STATUS",1,1));
    reply_to(old);reap_requests(&retired);assert(!retired);DeleteMsgPort(old);
    reply_to(host);assert(take_reply(&p,&rc,text,sizeof(text))==1);assert(rc==0);
    /* Same port name/address but a changed owning task also invalidates it. */
    assert(send_request(&p,"BITSKIM","STATUS",1,1));host->mp_SigTask=&other;
    assert(recover_request(&p,&retired)==1);reply_to(host);reap_requests(&retired);assert(!retired);
    DeleteMsgPort(host);
    for(i=0;i<5;++i){
        host=ports[i]=CreateMsgPort();assert(send_request(&p,"BITSKIM","STATUS",1,1));
        host=NULL;assert(recover_request(&p,&retired)==(i<4 ? 1 : -1));
    }
    /* Late replies reclaim every retired packet, keeping recovery bounded. */
    for(i=0;i<5;++i)reply_to(ports[i]);
    reap_requests(&retired);assert(!retired);assert(take_reply(&p,&rc,text,sizeof(text))==1);
    for(i=0;i<5;++i)DeleteMsgPort(ports[i]);
    walk=retired;(void)walk;assert(close_retired(retired)==0);
    assert(close_requests(&p)==0);assert(allocations==0);
    puts("BitSkim recovery: modal waits, owner replacement, late replies and bounded retirement passed.");
    return 0;
}
