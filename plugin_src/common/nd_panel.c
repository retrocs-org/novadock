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
#include <stdlib.h>
#include "nd_panel.h"
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct GfxBase *GfxBase=NULL;
static unsigned int panel_users;
struct NDPanel {
    struct Screen *screen;struct Window *window;
    struct Gadget *gadgets,*fields[12];APTR visual;
    UWORD background;
    unsigned char percentage[12],percent_dragging[12];
    unsigned int count,button_count,limits[12];struct Gadget *buttons[5];char button_text[5][32];
    LONG swatch_pen[12];unsigned long swatch_rgb[12];unsigned char swatch[12],swatch_valid[12];
    char title[128],text[12][160],labels[12][32];
};
void nd_panel_close(struct NDPanel *p)
{
    if(!p)return;
    {
        {unsigned int i;for(i=0;i<p->count;++i)if(p->swatch_valid[i])ReleasePen(p->screen->ViewPort.ColorMap,p->swatch_pen[i]);}
        if(p->window)CloseWindow(p->window);
        if(p->gadgets)FreeGadgets(p->gadgets);
        if(p->visual)FreeVisualInfo(p->visual);
        if(p->screen)UnlockPubScreen(NULL,p->screen);
        FreeVec(p);
    }
    if(--panel_users)return;
    if(GadToolsBase){CloseLibrary(GadToolsBase);GadToolsBase=NULL;}
    if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}
    if(IntuitionBase){CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}
}
struct NDPanel *nd_panel_open(const char *title,const struct NDPanelField *fields,
                              unsigned int count,const char **buttons,unsigned int buttons_count)
{
    struct NDPanel *p;struct Gadget *last;
    struct NewGadget ng;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};
    unsigned int i;int w,top,bw,label_x=92;
    if(!count || count>12 || buttons_count>5 || !buttons_count)return NULL;
    p=(struct NDPanel *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);if(!p)return NULL;
    if(!panel_users++){
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    }
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
    for(i=0;i<count;++i){int needed=(int)strlen(fields[i].label)*8+20;if(needed>label_x)label_x=needed;}
    if(label_x>w/2)label_x=w/2;
    for(i=0;i<count;++i){
        ng.ng_LeftEdge=label_x;ng.ng_TopEdge=top+(int)i*20;ng.ng_Width=w-label_x-16;ng.ng_Height=16;
        strncpy(p->labels[i],fields[i].label,sizeof(p->labels[i])-1);
        ng.ng_GadgetText=p->labels[i];ng.ng_Flags=PLACETEXT_LEFT;ng.ng_GadgetID=i+1;
        p->limits[i]=fields[i].limit;
        if(fields[i].limit==ND_PANEL_SWATCH){p->swatch[i]=1;last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)"",TAG_DONE);}
        else if(fields[i].limit==ND_PANEL_PERCENT){
            int level=atoi(fields[i].value);if(level<0)level=0;if(level>100)level=100;
            p->percentage[i]=1;ng.ng_Width-=56;
            last=CreateGadget(SLIDER_KIND,last,&ng,GTSL_Min,0,GTSL_Max,100,GTSL_Level,level,
                GTSL_LevelFormat,(ULONG)"%ld%%",GTSL_MaxLevelLen,4,GTSL_LevelPlace,PLACETEXT_RIGHT,
                PGA_Freedom,LORIENT_HORIZ,GA_Immediate,TRUE,GA_RelVerify,TRUE,TAG_DONE);
        }else if(fields[i].limit){
            strncpy(p->text[i],fields[i].value,sizeof(p->text[i])-1);
            last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)fields[i].value,
                             GTST_MaxChars,fields[i].limit-1,TAG_DONE);
        }
        else {
            strncpy(p->text[i],fields[i].value,sizeof(p->text[i])-1);
            last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)p->text[i],GTTX_Clipped,TRUE,TAG_DONE);
        }
        if(!last)goto fail;p->fields[i]=last;
    }
    p->button_count=buttons_count;
    bw=(w-32)/(int)buttons_count;
    for(i=0;i<buttons_count;++i){
        ng.ng_LeftEdge=12+(int)i*bw;ng.ng_TopEdge=top+(int)count*20+6;
        ng.ng_Width=bw-6;ng.ng_Height=20;ng.ng_GadgetID=100+i;
        strncpy(p->button_text[i],buttons[i],31);ng.ng_GadgetText=p->button_text[i];ng.ng_Flags=PLACETEXT_IN;
        last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto fail;p->buttons[i]=last;
    }
    p->window=OpenWindowTags(NULL,WA_Title,(ULONG)p->title,WA_PubScreen,(ULONG)p->screen,
        WA_Left,(p->screen->Width-w)/2,WA_Top,p->screen->BarHeight+10,WA_Width,w,
        WA_Height,ng.ng_TopEdge+ng.ng_Height+12,
        WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY|SLIDERIDCMP,
        WA_Gadgets,(ULONG)p->gadgets,TAG_DONE);
    if(!p->window)goto fail;GT_RefreshWindow(p->window,NULL);
    if(fields[0].limit && fields[0].limit!=ND_PANEL_PERCENT)ActivateGadget(p->fields[0],p->window,NULL);return p;
