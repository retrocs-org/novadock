#ifndef ND_BS_REQUEST_H
#define ND_BS_REQUEST_H
static void free_request(struct Pending *p)
{
    if (!p->message) return;
    if (p->message->rm_Result1==0 && p->message->rm_Result2 &&
        (p->message->rm_Action&RXFF_RESULT))
        DeleteArgstring((STRPTR)p->message->rm_Result2);
    DeleteArgstring(p->message->rm_Args[0]);
    DeleteRexxMsg(p->message);p->message=NULL;p->age=0;
    if(p->marker){DeleteArgstring(p->marker);p->marker=NULL;}
}
static int send_request(struct Pending *p,const char *port,
                         const char *command,int kind,int result)
{
    struct MsgPort *target;int delivered=0;
    if (!RexxSysBase || !p->reply || p->message || !port[0]) return 0;
    p->message=CreateRexxMsg(p->reply,(STRPTR)"rexx",(STRPTR)port);
    if (!p->message) return 0;
    /* Some OS 3.2 rexxsyslib configurations leave the documented REXX
     * message marker unset. Keep a request-owned marker alive through the
     * reply (or abandonment), so IsRexxMsg in the host can recognize it. */
    if(!p->message->rm_Node.mn_Node.ln_Name){
        p->marker=CreateArgstring("REXX",4);
        if(!p->marker){DeleteRexxMsg(p->message);p->message=NULL;return 0;}
        p->message->rm_Node.mn_Node.ln_Name=p->marker;
    }
    p->message->rm_Args[0]=CreateArgstring((STRPTR)command,strlen(command));
    if (!p->message->rm_Args[0]) {
        DeleteRexxMsg(p->message);p->message=NULL;
        if(p->marker){DeleteArgstring(p->marker);p->marker=NULL;}return 0;
    }
    p->message->rm_Action=RXCOMM|(result ? RXFF_RESULT : 0);
    p->kind=kind;p->age=0;p->obsolete=0;
    Forbid();target=FindPort((STRPTR)port);
    if (target) {
        p->target=target;p->owner=target->mp_SigTask;strcpy(p->host,port);
        PutMsg(target,(struct Message *)p->message);delivered=1;
    }
    Permit();
    if (!delivered) {p->message->rm_Result1=20;free_request(p);}
    return delivered;
}
static int take_reply(struct Pending *p,LONG *rc,char *text,unsigned int size)
{
    struct Message *reply;int kind;
    if (!p->message || !(reply=GetMsg(p->reply))) return 0;
    if (reply!=(struct Message *)p->message) return 0;
    kind=p->obsolete ? -1 : p->kind;
    *rc=p->message->rm_Result1;text[0]=0;
    if (*rc==0 && p->message->rm_Result2 &&
        (p->message->rm_Action&RXFF_RESULT))
        {strncpy(text,(const char *)p->message->rm_Result2,size-1);text[size-1]=0;}
    free_request(p);return kind;
}
/* A timed-out command still belongs to BitSkim. Leave ONE valid reply target
 * if it never comes back, as nd_client does for a lost host. Never free a
 * live RexxMsg or leave a signal addressed to this exiting process. */
static int close_requests(struct Pending *p)
{
    struct Message *reply;int abandoned=0;
    Forbid();
    reply=p->message ? GetMsg(p->reply) : NULL;
    if (p->message && !reply) {
        p->reply->mp_Flags=PA_IGNORE;p->reply->mp_SigTask=NULL;
        FreeSignal(p->reply->mp_SigBit);abandoned=1;
    }
    Permit();
    if (!abandoned) {free_request(p);if (p->reply) DeleteMsgPort(p->reply);}
    return abandoned;
}

#define BS_RETIRED_MAX 4
struct RetiredRequest {struct RetiredRequest *next;struct Pending pending;};
/* A timeout alone is not evidence of a dead host: modal save requesters can
 * wait indefinitely. Rotate only when the registered destination disappears
 * or changes owner. Never free a message still owned by that destination. */
static int request_target_changed(struct Pending *p)
{
    struct MsgPort *target;int changed;
    if(!p->message || !p->target)return 0;
    Forbid();target=FindPort((STRPTR)p->host);
    changed=!target || target!=p->target || target->mp_SigTask!=p->owner;
    Permit();return changed;
}
static int recover_request(struct Pending *p,struct RetiredRequest **retired)
{
    struct RetiredRequest *node,*walk;struct MsgPort *fresh;unsigned int n=0;
    if(!request_target_changed(p))return 0;
    for(walk=*retired;walk;walk=walk->next)++n;
    if(n>=BS_RETIRED_MAX)return -1;
    node=AllocVec(sizeof(*node),MEMF_PUBLIC|MEMF_CLEAR);
    if(!node)return -1;
    fresh=CreateMsgPort();
    if(!fresh){FreeVec(node);return -1;}
    node->pending=*p;node->pending.obsolete=1;
    node->next=*retired;*retired=node;
    memset(p,0,sizeof(*p));p->reply=fresh;
    return 1;
}
static void reap_requests(struct RetiredRequest **retired)
{
    struct RetiredRequest **link=retired,*node;LONG rc;char ignored[2];
    while((node=*link)!=NULL){
        if(take_reply(&node->pending,&rc,ignored,sizeof(ignored))){
            DeleteMsgPort(node->pending.reply);*link=node->next;FreeVec(node);
        }else link=&node->next;
    }
}
static int close_retired(struct RetiredRequest *retired)
{
    struct RetiredRequest *next;int abandoned=0;
    while(retired){next=retired->next;abandoned|=close_requests(&retired->pending);
        FreeVec(retired);retired=next;}
    return abandoned;
}

#endif
