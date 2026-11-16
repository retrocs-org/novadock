/* Borderless full-screen scene; input dismisses it without changing settings. */
struct NDXmasWindow {
 struct Screen *screen;struct Window *window;
 struct BitMap *bitmap;struct RastPort paint;
 UWORD pens[ND_XMAS_COLORS],black,white;int width,height,left,top,banner;
 unsigned long frame,ticks,story_ticks;
};
static void xmas_close(struct NDXmasWindow *s)
{
 if(s->window)CloseWindow(s->window);
 if(s->bitmap)FreeBitMap(s->bitmap);
 if(s->screen)UnlockPubScreen(NULL,s->screen);
 memset(s,0,sizeof(*s));
}
static void xmas_rect(void *ctx,int color,int x,int y,int w,int h)
{
 struct NDXmasWindow *s=(struct NDXmasWindow *)ctx;
 int left=x*s->width/ND_XMAS_WIDTH,top=y*s->height/ND_XMAS_HEIGHT;
 int right=(x+w)*s->width/ND_XMAS_WIDTH-1,bottom=(y+h)*s->height/ND_XMAS_HEIGHT-1;
 if(right<left || bottom<top)return;
 SetAPen(&s->paint,s->pens[color]);RectFill(&s->paint,left,top,right,bottom);
}
static const char *xmas_tale(unsigned long ticks)
{
 static const char *lines[]={"On a quiet Christmas Eve,","a little lantern lit the snow.","An elf had lost the final gift.","Then a robin showed the way.","Past the pines, a window glowed.","Santa placed the gift inside.","The village dreamed in peace.","And one small light shone on."};
 return lines[(ticks/400UL)%8UL];
}
static void xmas_banner_text(struct NDXmasWindow *s,const char *text,int baseline)
{
 struct RastPort *rp=s->window->RPort;struct TextExtent fit;ULONG length;
 length=TextFit(rp,(STRPTR)text,(ULONG)strlen(text),&fit,NULL,1,s->screen->Width-16,rp->TxHeight+2);
 SetAPen(rp,s->white);SetDrMd(rp,JAM1);Move(rp,(s->screen->Width-fit.te_Width)/2,baseline);Text(rp,(STRPTR)text,length);
}
static void xmas_blit(struct NDXmasWindow *s)
{
 if(s->window && s->bitmap){
  SetAPen(s->window->RPort,s->black);
  if(s->top){RectFill(s->window->RPort,0,0,s->screen->Width-1,s->top-1);RectFill(s->window->RPort,0,s->top+s->height,s->screen->Width-1,s->screen->Height-1);}
  if(s->left){RectFill(s->window->RPort,0,0,s->left-1,s->screen->Height-1);RectFill(s->window->RPort,s->left+s->width,0,s->screen->Width-1,s->screen->Height-1);}
  BltBitMapRastPort(s->bitmap,0,0,s->window->RPort,s->left,s->top,s->width,s->height,0xc0);
  SetAPen(s->window->RPort,s->black);
  RectFill(s->window->RPort,0,s->screen->Height-2*s->banner,s->screen->Width-1,s->screen->Height-1);
  xmas_banner_text(s,xmas_tale(s->story_ticks),s->screen->Height-2*s->banner+s->banner-5);
  xmas_banner_text(s,s->screen->Width>=520 ? "Music: Wintery loop - Emma_MA | OpenGameArt.org | CC0":"Wintery loop - Emma_MA (CC0)",s->screen->Height-5);
 }
}
static void xmas_paint(struct NDXmasWindow *s,int pattern,int lights)
{
 if(!s->window)return;
 nd_xmas_scene(s,xmas_rect,s->frame,pattern,lights);xmas_blit(s);
}
static int xmas_open(struct NDXmasWindow *s,int pattern,int lights)
{
 unsigned long rgb;LONG pen;int i;
 if(s->window){WindowToFront(s->window);ActivateWindow(s->window);return 1;}
 if(!IntuitionBase || !GfxBase)return 0;
 s->screen=LockPubScreen(NULL);if(!s->screen)return 0;
 s->banner=s->screen->RastPort.TxHeight+8;
 s->black=(UWORD)FindColor(s->screen->ViewPort.ColorMap,0,0,0,-1);
 s->white=(UWORD)FindColor(s->screen->ViewPort.ColorMap,0xffffffffUL,0xffffffffUL,0xffffffffUL,-1);
 s->width=s->screen->Width;s->height=s->width/2;
 if(s->height>s->screen->Height-2*s->banner){s->height=s->screen->Height-2*s->banner;s->width=s->height*2;}
 if(s->width<160 || s->height<80)goto failed;
 s->left=(s->screen->Width-s->width)/2;s->top=(s->screen->Height-2*s->banner-s->height)/2;
 s->bitmap=AllocBitMap(s->width,s->height,
  GetBitMapAttr(s->screen->RastPort.BitMap,BMA_DEPTH),BMF_CLEAR,s->screen->RastPort.BitMap);
 if(!s->bitmap)goto failed;
 InitRastPort(&s->paint);s->paint.BitMap=s->bitmap;SetDrMd(&s->paint,JAM1);
 for(i=0;i<ND_XMAS_COLORS;++i){
  rgb=nd_xmas_rgb[i];pen=FindColor(s->screen->ViewPort.ColorMap,
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
 xmas_paint(s,pattern,lights);return 1;
failed:
 xmas_close(s);return 0;
}
static void xmas_events(struct NDXmasWindow *s)
{
 struct IntuiMessage *im;ULONG kind;UWORD code;
 while(s->window && (im=(struct IntuiMessage *)GetMsg(s->window->UserPort))!=NULL){
  kind=im->Class;code=im->Code;ReplyMsg((struct Message *)im);
  if(kind==IDCMP_RAWKEY || (kind==IDCMP_MOUSEBUTTONS && !(code&IECODE_UP_PREFIX))){
   xmas_close(s);return;
  }
  if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(s->window);xmas_blit(s);EndRefresh(s->window,TRUE);}
  else if(kind==IDCMP_ACTIVEWINDOW || kind==IDCMP_INACTIVEWINDOW)s->ticks=0;
 }
}
