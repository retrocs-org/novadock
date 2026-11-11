/* Optional BitSkim bridge. Uses local ARexx only; BitSkim owns networking. */
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
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_bitskim.h"
const char nd_version[]=ND_VER_STRING("BitSkim");
long __stack=16384;
struct RxsLib *RexxSysBase=NULL;
struct Library *WorkbenchBase=NULL;
#define QUERY_STATUS 1
#define DO_COMMAND 2
#define QUERY_CAPS 3
#define QUERY_EVENTS 4
#define QUERY_LAST 5
#define QUERY_ERROR 6
#define REPLY_TIMEOUT 250UL
struct Pending {struct MsgPort *reply,*target;struct Task *owner;
    struct RexxMsg *message;STRPTR marker;ULONG age;int kind,obsolete;char host[64];};
#include "nd_bs_request.h"
static int find_bitskim(const struct NDBitSkimConfig *cfg,char *name)
{
    struct Node *node;name[0]=0;Forbid();
    if(strcmp(cfg->port,"AUTO")){if(FindPort((STRPTR)cfg->port))strcpy(name,cfg->port);}
    else for(node=SysBase->PortList.lh_Head;node->ln_Succ;node=node->ln_Succ)
        if(node->ln_Name && nd_bitskim_port(node->ln_Name) && (!name[0] || strcmp(node->ln_Name,name)<0))strcpy(name,node->ln_Name);
    Permit();return name[0]!=0;
}
static int quote_program(char *out,unsigned int size,const char *path)
{
    unsigned int n=0;unsigned char ch;if(size<3)return 0;out[n++]='"';
    while((ch=(unsigned char)*path++)!=0){if(ch<32 || ch==127 || n+4>=size)return 0;if(ch=='"' || ch=='*')out[n++]='*';out[n++]=(char)ch;}
    out[n++]='"';out[n]=0;return 1;
}
static int launch_bitskim(const char *program)
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
    if (!quote_program(command,sizeof(command),absolute)) return 0;
    input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
    if (!input || !output) {
        if (input) Close(input);if (output) Close(output);return 0;
    }
    result=SystemTags((STRPTR)command,SYS_Input,input,SYS_Output,output,
                      SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
    if (result==-1) {Close(input);Close(output);return 0;}
    return 1;
}

