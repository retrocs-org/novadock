/* Mixer: AHI output volume.  The tile shows the volume stored in AHI's
 * preferences; a click opens a small slider window.  A change is written to
 * ENV:Sys/ahi.prefs (every unit) and applied at once where the sound card's
 * AHI driver has an output volume control and the card is free; otherwise
 * it applies the next time a program starts playing.  ENVARC: is updated
 * when the window closes (option SAVE), so a floppy is not written per step. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_ahiprefs.h"
#include "nd_ahi.h"
#include "nd_prefsave.h"

const char nd_version[]=ND_VER_STRING("Mixer");
long __stack=16384;
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct Library *AHIBase=NULL;

#define ENV_PREFS "ENV:Sys/ahi.prefs"
#define ENVARC_PREFS "ENVARC:Sys/ahi.prefs"
#define G_SLIDER 1
#define G_MUTE 2
#define G_CLOSE 3
#define LIVE_APPLIED 1
#define LIVE_BUSY 0
#define LIVE_NO_CONTROL (-1)
#define LIVE_NO_AHI (-2)

struct Mixer {
    unsigned char prefs[ND_AHI_PREFS_MAX];
    ULONG prefs_length;
    int have_prefs,muted,envarc_dirty,save,live,save_failed;
    unsigned int percent,before_mute;
    struct Screen *screen;
    struct Window *win;
    APTR visual;
    struct Gadget *gadgets,*slider,*mute,*status;
    char status_text[64];
};

static int read_prefs(struct Mixer *m)
{
    BPTR file;
    LONG length;
    unsigned long fixed;
    APTR old=((struct Process *)FindTask(NULL))->pr_WindowPtr;
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    m->have_prefs=0;
    file=Open((STRPTR)ENV_PREFS,MODE_OLDFILE);
    if (file) {
        length=Read(file,m->prefs,sizeof(m->prefs));
        Close(file);
        if (length>0 && (ULONG)length<sizeof(m->prefs)) {
            m->prefs_length=(ULONG)length;
            if (nd_ahi_prefs_volume(m->prefs,m->prefs_length,&fixed)) {
                m->have_prefs=1;
                m->percent=nd_ahi_percent(fixed);
            }
        }
    }
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
    return m->have_prefs;
}
static int prefs_exists(void *ctx,const char *path)
{
    BPTR lock;LONG error;(void)ctx;
    lock=Lock((STRPTR)path,ACCESS_READ);
    if(lock){UnLock(lock);return 1;}
    error=IoErr();
    return error==ERROR_OBJECT_NOT_FOUND || error==ERROR_DIR_NOT_FOUND ? 0 : -1;
}
static int prefs_write(void *ctx,const char *path,const unsigned char *data,
                        unsigned long length)
{
    BPTR file;LONG written;int closed;(void)ctx;
    file=Open((STRPTR)path,MODE_NEWFILE);
    if(!file)return 0;
    written=Write(file,(APTR)data,(LONG)length);
    closed=Close(file)!=0;
    return written==(LONG)length && closed;
}
static int prefs_move(void *ctx,const char *from,const char *to)
{(void)ctx;return Rename((STRPTR)from,(STRPTR)to)!=0;}
static int prefs_remove(void *ctx,const char *path)
{(void)ctx;return DeleteFile((STRPTR)path)!=0;}
static int write_prefs(const char *path,const unsigned char *data,ULONG length)
{
    static const struct NDPrefIO io={prefs_exists,prefs_write,prefs_move,prefs_remove};
    APTR old=((struct Process *)FindTask(NULL))->pr_WindowPtr;
    int result;
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    result=nd_pref_replace(path,data,length,&io,NULL);
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
    return result;
}
/* Opening ahi.device for low-level use makes it re-read ENV:Sys/ahi.prefs,
 * and allocating the default audio mode sends the stored output volume to
 * the driver.  The card is released again at once. */
