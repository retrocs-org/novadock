/* Native GadTools browser: listings, categories, details and full readmes. */
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <graphics/text.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;
struct Library *GadToolsBase=NULL,*AslBase=NULL;
#define AM_ROWS 512
struct AMWindow {struct Screen *screen;struct Window *window;struct Gadget *gadgets,*list,*query;struct TextFont *font;APTR visual;struct List labels;struct Node nodes[AM_ROWS];char rows[AM_ROWS][160];int count,selected,width,height,download_active;unsigned long received,total;char query_text[81],status[128];};
static struct AMWindow *ui;
static void am_download_state(int active,unsigned long received,unsigned long total)
{if(ui){ui->download_active=active;ui->received=received;ui->total=total;}}
static void am_status(const char *text)
{
 struct RastPort *rp;struct DrawInfo *di;struct TextExtent fit;ULONG n;int fill;UWORD bg=0,fg=1,accent=1;
 if(!ui||!ui->window)return;if(text!=ui->status)strncpy(ui->status,text,127);ui->status[127]=0;rp=ui->window->RPort;SetFont(rp,ui->font);
 di=GetScreenDrawInfo(ui->screen);if(di){bg=di->dri_Pens[BACKGROUNDPEN];fg=di->dri_Pens[TEXTPEN];accent=di->dri_Pens[FILLPEN];FreeScreenDrawInfo(ui->screen,di);}
 SetDrMd(rp,JAM1);SetAPen(rp,bg);RectFill(rp,8,ui->height-37,ui->width-9,ui->height-7);
 if(ui->download_active){
  SetAPen(rp,fg);RectFill(rp,10,ui->height-35,ui->width-11,ui->height-25);
  SetAPen(rp,bg);RectFill(rp,11,ui->height-34,ui->width-12,ui->height-26);
  fill=ui->total ? (int)((ui->width-24)*(unsigned long)nd_aminet_percent(ui->received,ui->total)/100UL):0;
  if(fill){SetAPen(rp,accent);RectFill(rp,12,ui->height-33,11+fill,ui->height-27);}
 }SetAPen(rp,fg);
 n=TextFit(rp,ui->status,(ULONG)strlen(ui->status),&fit,NULL,1,ui->width-20,12);Move(rp,10,ui->height-11);Text(rp,ui->status,n);
}
static void am_detach(void){if(ui)GT_SetGadgetAttrs(ui->list,ui->window,NULL,GTLV_Labels,~0UL,TAG_DONE);}
static void am_clear(void){am_detach();NewList(&ui->labels);ui->count=0;ui->selected=-1;}
static void am_row(const char *text)
{int n=ui->count;if(n>=AM_ROWS)return;strncpy(ui->rows[n],text,159);ui->rows[n][159]=0;ui->nodes[n].ln_Name=ui->rows[n];AddTail(&ui->labels,&ui->nodes[n]);++ui->count;}
static void am_attach(void){GT_SetGadgetAttrs(ui->list,ui->window,NULL,GTLV_Labels,(ULONG)&ui->labels,GTLV_Top,0,GTLV_Selected,~0UL,TAG_DONE);}
static void am_close(void)
{if(!ui)return;if(ui->window)CloseWindow(ui->window);if(ui->gadgets)FreeGadgets(ui->gadgets);if(ui->visual)FreeVisualInfo(ui->visual);if(ui->font)CloseFont(ui->font);if(ui->screen)UnlockPubScreen(NULL,ui->screen);FreeVec(ui);ui=NULL;}
static int am_open(void)
{
 struct Gadget *last;struct NewGadget ng;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};int top,i,gap=6;static const char *nav[]={"Recent","Categories","Back"},*buttons[]={"Open","Readme","Get","Close"};
 if(ui){WindowToFront(ui->window);ActivateWindow(ui->window);return 1;}
 ui=AllocVec(sizeof(*ui),MEMF_PUBLIC|MEMF_CLEAR);if(!ui)return 0;ui->selected=-1;NewList(&ui->labels);ui->font=OpenFont(&font);if(!ui->font)goto fail;
 ui->screen=LockPubScreen("Workbench");if(!ui->screen)goto fail;ui->width=ui->screen->Width-12;if(ui->width>740)ui->width=740;ui->height=ui->screen->Height-ui->screen->BarHeight-8;if(ui->height>460)ui->height=460;if(ui->width<300||ui->height<208)goto fail;
 ui->visual=GetVisualInfo(ui->screen,TAG_DONE);if(!ui->visual)goto fail;last=CreateContext(&ui->gadgets);if(!last)goto fail;
 top=ui->screen->WBorTop+ui->screen->Font->ta_YSize+7;memset(&ng,0,sizeof(ng));ng.ng_TextAttr=&font;ng.ng_VisualInfo=ui->visual;ng.ng_Height=18;ng.ng_TopEdge=top;ng.ng_LeftEdge=10;ng.ng_Width=ui->width-80;ng.ng_GadgetID=1;
 last=ui->query=CreateGadget(STRING_KIND,last,&ng,GTST_MaxChars,80,GTST_String,(ULONG)"",TAG_DONE);if(!last)goto fail;
 ng.ng_LeftEdge=ui->width-64;ng.ng_Width=54;ng.ng_GadgetID=2;ng.ng_GadgetText="Search";last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto fail;
 for(i=0;i<3;++i){ng.ng_LeftEdge=10+i*(ui->width-20)/3;ng.ng_Width=(ui->width-32)/3;ng.ng_TopEdge=top+24;ng.ng_GadgetID=3+i;ng.ng_GadgetText=(STRPTR)nav[i];last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto fail;}
 ng.ng_LeftEdge=10;ng.ng_TopEdge=top+49;ng.ng_Width=ui->width-20;ng.ng_Height=ui->height-top-139;ng.ng_GadgetID=10;ng.ng_GadgetText=NULL;
 last=ui->list=CreateGadget(LISTVIEW_KIND,last,&ng,GTLV_Labels,(ULONG)&ui->labels,GTLV_ShowSelected,NULL,GTLV_ScrollWidth,18,GTLV_Selected,~0UL,TAG_DONE);if(!last)goto fail;
 for(i=0;i<2;++i){ng.ng_LeftEdge=10+i*82;ng.ng_TopEdge=ui->height-84;ng.ng_Width=76;ng.ng_Height=18;ng.ng_GadgetID=20+i;ng.ng_GadgetText=i ? "Next":"Previous";last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto fail;}
 for(i=0;i<4;++i){ng.ng_LeftEdge=10+i*(ui->width-20)/4;ng.ng_TopEdge=ui->height-60;ng.ng_Width=(ui->width-20)/4-gap;ng.ng_GadgetID=30+i;ng.ng_GadgetText=(STRPTR)buttons[i];last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto fail;}
 ui->window=OpenWindowTags(NULL,WA_Title,(ULONG)"NovaDock AMINET",WA_PubScreen,(ULONG)ui->screen,WA_Left,(ui->screen->Width-ui->width)/2,WA_Top,ui->screen->BarHeight+2,WA_Width,ui->width,WA_Height,ui->height,WA_Gadgets,(ULONG)ui->gadgets,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|LISTVIEWIDCMP|STRINGIDCMP|BUTTONIDCMP|IDCMP_REFRESHWINDOW,TAG_DONE);
 if(!ui->window)goto fail;GT_RefreshWindow(ui->window,NULL);am_status("Choose Recent, Categories or enter a search.");return 1;
