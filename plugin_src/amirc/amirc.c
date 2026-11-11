/* Local AmIRC queries, SHOW and event notifications; no IRC traffic is sent. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/ports.h>
#include <intuition/intuitionbase.h>
#include <proto/intuition.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/wb.h>
#include <proto/rexxsyslib.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <stdio.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_amirc.h"
const char nd_version[]=ND_VER_STRING("AmIRC");
long __stack=16384;
struct RxsLib *RexxSysBase=NULL;
struct Library *WorkbenchBase=NULL;
struct IntuitionBase *IntuitionBase=NULL;
#define QUERY_CONNECTED 1
#define QUERY_AWAY 2
#define QUERY_DETAIL 3
#define COMMAND_SHOW 4
#define REPLY_TIMEOUT 250UL
struct Pending {
    struct MsgPort *reply;
    struct RexxMsg *message;
    ULONG age;
    int kind,obsolete;
};
static int find_amirc(const struct NDAmIRCConfig *cfg,char *name)
{
    struct Node *node;
    name[0]=0;
    Forbid();
    if (strcmp(cfg->port,"AUTO")) {
        if (FindPort((STRPTR)cfg->port)) strcpy(name,cfg->port);
    } else {
        for (node=SysBase->PortList.lh_Head;node->ln_Succ;node=node->ln_Succ)
            if (node->ln_Name && strlen(node->ln_Name)<64 &&
                nd_amirc_port_name(node->ln_Name) &&
                (!name[0] || strcmp(node->ln_Name,name)<0))
                strcpy(name,node->ln_Name);
    }
    Permit();return name[0]!=0;
}
static void free_request(struct Pending *p)
{
    if (!p->message) return;
    if (p->message->rm_Result1==0 && p->message->rm_Result2 &&
        (p->message->rm_Action&RXFF_RESULT))
        DeleteArgstring((STRPTR)p->message->rm_Result2);
    DeleteArgstring(p->message->rm_Args[0]);
    DeleteRexxMsg(p->message);p->message=NULL;p->age=0;
}
static int send_request(struct Pending *p,const char *port,
                         const char *command,int kind,int result)
{
    struct MsgPort *target;int delivered=0;
    if (!RexxSysBase || !p->reply || p->message || !port[0]) return 0;
    p->message=CreateRexxMsg(p->reply,(STRPTR)"rexx",(STRPTR)port);
    if (!p->message) return 0;
    p->message->rm_Args[0]=CreateArgstring((STRPTR)command,strlen(command));
    if (!p->message->rm_Args[0]) {
        DeleteRexxMsg(p->message);p->message=NULL;return 0;
    }
    p->message->rm_Action=RXCOMM|(result ? RXFF_RESULT : 0);
    p->kind=kind;p->age=0;p->obsolete=0;
    Forbid();target=FindPort((STRPTR)port);
    if (target) {PutMsg(target,(struct Message *)p->message);delivered=1;}
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
        nd_amirc_clean(text,size,(const char *)p->message->rm_Result2);
    free_request(p);return kind;
}
/* A timed-out command still belongs to AmIRC. Leave ONE valid reply target
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
static int launch_amirc(const char *program)
{
    BPTR lock,input,output;
    APTR previous;
    struct Process *self=(struct Process *)FindTask(NULL);
    char absolute[512],command[1030];LONG result;int found;
    if (!program[0]) return 0;
    previous=self->pr_WindowPtr;self->pr_WindowPtr=(APTR)-1;
    lock=Lock((STRPTR)program,ACCESS_READ);
    found=lock && NameFromLock(lock,absolute,sizeof(absolute));
    if (lock) UnLock(lock);
    self->pr_WindowPtr=previous;
    if (!found) return 0;
    if (WorkbenchBase && OpenWorkbenchObjectA((STRPTR)absolute,NULL)) return 1;
    if (!nd_amirc_quote(command,sizeof(command),absolute)) return 0;
    input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
    if (!input || !output) {
        if (input) Close(input);if (output) Close(output);return 0;
    }
    result=SystemTags((STRPTR)command,SYS_Input,input,SYS_Output,output,
                      SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
    if (result==-1) {Close(input);Close(output);return 0;}
    return 1;
}
/* AmIRC's event script writes to this plugin-owned port, so the ordinary
 * status publisher cannot overwrite its counter as SETBADGE would. */
