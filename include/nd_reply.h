#ifndef ND_REPLY_H
#define ND_REPLY_H
/* Reply ports have explicit allocation ownership. An abandoned port remains
 * valid until the receiver finishes its message, then both allocations are
 * reclaimed there. Legacy receivers can still ReplyMsg safely. */
#define ND_REPLY_OWNED (-101)
static struct MsgPort *nd_reply_port(void)
{
    struct MsgPort *p;
    LONG bit=AllocSignal(-1L);
    if (bit<0) return NULL;
    p=(struct MsgPort *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) {FreeSignal(bit);return NULL;}
    p->mp_Node.ln_Type=NT_MSGPORT;
    p->mp_Node.ln_Pri=ND_REPLY_OWNED;
    p->mp_Flags=PA_SIGNAL;p->mp_SigBit=(UBYTE)bit;p->mp_SigTask=FindTask(NULL);
    p->mp_MsgList.lh_Head=(struct Node *)&p->mp_MsgList.lh_Tail;
    p->mp_MsgList.lh_TailPred=(struct Node *)&p->mp_MsgList.lh_Head;
    p->mp_MsgList.lh_Type=NT_MESSAGE;
    return p;
}
static void nd_reply_delete(struct MsgPort *p)
{
    if (p) {FreeSignal((LONG)p->mp_SigBit);FreeVec(p);}
}
/* Caller has stopped using the wire packet. Atomic with receiver completion.
 * If a legacy receiver has already replied, ownership has returned here. */
static void nd_reply_abandon(struct MsgPort *p)
{
    struct Message *answer;
    Forbid();
    p->mp_Flags=PA_IGNORE;p->mp_SigTask=NULL;
    FreeSignal((LONG)p->mp_SigBit);
    answer=GetMsg(p);
    if (answer) {FreeVec(answer);FreeVec(p);}
    Permit();
}
static int nd_reply_finish(struct Message *m)
{
    struct MsgPort *p=m->mn_ReplyPort;
    Forbid();
    if (p && p->mp_Node.ln_Pri==ND_REPLY_OWNED &&
        p->mp_Flags==PA_IGNORE && !p->mp_SigTask) {
        FreeVec(m);FreeVec(p);Permit();return 1;
    }
    ReplyMsg(m);Permit();return 0;
}
#endif
