/* Text-only, bounded, session-local clipboard history. Clipboard transfers
 * are asynchronous: a lazy/posted clipboard owner cannot block dock IPC. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/lists.h>
#include <devices/clipboard.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_version.h"
#include "nd_clipboard.h"
#include "nd_search.h"
#include "nd_clipboard_popup.h"
const char nd_version[]=ND_VER_STRING("Clipboard");
long __stack=16384;
struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
struct Library *GadToolsBase=NULL;
#define CL_READ 1
#define CL_FINISH 2
#define CL_WRITE 3
#define CL_UPDATE 4
struct ClipDevice {
    struct MsgPort *port;struct IOClipReq *io;
    unsigned char raw[ND_CLIP_RAW_MAX];
    unsigned long size,age;LONG seen;
    int opened,pending,state,error,discard;
};
struct ClipPanel {
    struct Screen *screen;struct Window *win;APTR vi;
    struct Gadget *gadgets,*last,*list,*pause,*search;
    struct List labels;struct Node nodes[ND_CLIP_MAX];
    char names[ND_CLIP_MAX][100],query[64];int map[ND_CLIP_MAX],visible,selected;
};
struct ClipOptions {unsigned int limit;int badge;};
static int option(void *context,const char *key,const char *value)
{
    struct ClipOptions *o=context;char *end;unsigned long n;
    if(!strcmp(key,"LIMIT")){
        n=strtoul(value,&end,10);if(!*value || *end || n<1 || n>ND_CLIP_MAX)return 0;
        o->limit=(unsigned int)n;
    }else if(!strcmp(key,"BADGE")){
        if(!strcmp(value,"YES"))o->badge=1;
        else if(!strcmp(value,"NO"))o->badge=0;else return 0;
    }else return 0;
    return 1;
}
static struct ClipDevice *open_clip(void)
{
    struct ClipDevice *c=AllocVec(sizeof(*c),MEMF_PUBLIC|MEMF_CLEAR);
    if(!c)return NULL;
    c->seen=-1;c->port=CreateMsgPort();
    if(c->port)c->io=(struct IOClipReq *)CreateIORequest(c->port,sizeof(*c->io));
    if(c->io && !OpenDevice("clipboard.device",0,(struct IORequest *)c->io,0))c->opened=1;
    return c;
}
static void submit(struct ClipDevice *c,int state,UWORD command,void *data,ULONG length)
{
    c->state=state;c->pending=1;c->age=0;
    c->io->io_Error=0;c->io->io_Actual=0;
    c->io->io_Command=command;c->io->io_Data=data;c->io->io_Length=length;
    SendIO((struct IORequest *)c->io);
}
static void close_clip(struct ClipDevice *c)
{
    if(!c)return;
    if(c->pending){
        /* Clipboard reads must end beyond EOF, and writes with UPDATE.
         * GOODBYE has already detached us from the dock: finish this owned
         * transfer here even if a posted owner replies late. Abandoning a
         * read would leave the system clipboard locked after its reply. */
        WaitIO((struct IORequest *)c->io);c->pending=0;
        if(c->state==CL_READ || c->state==CL_FINISH){
            if(c->io->io_Actual){
                do {
                    c->io->io_Error=0;c->io->io_Command=CMD_READ;
                    c->io->io_Data=NULL;c->io->io_Length=0x7fffffffUL;
                    DoIO((struct IORequest *)c->io);
                } while(!c->io->io_Error && c->io->io_Actual);
            }
        }else if(c->state==CL_WRITE && !c->io->io_Error){
            c->io->io_Error=0;c->io->io_Command=CMD_UPDATE;DoIO((struct IORequest *)c->io);
        }
    }
    if(c->opened)CloseDevice((struct IORequest *)c->io);
    if(c->io)DeleteIORequest((struct IORequest *)c->io);
    if(c->port)DeleteMsgPort(c->port);FreeVec(c);
}
static int service_clip(struct ClipDevice *c,struct NDClipHistory *history,
                        int paused,unsigned int elapsed,char *notice)
{
    char *text;int decoded,changed=0,state;
    if(!c || !c->opened)return 0;
    if(c->pending){
        c->age+=elapsed;if(!CheckIO((struct IORequest *)c->io))return 0;
        WaitIO((struct IORequest *)c->io);c->pending=0;state=c->state;
        if(c->io->io_Error)c->error=c->io->io_Error;
        if(state==CL_READ){
            c->size=c->io->io_Actual;
            /* An empty read already ends the transaction. Do not seek far
             * beyond EOF: use the device's documented null-buffer drain. */
            if(c->size){submit(c,CL_FINISH,CMD_READ,NULL,0x7fffffffUL);return 0;}
            c->state=0;
            /* An empty/unavailable initial clip and an obsolete ID during
             * concurrent copying are not failures of clipboard history. */
            return 0;
        }
        if(state==CL_FINISH){
            if(!c->io->io_Error && c->io->io_Actual){
                submit(c,CL_FINISH,CMD_READ,NULL,0x7fffffffUL);return 0;
            }
            c->state=0;
            if(c->error && c->error!=CBERR_OBSOLETEID){strcpy(notice,"READ ERROR");return 0;}
            text=malloc(ND_CLIP_TEXT_MAX+1);
            if(!text){strcpy(notice,"NO MEMORY");return 0;}
            decoded=nd_clip_decode(c->raw,c->size,text,ND_CLIP_TEXT_MAX+1);
            if(decoded==1 && !paused && !c->discard)changed=nd_clip_history_add(history,text);
            else if(decoded==-2)strcpy(notice,"TEXT TOO LARGE");
            else if(decoded<0)strcpy(notice,"SKIPPED CLIP");
            free(text);
        }else if(state==CL_WRITE){
            if(!c->error){submit(c,CL_UPDATE,CMD_UPDATE,NULL,0);return 0;}
            strcpy(notice,"COPY FAILED");
        }else if(state==CL_UPDATE){
            c->seen=c->io->io_ClipID;
            strcpy(notice,c->error ? "COPY FAILED" : "COPIED");
        }
        c->state=0;
    }
    if(!paused && !c->pending){
        c->io->io_Error=0;c->io->io_Command=CBD_CURRENTREADID;
        if(!DoIO((struct IORequest *)c->io) && c->io->io_ClipID>0 && c->io->io_ClipID!=c->seen){
            /* Remember the attempted ID even if a read reports no data.
             * Retrying the same unavailable clip would repeat the notice. */
            c->seen=c->io->io_ClipID;c->error=0;c->discard=0;c->io->io_ClipID=0;c->io->io_Offset=0;
            submit(c,CL_READ,CMD_READ,c->raw,sizeof(c->raw));
        }
    }
    return changed;
}
static int restore_clip(struct ClipDevice *c,const char *text)
{
    unsigned long size;
    if(!c || !c->opened || c->pending)return 0;
    size=nd_clip_encode(text,c->raw,sizeof(c->raw));if(!size)return 0;
    c->io->io_ClipID=0;c->io->io_Offset=0;c->error=0;
    submit(c,CL_WRITE,CMD_WRITE,c->raw,size);return 1;
}
static void init_list(struct List *list)
{list->lh_Head=(struct Node *)&list->lh_Tail;list->lh_Tail=NULL;list->lh_TailPred=(struct Node *)&list->lh_Head;}
static int selected_row(struct ClipPanel *p)
{
    ULONG selected=~0UL;
    if(p->win)GT_GetGadgetAttrs(p->list,p->win,NULL,GTLV_Selected,(ULONG)&selected,TAG_DONE);
    return selected==~0UL || selected>=(ULONG)p->visible ? -1 : p->map[selected];
}
static void refresh_panel(struct ClipPanel *p,const struct NDClipHistory *h)
{
    unsigned int i;int row=-1;char preview[80];
    if(!p->win)return;
    GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,(ULONG)~0UL,TAG_DONE);
    init_list(&p->labels);p->visible=0;
    for(i=0;i<h->count;++i){
        if(!nd_search_contains(h->text[i],p->query))continue;
        p->map[p->visible]=(int)i;if(p->selected==(int)i)row=p->visible;
        ++p->visible;
        nd_clip_preview(preview,64,h->text[i]);
        sprintf(p->names[i],"%s%2u  %s",h->pinned[i] ? "* " : "  ",i+1,preview);
        p->nodes[i].ln_Name=p->names[i];AddTail(&p->labels,&p->nodes[i]);
    }
    if(!p->visible){
        strcpy(p->names[0],h->count ? "No matching clips" : "Clipboard history is empty");
        p->nodes[0].ln_Name=p->names[0];AddTail(&p->labels,&p->nodes[0]);
    }
    if(p->selected<0 || (unsigned int)p->selected>=h->count)p->selected=-1;
    GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,(ULONG)&p->labels,
                      GTLV_Selected,row<0 ? (ULONG)~0UL : (ULONG)row,TAG_DONE);
}
static void close_panel(struct ClipPanel *p)
{
    if(p->win)CloseWindow(p->win);
    if(p->gadgets)FreeGadgets(p->gadgets);
    if(p->vi)FreeVisualInfo(p->vi);
    if(p->screen)UnlockPubScreen(NULL,p->screen);
    memset(p,0,sizeof(*p));p->selected=-1;
}
static int open_panel(struct ClipPanel *p,const struct NDClipHistory *h,int paused)
{
    static const char *buttons[]={"Copy","Delete","Clear","Pause","Close","Pin"};
    struct NewGadget ng;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};
    int w,height,list_height,i,bw,y;
    if(p->win){WindowToFront(p->win);ActivateWindow(p->win);return 1;}
    p->screen=LockPubScreen("Workbench");if(!p->screen)return 0;
    w=p->screen->Width-20;if(w>560)w=560;
    height=p->screen->Height-p->screen->BarHeight-14;if(height>236)height=236;
    if(w<440 || height<170)goto fail;
    p->vi=GetVisualInfo(p->screen,TAG_DONE);if(!p->vi)goto fail;
    p->last=CreateContext(&p->gadgets);memset(&ng,0,sizeof(ng));
    ng.ng_VisualInfo=p->vi;ng.ng_TextAttr=&font;ng.ng_LeftEdge=12;ng.ng_TopEdge=28;
    ng.ng_Width=w-24;ng.ng_Height=14;ng.ng_GadgetText="Search + Return; * pinned. Copy, then Amiga-V";
    p->last=CreateGadget(TEXT_KIND,p->last,&ng,TAG_DONE);if(!p->last)goto fail;
    ng.ng_TopEdge=46;ng.ng_Height=14;ng.ng_GadgetID=8;ng.ng_GadgetText="";
    p->search=p->last=CreateGadget(STRING_KIND,p->last,&ng,GTST_String,(ULONG)p->query,GTST_MaxChars,63,TAG_DONE);
    if(!p->last)goto fail;
    ng.ng_TopEdge=66;list_height=height-110;ng.ng_Height=list_height;ng.ng_GadgetID=1;
    init_list(&p->labels);
    p->list=p->last=CreateGadget(LISTVIEW_KIND,p->last,&ng,GTLV_Labels,(ULONG)&p->labels,
                                GTLV_ShowSelected,NULL,GTLV_ScrollWidth,16,TAG_DONE);
    if(!p->last)goto fail;
    y=height-30;bw=(w-24-5*6)/6;
    for(i=0;i<6;++i){
        ng.ng_LeftEdge=12+i*(bw+6);ng.ng_TopEdge=y;ng.ng_Width=bw;ng.ng_Height=18;
        ng.ng_GadgetID=i+2;ng.ng_GadgetText=(STRPTR)(i==3 && paused ? "Resume" : buttons[i]);
        p->last=CreateGadget(BUTTON_KIND,p->last,&ng,TAG_DONE);if(!p->last)goto fail;
        if(i==3)p->pause=p->last;
    }
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,WA_Title,(ULONG)"NovaDock Clipboard History",
        WA_Left,(p->screen->Width-w)/2,WA_Top,p->screen->BarHeight+4,WA_Width,w,WA_Height,height,
        WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Gadgets,(ULONG)p->gadgets,TAG_DONE);
    if(!p->win)goto fail;p->selected=h->count ? 0 : -1;GT_RefreshWindow(p->win,NULL);refresh_panel(p,h);return 1;
