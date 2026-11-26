/* Screen Mode: list available display modes and apply a selected mode
 * directly. Saved presets remain available as an alternate chooser. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <graphics/displayinfo.h>
#include <graphics/modeid.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <rexx/storage.h>
#include <rexx/rxslib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/rexxsyslib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_wait.h"
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_interaction.h"
#include "nd_scrmode.h"

const char nd_version[]=ND_PLUGIN_VER_STRING(SCREENMODE,"ScreenMode");
long __stack=16384;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
struct Library *GadToolsBase=NULL;
struct RxsLib *RexxSysBase=NULL;

#define MAX_PRESETS 128
#define G_LIST 1
#define G_USE 2
#define G_CANCEL 3

struct Preset {
    struct Node node;
    char label[96];
    char path[256];
    struct NDScreenModePreset mode;
    int generated;
};
struct Chooser {
    struct Screen *screen;
    struct Window *win;
    APTR visual;
    struct Gadget *gadgets,*list;
    struct List rows;
    struct Preset *presets;
    int count,selected;
};
struct Settings { char presets[128],program[128],chooser[16]; };
/* What identifies the current Workbench screen: size, depth and mode. */
struct WBSignature { struct Screen *screen; UWORD width,height,depth; ULONG mode; };

static void read_settings(const char *id,struct Settings *s)
{
    struct NDOptionString fields[3];
    fields[0].key="PRESETS";fields[0].fallback="SYS:Prefs/Presets";fields[0].out=s->presets;fields[0].limit=sizeof(s->presets);
    fields[1].key="PROGRAM";fields[1].fallback="SYS:Prefs/ScreenMode";fields[1].out=s->program;fields[1].limit=sizeof(s->program);
    fields[2].key="CHOOSER";fields[2].fallback="MODES";fields[2].out=s->chooser;fields[2].limit=sizeof(s->chooser);
    nd_plugin_option_strings(id,fields,3);
}
static int workbench_signature(struct WBSignature *sig)
{
    struct Screen *wb=LockPubScreen((STRPTR)"Workbench");
    if (!wb) return 0;
    sig->screen=wb;
    sig->width=(UWORD)wb->Width;sig->height=(UWORD)wb->Height;
    sig->depth=(UWORD)GetBitMapAttr(wb->RastPort.BitMap,BMA_DEPTH);
    sig->mode=GetVPModeID(&wb->ViewPort);
    UnlockPubScreen(NULL,wb);
    return 1;
}
static int same_signature(const struct WBSignature *a,const struct WBSignature *b)
{
    return a->screen==b->screen && a->width==b->width && a->height==b->height &&
           a->depth==b->depth && a->mode==b->mode;
}
static void mode_name(ULONG id,char *out,unsigned int size)
{
    struct NameInfo name;
    out[0]='\0';
    if (id==INVALID_ID) return;
    if (GetDisplayInfoData(NULL,(UBYTE *)&name,sizeof(name),DTAG_NAME,id)) {
        strncpy(out,(const char *)name.Name,size-1);
        out[size-1]='\0';
    }
}
/* Collect the ScreenMode presets in the configured drawer, sorted by name. */
static void scan_presets(struct Chooser *c,const char *drawer)
{
    struct FileInfoBlock *fib;
    struct NDScreenModePreset preset;
    unsigned char buffer[256];
    char size_text[24],name[40];
    BPTR lock,file;
    LONG length;
    int i,j;
    APTR old=((struct Process *)FindTask(NULL))->pr_WindowPtr;
    c->count=0;
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    lock=Lock((STRPTR)drawer,ACCESS_READ);
    if (fib && lock && Examine(lock,fib) && fib->fib_DirEntryType>0) {
        while (c->count<MAX_PRESETS && ExNext(lock,fib)) {
            struct Preset *p=&c->presets[c->count];
            size_t n=strlen(fib->fib_FileName);
            if (fib->fib_DirEntryType>0 ||
                (n>5 && !strcmp(fib->fib_FileName+n-5,".info"))) continue;
            if (!nd_join_path(p->path,sizeof(p->path),drawer,fib->fib_FileName))
                continue;
            file=Open((STRPTR)p->path,MODE_OLDFILE);
            if (!file) continue;
            length=Read(file,buffer,sizeof(buffer));
            Close(file);
            if (length<=0 || !nd_scrmode_parse(buffer,(unsigned long)length,&preset))
                continue;
            p->generated=0;p->mode=preset;
            (void)nd_scrmode_text(preset.width,preset.height,preset.depth,
                                  size_text,sizeof(size_text));
            mode_name(preset.display_id,name,sizeof(name));
            sprintf(p->label,"%.30s  %s%s%.32s",fib->fib_FileName,size_text,
                    name[0] ? "  " : "",name);
            ++c->count;
        }
    }
    if (lock) UnLock(lock);
    if (fib) FreeDosObject(DOS_FIB,fib);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
    for (i=1;i<c->count;++i)
        for (j=i;j>0 && strcmp(c->presets[j-1].label,c->presets[j].label)>0;--j) {
            struct Preset t=c->presets[j];
            c->presets[j]=c->presets[j-1];c->presets[j-1]=t;
        }
    /* Exec list for the listview. */
    c->rows.lh_Head=(struct Node *)&c->rows.lh_Tail;
    c->rows.lh_Tail=NULL;
    c->rows.lh_TailPred=(struct Node *)&c->rows.lh_Head;
    for (i=0;i<c->count;++i) {
        c->presets[i].node.ln_Name=c->presets[i].label;
        AddTail(&c->rows,&c->presets[i].node);
    }
}
/* Enumerate only installed, available Workbench modes. Native modes keep
 * the current colour depth where possible; fixed RTG modes use their depth. */
