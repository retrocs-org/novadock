#define IntuitionBase NDPictureIntuitionBase
#define GfxBase NDPictureGfxBase
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <string.h>
#include "nd_picture_view.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;
struct NDPictureView {struct Screen *screen;struct Window *window;struct NDPictureImage image;ULONG pens[16];int scale;char title[120];};
static void draw(struct NDPictureView *v)
{unsigned int x,y,start,c;int left=(v->window->Width-128*v->scale)/2,top=v->window->BorderTop+6;struct RastPort *rp=v->window->RPort;for(y=0;y<96;++y)for(x=0;x<128;){c=v->image.pixels[y*128+x];start=x;while(x<128&&v->image.pixels[y*128+x]==c)++x;if(c<16){SetAPen(rp,v->pens[c]);RectFill(rp,left+(int)start*v->scale,top+(int)y*v->scale,left+(int)x*v->scale-1,top+(int)(y+1)*v->scale-1);}}}
void nd_picture_view_set(struct NDPictureView *v,const struct NDPictureImage *image,const char *name)
{unsigned int i;if(!v)return;v->image=*image;for(i=0;i<16;++i)v->pens[i]=FindColor(v->screen->ViewPort.ColorMap,(ULONG)image->rgb[i][0]*0x01010101UL,(ULONG)image->rgb[i][1]*0x01010101UL,(ULONG)image->rgb[i][2]*0x01010101UL,-1);strncpy(v->title,name,119);v->title[119]=0;SetWindowTitles(v->window,v->title,(STRPTR)-1);draw(v);}
void nd_picture_view_close(struct NDPictureView *v)
{if(!v)return;if(v->window)CloseWindow(v->window);if(v->screen)UnlockPubScreen(NULL,v->screen);FreeVec(v);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);IntuitionBase=NULL;GfxBase=NULL;}
struct NDPictureView *nd_picture_view_open(const struct NDPictureImage *image,const char *name)
{struct NDPictureView *v=AllocVec(sizeof(*v),MEMF_PUBLIC|MEMF_CLEAR);int height;if(!v)return NULL;IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!IntuitionBase||!GfxBase)goto fail;v->screen=LockPubScreen("Workbench");if(!v->screen)goto fail;height=v->screen->Height-v->screen->BarHeight-50;v->scale=height/96;if(v->scale>4)v->scale=4;while(v->scale>1&&128*v->scale+24>v->screen->Width)--v->scale;if(v->scale<1)goto fail;
 v->window=OpenWindowTags(NULL,WA_PubScreen,(ULONG)v->screen,WA_InnerWidth,128*v->scale+12,WA_InnerHeight,96*v->scale+12,WA_Title,(ULONG)"Pictures",WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,WA_AutoAdjust,TRUE,WA_Flags,WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW,TAG_DONE);if(!v->window)goto fail;nd_picture_view_set(v,image,name);return v;
 fail:nd_picture_view_close(v);return NULL;}
void nd_picture_view_raise(struct NDPictureView *v){if(v){WindowToFront(v->window);ActivateWindow(v->window);}}
unsigned long nd_picture_view_signals(const struct NDPictureView *v){return v&&v->window ? 1UL<<v->window->UserPort->mp_SigBit:0;}
int nd_picture_view_event(struct NDPictureView *v)
{struct IntuiMessage *m;ULONG cls;UWORD code;int action=0;if(!v)return 0;while((m=(struct IntuiMessage *)GetMsg(v->window->UserPort))!=NULL){cls=m->Class;code=m->Code;ReplyMsg((struct Message *)m);if(cls==IDCMP_CLOSEWINDOW||(cls==IDCMP_RAWKEY&&code==0x45))action=1;if(cls==IDCMP_RAWKEY&&code==0x4f)action=2;if(cls==IDCMP_RAWKEY&&code==0x4e)action=3;if(cls==IDCMP_RAWKEY&&code==0x40)action=4;if(cls==IDCMP_REFRESHWINDOW){BeginRefresh(v->window);draw(v);EndRefresh(v->window,TRUE);}}return action;}