static struct MsgPort *open_alerts(void)
{
    struct MsgPort *p=CreateMsgPort();int available;
    if(!p)return NULL;p->mp_Node.ln_Name=(STRPTR)ND_AMIRC_ALERT_PORT;
    Forbid();available=FindPort(ND_AMIRC_ALERT_PORT)==NULL;
    if(available)AddPort(p);Permit();
    if(!available){DeleteMsgPort(p);return NULL;}return p;
}
static void receive_alerts(struct MsgPort *port,const char *host,int enabled,
                           struct NDAmIRCAlerts *alerts)
{
    struct RexxMsg *m;
    if(!port)return;
    while((m=(struct RexxMsg *)GetMsg(port))!=NULL){
        m->rm_Result1=10;m->rm_Result2=0;
        if(IsRexxMsg(m) && (m->rm_Action&RXCODEMASK)==RXCOMM &&
           nd_amirc_alert_command(m->rm_Args[0],host,enabled)){
            nd_amirc_alert_add(alerts);m->rm_Result1=0;
        }
        ReplyMsg((struct Message *)m);
    }
}
static int amirc_active(const char *host)
{
    struct MsgPort *port;struct Task *task=NULL;struct Window *win;ULONG lock;int active=0;
    if(!IntuitionBase || !host[0])return 0;
    Forbid();port=FindPort((STRPTR)host);if(port)task=port->mp_SigTask;Permit();
    if(!task)return 0;
    lock=LockIBase(0);win=IntuitionBase->ActiveWindow;
    if(win && win->UserPort && win->UserPort->mp_SigTask==task)active=1;
    UnlockIBase(lock);return active;
}
int main(int argc,char **argv)
{
    struct NDClient client;
    struct NDAmIRCConfig cfg;
    struct Pending pending;
    struct MsgPort *alert_port=NULL;struct NDAmIRCAlerts alerts;
    char badge[ND_BADGE_LEN];
    char port[64],host[64],detail[64],text[128],caption[32],value[32];
    unsigned int event,elapsed=0,wait_ticks;
    ULONG poll=0,launch_age=0,notice_age=0;
    LONG rc;
    int present,connected=-1,away=0,stage=0,kind;
    int reload=1,quit=0,show_pending=0,starting=0,unresponsive=0;
    const char *notice=NULL,*query;
    if (argc<2 || !nd_client_init(&client,argv[1])) return 20;
    memset(&pending,0,sizeof(pending));host[0]=detail[0]=0;
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);
    WorkbenchBase=OpenLibrary("workbench.library",44);
    if (RexxSysBase) pending.reply=CreateMsgPort();
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    if(RexxSysBase)alert_port=open_alerts();alerts.count=0;
    nd_amirc_defaults(&cfg);
    while (!quit) {
        if (reload) {
            nd_amirc_defaults(&cfg);
            if (nd_plugin_options_effective(argv[1],nd_amirc_apply_option,&cfg)!=1)
                nd_amirc_defaults(&cfg);
            if (pending.message) pending.obsolete=1;
            host[0]=detail[0]=0;connected=-1;away=0;poll=cfg.poll_ticks;
            stage=0;reload=0;show_pending=0;starting=0;
            if(!cfg.alerts)alerts.count=0;
            notice=NULL;notice_age=0;
        }
        present=find_amirc(&cfg,port);
        if (strcmp(host,port)) {
            strcpy(host,port);connected=-1;away=0;detail[0]=0;
            if (pending.message) pending.obsolete=1;
            stage=0;poll=cfg.poll_ticks;unresponsive=0;alerts.count=0;
        }
        receive_alerts(alert_port,host,cfg.alerts,&alerts);
        if(cfg.clear_on_focus && amirc_active(host))alerts.count=0;
        if (pending.message) pending.age+=elapsed;
        if (poll<30000UL) poll+=elapsed;
        if (notice_age>elapsed) notice_age-=elapsed;else {notice_age=0;notice=NULL;}
        kind=take_reply(&pending,&rc,text,sizeof(text));
        if (kind) {
            unresponsive=0;
            if (kind==QUERY_CONNECTED) {
                connected=rc==0 ? 1 : rc==5 ? 0 : -1;
                if (connected<=0) {away=0;detail[0]=0;stage=0;}
                else stage=QUERY_AWAY;
            } else if (kind==QUERY_AWAY) {
                away=rc==5;stage=nd_amirc_display_query(cfg.display) ? QUERY_DETAIL : 0;
            } else if (kind==QUERY_DETAIL) {
                nd_amirc_clean(detail,sizeof(detail),rc==0 ? text : "");stage=0;
            } else if (kind==COMMAND_SHOW) {
                if(rc!=0){notice="SHOW FAILED";notice_age=250;}
                else alerts.count=0;
            }
        }
        if (pending.message && pending.age>=REPLY_TIMEOUT) unresponsive=1;
        if (starting) {
            launch_age+=elapsed;
            if (present) {starting=0;show_pending=1;}
            else if (launch_age>=1500) {starting=0;notice="START FAILED";notice_age=250;}
        }
        if (present && !pending.message && RexxSysBase && pending.reply) {
            if (show_pending) {
                if (send_request(&pending,host,"SHOW",COMMAND_SHOW,0)) show_pending=0;
            } else {
                if (!stage && poll>=cfg.poll_ticks) {stage=QUERY_CONNECTED;poll=0;}
                query=stage==QUERY_CONNECTED ? "ISCONNECTED" :
                      stage==QUERY_AWAY ? "GETAWAYSTATE" :
                      stage==QUERY_DETAIL ? nd_amirc_display_query(cfg.display) : NULL;
                if (query && !send_request(&pending,host,query,stage,stage==QUERY_DETAIL))
                    stage=0;
            }
        }
        nd_amirc_status(caption,sizeof(caption),value,sizeof(value),present,
                        connected,away,cfg.display,detail,unresponsive);
        if (!RexxSysBase || !pending.reply) strcpy(value,"REXX UNAVAILABLE");
        else if (starting) strcpy(value,"STARTING");
        else if (notice) nd_amirc_clean(value,sizeof(value),notice);
        event=nd_client_update_quiet(&client,caption,value,ND_NO_METER,elapsed,500);
        /* Unacknowledged event counts take precedence over the connection
         * dot. Keep them through a disconnect until read or acknowledged. */
        if (!(event&ND_EVENT_QUIT)) {
            nd_amirc_badge(badge,sizeof(badge),present ? alerts.count : 0,
                nd_amirc_connection_indicator(present,connected,unresponsive));
            event|=nd_client_set_badge(&client,badge);
        }
        if (event&ND_EVENT_QUIT) quit=1;
        if (event&ND_EVENT_SETTINGS) reload=1;
        if (!quit && (event&ND_EVENT_CLICK)) {
            if (present) {show_pending=1;poll=cfg.poll_ticks;}
            else if (!starting) {
                if (!cfg.program[0]) {notice="SET PROGRAM";notice_age=250;}
                else if (launch_amirc(cfg.program)) {starting=1;launch_age=0;}
                else {notice="START FAILED";notice_age=250;}
            }
        }
        if (!quit) {
            wait_ticks=pending.message && !unresponsive ? 5 :
                starting || notice ? 25 : nd_client_delay_ticks(&client,cfg.poll_ticks);
            elapsed=nd_client_wait(&client,wait_ticks);
        }
    }
    if(alert_port){
        Forbid();RemPort(alert_port);Permit();
        receive_alerts(alert_port,"",0,&alerts);DeleteMsgPort(alert_port);
    }
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    nd_client_close(&client);
    kind=close_requests(&pending);
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (RexxSysBase && !kind) CloseLibrary((struct Library *)RexxSysBase);
    return 0;
}