static void scan_modes(struct Chooser *c)
{
    struct DisplayInfo display;struct DimensionInfo dims;
    struct WBSignature current;struct Preset *p;
    ULONG id=INVALID_ID;LONG width,height;char name[40];int i,j,have;
    have=workbench_signature(&current);c->count=0;
    /* Keep the active mode even when the installed list exceeds our bound. */
    if(have){
        p=&c->presets[c->count++];memset(p,0,sizeof(*p));p->generated=1;
        p->mode.display_id=current.mode;p->mode.width=current.width;
        p->mode.height=current.height;p->mode.depth=current.depth;p->mode.control=1;
        mode_name(current.mode,name,sizeof(name));
        sprintf(p->label,"* CURRENT  %ux%ux%u  %.31s",current.width,current.height,current.depth,name);
    }
    while((id=NextDisplayInfo(id))!=INVALID_ID){
        if(c->count>=MAX_PRESETS)break;
        if((have && id==current.mode) || ModeNotAvailable(id))continue;
        memset(&display,0,sizeof(display));memset(&dims,0,sizeof(dims));
        if(!GetDisplayInfoData(NULL,(UBYTE *)&display,sizeof(display),DTAG_DISP,id) ||
           !GetDisplayInfoData(NULL,(UBYTE *)&dims,sizeof(dims),DTAG_DIMS,id) ||
           display.NotAvailable || !(display.PropertyFlags&DIPF_IS_WB) ||
           !dims.MaxDepth || dims.MaxDepth>32)continue;
        width=(LONG)dims.Nominal.MaxX-dims.Nominal.MinX+1;
        height=(LONG)dims.Nominal.MaxY-dims.Nominal.MinY+1;
        if(width<1 || height<1 || width>65535L || height>65535L ||
           width<dims.MinRasterWidth || height<dims.MinRasterHeight ||
           width>dims.MaxRasterWidth || height>dims.MaxRasterHeight)continue;
        p=&c->presets[c->count];memset(p,0,sizeof(*p));p->generated=1;
        p->mode.display_id=id;p->mode.width=(unsigned int)width;
        p->mode.height=(unsigned int)height;p->mode.control=1;
        p->mode.depth=have ? current.depth:4;
        if((display.PropertyFlags&DIPF_IS_FOREIGN) || p->mode.depth>dims.MaxDepth)
            p->mode.depth=dims.MaxDepth;
        mode_name(id,name,sizeof(name));
        sprintf(p->label,"%ux%ux%u  %.31s%s",p->mode.width,p->mode.height,p->mode.depth,
                name,have && id==current.mode ? " (current)":"");++c->count;
    }
    for(i=1;i<c->count;++i)for(j=i;j>0;--j){
        struct Preset *a=&c->presets[j-1],*b=&c->presets[j];struct Preset swap;
        int order=a->mode.width!=b->mode.width ? (a->mode.width>b->mode.width ? 1:-1):
            a->mode.height!=b->mode.height ? (a->mode.height>b->mode.height ? 1:-1):
            a->mode.depth!=b->mode.depth ? (a->mode.depth>b->mode.depth ? 1:-1):strcmp(a->label,b->label);
        if(order<=0)break;swap=*a;*a=*b;*b=swap;
    }
    c->rows.lh_Head=(struct Node *)&c->rows.lh_Tail;c->rows.lh_Tail=NULL;
    c->rows.lh_TailPred=(struct Node *)&c->rows.lh_Head;
    for(i=0;i<c->count;++i){
        c->presets[i].node.ln_Name=c->presets[i].label;AddTail(&c->rows,&c->presets[i].node);
        if(have && c->presets[i].mode.display_id==current.mode)c->selected=i;
    }
}
static void close_chooser(struct Chooser *c)
{
    if (c->win) {CloseWindow(c->win);c->win=NULL;}
    if (c->gadgets) {FreeGadgets(c->gadgets);c->gadgets=NULL;}
    if (c->visual) {FreeVisualInfo(c->visual);c->visual=NULL;}
    if (c->screen) {UnlockPubScreen(NULL,c->screen);c->screen=NULL;}
    if(c->presets){FreeVec(c->presets);c->presets=NULL;}
    c->list=NULL;
}
static int open_chooser(struct Chooser *c,const struct Settings *s)
{
    struct NewGadget ng;
    struct Gadget *g;
    int fy,top,left,width=460,height,rows;
    c->presets=AllocVec(sizeof(struct Preset)*MAX_PRESETS,MEMF_PUBLIC|MEMF_CLEAR);
    if(!c->presets)return 0;
    c->selected=-1;
    if(!strcmp(s->chooser,"PRESETS"))scan_presets(c,s->presets);else scan_modes(c);
    c->screen=LockPubScreen(NULL);
    if (!c->screen) {close_chooser(c);return 0;}
    c->visual=GetVisualInfo(c->screen,TAG_DONE);
    if (!c->visual) {close_chooser(c);return 0;}
    fy=c->screen->Font ? c->screen->Font->ta_YSize : 8;
    if(width>c->screen->Width-12)width=c->screen->Width-12;
    rows=(c->screen->Height-3*fy-40)/(fy+2);if(rows>10)rows=10;if(rows<2)rows=2;
    top=c->screen->WBorTop+fy+1+4;
    left=c->screen->WBorLeft;
    g=CreateContext(&c->gadgets);
    memset(&ng,0,sizeof(ng));
    ng.ng_TextAttr=c->screen->Font;ng.ng_VisualInfo=c->visual;
    ng.ng_LeftEdge=left+8;ng.ng_TopEdge=top+fy+4;
    ng.ng_Width=width-16;ng.ng_Height=(fy+2)*rows+4;
    ng.ng_GadgetText=(UBYTE *)(!strcmp(s->chooser,"PRESETS") ?
        (c->count ? "Saved presets":"No saved presets"):
        (c->count==MAX_PRESETS ? "Available modes (first 128)":c->count ? "Available display modes":"No display modes available"));
    ng.ng_GadgetID=G_LIST;ng.ng_Flags=PLACETEXT_ABOVE;
    c->list=g=CreateGadget(LISTVIEW_KIND,g,&ng,GTLV_Labels,(ULONG)&c->rows,
                           GTLV_ShowSelected,(ULONG)NULL,
                           GTLV_Selected,c->selected>=0 ? (ULONG)c->selected:~0UL,
                           GTLV_MakeVisible,c->selected>=0 ? (ULONG)c->selected:0UL,TAG_DONE);
    ng.ng_TopEdge+=ng.ng_Height+8;ng.ng_Width=100;ng.ng_Height=fy+6;
    ng.ng_GadgetText=(UBYTE *)"Use";ng.ng_GadgetID=G_USE;
    ng.ng_Flags=PLACETEXT_IN;
    g=CreateGadget(BUTTON_KIND,g,&ng,GA_Disabled,c->selected<0,TAG_DONE);
    ng.ng_LeftEdge=left+width-108;ng.ng_GadgetText=(UBYTE *)"Cancel";
    ng.ng_GadgetID=G_CANCEL;
    g=CreateGadget(BUTTON_KIND,g,&ng,TAG_DONE);
    if (!g) {close_chooser(c);return 0;}
    height=ng.ng_TopEdge+ng.ng_Height+6-c->screen->WBorTop-fy-1;
    c->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)c->screen,
        WA_Left,c->screen->MouseX-width/2>0 ? c->screen->MouseX-width/2 : 0,
        WA_Top,c->screen->MouseY-height-30>0 ? c->screen->MouseY-height-30 : 0,
        WA_InnerWidth,width,WA_InnerHeight,height,WA_AutoAdjust,TRUE,
        WA_Title,(ULONG)"NovaDock Screen Mode",WA_Gadgets,(ULONG)c->gadgets,
        WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,LISTVIEWIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|
                 IDCMP_VANILLAKEY,
        TAG_DONE);
    if (!c->win) {close_chooser(c);return 0;}
    GT_RefreshWindow(c->win,NULL);
    return 1;
}
/* Returns -2 to close, -1 to keep open, or the preset index to use. */
static int chooser_events(struct Chooser *c)
{
    struct IntuiMessage *im;
    struct Gadget *gadget,*use;
    ULONG cls;UWORD code;
    int result=-1;
    while (result==-1 && c->win && (im=GT_GetIMsg(c->win->UserPort))!=NULL) {
        cls=im->Class;code=im->Code;gadget=(struct Gadget *)im->IAddress;
        GT_ReplyIMsg(im);
        if (cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_VANILLAKEY && code==27)) result=-2;
        else if (cls==IDCMP_REFRESHWINDOW) {GT_BeginRefresh(c->win);GT_EndRefresh(c->win,TRUE);}
        else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_CANCEL) result=-2;
        else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_LIST) {
            c->selected=code<c->count ? (int)code : -1;
            for (use=c->gadgets;use;use=use->NextGadget)
                if (use->GadgetID==G_USE)
                    GT_SetGadgetAttrs(use,c->win,NULL,GA_Disabled,
                                      (ULONG)(c->selected<0),TAG_DONE);
        } else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_USE &&
                   c->selected>=0) result=c->selected;
        else if (cls==IDCMP_VANILLAKEY && code=='\r' && c->selected>=0)
            result=c->selected;
    }
    return result;
}
/* Send COMMAND to the dock's ARexx host and wait briefly for the reply.
 * The dock answers at once; a reply that never comes leaves an inert port
 * rather than a freed message the dock might still reply to. */