fail:nd_panel_close(p);return NULL;
}
static void draw_swatches(struct NDPanel *p)
{unsigned int i;struct RastPort *rp=p->window->RPort;UWORD pen=rp->FgPen;for(i=0;i<p->count;++i)if(p->swatch_valid[i]){struct Gadget *g=p->fields[i];SetAPen(rp,p->swatch_pen[i]);RectFill(rp,g->LeftEdge+2,g->TopEdge+2,g->LeftEdge+g->Width-3,g->TopEdge+g->Height-3);}SetAPen(rp,pen);}
void nd_panel_swatch(struct NDPanel *p,unsigned int index,unsigned long rgb)
{LONG pen;if(!p||index>=p->count||!p->swatch[index]||(p->swatch_valid[index]&&p->swatch_rgb[index]==rgb))return;pen=ObtainBestPen(p->screen->ViewPort.ColorMap,((rgb>>16)&255)*0x01010101UL,((rgb>>8)&255)*0x01010101UL,(rgb&255)*0x01010101UL,TAG_DONE);if(pen<0)return;if(p->swatch_valid[index])ReleasePen(p->screen->ViewPort.ColorMap,p->swatch_pen[index]);p->swatch_pen[index]=pen;p->swatch_rgb[index]=rgb;p->swatch_valid[index]=1;draw_swatches(p);}
int nd_panel_replace_string(struct NDPanel *p,unsigned int index,const char *value)
{if(!p||index>=p->count||!p->limits[index]||p->limits[index]>=ND_PANEL_SWATCH||!value||strlen(value)>=p->limits[index])return 0;if(strcmp(nd_panel_get(p,index),value))GT_SetGadgetAttrs(p->fields[index],p->window,NULL,GTST_String,(ULONG)value,TAG_DONE);strncpy(p->text[index],value,sizeof(p->text[index])-1);p->text[index][sizeof(p->text[index])-1]=0;return 1;}
void nd_panel_button(struct NDPanel *p,unsigned int index,const char *label,int enabled)
{struct Gadget *g;if(!p||index>=p->button_count||!label)return;g=p->buttons[index];if(strcmp(p->button_text[index],label)){strncpy(p->button_text[index],label,31);p->button_text[index][31]=0;if(g->GadgetText){g->GadgetText->IText=p->button_text[index];g->GadgetText->LeftEdge=(g->Width-(int)strlen(p->button_text[index])*8)/2;}RefreshGList(g,p->window,NULL,1);}if(((g->Flags&GFLG_DISABLED)!=0)!=(!enabled))GT_SetGadgetAttrs(g,p->window,NULL,GA_Disabled,(ULONG)!enabled,TAG_DONE);}
void nd_panel_raise(struct NDPanel *p)
{if(p && p->window){WindowToFront(p->window);ActivateWindow(p->window);}}
int nd_panel_event(struct NDPanel *p)
{
    struct IntuiMessage *msg;ULONG cls;UWORD code,id;
    if(!p)return 0;
    while((msg=GT_GetIMsg(p->window->UserPort))!=NULL){
        cls=msg->Class;code=msg->Code;
        id=(cls==IDCMP_GADGETUP || cls==IDCMP_GADGETDOWN) && msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID:0;
        GT_ReplyIMsg(msg);
        if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45))return -1;
        if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p->window);GT_EndRefresh(p->window,TRUE);draw_swatches(p);}
        if(id && id<=p->count && p->percentage[id-1]){
            if(cls==IDCMP_GADGETDOWN){p->percent_dragging[id-1]=1;continue;}
            p->percent_dragging[id-1]=0;
        }else if(cls==IDCMP_GADGETDOWN)continue;
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
{if(IntuitionBase)DisplayBeep(p ? p->screen:NULL);else{struct IntuitionBase *temporary=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(temporary){IntuitionBase=temporary;DisplayBeep(NULL);IntuitionBase=NULL;CloseLibrary((struct Library *)temporary);}}}

unsigned long nd_panel_signals(const struct NDPanel *p)
{return p && p->window ? 1UL<<p->window->UserPort->mp_SigBit:0;}

int nd_panel_percent_get(struct NDPanel *p,unsigned int index)
{
 ULONG level=0;if(!p || index>=p->count || !p->percentage[index])return -1;
 GT_GetGadgetAttrs(p->fields[index],p->window,NULL,GTSL_Level,(ULONG)&level,TAG_DONE);
 return level<=100 ? (int)level:-1;
}
void nd_panel_percent_set(struct NDPanel *p,unsigned int index,int value,int enabled)
{
 if(!p || index>=p->count || !p->percentage[index])return;
 if(value>100)value=100;
 if(!enabled)p->percent_dragging[index]=0;
 /* Never reset the knob while its user is dragging it. */
 if(value>=0 && !p->percent_dragging[index] && !(p->fields[index]->Flags&GFLG_SELECTED))
  GT_SetGadgetAttrs(p->fields[index],p->window,NULL,GTSL_Level,value,GA_Disabled,(ULONG)!enabled,TAG_DONE);
 else GT_SetGadgetAttrs(p->fields[index],p->window,NULL,GA_Disabled,(ULONG)!enabled,TAG_DONE);
}

void nd_panel_enable(struct NDPanel *p,unsigned int index,int enabled)
{if(p && index<p->count && ((p->fields[index]->Flags&GFLG_DISABLED)!=0)!=(!enabled))GT_SetGadgetAttrs(p->fields[index],p->window,NULL,GA_Disabled,(ULONG)!enabled,TAG_DONE);}

void nd_panel_sync_string(struct NDPanel *p,unsigned int index,const char *value)
{const char *current;if(!p || index>=p->count || !value || strlen(value)>=sizeof(p->text[index]) || (p->fields[index]->Flags&GFLG_SELECTED))return;current=nd_panel_get(p,index);if(strcmp(current,p->text[index]))return;if(strcmp(current,value))GT_SetGadgetAttrs(p->fields[index],p->window,NULL,GTST_String,(ULONG)value,TAG_DONE);strcpy(p->text[index],value);}
void nd_panel_commit_string(struct NDPanel *p,unsigned int index)
{const char *current;if(!p || index>=p->count)return;current=nd_panel_get(p,index);if(strlen(current)<sizeof(p->text[index]))strcpy(p->text[index],current);}
