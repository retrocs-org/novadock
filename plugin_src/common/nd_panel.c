#define IntuitionBase NDPanelIntuitionBase
#define GadToolsBase NDPanelGadToolsBase
#define GfxBase NDPanelGfxBase
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <string.h>
#include "nd_panel.h"
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct GfxBase *GfxBase=NULL;
struct NDPanel {
    struct Screen *screen;struct Window *window;
    struct Gadget *gadgets,*fields[12];APTR visual;
    UWORD background;
    unsigned int count;char title[128],text[12][160],labels[12][32];
};
void nd_panel_close(struct NDPanel *p)
{
    if(p){
        if(p->window)CloseWindow(p->window);
        if(p->gadgets)FreeGadgets(p->gadgets);
        if(p->visual)FreeVisualInfo(p->visual);
        if(p->screen)UnlockPubScreen(NULL,p->screen);
        FreeVec(p);
    }
    if(GadToolsBase){CloseLibrary(GadToolsBase);GadToolsBase=NULL;}
    if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}
    if(IntuitionBase){CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}
}
struct NDPanel *nd_panel_open(const char *title,const struct NDPanelField *fields,
                              unsigned int count,const char **buttons,unsigned int buttons_count)
{
    struct NDPanel *p;struct Gadget *last;
    struct NewGadget ng;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};
    unsigned int i;int w,top,bw;
    if(!count || count>12 || buttons_count>4 || !buttons_count)return NULL;
    p=(struct NDPanel *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);if(!p)return NULL;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    if(!IntuitionBase || !GadToolsBase || !GfxBase)goto fail;
    p->screen=LockPubScreen("Workbench");if(!p->screen)goto fail;
    {struct DrawInfo *info=GetScreenDrawInfo(p->screen);
     if(info){p->background=info->dri_Pens[BACKGROUNDPEN];FreeScreenDrawInfo(p->screen,info);}}
    w=p->screen->Width-24;if(w>560)w=560;if(w<280)goto fail;
    p->visual=GetVisualInfo(p->screen,TAG_DONE);if(!p->visual)goto fail;
    p->count=count;strncpy(p->title,title,sizeof(p->title)-1);
    last=CreateContext(&p->gadgets);if(!last)goto fail;
    memset(&ng,0,sizeof(ng));ng.ng_VisualInfo=p->visual;ng.ng_TextAttr=&font;
    top=p->screen->WBorTop+p->screen->Font->ta_YSize+10;
    if(top+(int)count*20+40>p->screen->Height-p->screen->BarHeight-10)goto fail;
    for(i=0;i<count;++i){
        ng.ng_LeftEdge=92;ng.ng_TopEdge=top+(int)i*20;ng.ng_Width=w-108;ng.ng_Height=16;
        strncpy(p->labels[i],fields[i].label,sizeof(p->labels[i])-1);
        ng.ng_GadgetText=p->labels[i];ng.ng_Flags=PLACETEXT_LEFT;ng.ng_GadgetID=i+1;
        if(fields[i].limit)
            last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)fields[i].value,
                             GTST_MaxChars,fields[i].limit-1,TAG_DONE);
        else {
            strncpy(p->text[i],fields[i].value,sizeof(p->text[i])-1);
            last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)p->text[i],TAG_DONE);
        }
        if(!last)goto fail;p->fields[i]=last;
    }
    bw=(w-32)/(int)buttons_count;
    for(i=0;i<buttons_count;++i){
        ng.ng_LeftEdge=12+(int)i*bw;ng.ng_TopEdge=top+(int)count*20+6;
        ng.ng_Width=bw-6;ng.ng_Height=20;ng.ng_GadgetID=100+i;
        ng.ng_GadgetText=(STRPTR)buttons[i];ng.ng_Flags=PLACETEXT_IN;
        last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto fail;
    }
    p->window=OpenWindowTags(NULL,WA_Title,(ULONG)p->title,WA_PubScreen,(ULONG)p->screen,
        WA_Left,(p->screen->Width-w)/2,WA_Top,p->screen->BarHeight+10,WA_Width,w,
        WA_Height,ng.ng_TopEdge+ng.ng_Height+12,
        WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Gadgets,(ULONG)p->gadgets,TAG_DONE);
    if(!p->window)goto fail;GT_RefreshWindow(p->window,NULL);
    if(fields[0].limit)ActivateGadget(p->fields[0],p->window,NULL);return p;
fail:nd_panel_close(p);return NULL;
}
void nd_panel_raise(struct NDPanel *p)
{if(p && p->window){WindowToFront(p->window);ActivateWindow(p->window);}}
int nd_panel_event(struct NDPanel *p)
{
    struct IntuiMessage *msg;ULONG cls;UWORD code,id;
    if(!p)return 0;
    while((msg=GT_GetIMsg(p->window->UserPort))!=NULL){
        cls=msg->Class;code=msg->Code;
        id=cls==IDCMP_GADGETUP ? ((struct Gadget *)msg->IAddress)->GadgetID:0;
        GT_ReplyIMsg(msg);
        if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45))return -1;
        if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p->window);GT_EndRefresh(p->window,TRUE);}
        if(id)return (int)id;
    }
    return 0;
}
const char *nd_panel_get(struct NDPanel *p,unsigned int index)
{return p && index<p->count && p->fields[index]->SpecialInfo ?
    ((struct StringInfo *)p->fields[index]->SpecialInfo)->Buffer:"";}
void nd_panel_set_label(struct NDPanel *p,unsigned int index,const char *label)
{
    struct Gadget *g;struct RastPort *rp;UWORD pen,mode;
    if(!p || index>=p->count || !label || !strcmp(p->labels[index],label))return;
    strncpy(p->labels[index],label,sizeof(p->labels[index])-1);
    p->labels[index][sizeof(p->labels[index])-1]=0;
    g=p->fields[index];rp=p->window->RPort;pen=rp->FgPen;mode=rp->DrawMode;
    SetDrMd(rp,JAM1);SetAPen(rp,p->background);
    RectFill(rp,4,g->TopEdge,g->LeftEdge-5,g->TopEdge+g->Height-1);
    SetAPen(rp,pen);SetDrMd(rp,mode);
    RefreshGList(g,p->window,NULL,1);
}
void nd_panel_set(struct NDPanel *p,unsigned int index,const char *text)
{if(p && index<p->count && strcmp(p->text[index],text)){
    strncpy(p->text[index],text,sizeof(p->text[index])-1);p->text[index][sizeof(p->text[index])-1]=0;
    GT_SetGadgetAttrs(p->fields[index],p->window,NULL,GTTX_Text,(ULONG)p->text[index],TAG_DONE);
}}
void nd_panel_title(struct NDPanel *p,const char *title)
{if(p && strcmp(p->title,title)){strncpy(p->title,title,sizeof(p->title)-1);p->title[sizeof(p->title)-1]=0;SetWindowTitles(p->window,p->title,(STRPTR)-1);}}
void nd_panel_beep(struct NDPanel *p)
{if(IntuitionBase)DisplayBeep(p ? p->screen:NULL);}

unsigned long nd_panel_signals(const struct NDPanel *p)
{return p && p->window ? 1UL<<p->window->UserPort->mp_SigBit:0;}
