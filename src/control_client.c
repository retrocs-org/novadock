/* Synchronous command sender, used by NovaDockPrefs.
 * No fixed port pointer is cached across Forbid/Permit boundaries. */
#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "nd_control.h"
#include "nd_config.h"
#include "nd_protocol.h"
#include "nd_reply.h"
static void stop_timer(struct IORequest *request)
{
    if (!request) return;
    if (!CheckIO(request)) AbortIO(request);
    WaitIO(request);
}
/* Keep the inert target valid until the dock returns ownership. */
static void abandon_port(struct MsgPort *port)
{
    /* PA_IGNORE ports are reclaimed by nd_reply_finish in the dock. */
    nd_reply_abandon(port);
}
static LONG control_exchange(APTR packet,ULONG size,ULONG timeout_seconds)
{
    struct NDControlMessage *control;
    struct MsgPort *reply,*timer_port,*host;
    struct IORequest *timer_request;
    struct timerequest *timer;
    struct Message *answer;
    APTR wire;
    ULONG signals,reply_signal,timer_signal;
    LONG result=ND_CONTROL_NOMEM;
    int opened=0,expired=0;
    if (!packet || size<sizeof(struct NDControlMessage))
        return ND_CONTROL_INVALID;
    wire=AllocVec(size,MEMF_PUBLIC|MEMF_CLEAR);
    if (!wire) return ND_CONTROL_NOMEM;
    memcpy(wire,packet,size);
    control=(struct NDControlMessage *)wire;
    reply=nd_reply_port();
    timer_port=CreateMsgPort();
    timer_request=NULL;
    if (!reply || !timer_port) goto cleanup;
    timer_request=CreateIORequest(timer_port,sizeof(struct timerequest));
    if (!timer_request ||
       OpenDevice(TIMERNAME,UNIT_VBLANK,timer_request,0)!=0) goto cleanup;
    opened=1;
    control->msg.mn_ReplyPort=reply;
    control->msg.mn_Length=(UWORD)size;
    timer=(struct timerequest *)timer_request;
    timer->tr_node.io_Command=TR_ADDREQUEST;
    timer->tr_time.tv_secs=timeout_seconds;
    timer->tr_time.tv_micro=0;
    SendIO(timer_request);
    Forbid();
    host = FindPort(ND_CONTROL_PORT);
    if (host) PutMsg(host,(struct Message *)wire);
    Permit();
    if (!host) {
        stop_timer(timer_request);result=ND_CONTROL_NOHOST;goto cleanup;
    }
    reply_signal=1UL<<reply->mp_SigBit;
    timer_signal=1UL<<timer_port->mp_SigBit;
    answer=NULL;
    while (!answer && !expired) {
        signals=Wait(reply_signal|timer_signal);
        Forbid();
        answer=GetMsg(reply);
        if (!answer && ((signals&timer_signal) || CheckIO(timer_request))) {
            /* The dock may still own WIRE. Keep both it and the reply target
             * valid, but prevent a late ReplyMsg from signalling this task. */
            expired=1;
        }
        Permit();
    }
    stop_timer(timer_request);
    if (expired || answer!=(struct Message *)wire) {
        abandon_port(reply);
        reply=NULL;wire=NULL;result=ND_CONTROL_TIMEOUT;goto cleanup;
    }
    memcpy(packet,wire,size);
    result=((struct NDControlMessage *)packet)->result;
cleanup:
    if (opened) CloseDevice(timer_request);
    if (timer_request) DeleteIORequest(timer_request);
    if (timer_port) DeleteMsgPort(timer_port);
    if (reply) nd_reply_delete(reply);
    if (wire) FreeVec(wire);
    return result;
}
static LONG control_send_with_timeout(UWORD action,ULONG timeout_seconds)
{
    struct NDControlMessage pkt;
    memset(&pkt,0,sizeof(pkt));
    pkt.magic=ND_CONTROL_MAGIC;
    pkt.version=ND_CONTROL_VERSION;
    pkt.action=action;
    pkt.result=ND_CONTROL_NOHOST;
    return control_exchange(&pkt,sizeof(pkt),timeout_seconds);
}
LONG nd_control_send(UWORD action)
{
    return control_send_with_timeout(action,ND_CONTROL_TIMEOUT_SECONDS);
}
static int prepare_settings(UWORD action)
{
    LONG rc;int n;
    for(n=0;n<300;++n){
        rc=nd_control_send(action);
        if(rc==ND_CONTROL_OK || rc==ND_CONTROL_NOHOST || rc==ND_CONTROL_INVALID)return 1;
        if(rc!=ND_CONTROL_PENDING)return 0;
        Delay(5);
    }
    return 0;
}
int nd_control_sync_settings(void)
{
    return prepare_settings(ND_CONTROL_SAVE_FLUSH);
}
int nd_control_write_config(const struct NDConfig *cfg,const char *path)
{
    int live=path && (!strcmp(path,"ENV:NovaDock/novadock.cfg") ||
                     !strcmp(path,"ENVARC:NovaDock/novadock.cfg"));
    int ok;
    if(live && !prepare_settings(ND_CONTROL_SAVE_LOCK)){
        (void)nd_control_send(ND_CONTROL_SAVE_UNLOCK);return 0;
    }
    ok=nd_write_config(cfg,path);
    if(!ok && live)(void)nd_control_send(ND_CONTROL_SAVE_UNLOCK);
    return ok;
}
static int ports_match(int running)
{
    int control,plugins;
    Forbid();
    control=FindPort(ND_CONTROL_PORT)!=NULL;
    plugins=FindPort(ND_PORT_NAME)!=NULL;
    Permit();
    return running ? (control && plugins) : (!control && !plugins);
}
static int wait_for_ports(int running,int ticks)
{
    while (ticks>0) {
        if (ports_match(running)) return 1;
        Delay(2);ticks-=2;
    }
    return ports_match(running);
}
/* The one way a NovaDock component starts the dock (Preferences' Start and
 * Restart, and the reload fallback below): the dock beside this program
 * (PROGDIR:NovaDock, for development and drawer installs) before the
 * standard install locations, files only, resolved to a real path because
 * PROGDIR: means nothing to the Shell that runs it.  Probes run with
 * requesters quietened, and the dock gets the 32 KB stack it needs (the
 * Shell default is 4 KB). */
