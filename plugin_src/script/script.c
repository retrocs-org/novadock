/* Script tile: shows what an ARexx script returns, without writing C.
 * Every INTERVAL seconds (and on click, verb CLICK) the tile asks RexxMast
 * to run SCRIPT with the arguments "UPDATE <id>" or "CLICK <id>".  The
 * script's RESULT becomes the tile: "value", "caption|value",
 * "caption|value|meter" or "caption|value|meter|badge".  Its default host
 * is NOVADOCK, so it can also send dock commands directly.  The script runs
 * in RexxMast's own process: a slow script never stalls this tile or the
 * dock, and a second run never starts while one is still going. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_script.h"

const char nd_version[]=ND_VER_STRING("Script");
long __stack=8192;
struct RxsLib *RexxSysBase=NULL;

struct ScriptSettings {
    char script[256],caption[ND_SCRIPT_TEXT_LEN];
    unsigned int interval,timeout;  /* ticks */
    int click_runs;
};
struct Run {
    struct MsgPort *reply;
    struct RexxMsg *msg;
    STRPTR marker;
    unsigned int waited;            /* ticks since it started */
    int timed_out;
};

static void read_settings(const char *id,struct ScriptSettings *s)
{
    char interval[16],timeout[16],click[8];
    unsigned long n;
    struct NDOptionString fields[5];
    fields[0].key="SCRIPT";fields[0].fallback="";fields[0].out=s->script;fields[0].limit=sizeof(s->script);
    fields[1].key="CAPTION";fields[1].fallback="";fields[1].out=s->caption;fields[1].limit=sizeof(s->caption);
    fields[2].key="INTERVAL";fields[2].fallback="30";fields[2].out=interval;fields[2].limit=sizeof(interval);
    fields[3].key="TIMEOUT";fields[3].fallback="30";fields[3].out=timeout;fields[3].limit=sizeof(timeout);
    fields[4].key="CLICK";fields[4].fallback="RUN";fields[4].out=click;fields[4].limit=sizeof(click);
    nd_plugin_option_strings(id,fields,5);
    /* Counters are unsigned int (32 bits in the supported vbcc ABI). */
    n=strtoul(interval,NULL,10);if(n<1 || n>86400UL)n=30;
    s->interval=(unsigned int)(n*50UL);
    n=strtoul(timeout,NULL,10);if(n<1 || n>600UL)n=30;
    s->timeout=(unsigned int)(n*50UL);
    s->click_runs=strcmp(click,"NONE")!=0;
}
static void free_run(struct Run *r)
{
    if (r->msg) {
        if (r->msg->rm_Args[0]) DeleteArgstring(r->msg->rm_Args[0]);
        DeleteRexxMsg(r->msg);
        r->msg=NULL;
    }
    if (r->marker) {DeleteArgstring(r->marker);r->marker=NULL;}
}
/* Returns NULL when the script was handed to RexxMast, else tile text. */
static const char *start_run(struct Run *r,const struct ScriptSettings *s,
                             const char *verb,const char *id)
{
    char command[300];
    struct MsgPort *rexx;
    if (!s->script[0]) return "NO SCRIPT";
    if (!nd_script_command(command,sizeof(command),s->script,verb,id))
        return "BAD SCRIPT";
    r->msg=CreateRexxMsg(r->reply,(STRPTR)"rexx",(STRPTR)"NOVADOCK");
    if (!r->msg) return "NO MEMORY";
    /* Some rexxsyslib versions leave the node name empty; RexxMast wants
     * a name on messages it replies to. */
    if (!r->msg->rm_Node.mn_Node.ln_Name) {
        r->marker=CreateArgstring((STRPTR)"REXX",4);
        r->msg->rm_Node.mn_Node.ln_Name=(char *)r->marker;
    }
    r->msg->rm_Args[0]=CreateArgstring((STRPTR)command,(ULONG)strlen(command));
    r->msg->rm_Action=RXCOMM|RXFF_RESULT;
    if (!r->msg->rm_Args[0]) {free_run(r);return "NO MEMORY";}
    Forbid();
    rexx=FindPort((STRPTR)"REXX");
    if (rexx) PutMsg(rexx,(struct Message *)r->msg);
    Permit();
    if (!rexx) {free_run(r);return "NO AREXX";}
    r->waited=0;r->timed_out=0;
    return NULL;
}
/* The script finished: turn its reply into tile text. */
static void finish_run(struct Run *r,const struct ScriptSettings *s,
                       char *caption,char *value,unsigned int *meter,
                       char *badge,int *badge_set)
{
    struct NDScriptResult result;
    LONG rc=r->msg->rm_Result1;
    STRPTR text=(STRPTR)r->msg->rm_Result2;
    if (rc==0 && text) {
        if (nd_script_parse((const char *)text,&result)) {
            strcpy(caption,result.has_caption ? result.caption :
                           (s->caption[0] ? s->caption : "SCRIPT"));
            strcpy(value,result.value);
            *meter=result.has_meter ? result.meter : ND_NO_METER;
            if (result.has_badge) {strcpy(badge,result.badge);*badge_set=1;}
        } else strcpy(value,"BAD RESULT");
        DeleteArgstring(text);
    } else if (rc!=0) {
        sprintf(value,"RC %ld",(long)rc);
        *meter=ND_NO_METER;
    }
    /* RC 0 without a RESULT keeps the previous text: the script may only
     * have sent dock commands. */
    free_run(r);
}
int main(int argc,char **argv)
{
    struct NDClient client;
    struct ScriptSettings settings;
    struct Run run;
    char caption[ND_SCRIPT_TEXT_LEN],value[ND_SCRIPT_TEXT_LEN];
    char badge[ND_SCRIPT_BADGE_LEN];
    const char *problem;
    unsigned int event,elapsed=0,since,meter=ND_NO_METER,wait;
    int reread=1,badge_set=0,click_pending=0,run_now=1;
    if (argc<2) return 20;
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);
    memset(&run,0,sizeof(run));
    run.reply=CreateMsgPort();
    if (!RexxSysBase || !run.reply || !nd_client_init(&client,argv[1])) {
        if (run.reply) DeleteMsgPort(run.reply);
        if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
        return 20;
    }
    strcpy(caption,"SCRIPT");strcpy(value,"...");badge[0]='\0';
    since=0;
    for (;;) {
        if (reread) {
            read_settings(argv[1],&settings);
            if (settings.caption[0]) strcpy(caption,settings.caption);
            reread=0;
        }
        /* A finished script? */
        if (run.msg && GetMsg(run.reply))
            finish_run(&run,&settings,caption,value,&meter,badge,&badge_set);
        else if (run.msg) {
            run.waited+=elapsed;
            if (!run.timed_out && run.waited>=settings.timeout) {
                /* It cannot be stopped from here; show it and keep waiting. */
                run.timed_out=1;strcpy(value,"TIMEOUT");meter=ND_NO_METER;
            }
        }
        if (!run.msg && (run_now || click_pending || since>=settings.interval)) {
            problem=start_run(&run,&settings,click_pending ? "CLICK" : "UPDATE",argv[1]);
            if (problem) {strcpy(value,problem);meter=ND_NO_METER;}
            click_pending=0;run_now=0;since=0;
        }
        event=nd_client_update_quiet(&client,caption,value,meter,elapsed,500U);
        if (badge_set) {event|=nd_client_set_badge(&client,badge);badge_set=0;}
        if (event&ND_EVENT_QUIT) break;
        if (event&ND_EVENT_SETTINGS) {reread=1;run_now=1;}
        if ((event&ND_EVENT_CLICK) && settings.click_runs) click_pending=1;
        if (nd_client_events2(&client)&ND_EVENT2_REFRESH) run_now=1;
        /* While a script runs, look for its reply twice a second. */
        wait=nd_script_wait_ticks(run.msg!=NULL,since,settings.interval,
                                  click_pending || run_now);
        if (!run.msg) wait=nd_client_delay_ticks(&client,wait);
        if (click_pending || run_now) wait=1U;
        elapsed=nd_client_wait(&client,wait);
        since=elapsed>=settings.interval || since>=settings.interval-elapsed ?
              settings.interval : since+elapsed;
    }
    if (run.msg) {
        /* RexxMast still owns the message.  Leave an inert port for its
         * reply instead of freeing memory it will write to. */
        Forbid();
        if (GetMsg(run.reply)) {Permit();free_run(&run);DeleteMsgPort(run.reply);}
        else {
            run.reply->mp_Flags=PA_IGNORE;run.reply->mp_SigTask=NULL;
            FreeSignal((LONG)run.reply->mp_SigBit);
            Permit();
        }
    } else DeleteMsgPort(run.reply);
    nd_client_close(&client);
    CloseLibrary((struct Library *)RexxSysBase);
    return 0;
}
