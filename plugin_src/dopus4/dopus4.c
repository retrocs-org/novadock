/* Directory Opus 4 bridge for NovaDock.
 * The plugin never links against or redistributes DOpus.  It detects the
 * configured ARexx port, restores and fronts DOpus on click, or starts the
 * configured executable when DOpus is offline.
 */
#include <exec/types.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/rexxsyslib.h>
#include <proto/wb.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("DOpus4");
/* Honoured by the vbcc and libnix startup code; harmless elsewhere. */
long __stack=16384;

struct RxsLib *RexxSysBase=NULL;
struct Library *WorkbenchBase=NULL;
struct IntuitionBase *IntuitionBase=NULL;

struct DOpusConfig {
    char port[32];
    char alternate_port[32];
    char restore_command[64];
    char front_command[64];
    char program[192];
    char config[192];
    unsigned int poll_ticks;
};

static void defaults(struct DOpusConfig *cfg)
{
    strcpy(cfg->port,"DOPUS.1");
    strcpy(cfg->alternate_port,"DOPUS");
    strcpy(cfg->restore_command,"UNICONIFY");
    strcpy(cfg->front_command,"DOPUSTOFRONT");
    strcpy(cfg->program,"SYS:DirOpus4/DirectoryOpus");
    cfg->config[0]=0;
    cfg->poll_ticks=25;
}
static int clean_text(const char *value,unsigned int maximum)
{
    return value && *value && strlen(value)<maximum &&
           !strchr(value,'\r') && !strchr(value,'\n') && !strchr(value,'|');
}
static int apply_option(void *context,const char *key,const char *value)
{
    struct DOpusConfig *cfg=(struct DOpusConfig *)context;
    char *end;
    unsigned long ticks;
    if (!strcmp(key,"PORT") && clean_text(value,sizeof(cfg->port)))
        strcpy(cfg->port,value);
    else if (!strcmp(key,"ALT_PORT") && clean_text(value,sizeof(cfg->alternate_port)))
        strcpy(cfg->alternate_port,value);
    else if (!strcmp(key,"RESTORE_COMMAND") && clean_text(value,sizeof(cfg->restore_command)))
        strcpy(cfg->restore_command,value);
    else if (!strcmp(key,"FRONT_COMMAND") && clean_text(value,sizeof(cfg->front_command)))
        strcpy(cfg->front_command,value);
    else if (!strcmp(key,"PROGRAM") && clean_text(value,sizeof(cfg->program)) &&
             !strchr(value,'\"')) strcpy(cfg->program,value);
    else if (!strcmp(key,"CONFIG") && strlen(value)<sizeof(cfg->config) &&
             !strchr(value,'\"') && !strchr(value,'\r') &&
             !strchr(value,'\n') && !strchr(value,'|')) strcpy(cfg->config,value);
    else if (!strcmp(key,"POLL_TICKS")) {
        ticks=strtoul(value,&end,10);
        if (*end || ticks<10 || ticks>500) return 0;
        cfg->poll_ticks=(unsigned int)ticks;
    } else return 0;
    return 1;
}
static int port_exists(const char *name)
{
    int found;
    if (!clean_text(name,32)) return 0;
    Forbid();found=FindPort((STRPTR)name)!=NULL;Permit();
    return found;
}
static const char *active_port(const struct DOpusConfig *cfg)
{
    static char discovered[16];
    int number;
    if (port_exists(cfg->port)) return cfg->port;
    if (port_exists(cfg->alternate_port)) return cfg->alternate_port;
    /* DOpus creates the first free DOPUS.n port, not necessarily DOPUS.1.
     * Its own 4.16 source scans this range when locating another copy. */
    for (number=1;number<=9;++number) {
        sprintf(discovered,"DOPUS.%d",number);
        if (port_exists(discovered)) return discovered;
    }
    return NULL;
}
static int dopus_resident(void)
{
    int found;
    /* Public task name used by DOpus itself to reject a second copy. */
    Forbid();found=FindTask((STRPTR)"dopus_hotkeez")!=NULL;Permit();
    return found;
}
static struct Task *port_task(const char *port)
{
    struct MsgPort *found;
    struct Task *task=NULL;
    if (!clean_text(port,32)) return NULL;
    Forbid();
    found=FindPort((STRPTR)port);
    if (found) task=found->mp_SigTask;
    Permit();
    return task;
}
static int window_is_dopus(const struct Window *window,
                           const struct Task *owner,
                           const char *port)
{
    if (!window) return 0;
    if (window->Title && port && !strcmp(window->Title,port)) return 1;
    if (!window->UserPort || window->UserPort->mp_SigTask!=owner) return 0;
    /* DOpus's normal custom screen belongs to its main task.  When configured
     * on Workbench, only count its full application window; the tiny iconify
     * window must still be reported as ICONIFIED. */
    if (window->WScreen &&
        (window->WScreen->Flags&SCREENTYPE)!=WBENCHSCREEN) return 1;
    return window->WScreen &&
           window->Width>=window->WScreen->Width/2 &&
           window->Height>=window->WScreen->Height/2;
}
static int dopus_visible(const char *port)
{
    struct Task *owner;
    struct Screen *screen;
    struct Window *window;
    ULONG lock;
    int visible=0;
    if (!IntuitionBase || !(owner=port_task(port))) return 0;
    lock=LockIBase(0);
    for (screen=IntuitionBase->FirstScreen;screen && !visible;
         screen=screen->NextScreen)
        for (window=screen->FirstWindow;window;window=window->NextWindow)
            if (window_is_dopus(window,owner,port)) {visible=1;break;}
    UnlockIBase(lock);
    return visible;
}
static int dopus_frontmost(const char *port)
{
    struct Task *owner;
    struct Screen *screen;
    struct Window *window;
    ULONG lock;
    int front=0;
    if (!IntuitionBase || !(owner=port_task(port))) return 0;
    lock=LockIBase(0);
    for (screen=IntuitionBase->FirstScreen;screen && !front;
         screen=screen->NextScreen)
        for (window=screen->FirstWindow;window;window=window->NextWindow)
            if (window_is_dopus(window,owner,port)) {
                front=screen==IntuitionBase->FirstScreen;
                break;
            }
    UnlockIBase(lock);
    return front;
}
static int front_named_screen(const char *name)
{
    struct Screen *screen;
    if (!IntuitionBase || !clean_text(name,32)) return 0;
    /* DOpus normally publishes its custom screen using the same DOPUS.n
     * name as its ARexx port. Holding the public-screen lock keeps the screen
     * valid while Intuition moves it to the front. */
    screen=LockPubScreen((STRPTR)name);
    if (!screen) return 0;
    ScreenToFront(screen);
    UnlockPubScreen((STRPTR)name,screen);
    return 1;
}
static int signal_port_uniconify(const char *port)
{
    struct MsgPort *found;
    int sent=0;
    /* DOpus 4's iconified main loop waits on SIGBREAKF_CTRL_E
     * (INPUTSIG_UNICONIFY). The ARexx port is created by that main process,
     * so its signal task is a reliable target and requires no configured
     * hotkey or guessed task name. */
    if (!clean_text(port,32)) return 0;
    Forbid();
    found=FindPort((STRPTR)port);
    if (found && found->mp_SigTask) {
        Signal(found->mp_SigTask,SIGBREAKF_CTRL_E);
        sent=1;
    }
    Permit();
    return sent;
}
static LONG send_arexx(const char *port,const char *command)
{
    struct MsgPort *reply,*target;
    struct RexxMsg *message,*response;
    LONG result=20;
    int delivered=0;
    if (!RexxSysBase || !clean_text(port,32) || !clean_text(command,64)) return 20;
    reply=CreateMsgPort();
    if (!reply) return 20;
    message=CreateRexxMsg(reply,(STRPTR)"rexx",(STRPTR)port);
    if (!message) {DeleteMsgPort(reply);return 20;}
    message->rm_Args[0]=CreateArgstring((STRPTR)command,(LONG)strlen(command));
    if (!message->rm_Args[0]) {
        DeleteRexxMsg(message);DeleteMsgPort(reply);return 20;
    }
    message->rm_Action=RXCOMM;
    Forbid();target=FindPort((STRPTR)port);
    if (target) {PutMsg(target,(struct Message *)message);delivered=1;}
    Permit();
    if (delivered) {
        WaitPort(reply);response=(struct RexxMsg *)GetMsg(reply);
        if (response==message) result=message->rm_Result1;
    }
    DeleteArgstring(message->rm_Args[0]);
    DeleteRexxMsg(message);DeleteMsgPort(reply);
    return result;
}
static LONG restore_and_front(const struct DOpusConfig *cfg,const char *port)
{
    LONG restore_result=20,front_result=20;
    const char *current;
    int wait;
    if (dopus_frontmost(port)) return 0;
    /* DOpus uses the same activation signal for both states: it uniconifies
     * a closed screen and brings an open-but-behind custom screen forward.
     * Previously we skipped this signal whenever any DOpus window existed,
     * which is exactly the screen-depth-gadget case. */
    if (signal_port_uniconify(port)) {
        restore_result=0;
        for (wait=0;wait<20 && !dopus_frontmost(port);++wait) Delay(2);
    } else {
        restore_result=send_arexx(port,cfg->restore_command);
        if (restore_result==0)
            for (wait=0;wait<20 && !dopus_frontmost(port);++wait) Delay(2);
    }
    if (dopus_frontmost(port)) return 0;
    current=active_port(cfg);
    if (current) {
        front_result=send_arexx(current,cfg->front_command);
        if (front_result==0)
            for (wait=0;wait<10 && !dopus_frontmost(current);++wait) Delay(2);
        if (dopus_frontmost(current)) return 0;
        /* Some DOpus builds accept DopusToFront but leave a depth-gadgeted
         * screen behind Workbench. Use Intuition directly when it is public. */
        if (front_named_screen(current)) return 0;
    }
    if (front_named_screen(port) || front_named_screen(cfg->port) ||
        front_named_screen(cfg->alternate_port)) return 0;
    return 20;
}
static int start_program(const char *path,const char *configuration)
{
    BPTR input,output,drawer,previous=0;
    char command[404],directory[192],*slash,*colon,*cut;
    LONG result;
    if (!clean_text(path,192) || strchr(path,'\"')) return 0;
    if (configuration && *configuration) {
        if (!clean_text(configuration,192) || strchr(configuration,'\"')) return 0;
        sprintf(command,"\"%s\" -c\"%s\"",path,configuration);
    } else sprintf(command,"\"%s\"",path);
    strcpy(directory,path);slash=strrchr(directory,'/');colon=strrchr(directory,':');
    cut=slash && (!colon || slash>colon) ? slash : colon;
    if (cut) {
        if (*cut==':') cut[1]=0; else *cut=0;
        drawer=Lock((STRPTR)directory,ACCESS_READ);
        if (drawer) previous=CurrentDir(drawer);
    } else drawer=0;
    input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
    if (!input || !output) {
        if (drawer) {CurrentDir(previous);UnLock(drawer);}
        if (input) Close(input);if (output) Close(output);return 0;
    }
    result=SystemTags((STRPTR)command,SYS_Input,input,SYS_Output,output,
                      SYS_Asynch,TRUE,TAG_DONE);
    if (drawer) {CurrentDir(previous);UnLock(drawer);}
    if (result==-1) {Close(input);Close(output);return 0;}
    return 1;
}
static int start_configured_program(const struct DOpusConfig *cfg)
{
    /* With no explicit config, launch the icon exactly as Workbench does.
     * DOpus reads ICONSTART, BUTTONSTART, CONFIGFILE and its other normal
     * tooltypes only from a WBStartup message. A plain CLI SystemTags()
     * launch silently bypasses that default startup context. */
    if (!cfg->config[0] && WorkbenchBase && WorkbenchBase->lib_Version>=44 &&
        OpenWorkbenchObject((STRPTR)cfg->program,TAG_DONE)) return 1;
    /* Explicit -c remains a deliberate CLI launch and is never rewritten. */
    return start_program(cfg->program,cfg->config);
}
int main(int argc,char **argv)
{
    struct NDClient client;
    struct DOpusConfig cfg;
    const char *port;
    const char *status;
    unsigned int event;
    unsigned int pending_ticks=0,elapsed_ticks=0,heartbeat_ticks;
    int quit=0,reload=1,online,resident,visible,pending_front=0;
    int start_failed=0,front_failed=0;
    if (argc<2 || !nd_client_init(&client,argv[1])) return 20;
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);
    WorkbenchBase=OpenLibrary("workbench.library",44);
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",37);
    defaults(&cfg);
    while (!quit) {
        if (reload) {
            defaults(&cfg);
            if (nd_plugin_options_effective("DOPUS4",apply_option,&cfg)!=1)
                defaults(&cfg);
            pending_front=0;pending_ticks=0;
            start_failed=0;front_failed=0;
            reload=0;
        }
        port=active_port(&cfg);online=port!=NULL && RexxSysBase!=NULL;
        resident=dopus_resident();
        visible=online && dopus_visible(port);
        if (online && pending_front) {
            if (restore_and_front(&cfg,port)!=0) front_failed=8;
            pending_front=0;
            pending_ticks=0;
        } else if (resident && pending_front) {
            /* A resident copy normally retains an ARexx port. If a very old
             * build does not, wait for it rather than starting a duplicate. */
            front_failed=8;
            pending_front=0;
            pending_ticks=0;
        } else if (!online && pending_front && ++pending_ticks>=40) {
            pending_front=0;
            pending_ticks=0;
            start_failed=8;
        }
        if (start_failed) status="START FAILED";
        else if (front_failed) status="FRONT FAILED";
        else if (online || resident) status=visible ? "READY" : "ICONIFIED";
        else if (pending_front) status="STARTING";
        else status="OFFLINE";
        heartbeat_ticks=500U;
        event=nd_client_update_quiet(&client,"DOPUS 4",status,ND_NO_METER,
                                     elapsed_ticks,heartbeat_ticks);
        if (!(event&ND_EVENT_QUIT))
            event|=nd_client_set_badge(&client,(online || resident) ? "*" : "");
        if (start_failed) --start_failed;
        if (front_failed) --front_failed;
        if (event&ND_EVENT_QUIT) quit=1;
        if (event&ND_EVENT_SETTINGS) reload=1;
        if (!quit && (event&ND_EVENT_CLICK)) {
            if (online) {
                if (restore_and_front(&cfg,port)!=0) front_failed=8;
            } else if (resident) {
                /* Never launch a duplicate resident. A missing port gives no
                 * safe route to its main process, so report the failure. */
                front_failed=8;
            } else if (start_configured_program(&cfg)) {
                pending_front=1;pending_ticks=0;start_failed=0;
            } else start_failed=8;
        }
        if (!quit) {
            elapsed_ticks=pending_front ? cfg.poll_ticks :
                nd_client_delay_ticks(&client,cfg.poll_ticks);
            /* Wakes immediately when the tile is clicked. */
            elapsed_ticks=nd_client_wait(&client,elapsed_ticks);
        }
    }
    nd_client_close(&client);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
    return 0;
}
