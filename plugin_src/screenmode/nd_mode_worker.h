#ifndef ND_MODE_WORKER_H
#define ND_MODE_WORKER_H
#define MODE_GUARD "NovaDock.ScreenMode.Switch"
#define MODE_MAGIC 0x4E444D53UL
struct ModeMessage {struct Message msg;ULONG magic;int kind;LONG result;char command[600];};
static ULONG mode_ticks(void)
{
    struct DateStamp d;DateStamp(&d);
    return ((ULONG)d.ds_Days*1440UL+(ULONG)d.ds_Minute)*3000UL+(ULONG)d.ds_Tick;
}
static int mode_send(const char *name,struct ModeMessage *m)
{
    struct MsgPort *port;int sent=0;
    Forbid();port=FindPort((STRPTR)name);
    if(port){PutMsg(port,(struct Message *)m);sent=1;}
    Permit();return sent;
}
/* A separately loaded executable owns the possibly blocking editor command.
 * Only a copied command crosses the handshake. The resident may time out or
 * quit without freeing worker memory or leaving a dangling reply target. */
static int mode_worker(const char *parent)
{
    struct MsgPort *guard=CreateMsgPort(),*reply=CreateMsgPort();
    struct ModeMessage *m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);
    BPTR input,output;int owner=0,replied=0,i;
    if(!guard || !reply || !m){
        if(guard)DeleteMsgPort(guard);if(reply)DeleteMsgPort(reply);
        if(m)FreeVec(m);return 20;
    }
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    Forbid();
    if(!FindPort((STRPTR)MODE_GUARD)){
        guard->mp_Node.ln_Name=(STRPTR)MODE_GUARD;AddPort(guard);owner=1;
    }
    Permit();
    if(!owner){DeleteMsgPort(guard);DeleteMsgPort(reply);FreeVec(m);return 10;}
    m->magic=MODE_MAGIC;m->kind=1;m->msg.mn_Length=sizeof(*m);
    m->msg.mn_ReplyPort=reply;
    if(mode_send(parent,m)){
        for(i=0;i<150 && !replied;i+=2){
            if(GetMsg(reply))replied=1;else Delay(2);
        }
        Forbid();
        if(!replied && GetMsg(reply))replied=1;
        if(!replied){
            reply->mp_Flags=PA_IGNORE;reply->mp_SigTask=NULL;
            FreeSignal(reply->mp_SigBit);
        }
        Permit();
        if(!replied){
            /* Parent might still own the handshake; retain its reply target. */
            Forbid();RemPort(guard);Permit();DeleteMsgPort(guard);return 20;
        }
        DeleteMsgPort(reply);reply=NULL;
        if(m->kind==1 && m->command[0]){
            input=Open((STRPTR)"NIL:",MODE_OLDFILE);
            output=Open((STRPTR)"NIL:",MODE_NEWFILE);m->result=-1;
            if(input && output)m->result=SystemTags((STRPTR)m->command,
                SYS_Input,input,SYS_Output,output,NP_StackSize,16384UL,TAG_DONE);
            if(input)Close(input);if(output)Close(output);
            m->kind=2;m->msg.mn_ReplyPort=NULL;
            if(mode_send(parent,m))m=NULL;
        }
    }
    if(reply)DeleteMsgPort(reply);
    if(m)FreeVec(m);
    Forbid();RemPort(guard);Permit();DeleteMsgPort(guard);return 0;
}
static int start_mode_worker(const char *program,const char *parent)
{
    char command[700];BPTR input,output;LONG rc;size_t n;
    if(!nd_quote_path(command,sizeof(command)-100,program))return 0;
    n=strlen(command);sprintf(command+n," WORK %s",parent);
    input=Open((STRPTR)"NIL:",MODE_OLDFILE);
    output=Open((STRPTR)"NIL:",MODE_NEWFILE);
    if(!input || !output){if(input)Close(input);if(output)Close(output);return 0;}
    rc=SystemTags((STRPTR)command,SYS_Input,input,SYS_Output,output,
                  SYS_Asynch,TRUE,NP_StackSize,16384UL,TAG_DONE);
    if(rc==-1){Close(input);Close(output);return 0;}
    return 1; /* child owns its asynchronous streams */
}
/* Remove the rendezvous before draining it, so a late worker either owns
 * its result or hands it to this loop, never a deleted port. */
static void close_mode_port(struct MsgPort *port)
{
    struct ModeMessage *m;
    Forbid();RemPort(port);
    while((m=(struct ModeMessage *)GetMsg(port))!=NULL){
        if(m->kind==1 && m->msg.mn_ReplyPort){m->kind=-1;ReplyMsg((struct Message *)m);}
        else FreeVec(m);
    }
    Permit();DeleteMsgPort(port);
}
#endif
