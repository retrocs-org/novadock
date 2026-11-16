#ifndef ND_FETCH_WORKER_H
#define ND_FETCH_WORKER_H
#define FETCH_MAGIC 0x4e445746UL
#ifndef FETCH_GUARD
#define FETCH_GUARD "NovaDock.Weather.Fetch"
#endif
#ifndef FETCH_PREFIX
#define FETCH_PREFIX "T:NovaDockWeather-"
#endif
#ifndef FETCH_DIR_VALID
#define FETCH_DIR_VALID(path) (!strncmp((path),FETCH_PREFIX,sizeof(FETCH_PREFIX)-1))
#endif
#ifndef FETCH_STACK_SIZE
#define FETCH_STACK_SIZE 131072UL
#endif
struct FetchMessage {struct Message msg;ULONG magic;int kind;LONG result;char command[1800],directory[160];};
static ULONG weather_ticks(void)
{struct DateStamp d;DateStamp(&d);return ((ULONG)d.ds_Days*1440UL+(ULONG)d.ds_Minute)*3000UL+(ULONG)d.ds_Tick;}
static int fetch_send(const char *name,struct FetchMessage *m)
{struct MsgPort *p;int sent=0;Forbid();p=FindPort((STRPTR)name);if(p){PutMsg(p,(struct Message *)m);sent=1;}Permit();return sent;}
static void fetch_clean(const char *directory)
{
    char path[200];if(!directory || !FETCH_DIR_VALID(directory) || strlen(directory)>150)return;
    sprintf(path,"%s/response",directory);DeleteFile(path);sprintf(path,"%s/fetch.log",directory);DeleteFile(path);
#ifdef FETCH_PROGRESS_FILE
    sprintf(path,"%s/%s",directory,FETCH_PROGRESS_FILE);DeleteFile(path);
#endif
    DeleteFile((STRPTR)directory);
}
static void fetch_close_port(struct MsgPort *port)
{
    struct FetchMessage *m;if(!port)return;
    Forbid();RemPort(port);Permit();
    while((m=(struct FetchMessage *)GetMsg(port))!=NULL){
        if(m->kind==1 && m->msg.mn_ReplyPort){m->kind=-1;ReplyMsg((struct Message *)m);}
        else{fetch_clean(m->directory);FreeVec(m);}
    }
    DeleteMsgPort(port);
}
/* A separately loaded process owns the downloader and its unique directory.
 * If its resident quits or times out, it finishes and cleans up by itself. */
static int fetch_worker(const char *parent)
{
    struct MsgPort *guard=CreateMsgPort(),*reply=CreateMsgPort();
    struct FetchMessage *m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);
    BPTR input,output;char log[200];int owner=0,replied=0,i;
    if(!guard || !reply || !m){if(guard)DeleteMsgPort(guard);if(reply)DeleteMsgPort(reply);if(m)FreeVec(m);return 20;}
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    Forbid();if(!FindPort(FETCH_GUARD)){guard->mp_Node.ln_Name=FETCH_GUARD;AddPort(guard);owner=1;}Permit();
    if(!owner){DeleteMsgPort(guard);DeleteMsgPort(reply);FreeVec(m);return 10;}
    m->magic=FETCH_MAGIC;m->kind=1;m->msg.mn_Length=sizeof(*m);m->msg.mn_ReplyPort=reply;
    if(fetch_send(parent,m)){
        for(i=0;i<150 && !replied;i+=2){if(GetMsg(reply))replied=1;else Delay(2);}
        Forbid();if(!replied && GetMsg(reply))replied=1;
        if(!replied){reply->mp_Flags=PA_IGNORE;reply->mp_SigTask=NULL;FreeSignal(reply->mp_SigBit);}Permit();
        if(!replied){Forbid();RemPort(guard);Permit();DeleteMsgPort(guard);return 20;}
        DeleteMsgPort(reply);reply=NULL;
        if(m->kind==1 && m->command[0]){
            sprintf(log,"%s/fetch.log",m->directory);input=Open("NIL:",MODE_OLDFILE);output=Open(log,MODE_NEWFILE);
            m->result=-1;
            if(input && output)m->result=SystemTags(m->command,SYS_Input,input,SYS_Output,output,NP_Error,output,NP_CloseError,FALSE,NP_Priority,-1L,NP_StackSize,FETCH_STACK_SIZE,TAG_DONE);
            if(input)Close(input);if(output)Close(output);
            Forbid();RemPort(guard);Permit();DeleteMsgPort(guard);guard=NULL;
            m->kind=2;m->msg.mn_ReplyPort=NULL;
            if(fetch_send(parent,m))m=NULL;else fetch_clean(m->directory);
        }
    }
    if(reply)DeleteMsgPort(reply);if(m)FreeVec(m);
    if(guard){Forbid();RemPort(guard);Permit();DeleteMsgPort(guard);}return 0;
}
#endif