#include "nd_bitskim_ui.h"
int main(int argc,char **argv)
{
    struct NDClient client;struct NDBitSkimConfig cfg;struct NDBitSkimStatus status;
    struct RetiredRequest *retired=NULL;
    struct Pending pending;struct NDBitSkimTracker tracker;struct NDBitSkimTransfer transfer;
    char host[64]="",port[64],text[4096],value[32],badge[8],query[64],error[256],encoded[770],format[16];
    unsigned long capability;int extensions=0,stage=QUERY_CAPS;
    unsigned int event,elapsed=0,wait_ticks,meter;ULONG poll=0,start_age=0,notice_age=0;
    unsigned int actions=0,selected=0;LONG rc;int reload=1,quit=0,known=0,present,kind,starting=0,unresponsive=0,abandoned;
    const char *command=NULL,*notice=NULL;
    if(argc>=3 && (!strcmp(argv[2],"SEND") || !strcmp(argv[2],"DETAILS")))return bitskim_helper(argc,argv);
    if(argc<2 || !nd_client_init(&client,argv[1]))return 20;
    memset(&pending,0,sizeof(pending));memset(&status,0,sizeof(status));memset(&tracker,0,sizeof(tracker));
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);
    WorkbenchBase=OpenLibrary("workbench.library",44);
    if(RexxSysBase)pending.reply=CreateMsgPort();nd_bitskim_defaults(&cfg);
    while(!quit){
        if(reload){
            nd_bitskim_defaults(&cfg);if(nd_plugin_options_effective(argv[1],nd_bitskim_option,&cfg)!=1)nd_bitskim_defaults(&cfg);
            if(pending.message)pending.obsolete=1;
            known=0;poll=cfg.poll_ticks;reload=0;notice=NULL;notice_age=0;stage=QUERY_CAPS;extensions=0;
        }
        present=find_bitskim(&cfg,port);
        if(strcmp(host,port)){
            strcpy(host,port);known=0;poll=cfg.poll_ticks;unresponsive=0;extensions=0;stage=QUERY_CAPS;memset(&tracker,0,sizeof(tracker));
            if(pending.message)pending.obsolete=1;
        }
        if(pending.message)pending.age+=elapsed;if(poll<30000UL)poll+=elapsed;
        if(notice_age>elapsed)notice_age-=elapsed;else {notice=NULL;notice_age=0;}
        reap_requests(&retired);
        kind=take_reply(&pending,&rc,text,sizeof(text));
        if(kind>0){
            unresponsive=0;
            if(kind==QUERY_CAPS){
                extensions=rc==0 && nd_bitskim_number(text,"AREXX",&capability) && capability>=2 &&
                    nd_bitskim_field(text,"FORMAT",format,sizeof(format)) && !strcmp(format,"PCTBYTES") &&
                    nd_bitskim_number(text,"LAST",&capability) && capability==1 &&
                    nd_bitskim_number(text,"EVENTDETAILS",&capability) && capability==1;
                stage=0;poll=cfg.poll_ticks;
            }else if(kind==QUERY_STATUS){
                known=rc==0 && nd_bitskim_parse(&status,text);
                if(!known){notice="STATUS FAILED";notice_age=250;}
                else {
                    if(notice && !strcmp(notice,"STATUS FAILED")){notice=NULL;notice_age=0;}
                    if(extensions)stage=QUERY_EVENTS;
                }
            }else if(kind==QUERY_EVENTS){
                kind=rc==0 ? nd_bitskim_events(&tracker,text):-1;
                if(kind>0)stage=QUERY_LAST;
                else if(kind<0 && rc==10)extensions=0;
            }else if(kind==QUERY_LAST){
                if(rc==0 && nd_bitskim_transfer(&transfer,text))nd_bitskim_track(&tracker,&transfer);
            }else if(kind==QUERY_ERROR){
                if(rc==0 && nd_bitskim_field(text,"TEXT",encoded,sizeof(encoded)) && nd_bitskim_decode(encoded,error,sizeof(error))){
                    nd_bitskim_local(value,sizeof(value),error);strcpy(error,value);notice=error;notice_age=250;
                }
            }else if(rc!=0){notice="ACTION FAILED";notice_age=250;if(extensions)stage=QUERY_ERROR;}else poll=cfg.poll_ticks;
        }
        if(recover_request(&pending,&retired)>0){
            known=0;unresponsive=0;extensions=0;stage=QUERY_CAPS;poll=cfg.poll_ticks;
            notice=NULL;notice_age=0;memset(&tracker,0,sizeof(tracker));
        }
        if(pending.message && pending.age>=REPLY_TIMEOUT)unresponsive=1;
        if(starting){start_age+=elapsed;if(present){starting=0;actions|=ND_EVENT_CLICK;}else if(start_age>=1500){starting=0;notice="START FAILED";notice_age=250;}}
        if(present && RexxSysBase && pending.reply && !pending.message){
            selected=0;command=NULL;
            if(actions&ND_EVENT_CLICK){selected=ND_EVENT_CLICK;command="SHOW";}
            else if(actions&ND_EVENT_BITSKIM_PREFS){selected=ND_EVENT_BITSKIM_PREFS;command="PREFS";}
            else if(actions&ND_EVENT_BITSKIM_CANCEL){selected=ND_EVENT_BITSKIM_CANCEL;command="CANCEL";}
            else if((actions&ND_EVENT_BITSKIM_RECEIVING) && known){selected=ND_EVENT_BITSKIM_RECEIVING;command=status.receiving ? "RECEIVING OFF":"RECEIVING ON";}
            else if(actions&ND_EVENT_BITSKIM_REFRESH){selected=ND_EVENT_BITSKIM_REFRESH;command="REFRESH";}
            if(command){if(send_request(&pending,host,command,DO_COMMAND,0))actions&=~selected;}
            else if(stage){
                command=stage==QUERY_CAPS ? "CAPABILITIES":stage==QUERY_LAST ? "LAST":stage==QUERY_ERROR ? "ERROR":NULL;
                if(stage==QUERY_EVENTS){sprintf(query,"EVENTDETAILS %lu",tracker.cursor);command=query;}
                if(command && send_request(&pending,host,command,stage,1))stage=0;
            }else if(poll>=cfg.poll_ticks || (actions&ND_EVENT_BITSKIM_RECEIVING)){
                if(send_request(&pending,host,"STATUS",QUERY_STATUS,1))poll=0;
            }
        }
        meter=ND_NO_METER;badge[0]=0;
        if(!RexxSysBase || !pending.reply)strcpy(value,"REXX UNAVAILABLE");
        else if(starting)strcpy(value,"STARTING");
        else if(!present)strcpy(value,"OFFLINE");
        /* BitSkim's modal accept/save requesters pause its ARexx loop.
         * A late reply means waiting, not a failed transfer. Keep the one
         * outstanding request alive and resume normally when it returns. */
        else if(unresponsive)strcpy(value,"WAIT BITSKIM");
        else if(!known)strcpy(value,"CHECKING");
        else nd_bitskim_display(&status,value,&meter,badge,cfg.badge);
        if(present && known && !unresponsive && !status.active && tracker.unread){
            strcpy(value,nd_bitskim_result(tracker.last.result,tracker.last.incoming));
            if(cfg.badge)strcpy(badge,tracker.last.result==1 ? "1":"!");
        }
        if(notice){strcpy(value,notice);meter=ND_NO_METER;}
        if(unresponsive || !present)badge[0]=0;
        event=nd_client_update_quiet(&client,"BITSKIM",value,meter,elapsed,500);
        if(!(event&ND_EVENT_QUIT))event|=nd_client_set_badge(&client,badge);
        if(event&ND_EVENT_QUIT)quit=1;if(event&ND_EVENT_SETTINGS)reload=1;
        if(!quit && (event&(ND_EVENT_CLICK|ND_EVENT_BITSKIM_ACTIONS))){
            if(present){actions|=event&(ND_EVENT_CLICK|ND_EVENT_BITSKIM_ACTIONS);if(event&ND_EVENT_CLICK)tracker.unread=0;}
            else if(event&ND_EVENT_CLICK){
                if(!starting){if(launch_bitskim(cfg.program)){starting=1;start_age=0;}else {notice=cfg.program[0] ? "START FAILED":"SET PROGRAM";notice_age=250;}}
            }else {notice="OPEN BITSKIM";notice_age=250;}
        }
        if(!present && !starting)actions=0;
        if(!quit){wait_ticks=pending.message && !unresponsive ? 5:starting || notice ? 25:nd_client_delay_ticks(&client,cfg.poll_ticks);elapsed=nd_client_wait(&client,wait_ticks);}
    }
    nd_client_close(&client);abandoned=close_requests(&pending);
    abandoned|=close_retired(retired);
    if(WorkbenchBase)CloseLibrary(WorkbenchBase);
    if(RexxSysBase && !abandoned)CloseLibrary((struct Library *)RexxSysBase);return 0;
}
