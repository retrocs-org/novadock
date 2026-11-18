/* Isolated local protocol test: mock chat client and dock, no server accounts. */
#include <exec/types.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
static FILE *qa_log;
static int qa_puts(const char *s){int n=fprintf(qa_log,"%s\n",s);fflush(qa_log);return n;}
static int qa_printf(const char *fmt,...){int n;va_list ap;va_start(ap,fmt);n=vfprintf(qa_log,fmt,ap);va_end(ap);fflush(qa_log);return n;}
#define puts qa_puts
#define printf qa_printf
#include "nd_protocol.h"
#include "nd_control_client.h"
long __stack=65536;
struct RxsLib *RexxSysBase=NULL;
static char subscriber[64],badge[8],value[32];static int art=-1,shows,subscriptions,goodbye,click,stop;
static unsigned long unread=42;static int epoch=1;
static struct MsgPort *dock,*rocket,*notifications;static struct RexxMsg *held,*note;
static void answer(struct RexxMsg *m)
{
    char text[512];m->rm_Result1=0;m->rm_Result2=0;
    if(!strcmp(m->rm_Args[0],"DOCKSTATUS")){
        sprintf(text,"version=2 epoch=qa-%d seq=1 connected=1 connected_servers=1 servers=1 unread=%lu alerts=0 iconified=0\nserver=1 state=connected unread=%lu",epoch,unread,unread);
        m->rm_Result2=(LONG)CreateArgstring(text,strlen(text));
    }else if(!strncmp(m->rm_Args[0],"SUBSCRIBE ",10)){
        if(sscanf(m->rm_Args[0],"SUBSCRIBE %63s",subscriber)==1)++subscriptions;else m->rm_Result1=10;
        m->rm_Result2=(LONG)CreateArgstring("OK",2);
    }else if(!strcmp(m->rm_Args[0],"SHOW"))++shows;
    else{puts("FAIL: unexpected command (plugin must never send chat text).");m->rm_Result1=10;stop=1;}
    ReplyMsg((struct Message *)m);
}
static void wake(void)
{
    struct MsgPort *p;Forbid();p=FindPort(ND_WAKE_PORT_PREFIX "ROCKETQA");if(p)Signal(p->mp_SigTask,ND_WAKE_SIGNAL);Permit();
}
static void notify(void)
{
    struct MsgPort *p;const char *text="AMIROCKET_EVENT2 qa-1 2 UNREAD \"\" \"\" \"\" dropped=0 truncated=0";
    if(note || !*subscriber)return;
    note=CreateRexxMsg(notifications,"rexx","AMIROCKET");if(!note)return;
    note->rm_Action=RXCOMM;note->rm_Args[0]=CreateArgstring((STRPTR)text,strlen(text));
    Forbid();p=FindPort(subscriber);if(p)PutMsg(p,(struct Message *)note);Permit();
    if(!p){DeleteArgstring(note->rm_Args[0]);DeleteRexxMsg(note);note=NULL;}
}
static void pump(int stall)
{
    struct NDExtMessage *ext;struct NDMessage *m;struct RexxMsg *r;
    while((ext=(struct NDExtMessage *)GetMsg(dock))!=NULL){
        m=&ext->media.base;
        if(m->command==ND_MSG_BADGE){strncpy(badge,m->value,7);badge[7]=0;}
        if(m->command==ND_MSG_UPDATE){strncpy(value,m->value,31);value[31]=0;if(ext->flags&ND_EXT_HAS_ARTWORK)art=ext->reserved;}
        if(m->command==ND_MSG_GOODBYE)goodbye=1;
        m->event=ND_EVENT_HOST_BADGES|(stop ? ND_EVENT_QUIT:0);
        if(click && m->command==ND_MSG_UPDATE){m->event|=ND_EVENT_CLICK;click=0;}
        ext->event2=ND_EVENT2_HOST_ARTWORK|ND_EVENT2_HOST_ARTWORK_META;
        ReplyMsg((struct Message *)m);
    }
    if(rocket)while((r=(struct RexxMsg *)GetMsg(rocket))!=NULL){
        if(stall && !held && !strcmp(r->rm_Args[0],"DOCKSTATUS"))held=r;else answer(r);
    }
    if(note && GetMsg(notifications)){DeleteArgstring(note->rm_Args[0]);DeleteRexxMsg(note);note=NULL;}
}
int main(void)
{
    int phase=0,ticks=0,result=10;BPTR marker,in,out;LONG rc;struct MsgPort *old=NULL;
    marker=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!marker)return 20;UnLock(marker);
    qa_log=fopen("QATests:protocol-result.txt","w");if(!qa_log)return 20;
    puts("START: isolated native AmiRocket test.");
    rc=nd_control_send(ND_CONTROL_QUIT);printf("Initial dock quit result: %ld\n",rc);
    for(ticks=0;ticks<500; ticks+=5){int active;Forbid();active=FindPort(ND_PORT_NAME)!=NULL;Permit();if(!active)break;Delay(5);}
    Forbid();rc=FindPort(ND_PORT_NAME)!=NULL || FindPort("AMIROCKET")!=NULL;Permit();if(rc){puts("FAIL: existing dock/client in disposable guest.");return 20;}
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);if(!RexxSysBase)return 20;
    dock=CreateMsgPort();rocket=CreateMsgPort();notifications=CreateMsgPort();if(!dock || !rocket || !notifications)return 20;
    dock->mp_Node.ln_Name=ND_PORT_NAME;dock->mp_Node.ln_Pri=ND_HOST_ABI;rocket->mp_Node.ln_Name="AMIROCKET";
    Forbid();AddPort(dock);AddPort(rocket);Permit();
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    rc=SystemTags("QATests:AmiRocket ROCKETQA",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
    if(rc==-1){Close(in);Close(out);goto finished;}
    for(ticks=0;ticks<4500 && !goodbye;ticks+=5){
        pump(phase==3);
        if(phase==0 && !strcmp(badge,"42") && art==2 && subscriptions){puts("PASS: subscribed protocol 2, online state and unread badge 42.");unread=0;notify();phase=1;}
        else if(phase==1 && !*badge && art==1){puts("PASS: notification reconciles unread badge to zero without marking chat read.");click=1;wake();phase=2;}
        else if(phase==2 && shows){puts("PASS: tile click sends SHOW, never SEND.");unread=7;notify();phase=3;}
        else if(phase==3 && held && !strcmp(value,"NOT RESPONDING") && !*badge){puts("PASS: stalled client clears stale badge while dock keeps answering.");answer(held);held=NULL;phase=4;}
        else if(phase==4 && !strcmp(badge,"7") && art==2){puts("PASS: late reply restores confirmed unread state.");Forbid();RemPort(rocket);Permit();old=rocket;rocket=NULL;phase=5;}
        else if(phase==5 && !strcmp(value,"NOT RUNNING") && !*badge && art==0){
            rocket=CreateMsgPort();rocket->mp_Node.ln_Name="AMIROCKET";Forbid();AddPort(rocket);Permit();epoch=2;unread=3;subscriber[0]=0;subscriptions=0;phase=6;
        }else if(phase==6 && !strcmp(badge,"3") && art==2 && subscriptions){puts("PASS: client replacement is rediscovered and resubscribed.");stop=1;wake();phase=7;}
        Delay(5);
    }
    if(goodbye && phase==7 && !note){puts("PASS: clean plugin shutdown, no account or network traffic.");result=0;}
    else printf("FAIL: phase=%d ticks=%d badge=%s status=%s\n",phase,ticks,badge,value);
finished:
    stop=1;wake();for(ticks=0;ticks<100 && !goodbye;ticks+=5){pump(0);Delay(5);}
    Forbid();RemPort(dock);if(rocket)RemPort(rocket);Permit();
    DeleteMsgPort(dock);if(rocket)DeleteMsgPort(rocket);if(old)DeleteMsgPort(old);DeleteMsgPort(notifications);CloseLibrary((struct Library *)RexxSysBase);fclose(qa_log);return result;
}
