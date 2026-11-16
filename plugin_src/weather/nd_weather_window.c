/* Compact forecast table. Repaint only on data changes or window refresh. */
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <string.h>
#include "nd_weather_window.h"
#include "nd_weather_codes.h"
#include "nd_weather_art.h"
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;struct Library *GadToolsBase=NULL;
struct NDWeatherView {
 struct Screen *screen;struct Window *window;struct Gadget *gadgets;APTR visual;
 struct TextFont *font;struct NDWeather weather;int have,stale,fahrenheit,compact,top,row,width;
 UWORD bg,text,accent,line,icons[5];char status[96],title[128];
};
struct IconContext {struct NDWeatherView *view;int x,y,size;};
static void label(struct NDWeatherView *p,int x,int y,int width,const char *text)
{
 struct TextExtent fit;ULONG n;if(width<=0)return;
 n=TextFit(p->window->RPort,(STRPTR)text,(ULONG)strlen(text),&fit,NULL,1,width,12);
 Move(p->window->RPort,x,y);Text(p->window->RPort,(STRPTR)text,n);
}
static void icon_box(void *data,int c,int x,int y,int w,int h)
{
 struct IconContext *ctx=data;int a=x*ctx->size/32,b=y*ctx->size/32,r=(x+w)*ctx->size/32-1,d=(y+h)*ctx->size/32-1;
 if(r<a||d<b)return;SetAPen(ctx->view->window->RPort,ctx->view->icons[c]);RectFill(ctx->view->window->RPort,ctx->x+a,ctx->y+b,ctx->x+r,ctx->y+d);
}
static void weather_icon(struct NDWeatherView *p,int code,int x,int y,int size)
{struct IconContext c;c.view=p;c.x=x;c.y=y;c.size=size;nd_weather_art(&c,icon_box,nd_weather_art_state(code));SetAPen(p->window->RPort,p->text);}
static void paint(struct NDWeatherView *p)
{
 struct RastPort *rp=p->window->RPort;char line[160],low[24],high[24],amount[48],now[24],updated[24];int i,y,start=p->top+44,right=p->width-16;
 SetFont(rp,p->font);SetDrMd(rp,JAM1);SetAPen(rp,p->bg);RectFill(rp,5,p->top-3,p->width-6,p->window->Height-30);SetAPen(rp,p->text);
 if(p->have){
  weather_icon(p,p->weather.code,14,p->top,26);nd_weather_temperature(p->weather.temperature,p->fahrenheit,now);
  sprintf(line,"Now  %s  -  %s",now,nd_weather_condition(p->weather.code));label(p,52,p->top+14,p->width-66,line);
 }else label(p,14,p->top+14,p->width-28,"Forecast not available - check settings or Refresh");
 SetAPen(rp,p->accent);if(!p->compact){label(p,14,p->top+35,96,"Day");label(p,140,p->top+35,128,"Conditions");label(p,276,p->top+35,64,"Low");label(p,342,p->top+35,64,"High");label(p,408,p->top+35,64,"Chance %");label(p,480,p->top+35,right-480,"Rain / snow");}
 else label(p,14,p->top+35,right-14,"Condition / chance % / amount");
 for(i=0;i<5;++i){y=start+i*p->row;SetAPen(rp,p->line);Move(rp,12,y-3);Draw(rp,right,y-3);SetAPen(rp,p->text);
  if(!p->have||i>=(int)p->weather.count){label(p,14,y+9,right-14,"--");continue;}
  nd_weather_temperature(p->weather.days[i].low,p->fahrenheit,low);nd_weather_temperature(p->weather.days[i].high,p->fahrenheit,high);
  nd_weather_amount(&p->weather.days[i],amount);
  if(!p->compact){
   label(p,14,y+10,94,nd_weather_weekday(p->weather.days[i].date));weather_icon(p,p->weather.days[i].code,111,y-2,20);
   label(p,140,y+10,128,nd_weather_condition(p->weather.days[i].code));label(p,276,y+10,64,low);label(p,342,y+10,64,high);
   sprintf(line,"%d%%",p->weather.days[i].rain);label(p,408,y+10,64,line);label(p,480,y+10,right-480,amount);
  }else{
   weather_icon(p,p->weather.days[i].code,14,y-1,20);label(p,44,y+7,90,nd_weather_weekday(p->weather.days[i].date));
   sprintf(line,"%s / %s",low,high);label(p,138,y+7,right-138,line);
   label(p,44,y+17,72,nd_weather_condition(p->weather.days[i].code));
   sprintf(line,"%d%%",p->weather.days[i].rain);label(p,128,y+17,40,line);label(p,176,y+17,right-176,amount);
  }
 }
 y=start+5*p->row+4;SetAPen(rp,p->text);
 strcpy(updated,p->have ? p->weather.updated:"");if(strlen(updated)>10)updated[10]=' ';
 sprintf(line,"%s%s%s",p->stale ? "Cached / old: ":"Updated: ",updated,p->have ? "":"--");label(p,14,y,p->width-28,line);
 if(strcmp(p->status,"Updated"))label(p,14,y+12,p->width-28,p->status);
 label(p,14,y+24,p->width-28,p->compact ? "Open-Meteo.com / CC BY 4.0":"Chance of rain/snow; amounts: mm rain + cm snow. Open-Meteo / CC BY 4.0");
}
void nd_weather_view_close(struct NDWeatherView *p)
{
 if(p){if(p->window)CloseWindow(p->window);if(p->gadgets)FreeGadgets(p->gadgets);if(p->visual)FreeVisualInfo(p->visual);if(p->font)CloseFont(p->font);if(p->screen)UnlockPubScreen(NULL,p->screen);FreeVec(p);}
 if(GadToolsBase){CloseLibrary(GadToolsBase);GadToolsBase=NULL;}if(GfxBase){CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;}if(IntuitionBase){CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}
}
struct NDWeatherView *nd_weather_view_open(const char *location,int fahrenheit,const struct NDWeather *weather,int have,int stale,const char *status)
{
 struct NDWeatherView *p;struct DrawInfo *di;struct Gadget *last;struct NewGadget ng;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};int h,i;static const char *buttons[]={"Refresh","Close"};static const ULONG colors[5]={0x263342UL,0x719bc4UL,0xe1e9efUL,0x72c3d3UL,0xe6c879UL};
 p=AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);if(!p)return NULL;
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);if(!IntuitionBase||!GfxBase||!GadToolsBase)goto fail;
 p->screen=LockPubScreen("Workbench");if(!p->screen)goto fail;p->font=OpenFont(&font);if(!p->font)goto fail;
 p->width=p->screen->Width-24;if(p->width>630)p->width=630;if(p->width<296)goto fail;p->compact=p->width<616;p->row=p->compact ? 29:26;
 p->top=p->screen->WBorTop+p->screen->Font->ta_YSize+8;h=p->top+44+5*p->row+60;
 if(h>p->screen->Height-p->screen->BarHeight-4){p->row=22;h=p->top+44+5*p->row+60;}if(h>p->screen->Height-p->screen->BarHeight-4)goto fail;
 di=GetScreenDrawInfo(p->screen);if(!di)goto fail;p->bg=di->dri_Pens[BACKGROUNDPEN];p->text=di->dri_Pens[TEXTPEN];p->accent=di->dri_Pens[TEXTPEN];p->line=di->dri_Pens[SHADOWPEN];FreeScreenDrawInfo(p->screen,di);
 for(i=0;i<5;++i)p->icons[i]=(UWORD)FindColor(p->screen->ViewPort.ColorMap,((colors[i]>>16)&255UL)*0x01010101UL,((colors[i]>>8)&255UL)*0x01010101UL,(colors[i]&255UL)*0x01010101UL,-1);
 p->visual=GetVisualInfo(p->screen,TAG_DONE);if(!p->visual)goto fail;last=CreateContext(&p->gadgets);if(!last)goto fail;
 memset(&ng,0,sizeof(ng));ng.ng_VisualInfo=p->visual;ng.ng_TextAttr=&font;ng.ng_Flags=PLACETEXT_IN;ng.ng_TopEdge=h-26;ng.ng_Height=18;
 for(i=0;i<2;++i){ng.ng_LeftEdge=12+i*(p->width-24)/2;ng.ng_Width=(p->width-32)/2;ng.ng_GadgetID=100+i;ng.ng_GadgetText=(STRPTR)buttons[i];last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto fail;}
 sprintf(p->title,"%.90s - Weather",*location ? location:"NovaDock");p->weather=*weather;p->have=have;p->stale=stale;p->fahrenheit=fahrenheit;strncpy(p->status,status,95);
 p->window=OpenWindowTags(NULL,WA_Title,(ULONG)p->title,WA_PubScreen,(ULONG)p->screen,WA_Left,(p->screen->Width-p->width)/2,WA_Top,p->screen->BarHeight+2,WA_Width,p->width,WA_Height,h,WA_Gadgets,(ULONG)p->gadgets,
 WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|IDCMP_GADGETUP,TAG_DONE);
 if(!p->window)goto fail;GT_RefreshWindow(p->window,NULL);paint(p);return p;