static int dock_command(const char *command)
{
    struct MsgPort *reply,*host;
    struct RexxMsg *msg;
    STRPTR marker=NULL;
    int replied=0,ok=0;
    if (!RexxSysBase || !(reply=CreateMsgPort())) return 0;
    msg=CreateRexxMsg(reply,(STRPTR)"rexx",(STRPTR)"NOVADOCK");
    if (!msg) {DeleteMsgPort(reply);return 0;}
    if (!msg->rm_Node.mn_Node.ln_Name) {
        marker=CreateArgstring((STRPTR)"REXX",4);
        msg->rm_Node.mn_Node.ln_Name=(char *)marker;
    }
    msg->rm_Args[0]=CreateArgstring((STRPTR)command,(ULONG)strlen(command));
    msg->rm_Action=RXCOMM;
    if (msg->rm_Args[0]) {
        Forbid();
        host=FindPort((STRPTR)"NOVADOCK");
        if (host) PutMsg(host,(struct Message *)msg);
        Permit();
        if (host) {
            /* Sleep until NovaDock answers (at most three seconds). */
            if (nd_wait_port(NULL,reply,150UL) && GetMsg(reply)) replied=1;
            if (!replied) {
                Forbid();
                if (GetMsg(reply)) replied=1;
                else {
                    reply->mp_Flags=PA_IGNORE;reply->mp_SigTask=NULL;
                    FreeSignal((LONG)reply->mp_SigBit);
                }
                Permit();
                if (!replied) return 0;
            }
            ok=msg->rm_Result1==0;
        }
        DeleteArgstring(msg->rm_Args[0]);
    }
    DeleteRexxMsg(msg);
    if (marker) DeleteArgstring(marker);
    DeleteMsgPort(reply);
    return ok;
}
#include "nd_mode_worker.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
/* Keep normal dock heartbeats while the separate editor process runs.
 * Deadline expiry resumes the dock without killing a live Amiga process.
 * The worker's global guard prevents overlapping switches until it exits. */
