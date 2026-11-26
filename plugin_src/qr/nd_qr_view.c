#define IntuitionBase NDQRIntuitionBase
#define GfxBase NDQRGfxBase
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <graphics/gfxmacros.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <utility/tagitem.h>
#include <stdio.h>
#include <string.h>
#include "nd_qr_view.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;
static char view_error[96]="Preview could not be opened";
const char *nd_qr_view_error(void){return view_error;}
struct NDQRView {struct Screen *screen;struct Window *window;struct NDQR qr;LONG white,black;int scale;char title[80];};
static void draw(struct NDQRView *v)
{int row,x,start,side=(int)v->qr.size+8,left=(v->window->Width-side*v->scale)/2,top=v->window->BorderTop+8;struct RastPort *rp=v->window->RPort;
 SetAPen(rp,v->white);RectFill(rp,v->window->BorderLeft,v->window->BorderTop,v->window->Width-v->window->BorderRight-1,v->window->Height-v->window->BorderBottom-1);SetAPen(rp,v->black);
 for(row=0;row<side;++row)for(x=0;x<side;){while(x<side&&!nd_qr_pixel(&v->qr,x,row))++x;start=x;while(x<side&&nd_qr_pixel(&v->qr,x,row))++x;if(x>start)RectFill(rp,left+start*v->scale,top+row*v->scale,left+x*v->scale-1,top+(row+1)*v->scale-1);}}
void nd_qr_view_close(struct NDQRView *v)
{if(!v)return;if(v->window)CloseWindow(v->window);if(v->screen){if(v->white>=0)ReleasePen(v->screen->ViewPort.ColorMap,v->white);if(v->black>=0)ReleasePen(v->screen->ViewPort.ColorMap,v->black);UnlockPubScreen(NULL,v->screen);}FreeVec(v);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);GfxBase=NULL;IntuitionBase=NULL;}
struct NDQRView *nd_qr_view_open(const struct NDQR *q)
{struct NDQRView *v;int side,available,w;struct TagItem tags[]={{OBP_Precision,PRECISION_EXACT},{OBP_FailIfBad,TRUE},{TAG_DONE,0}};
 strcpy(view_error,"Not enough memory to open preview");
 if(!q || q->size<21 || q->size>89)return NULL;v=AllocVec(sizeof(*v),MEMF_PUBLIC|MEMF_CLEAR);if(!v)return NULL;v->white=v->black=-1;v->qr=*q;
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!IntuitionBase||!GfxBase)goto fail;v->screen=LockPubScreen("Workbench");if(!v->screen)goto fail;
 side=q->size+8;available=v->screen->Height-v->screen->BarHeight-12-v->screen->WBorTop-v->screen->Font->ta_YSize-v->screen->WBorBottom-16;v->scale=available/side;if(v->scale>6)v->scale=6;if(v->scale<2){sprintf(view_error,"Preview needs %u pixels high at 2x; export ILBM",(unsigned int)(side*2+40));goto fail;}
 w=side*v->scale+16;if(w<200)w=200;if(w+v->screen->WBorLeft+v->screen->WBorRight>v->screen->Width-12)goto fail;
 v->white=ObtainBestPenA(v->screen->ViewPort.ColorMap,0xffffffffUL,0xffffffffUL,0xffffffffUL,tags);v->black=ObtainBestPenA(v->screen->ViewPort.ColorMap,0,0,0,tags);if(v->white<0 || v->black<0 || v->white==v->black){sprintf(view_error,"Exact black/white pens unavailable; export ILBM");goto fail;}
 sprintf(v->title,"QR - %u modules, UTF-8",q->size);
 v->window=OpenWindowTags(NULL,WA_PubScreen,(ULONG)v->screen,WA_InnerWidth,w,WA_InnerHeight,side*v->scale+16,WA_Title,(ULONG)v->title,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,WA_AutoAdjust,TRUE,WA_Flags,WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW,TAG_DONE);if(!v->window)goto fail;draw(v);return v;
 fail:nd_qr_view_close(v);return NULL;}
void nd_qr_view_raise(struct NDQRView *v){if(v&&v->window){WindowToFront(v->window);ActivateWindow(v->window);}}
unsigned long nd_qr_view_signals(const struct NDQRView *v){return v&&v->window ? 1UL<<v->window->UserPort->mp_SigBit:0;}
int nd_qr_view_event(struct NDQRView *v)
{struct IntuiMessage *m;ULONG cls;UWORD code;int close=0;if(!v||!v->window)return 0;while((m=(struct IntuiMessage *)GetMsg(v->window->UserPort))!=NULL){cls=m->Class;code=m->Code;ReplyMsg((struct Message *)m);if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY&&code==0x45))close=1;if(cls==IDCMP_REFRESHWINDOW){BeginRefresh(v->window);draw(v);EndRefresh(v->window,TRUE);}}return close;}