int nd_control_launch_dock(void)
{
    static const char *paths[]={
        "PROGDIR:NovaDock","NovaDockApp:NovaDock","SYS:Utilities/NovaDock","NovaDock:NovaDock","SYS:Utilities/NovaDock/NovaDock",
        "Bin:NovaDock/NovaDock","C:NovaDock",NULL
    };
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR requesters;
    BPTR lock,input,output;
    char command[300],found[256];
    LONG rc;
    int i;
    found[0]='\0';
    requesters=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1L;
    for (i=0;paths[i] && !found[0];++i) {
        if (i==0 && !GetProgramDir()) continue;
        lock=Lock((STRPTR)paths[i],ACCESS_READ);
        if (lock) {
            struct FileInfoBlock *fib=
                (struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
            if (fib && Examine(lock,fib) && fib->fib_DirEntryType<0 &&
                !NameFromLock(lock,found,sizeof(found))) found[0]='\0';
            if (fib) FreeDosObject(DOS_FIB,fib);
            UnLock(lock);
        }
    }
    self->pr_WindowPtr=requesters;
    if (!found[0]) return 0;
    /* AmigaDOS quoting: '*' is the escape character inside quotes, so a
     * literal '*' or '"' in the path is written as "**" or "*"". */
    {
        size_t n=0;
        const char *p;
        command[n++]='"';
        for (p=found;*p;++p) {
            if ((unsigned char)*p<32) return 0;
            if (*p=='*' || *p=='"') {
                if (n+1>=sizeof(command)-2) return 0;
                command[n++]='*';
            }
            if (n>=sizeof(command)-2) return 0;
            command[n++]=*p;
        }
        command[n++]='"';command[n]='\0';
    }
    input=Open("NIL:",MODE_OLDFILE);
    output=Open("NIL:",MODE_NEWFILE);
    if (!input || !output) {
        if (input) Close(input);
        if (output) Close(output);
        return 0;
    }
    rc=SystemTags((STRPTR)command,SYS_Input,input,SYS_Output,output,
                  SYS_Asynch,TRUE,NP_StackSize,32768,TAG_DONE);
    if (rc==-1) {Close(input);Close(output);return 0;}
    return 1;
}
/* Two-stage reload: returns the reload's own result, or ND_CONTROL_PENDING
 * when the dock accepted it but was still busy when we stopped waiting.
 * Returns ND_CONTROL_INVALID without side effects if the dock predates the
 * two-stage protocol, so the caller can use the single-stage request. */
static LONG reload_two_stage(void)
{
    struct NDControlMessage begin;
    struct NDControlReloadMessage status;
    ULONG ticket;
    LONG rc;
    int waited=0;
    memset(&begin,0,sizeof(begin));
    begin.magic=ND_CONTROL_MAGIC;
    begin.version=ND_CONTROL_VERSION;
    begin.action=ND_CONTROL_RELOAD_BEGIN;
    begin.result=ND_CONTROL_NOHOST;
    rc=control_exchange(&begin,sizeof(begin),ND_CONTROL_TIMEOUT_SECONDS);
    if (rc<=0) return rc; /* NOHOST, TIMEOUT, or INVALID from an older dock */
    ticket=(ULONG)rc;
    /* The dock answers STATUS as soon as the reload it is running ends. */
    while (waited<(int)ND_CONTROL_RELOAD_WAIT_SECONDS) {
        memset(&status,0,sizeof(status));
        status.control.magic=ND_CONTROL_MAGIC;
        status.control.version=ND_CONTROL_VERSION;
        status.control.action=ND_CONTROL_RELOAD_STATUS;
        status.control.result=ND_CONTROL_NOHOST;
        status.ticket=ticket;
        rc=control_exchange(&status,sizeof(status),
                            ND_CONTROL_RELOAD_WAIT_SECONDS-(ULONG)waited);
        if (rc==ND_CONTROL_OK) return status.outcome;
        if (rc==ND_CONTROL_TIMEOUT) return ND_CONTROL_PENDING;
        if (rc!=ND_CONTROL_PENDING) return rc==ND_CONTROL_NOHOST ?
                                           ND_CONTROL_NOHOST : ND_CONTROL_PENDING;
        Delay(25);++waited; /* half a second per poll, counted generously */
    }
    return ND_CONTROL_PENDING;
}
LONG nd_control_reload(void)
{
    LONG rc=reload_two_stage();
    if (rc==ND_CONTROL_INVALID)
        rc=control_send_with_timeout(ND_CONTROL_RELOAD,
                                     ND_CONTROL_RELOAD_TIMEOUT_SECONDS);
    /* A generated configuration has already passed the editor's parser and
     * fit checks.  INVALID here therefore means the live process predates a
     * compatible configuration extension.  Replace that process once. */
    if (rc!=ND_CONTROL_INVALID) return rc;
    if (nd_control_send(ND_CONTROL_QUIT)!=ND_CONTROL_OK ||
        !wait_for_ports(0,250) || !nd_control_launch_dock() ||
        !wait_for_ports(1,250)) return ND_CONTROL_INVALID;
    return ND_CONTROL_OK;
}
LONG nd_control_diagnostics(struct NDDiagnosticsSnapshot *snapshot)
{
    struct NDControlDiagnosticsMessage pkt;
    LONG result;
    if (!snapshot) return ND_CONTROL_INVALID;
    memset(&pkt,0,sizeof(pkt));
    memset(snapshot,0,sizeof(*snapshot));
    pkt.control.magic=ND_CONTROL_MAGIC;
    pkt.control.version=ND_CONTROL_VERSION;
    pkt.control.action=ND_CONTROL_DIAGNOSTICS;
    pkt.control.result=ND_CONTROL_NOHOST;
    result=control_exchange(&pkt,sizeof(pkt),ND_CONTROL_TIMEOUT_SECONDS);
    if (result==ND_CONTROL_OK) *snapshot=pkt.snapshot;
    return result;
}