static int apply_live(void)
{
    struct MsgPort *port;
    struct NDAHIRequest *request;
    struct NDAHIAudioCtrl *ctrl;
    struct TagItem alloc[5],attrs[3];
    LONG minimum=0,maximum=0;
    int result=LIVE_NO_AHI;
    port=CreateMsgPort();
    if (!port) return result;
    request=(struct NDAHIRequest *)CreateIORequest(port,sizeof(*request));
    if (request) {
        request->ahir_Version=4;
        if (!OpenDevice((STRPTR)ND_AHI_NAME,ND_AHI_NO_UNIT,
                        (struct IORequest *)request,0)) {
            AHIBase=(struct Library *)request->ahir_Std.io_Device;
            alloc[0].ti_Tag=ND_AHIA_AudioID;alloc[0].ti_Data=ND_AHI_DEFAULT_ID;
            alloc[1].ti_Tag=ND_AHIA_MixFreq;alloc[1].ti_Data=ND_AHI_DEFAULT_FREQ;
            alloc[2].ti_Tag=ND_AHIA_Channels;alloc[2].ti_Data=1;
            alloc[3].ti_Tag=ND_AHIA_Sounds;alloc[3].ti_Data=1;
            alloc[4].ti_Tag=TAG_DONE;alloc[4].ti_Data=0;
            ctrl=ND_AHI_AllocAudioA(alloc);
            if (ctrl) {
                attrs[0].ti_Tag=ND_AHIDB_MinOutputVolume;
                attrs[0].ti_Data=(ULONG)&minimum;
                attrs[1].ti_Tag=ND_AHIDB_MaxOutputVolume;
                attrs[1].ti_Data=(ULONG)&maximum;
                attrs[2].ti_Tag=TAG_DONE;attrs[2].ti_Data=0;
                (void)ND_AHI_GetAudioAttrsA(ND_AHI_INVALID_ID,ctrl,attrs);
                ND_AHI_FreeAudio(ctrl);
                result=maximum>minimum ? LIVE_APPLIED : LIVE_NO_CONTROL;
            } else result=LIVE_BUSY;
            AHIBase=NULL;
            CloseDevice((struct IORequest *)request);
        }
        DeleteIORequest((struct IORequest *)request);
    }
    DeleteMsgPort(port);
    return result;
}
static void show_status(struct Mixer *m,const char *text)
{
    strncpy(m->status_text,text,sizeof(m->status_text)-1);
    m->status_text[sizeof(m->status_text)-1]='\0';
    if (m->win && m->status)
        GT_SetGadgetAttrs(m->status,m->win,NULL,GTTX_Text,(ULONG)m->status_text,TAG_DONE);
}
static void set_volume(struct Mixer *m,unsigned int percent)
{
    if (!read_prefs(m)) {
        show_status(m,"No AHI prefs: Save in AHI prefs once");
        return;
    }
    if (!nd_ahi_prefs_set_volume(m->prefs,m->prefs_length,nd_ahi_fixed(percent)) ||
        write_prefs(ENV_PREFS,m->prefs,m->prefs_length)!=1) {
        show_status(m,"Could not write ENV:Sys/ahi.prefs");
        return;
    }
    m->percent=percent>100 ? 100 : percent;
    m->envarc_dirty=1;
    m->live=apply_live();
    show_status(m,m->live==LIVE_APPLIED ? "Applied" :
                  m->live==LIVE_BUSY ? "Card in use: applies on next playback" :
                  m->live==LIVE_NO_CONTROL ? "Driver has no volume control" :
                  "ahi.device not available");
}
static void close_window(struct Mixer *m)
{
    if (m->win) {CloseWindow(m->win);m->win=NULL;}
    if (m->gadgets) {FreeGadgets(m->gadgets);m->gadgets=NULL;}
    if (m->visual) {FreeVisualInfo(m->visual);m->visual=NULL;}
    if (m->screen) {UnlockPubScreen(NULL,m->screen);m->screen=NULL;}
    m->slider=m->mute=m->status=NULL;
    /* Remember the volume across reboots, once per window session. */
    if(m->envarc_dirty && m->save){
        int result=read_prefs(m) ? write_prefs(ENVARC_PREFS,m->prefs,m->prefs_length) : 0;
        m->save_failed=result!=1;
        if(result==1)m->envarc_dirty=0;
        else show_status(m,result<0 ? "Save failed: original in ahi.prefs.bak" :
                         "Permanent save failed; close again to retry");
    }else if(!m->save){m->envarc_dirty=0;m->save_failed=0;}
}
static int open_window(struct Mixer *m)
{
    struct NewGadget ng;
    struct Gadget *g;
    int fy,top,left,width=300,height;
    m->screen=LockPubScreen(NULL);
    if (!m->screen) return 0;
    m->visual=GetVisualInfo(m->screen,TAG_DONE);
    if (!m->visual) {close_window(m);return 0;}
    fy=m->screen->Font ? m->screen->Font->ta_YSize : 8;
    top=m->screen->WBorTop+fy+1+6;
    left=m->screen->WBorLeft;
    g=CreateContext(&m->gadgets);
    memset(&ng,0,sizeof(ng));
    ng.ng_TextAttr=m->screen->Font;ng.ng_VisualInfo=m->visual;
    ng.ng_LeftEdge=left+70;ng.ng_TopEdge=top;ng.ng_Width=170;
    ng.ng_Height=fy+4;ng.ng_GadgetText=(UBYTE *)"Volume";
    ng.ng_GadgetID=G_SLIDER;ng.ng_Flags=PLACETEXT_LEFT;
    m->slider=g=CreateGadget(SLIDER_KIND,g,&ng,GTSL_Min,0,GTSL_Max,100,
        GTSL_Level,(ULONG)(m->muted ? 0 : m->percent),
        GTSL_LevelFormat,(ULONG)"%3ld%%",GTSL_MaxLevelLen,4,
        GTSL_LevelPlace,PLACETEXT_RIGHT,GA_RelVerify,TRUE,TAG_DONE);
    ng.ng_TopEdge+=fy+10;ng.ng_Width=26;ng.ng_Height=fy+3;
    ng.ng_GadgetText=(UBYTE *)"Mute";ng.ng_GadgetID=G_MUTE;
    m->mute=g=CreateGadget(CHECKBOX_KIND,g,&ng,GTCB_Checked,(ULONG)m->muted,TAG_DONE);
    ng.ng_LeftEdge=left+10;ng.ng_TopEdge+=fy+10;ng.ng_Width=width-20;
    ng.ng_Height=fy+4;ng.ng_GadgetText=NULL;ng.ng_Flags=0;
    m->status=g=CreateGadget(TEXT_KIND,g,&ng,GTTX_Text,(ULONG)m->status_text,
                             GTTX_Border,TRUE,TAG_DONE);
    ng.ng_LeftEdge=left+width-90;ng.ng_TopEdge+=fy+10;ng.ng_Width=80;
    ng.ng_GadgetText=(UBYTE *)"Close";ng.ng_GadgetID=G_CLOSE;
    ng.ng_Flags=PLACETEXT_IN;
    g=CreateGadget(BUTTON_KIND,g,&ng,TAG_DONE);
    if (!g) {close_window(m);return 0;}
    height=ng.ng_TopEdge+ng.ng_Height+6-m->screen->WBorTop-fy-1;
    m->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)m->screen,
        WA_Left,m->screen->MouseX-width/2>0 ? m->screen->MouseX-width/2 : 0,
        WA_Top,m->screen->MouseY-height-30>0 ? m->screen->MouseY-height-30 : 0,
        WA_InnerWidth,width,WA_InnerHeight,height,
        WA_Title,(ULONG)"NovaDock Mixer",WA_Gadgets,(ULONG)m->gadgets,
        WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,WA_AutoAdjust,TRUE,
        WA_IDCMP,SLIDERIDCMP|CHECKBOXIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|
                 IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY,
        TAG_DONE);
    if (!m->win) {close_window(m);return 0;}
    GT_RefreshWindow(m->win,NULL);
    return 1;
}
/* Returns 0 when the window should close. */
static int window_events(struct Mixer *m)
{
    struct IntuiMessage *im;
    ULONG cls;
    UWORD code;
    struct Gadget *gadget;
    int keep=1;
    while (m->win && (im=GT_GetIMsg(m->win->UserPort))!=NULL) {
        cls=im->Class;code=im->Code;gadget=(struct Gadget *)im->IAddress;
        GT_ReplyIMsg(im);
        if (cls==IDCMP_CLOSEWINDOW ||
            (cls==IDCMP_VANILLAKEY && (code==27 || code=='\r'))) keep=0;
        else if (cls==IDCMP_REFRESHWINDOW) {
            GT_BeginRefresh(m->win);GT_EndRefresh(m->win,TRUE);
        } else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_CLOSE) keep=0;
        else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_SLIDER) {
            m->muted=0;
            GT_SetGadgetAttrs(m->mute,m->win,NULL,GTCB_Checked,FALSE,TAG_DONE);
            set_volume(m,code);
        } else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_MUTE) {
            m->muted=(m->mute->Flags&GFLG_SELECTED)!=0;
            if (m->muted) {m->before_mute=m->percent;set_volume(m,0);}
            else set_volume(m,m->before_mute ? m->before_mute : 75);
            GT_SetGadgetAttrs(m->slider,m->win,NULL,GTSL_Level,
                              (ULONG)(m->muted ? 0 : m->percent),TAG_DONE);
        }
    }
    return keep;
}
int main(int argc,char **argv)
{
    struct NDClient client;
    struct Mixer *m;
    char value[32],option[16],save[8];
    struct NDOptionString fields[2];
    unsigned int event,elapsed=0,poll_ticks=100,since_read=100000U;
    unsigned long events2;
    int reread=1;
    if (argc<2) return 20;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    m=(struct Mixer *)AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);
    if (!IntuitionBase || !GadToolsBase || !m || !nd_client_init(&client,argv[1])) {
        if (m) FreeVec(m);
        if (GadToolsBase) CloseLibrary(GadToolsBase);
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    strcpy(m->status_text,"Drag the slider; release to apply");
    for (;;) {
        if (reread) {
            fields[0].key="SAVE";fields[0].fallback="YES";fields[0].out=save;fields[0].limit=sizeof(save);
            fields[1].key="POLL_TICKS";fields[1].fallback="100";fields[1].out=option;fields[1].limit=sizeof(option);
            nd_plugin_option_strings(argv[1],fields,2);m->save=!strcmp(save,"YES");
            poll_ticks=(unsigned int)atoi(option);
            if (poll_ticks<50 || poll_ticks>3000) poll_ticks=100;
            reread=0;since_read=poll_ticks;
        }
        if (since_read>=poll_ticks) {(void)read_prefs(m);since_read=0;}
        if(m->save_failed)strcpy(value,"SAVE FAILED");
        else if (!m->have_prefs) strcpy(value,"NO AHI");
        else if (m->muted) strcpy(value,"MUTED");
        else sprintf(value,"%u%%",m->percent);
        event=nd_client_update_quiet(&client,"VOLUME",value,
                                     m->have_prefs ? m->percent : ND_NO_METER,
                                     elapsed,500U);
        if (event&ND_EVENT_QUIT) break;
        if (event&ND_EVENT_SETTINGS) reread=1;
        events2=nd_client_events2(&client);
        if (events2&ND_EVENT2_REFRESH) since_read=poll_ticks;
        if (event&ND_EVENT_CLICK) {
            if (m->win) close_window(m);
            else {(void)read_prefs(m);(void)open_window(m);}
        }
        if (m->win) {
            if (!window_events(m)) close_window(m);
            Delay(5);elapsed=5;
        } else elapsed=nd_client_wait(&client,nd_client_delay_ticks(&client,poll_ticks));
        since_read+=elapsed;
    }
    close_window(m);
    nd_client_close(&client);
    if(m->save_failed){
        struct EasyStruct es={sizeof(struct EasyStruct),0,"NovaDock Mixer",
            "Could not save AHI volume permanently.\nThe original preferences are preserved (or in ahi.prefs.bak).\nCheck free space and write access to ENVARC:Sys.","OK"};
        EasyRequestArgs(NULL,&es,NULL,NULL);
    }
    FreeVec(m);
    CloseLibrary(GadToolsBase);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
