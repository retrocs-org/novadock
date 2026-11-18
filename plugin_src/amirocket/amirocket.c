/* Standard local ARexx packets; no interpreter scripts or chat sends. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/ports.h>
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
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_amirocket.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(AMIROCKET,"AmiRocket");
long __stack=32768;
struct RxsLib *RexxSysBase=NULL;
struct Library *WorkbenchBase=NULL;
struct Pending {struct MsgPort *reply,*target;struct Task *owner;struct RexxMsg *message;STRPTR marker;ULONG age;int kind,obsolete;char host[64];};
#include "nd_request.h"
#define QUERY 1
#define SUBSCRIBE 2
#define SHOW 3
static struct MsgPort *find_client(const struct NDRocketConfig *cfg,char *name,struct Task **owner)
{
    struct Node *n;struct MsgPort *port=NULL;name[0]=0;*owner=NULL;
    Forbid();
    if(strcmp(cfg->port,"AUTO")){port=FindPort((STRPTR)cfg->port);if(port)strcpy(name,cfg->port);}
    else{
        for(n=SysBase->PortList.lh_Head;n->ln_Succ;n=n->ln_Succ)
            if(n->ln_Name && strlen(n->ln_Name)<64 && nd_rocket_port(n->ln_Name) && (!*name || strcmp(n->ln_Name,name)<0))strcpy(name,n->ln_Name);
        if(*name)port=FindPort((STRPTR)name);
    }
    if(port)*owner=port->mp_SigTask;Permit();return port;
}
static struct MsgPort *open_events(char *name,const char *id)
{
    struct MsgPort *p=CreateMsgPort();int available;if(!p)return NULL;
    sprintf(name,"NOVADOCK.%s.EVENTS",id);p->mp_Node.ln_Name=(STRPTR)name;
    Forbid();available=FindPort(name)==NULL;if(available)AddPort(p);Permit();
    if(!available){DeleteMsgPort(p);return NULL;}return p;
}
static int receive_events(struct MsgPort *port)
{
    struct RexxMsg *m;unsigned int count=0;int dirty=0;if(!port)return 0;
    while(count++<32 && (m=(struct RexxMsg *)GetMsg(port))!=NULL){
        /* Notifications are reconciliation hints only. Their text is never run. */
        if(m->rm_Node.mn_Length==sizeof(*m) && (m->rm_Action&RXCODEMASK)==RXCOMM &&
           m->rm_Args[0] && !strncmp(m->rm_Args[0],"AMIROCKET_EVENT2 ",17))dirty=1;
        m->rm_Result1=0;m->rm_Result2=0;ReplyMsg((struct Message *)m);
    }
    return dirty;
}
static int launch_client(const char *program)
{
    struct Process *self=(struct Process *)FindTask(NULL);APTR previous;
    BPTR lock,input,output;char path[512],command[1030];unsigned int n=0,i;LONG rc;int found;
    if(!*program)return 0;
    previous=self->pr_WindowPtr;self->pr_WindowPtr=(APTR)-1;lock=Lock((STRPTR)program,ACCESS_READ);
    found=lock && NameFromLock(lock,path,sizeof(path));if(lock)UnLock(lock);self->pr_WindowPtr=previous;if(!found)return 0;
    if(WorkbenchBase && OpenWorkbenchObjectA(path,NULL))return 1;
    command[n++]='"';for(i=0;path[i];++i){unsigned char ch=(unsigned char)path[i];if(ch<32 || ch==127 || n+4>=sizeof(command))return 0;if(ch=='*' || ch=='"')command[n++]='*';command[n++]=(char)ch;}command[n++]='"';command[n]=0;
    input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
    if(!input || !output){if(input)Close(input);if(output)Close(output);return 0;}
    rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
    if(rc==-1){Close(input);Close(output);return 0;}return 1;
}
int main(int argc,char **argv)
{
    struct NDClient client;struct NDRocketConfig cfg;struct NDRocketStatus status;
    struct Pending pending;struct RetiredRequest *retired=NULL;
    struct MsgPort *events=NULL,*target,*last_target=NULL;struct Task *owner,*last_owner=NULL;
    char host[64]="",name[64],event_name[64],text[4096],command[128],badge[8],value[32];
    unsigned int event,elapsed=0,art;ULONG poll=500,subscription=1500,start_age=0,notice_age=0;
    LONG rc;int kind,known=0,reload=1,quit=0,show=0,starting=0,dirty=0,unresponsive=0,abandoned;
    const char *notice=NULL;
    if(argc<2 || !nd_client_init(&client,argv[1]))return 20;
    memset(&pending,0,sizeof(pending));memset(&status,0,sizeof(status));nd_rocket_defaults(&cfg);
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);WorkbenchBase=OpenLibrary("workbench.library",44);
    if(RexxSysBase){pending.reply=CreateMsgPort();events=open_events(event_name,argv[1]);}
    while(!quit){
        if(reload){nd_rocket_defaults(&cfg);if(nd_plugin_options_effective(argv[1],nd_rocket_option,&cfg)!=1)nd_rocket_defaults(&cfg);
            if(pending.message)pending.obsolete=1;host[0]=0;known=0;show=0;starting=0;notice=NULL;reload=0;}
        target=find_client(&cfg,name,&owner);
        if(strcmp(host,name) || target!=last_target || owner!=last_owner){
            strcpy(host,name);last_target=target;last_owner=owner;known=0;poll=500;subscription=1500;show=0;dirty=0;notice=NULL;
            if(pending.message)pending.obsolete=1;
        }
        reap_requests(&retired);
        if(pending.message)pending.age+=elapsed;
        if(poll<3000)poll+=elapsed;if(subscription<3000)subscription+=elapsed;
        if(notice_age>elapsed)notice_age-=elapsed;else notice=NULL;
        if(receive_events(events))dirty=1;
        kind=take_reply(&pending,&rc,text,sizeof(text));
        if(kind>0){
            if(kind==QUERY){known=rc==0 && nd_rocket_parse(&status,text);notice=known ? NULL:"UPDATE AMIROCKET";notice_age=500;}
            else if(kind==SUBSCRIBE){subscription=0;}
            else if(rc!=0){notice="SHOW FAILED";notice_age=250;}
        }
        if(recover_request(&pending,&retired)>0){known=0;poll=500;subscription=1500;show=0;}
        unresponsive=pending.message && pending.age>=250;
        if(unresponsive){known=0;show=0;}
        if(starting){start_age+=elapsed;if(target){starting=0;show=1;}else if(start_age>=1500){starting=0;notice="START FAILED";notice_age=250;}}
        if(target && pending.reply && !pending.message){
            if(show){if(send_request(&pending,host,"SHOW",SHOW,0))show=0;}
            else if(poll>=500 || (dirty && poll>=50)){
                if(send_request(&pending,host,"DOCKSTATUS",QUERY,1)){poll=0;dirty=0;}
            }else if(events && subscription>=1500){
                sprintf(command,"SUBSCRIBE %s 2",event_name);
                if(send_request(&pending,host,command,SUBSCRIBE,1))subscription=0;
            }
        }
        art=0;
        if(!RexxSysBase || !pending.reply)strcpy(value,"REXX UNAVAILABLE");
        else if(starting){strcpy(value,"STARTING");art=3;}
        else if(!target)strcpy(value,"NOT RUNNING");
        else if(unresponsive){strcpy(value,"NOT RESPONDING");art=3;}
        else if(notice)strcpy(value,notice);
        else if(!known){strcpy(value,"CHECKING");art=3;}
        else if(!status.connected)strcpy(value,"DISCONNECTED");
        else{sprintf(value,"%lu/%lu ONLINE",status.connected_servers,status.servers);art=status.unread ? 2:1;}
        nd_rocket_badge(badge,status.unread,cfg.badge,known && status.connected);
        event=nd_client_set_badge(&client,badge);
        event|=nd_client_update_art_quiet(&client,"AMIROCKET",value,"Rocket",art,elapsed,250);
        if(event&ND_EVENT_QUIT)quit=1;
        if(event&ND_EVENT_SETTINGS)reload=1;
        if(event&ND_EVENT_CLICK){
            if(target && !unresponsive)show=1;
            else if(!target && !starting){starting=launch_client(cfg.program);start_age=0;if(!starting){notice="SET PROGRAM";notice_age=250;}}
        }
        if(!quit)elapsed=nd_client_wait_signals(&client,nd_client_delay_ticks(&client,25),events ? 1UL<<events->mp_SigBit:0);
    }
    /* Removing the public port makes AmiRocket discard its subscription. */
    if(events){Forbid();RemPort(events);Permit();while(receive_events(events)){}DeleteMsgPort(events);}
    abandoned=close_requests(&pending);abandoned|=close_retired(retired);
    if(WorkbenchBase)CloseLibrary(WorkbenchBase);if(RexxSysBase && !abandoned)CloseLibrary((struct Library *)RexxSysBase);
    nd_client_close(&client);return 0;
}