fail:close_panel(p);return 0;
}
static int panel_events(struct ClipPanel *p,struct NDClipHistory *h,struct ClipDevice *c,int *paused,char *notice)
{
    struct IntuiMessage *msg;ULONG cls;UWORD id,code;int changed=0,close=0;
    if(!p->win)return 0;
    while((msg=GT_GetIMsg(p->win->UserPort))!=NULL){
        cls=msg->Class;code=msg->Code;id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID : 0;
        GT_ReplyIMsg(msg);
        if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45))close=1;
        else if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);}
        else if(cls==IDCMP_GADGETUP){
            p->selected=selected_row(p);
            if(id==8){
                STRPTR query=NULL;
                GT_GetGadgetAttrs(p->search,p->win,NULL,GTST_String,(ULONG)&query,TAG_DONE);
                if(query){strncpy(p->query,query,sizeof(p->query)-1);p->query[sizeof(p->query)-1]=0;}
                changed=1;
            }
            else if(id==7 && p->selected>=0)changed|=nd_clip_history_pin(h,(unsigned int)p->selected);
            else if(id==1)p->selected=code<(UWORD)p->visible ? p->map[code] : -1;
            else if(id==2 && p->selected>=0 && (unsigned int)p->selected<h->count){
                if(restore_clip(c,h->text[p->selected])){strcpy(notice,"COPYING");close=1;}
                else {strcpy(notice,"CLIPBOARD BUSY");DisplayBeep(p->screen);}
            }else if(id==3 && p->selected>=0){changed|=nd_clip_history_delete(h,(unsigned int)p->selected);}
            else if(id==4){nd_clip_history_clear(h);if(c)c->discard=1;changed=1;}
            else if(id==5){*paused=!*paused;close=1;}
            else if(id==6)close=1;
        }
    }
    if(close)close_panel(p);else if(changed)refresh_panel(p,h);return changed;
}
int main(int argc,char **argv)
{
    struct NDClient client;struct NDClipHistory history;struct ClipOptions options;
    struct ClipDevice *device;struct ClipPanel *panel;struct NDClipQuick *quick;const char *chosen;
    char value[32],badge[12],notice[32];unsigned int event,elapsed=0,notice_age=0;
    unsigned int poll_age=50,i;int quit=0,reload=1,paused=0,changed;const char *selected_text;
    if(argc<2 || !nd_client_init(&client,argv[1]))return 20;
    nd_clip_history_init(&history,10);notice[0]=0;
    panel=AllocVec(sizeof(*panel),MEMF_PUBLIC|MEMF_CLEAR);device=open_clip();quick=nd_clip_quick_create();
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    while(!quit){
        if(reload){
            options.limit=10;options.badge=1;
            if(nd_plugin_options_effective(argv[1],option,&options)!=1){options.limit=10;options.badge=1;}
            nd_clip_history_limit(&history,options.limit);reload=0;
            if(panel && panel->win)refresh_panel(panel,&history);
        }
        if(notice_age>elapsed)notice_age-=elapsed;else {notice_age=0;notice[0]=0;}
        changed=0;poll_age+=elapsed;
        selected_text=NULL;
        if(panel && panel->win){
            panel->selected=selected_row(panel);
            if(panel->selected>=0 && (unsigned int)panel->selected<history.count)
                selected_text=history.text[panel->selected];
        }
        if((device && device->pending) || poll_age>=50){
            changed=service_clip(device,&history,paused,elapsed,notice);poll_age=0;
        }
        if(panel && panel->win && changed){
            panel->selected=-1;
            for(i=0;i<history.count;++i)if(history.text[i]==selected_text)panel->selected=(int)i;
            refresh_panel(panel,&history);
        }
        if(panel && panel->win)changed|=panel_events(panel,&history,device,&paused,notice);
        chosen=nd_clip_quick_events(quick);
        if(chosen){
            if(restore_clip(device,chosen)){strcpy(notice,"COPYING");nd_clip_quick_close(quick);}
            else strcpy(notice,"CLIPBOARD BUSY");
        }
        if(notice[0] && !notice_age)notice_age=150;
        if(!device || !device->opened)strcpy(value,"UNAVAILABLE");
        else if(notice[0])strcpy(value,notice);
        else if(paused)strcpy(value,"PAUSED");
        else if(device->pending && device->age>=250)strcpy(value,"CLIPBOARD BUSY");
        else if(!history.count)strcpy(value,"EMPTY");
        else sprintf(value,"%u %s",history.count,history.count==1 ? "CLIP" : "CLIPS");
        event=nd_client_update_quiet(&client,"CLIPPY",value,ND_NO_METER,elapsed,500);
        if(!(event&ND_EVENT_QUIT)){
            if(options.badge && history.count)sprintf(badge,"%u",history.count);else badge[0]=0;
            event|=nd_client_set_badge(&client,badge);
        }
        if(event&ND_EVENT_QUIT)quit=1;
        if(event&ND_EVENT_SETTINGS)reload=1;
        if(!quit && (event&ND_EVENT_MANAGE_HISTORY)){
            nd_clip_quick_close(quick);
            if(!panel || !IntuitionBase || !GfxBase || !GadToolsBase || !open_panel(panel,&history,paused)){
                strcpy(notice,"CANNOT OPEN HISTORY");notice_age=150;
            }
        }else if(!quit && (event&ND_EVENT_CLICK)){
            if(panel)close_panel(panel);
            if(!quick || !IntuitionBase || !GfxBase || !nd_clip_quick_open(quick,&history)){
                strcpy(notice,"CANNOT OPEN HISTORY");notice_age=150;
            }
        }
        if(!quit)elapsed=nd_client_wait(&client,(panel && panel->win) || nd_clip_quick_active(quick) ? 5 : device && device->pending ? 5 : 50);
    }
    nd_clip_quick_free(quick);
    if(panel){close_panel(panel);FreeVec(panel);}
    nd_clip_history_clear(&history);nd_client_close(&client);close_clip(device);
    if(GadToolsBase)CloseLibrary(GadToolsBase);
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
