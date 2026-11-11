/* Exec ownership model: normal replies, late replies and sender exit. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef long LONG;typedef unsigned char UBYTE;
#define MEMF_PUBLIC 1
#define MEMF_CLEAR 2
#define NT_MSGPORT 4
#define NT_MESSAGE 5
#define PA_SIGNAL 0
#define PA_IGNORE 2
struct Node {struct Node *ln_Succ,*ln_Pred;UBYTE ln_Type;signed char ln_Pri;};
struct List {struct Node *lh_Head,*lh_Tail,*lh_TailPred;UBYTE lh_Type;};
struct Message;
struct MsgPort {struct Node mp_Node;UBYTE mp_Flags,mp_SigBit;void *mp_SigTask;struct List mp_MsgList;struct Message *queued;};
struct Message {struct MsgPort *mn_ReplyPort;};
static int allocations,signals,forbid;
static LONG AllocSignal(LONG n){(void)n;++signals;return 3;}
static void FreeSignal(LONG n){(void)n;assert(signals>0);--signals;}
static void *AllocVec(size_t n,int flags){void *p=calloc(1,n);(void)flags;if(p)++allocations;return p;}
static void FreeVec(void *p){assert(p && allocations>0);--allocations;free(p);}
static void *FindTask(void *p){(void)p;return (void *)1;}
static void Forbid(void){++forbid;}
static void Permit(void){assert(forbid>0);--forbid;}
static struct Message *GetMsg(struct MsgPort *p){struct Message *m=p->queued;p->queued=NULL;return m;}
static void ReplyMsg(struct Message *m){m->mn_ReplyPort->queued=m;}
#include "nd_reply.h"
int main(void){int i;struct MsgPort *p;struct Message *m;
 p=nd_reply_port();m=AllocVec(sizeof(*m),0);m->mn_ReplyPort=p;
 nd_reply_finish(m);assert(GetMsg(p)==m);FreeVec(m);nd_reply_delete(p);
 assert(!signals && !allocations);
 for(i=0;i<1000;++i){p=nd_reply_port();m=AllocVec(sizeof(*m),0);m->mn_ReplyPort=p;
  nd_reply_abandon(p);assert(!signals && allocations==2);
  nd_reply_finish(m);assert(!allocations && !signals && !forbid);}
 /* Reply racing just before timeout: sender recovers returned allocations. */
 p=nd_reply_port();m=AllocVec(sizeof(*m),0);m->mn_ReplyPort=p;
 nd_reply_finish(m);nd_reply_abandon(p);assert(!allocations && !signals);
 puts("Reply lifecycle: normal, late, raced and 1000 retired exchanges passed.");return 0;}