fail:nd_weather_view_close(p);return NULL;
}
void nd_weather_view_update(struct NDWeatherView *p,int f,const struct NDWeather *w,int have,int stale,const char *status)
{if(!p)return;if(p->fahrenheit==f&&p->have==have&&p->stale==stale&&!strcmp(p->status,status)&&!memcmp(&p->weather,w,sizeof(*w)))return;p->weather=*w;p->fahrenheit=f;p->have=have;p->stale=stale;strncpy(p->status,status,95);p->status[95]=0;paint(p);}
void nd_weather_view_raise(struct NDWeatherView *p){if(p){WindowToFront(p->window);ActivateWindow(p->window);}}
unsigned long nd_weather_view_signals(const struct NDWeatherView *p){return p ? 1UL<<p->window->UserPort->mp_SigBit:0;}
int nd_weather_view_event(struct NDWeatherView *p)
{
 struct IntuiMessage *m;ULONG cls;UWORD code,id;if(!p)return 0;
 while((m=GT_GetIMsg(p->window->UserPort))!=NULL){cls=m->Class;code=m->Code;id=cls==IDCMP_GADGETUP ? ((struct Gadget *)m->IAddress)->GadgetID:0;GT_ReplyIMsg(m);
  if(cls==IDCMP_CLOSEWINDOW||(cls==IDCMP_RAWKEY&&code==0x45))return -1;
  if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(p->window);paint(p);GT_EndRefresh(p->window,TRUE);GT_RefreshWindow(p->window,NULL);}if(id)return id;
 }return 0;
}