static const char *apply_preset(struct NDClient *client,const struct Settings *s,
                                const char *preset,const char *program,int *quit,
                                const char *cleanup,int *transferred)
{
    char command[600],portname[64];
    struct WBSignature before,now;
    struct MsgPort *port,*guard;
    struct ModeMessage *m;
    ULONG started,finished_at=0,age;
    LONG rc=-1;
    int had=workbench_signature(&before),seen_closed=0,claimed=0,done=0;
    const char *problem=NULL;
    unsigned int event;
    size_t n;
    Forbid();guard=FindPort((STRPTR)MODE_GUARD);Permit();
    *transferred=0;
    if(guard)return "SWITCH STILL RUNNING";
    if(!nd_quote_path(command,sizeof(command)/2,s->program))return "BAD PROGRAM";
    strcat(command," FROM ");n=strlen(command);
    if(!nd_quote_path(command+n,sizeof(command)-n-8,preset))return "BAD PRESET";
    strcat(command," USE");
    port=CreateMsgPort();if(!port)return "NO MEMORY";
    started=mode_ticks();
    sprintf(portname,"NovaDock.Mode.%08lx.%08lx",(ULONG)FindTask(NULL),started);
    port->mp_Node.ln_Name=portname;
    Forbid();
    guard=FindPort(portname);
    if(!guard)AddPort(port);
    Permit();
    if(guard){DeleteMsgPort(port);return "SWITCH BUSY";}
    if(!dock_command("SUSPEND")){close_mode_port(port);return "DOCK NOT READY";}
    if(!start_mode_worker(program,portname))problem="NOT STARTED";
    started=mode_ticks();
    while(!problem){
        while((m=(struct ModeMessage *)GetMsg(port))!=NULL){
            if(m->magic!=MODE_MAGIC || m->msg.mn_Length!=sizeof(*m)){
                if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);else FreeVec(m);
                continue;
            }
            if(m->kind==1){
                if(claimed){m->kind=-1;}else {
                    strcpy(m->command,command);m->cleanup[0]=0;
                    if(cleanup && *cleanup){strcpy(m->cleanup,cleanup);*transferred=1;}
                    claimed=1;
                }
                ReplyMsg((struct Message *)m);
            }else {
                rc=m->result;done=1;finished_at=mode_ticks();FreeVec(m);
            }
        }
        age=mode_ticks()-started;
        if(done){
            if(rc==-1){problem="NOT STARTED";break;}
            if(rc!=0){problem="FAILED";break;}
            if(!had)break;
            if(!workbench_signature(&now))seen_closed=1;
            else if(seen_closed || !same_signature(&before,&now))break;
            if(mode_ticks()-finished_at>=600UL)break;
        }else if(age>=750UL){problem=claimed ? "COMMAND TIMEOUT":"NOT STARTED";break;}
        event=nd_client_update_art_quiet(client,"SMODE","SWITCHING","modes",0,10,250U);
        if(event&ND_EVENT_QUIT){*quit=1;problem="CANCELLED";break;}
        (void)nd_client_wait(client,10);
    }
    (void)dock_command("RESUME");
    close_mode_port(port);
    return problem;
}
static int write_mode_preset(const struct NDScreenModePreset *mode,char *directory,char *path)
{
    unsigned char buffer[62];unsigned long size=nd_scrmode_encode(mode,buffer,sizeof(buffer));
    BPTR lock,file;int serial,ok;
    directory[0]=path[0]=0;if(!size)return 0;
    for(serial=0;serial<20;++serial){
        sprintf(directory,"T:NovaDockMode-%08lx-%08lx-%d",(ULONG)FindTask(NULL),mode_ticks(),serial);
        lock=CreateDir(directory);if(lock){UnLock(lock);break;}
    }
    if(serial==20){directory[0]=0;return 0;}
    sprintf(path,"%s/mode.prefs",directory);file=Open(path,MODE_NEWFILE);
    if(!file){mode_clean_preset(directory);directory[0]=0;return 0;}
    ok=Write(file,buffer,(LONG)size)==(LONG)size;
    if(!Close(file))ok=0;
    if(!ok){mode_clean_preset(directory);directory[0]=0;}return ok;
}
int main(int argc,char **argv)
{
    struct NDClient client;
    struct Settings settings;
    struct Chooser *chooser;
    struct WBSignature sig;
    char value[32];
    const char *problem=NULL;
    unsigned int event,elapsed=0,since=100000U,message=0;
    int reread=1,choice,quit_requested=0,transferred;
    if(argc==3 && !strcmp(argv[1],"WORK"))return mode_worker(argv[2]);
    if (argc<2) return 20;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);
    chooser=(struct Chooser *)AllocVec(sizeof(*chooser),MEMF_PUBLIC|MEMF_CLEAR);
    if (!IntuitionBase || !GfxBase || !GadToolsBase || !chooser ||
        !nd_client_init(&client,argv[1])) {
        if (chooser) FreeVec(chooser);
        if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
        if (GadToolsBase) CloseLibrary(GadToolsBase);
        if (GfxBase) CloseLibrary((struct Library *)GfxBase);
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    strcpy(value,"?");
    for (;;) {
        if (reread) {read_settings(argv[1],&settings);reread=0;}
        if (message) {
            message=message>elapsed ? message-elapsed : 0;
            if (!message) since=100000U;
        } else if (since>=250U) {
            since=0;
            if (workbench_signature(&sig))
                (void)nd_scrmode_text(sig.width,sig.height,sig.depth,value,sizeof(value));
            else strcpy(value,"NO WORKBENCH");
        }
        event=nd_client_update_art_quiet(&client,"SMODE",value,"modes",0,elapsed,500U);
        if (event&ND_EVENT_QUIT) break;
        if (event&ND_EVENT_SETTINGS) reread=1;
        if (nd_client_events2(&client)&ND_EVENT2_REFRESH) since=100000U;
        if (event&ND_EVENT_CLICK) {
            if (chooser->win) close_chooser(chooser);
            else if(!open_chooser(chooser,&settings)){strcpy(value,"PICKER FAILED");message=250;}
        }
        if (chooser->win) {
            choice=chooser_events(chooser);
            if (choice>=0) {
                char preset[280],directory[256];struct NDScreenModePreset selected;
                int generated=chooser->presets[choice].generated,same=0;
                selected=chooser->presets[choice].mode;directory[0]=0;
                if(generated){
                    if(workbench_signature(&sig))same=selected.display_id==sig.mode &&
                        selected.width==sig.width && selected.height==sig.height && selected.depth==sig.depth;
                    if(!same && !write_mode_preset(&selected,directory,preset)){
                        strcpy(value,"PRESET WRITE FAILED");message=250;close_chooser(chooser);elapsed=0;continue;
                    }
                }else strcpy(preset,chooser->presets[choice].path);
                /* Close our visitor window and release the screen lock before Use. */
                close_chooser(chooser);
                if(!same){
                    problem=apply_preset(&client,&settings,preset,argv[0],&quit_requested,directory,&transferred);
                    if(!transferred && directory[0])mode_clean_preset(directory);
                    if(quit_requested)break;
                    if(problem){strcpy(value,problem);message=250;}else since=100000U;
                }else since=100000U;
            } else if (choice==-2) close_chooser(chooser);
            if (chooser->win)elapsed=nd_client_wait_signals(&client,250U,1UL<<chooser->win->UserPort->mp_SigBit);
            else elapsed=0;
        } else elapsed=nd_client_wait(&client,
                                      message ? 50U : nd_client_delay_ticks(&client,250U));
        since+=elapsed;
    }
    close_chooser(chooser);
    nd_client_close(&client);
    FreeVec(chooser);
    if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