fail:am_close();return 0;
}
static int am_event(void)
{
 struct IntuiMessage *m;ULONG cls;UWORD code,id;if(!ui)return 0;
 while((m=GT_GetIMsg(ui->window->UserPort))!=NULL){cls=m->Class;code=m->Code;id=cls==IDCMP_GADGETUP ? ((struct Gadget *)m->IAddress)->GadgetID:0;GT_ReplyIMsg(m);
  if(cls==IDCMP_CLOSEWINDOW||(cls==IDCMP_RAWKEY&&code==0x45))return 33;
  if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(ui->window);am_status(ui->status);GT_EndRefresh(ui->window,TRUE);GT_RefreshWindow(ui->window,NULL);}
  if(id==10){ui->selected=(int)code;return 10;}if(id){if(id==1)id=2;return id;}
 }return 0;
}
static int am_save_path(const char *drawer,const char *name,char *out,unsigned int cap)
{
 struct FileRequester *q;int ok=0;q=AllocAslRequestTags(ASL_FileRequest,TAG_DONE);if(!q)return 0;
 if(AslRequestTags(q,ASLFR_TitleText,(ULONG)"Save Aminet package",ASLFR_InitialDrawer,(ULONG)drawer,ASLFR_InitialFile,(ULONG)name,ASLFR_DoSaveMode,TRUE,ASLFR_RejectIcons,TRUE,TAG_DONE)){
  if(strlen(q->fr_Drawer)<cap){strcpy(out,q->fr_Drawer);ok=AddPart(out,q->fr_File,cap)!=0;}}
 FreeAslRequest(q);return ok;
}
