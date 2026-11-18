#include "nd_saver_stats.h"
/* Borderless full-screen scene; input dismisses it without changing settings. */
struct NDSaverBand {UWORD x,y,w,h;};
struct NDSaverWindow {
 struct Screen *screen;struct Window *window;
 struct BitMap *bitmap;struct RastPort paint;
 UWORD pens[ND_SAVER_COLORS];int width,height,left,top;
 int xmap[321],ymap[161],last_pen,border_valid;
 int dirty_x,dirty_y,dirty_w,dirty_h;
 struct BitMap *background,*sprites;struct NDSaverBand bands[ND_BOING_SIZE];
 unsigned int band_count;
 int sprite_w,sprite_h,cache_failed,fast_previous,fast_x,fast_y;
 unsigned long sprite_ready,clock_tick;
 unsigned long frame,ticks;
 struct NDSaverStats stats;int diagnostics;unsigned long depth;
};
static unsigned long saver_clock(void)
{struct DateStamp d;DateStamp(&d);return (ULONG)d.ds_Days*4320000UL+(ULONG)d.ds_Minute*3000UL+(ULONG)d.ds_Tick;}
static void saver_report(struct NDSaverWindow *s)
{
 FILE *f;if(!s->diagnostics || !s->stats.total_frames)return;
 f=fopen("RAM:NovaDock-SAVER.txt","w");if(!f)return;
 fprintf(f,"NovaDock SAVER performance\nRender: %d x %d, depth %lu\nFrames: %lu\nAverage FPS: %lu.%lu\nAverage draw: %lu ms\nPeak draw: %lu ms\nBoing cache: %s\nTiming resolution: 20 ms; draw includes bitmap completion, excludes overlay.\n",
 s->width,s->height,s->depth,s->stats.total_frames,
 s->stats.total_ticks ? s->stats.total_intervals*500UL/s->stats.total_ticks/10UL:0,
 s->stats.total_ticks ? s->stats.total_intervals*500UL/s->stats.total_ticks%10UL:0,
 s->stats.total_draw*20UL/s->stats.total_frames,s->stats.total_max*20UL,
 s->fast_previous ? "active":"inactive");fclose(f);
}
static void saver_free_cache(struct NDSaverWindow *s);
static void saver_close(struct NDSaverWindow *s)
{
 saver_report(s);
 if(s->bitmap)WaitBlit();
 if(s->window)CloseWindow(s->window);
 saver_free_cache(s);
 if(s->bitmap)FreeBitMap(s->bitmap);
 if(s->screen)UnlockPubScreen(NULL,s->screen);
 memset(s,0,sizeof(*s));
}
static void saver_rect(void *ctx,int color,int x,int y,int w,int h)
{
 struct NDSaverWindow *s=(struct NDSaverWindow *)ctx;
 int left=s->xmap[x],top=s->ymap[y];
 int right=s->xmap[x+w]-1,bottom=s->ymap[y+h]-1;
 if(right<left || bottom<top)return;
 if(s->last_pen!=color){SetAPen(&s->paint,s->pens[color]);s->last_pen=color;}RectFill(&s->paint,left,top,right,bottom);
}
#include "nd_saver_fast.h"
static void saver_present(struct NDSaverWindow *s,int full)
{
 if(s->window && s->bitmap){
  if(full || !s->border_valid){SetAPen(s->window->RPort,s->pens[0]);
  if(s->top>0)RectFill(s->window->RPort,0,0,s->screen->Width-1,s->top-1);
  if(s->top+s->height<s->screen->Height)RectFill(s->window->RPort,0,s->top+s->height,s->screen->Width-1,s->screen->Height-1);
  if(s->left>0)RectFill(s->window->RPort,0,0,s->left-1,s->screen->Height-1);
  if(s->left+s->width<s->screen->Width)RectFill(s->window->RPort,s->left+s->width,0,s->screen->Width-1,s->screen->Height-1);
   s->border_valid=1;}
  if(full || !s->dirty_w)BltBitMapRastPort(s->bitmap,0,0,s->window->RPort,s->left,s->top,s->width,s->height,0xc0);
  else BltBitMapRastPort(s->bitmap,s->dirty_x,s->dirty_y,s->window->RPort,
   s->left+s->dirty_x,s->top+s->dirty_y,s->dirty_w,s->dirty_h,0xc0);
 }
}
static void saver_blit(struct NDSaverWindow *s){saver_present(s,1);}
static void saver_overlay(struct NDSaverWindow *s)
{
 char line[100];struct RastPort *rp;int height,y,n;
 if(!s->window || !s->diagnostics)return;
 rp=s->window->RPort;height=rp->TxHeight;y=rp->TxBaseline+2;
 SetAPen(rp,s->pens[0]);RectFill(rp,0,0,s->screen->Width-1,height*2+3);
 SetAPen(rp,s->pens[1]);
 sprintf(line,"FPS %lu.%lu  draw %lu/%lums",s->stats.fps10/10UL,s->stats.fps10%10UL,s->stats.average_ms,s->stats.peak_ms);
 for(n=(int)strlen(line);n>0 && TextLength(rp,line,n)>s->screen->Width-4;--n){}
 Move(rp,2,y);Text(rp,line,n);
 sprintf(line,"%dx%dx%lu %s",s->width,s->height,s->depth,s->fast_previous ? "cached":"direct");
 for(n=(int)strlen(line);n>0 && TextLength(rp,line,n)>s->screen->Width-4;--n){}
 Move(rp,2,y+height);Text(rp,line,n);
}
static void saver_paint(struct NDSaverWindow *s,int pattern,int lights)
{
 unsigned long start=0,end;
 if(!s->window)return;
 if(s->diagnostics)start=saver_clock();
 s->dirty_w=0;
 if(pattern!=3 || !saver_fast_boing(s)){s->fast_previous=0;nd_saver_scene(s,saver_rect,s->frame,pattern,lights);}
 saver_present(s,0);
 if(s->diagnostics){WaitBlit();end=saver_clock();nd_saver_stats_frame(&s->stats,end,end-start);saver_overlay(s);}
}
static int saver_open(struct NDSaverWindow *s,int pattern,int lights,const char *render,int diagnostics)
{
 unsigned long rgb,depth;LONG pen;int i;
 if(s->window){WindowToFront(s->window);ActivateWindow(s->window);return 1;}
 if(!IntuitionBase || !GfxBase)return 0;
 s->screen=LockPubScreen(NULL);if(!s->screen)return 0;
 s->diagnostics=diagnostics;
 depth=GetBitMapAttr(s->screen->RastPort.BitMap,BMA_DEPTH);
 s->depth=depth;
 if(!nd_saver_dimensions(s->screen->Width,s->screen->Height,depth,render,&s->width,&s->height))goto failed;
 for(i=0;i<=320;++i)s->xmap[i]=i*s->width/320;
 for(i=0;i<=160;++i)s->ymap[i]=i*s->height/160;
 s->last_pen=-1;
 s->left=(s->screen->Width-s->width)/2;s->top=(s->screen->Height-s->height)/2;
 s->bitmap=AllocBitMap(s->width,s->height,
  depth,BMF_CLEAR,s->screen->RastPort.BitMap);
 if(!s->bitmap)goto failed;
 InitRastPort(&s->paint);s->paint.BitMap=s->bitmap;SetDrMd(&s->paint,JAM1);
 for(i=0;i<ND_SAVER_COLORS;++i){
  rgb=nd_saver_rgb[i];pen=FindColor(s->screen->ViewPort.ColorMap,
   ((rgb>>16)&255UL)*0x01010101UL,((rgb>>8)&255UL)*0x01010101UL,
   (rgb&255UL)*0x01010101UL,-1);
  s->pens[i]=(UWORD)(pen<0 ? 0:pen);
 }
 s->window=OpenWindowTags(NULL,WA_CustomScreen,(ULONG)s->screen,
  WA_Left,0,WA_Top,0,WA_Width,s->screen->Width,WA_Height,s->screen->Height,
  WA_Borderless,TRUE,WA_Activate,TRUE,WA_RMBTrap,TRUE,
  WA_IDCMP,IDCMP_MOUSEBUTTONS|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|IDCMP_ACTIVEWINDOW|IDCMP_INACTIVEWINDOW,
  WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE|WFLG_BORDERLESS|WFLG_RMBTRAP,TAG_DONE);
 if(!s->window)goto failed;
 WindowToFront(s->window);ActivateWindow(s->window);
 saver_paint(s,pattern,lights);return 1;
failed:
 saver_close(s);return 0;
}
static void saver_events(struct NDSaverWindow *s)
{
 struct IntuiMessage *im;ULONG kind;UWORD code;
 while(s->window && (im=(struct IntuiMessage *)GetMsg(s->window->UserPort))!=NULL){
  kind=im->Class;code=im->Code;ReplyMsg((struct Message *)im);
  if(kind==IDCMP_RAWKEY || (kind==IDCMP_MOUSEBUTTONS && !(code&IECODE_UP_PREFIX))){
   saver_close(s);return;
  }
  if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(s->window);saver_blit(s);EndRefresh(s->window,TRUE);}
  else if(kind==IDCMP_ACTIVEWINDOW || kind==IDCMP_INACTIVEWINDOW){s->ticks=0;s->stats.last=0;s->stats.frames=0;}
 }
}
