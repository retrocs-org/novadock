/* Borderless full-screen scene; input dismisses it without changing settings. */
struct NDSaverWindow {
 struct Screen *screen;struct Window *window;
 struct BitMap *bitmap;struct RastPort paint;
 UWORD pens[ND_SAVER_COLORS];int width,height,left,top;
 unsigned long frame,ticks;
};
static void saver_close(struct NDSaverWindow *s)
{
 if(s->window)CloseWindow(s->window);
 if(s->bitmap)FreeBitMap(s->bitmap);
 if(s->screen)UnlockPubScreen(NULL,s->screen);
 memset(s,0,sizeof(*s));
}
static void saver_rect(void *ctx,int color,int x,int y,int w,int h)
{
 struct NDSaverWindow *s=(struct NDSaverWindow *)ctx;
 int left=x*s->width/ND_SAVER_WIDTH,top=y*s->height/ND_SAVER_HEIGHT;
 int right=(x+w)*s->width/ND_SAVER_WIDTH-1,bottom=(y+h)*s->height/ND_SAVER_HEIGHT-1;
 if(right<left || bottom<top)return;
 SetAPen(&s->paint,s->pens[color]);RectFill(&s->paint,left,top,right,bottom);
}
static void saver_blit(struct NDSaverWindow *s)
{
 if(s->window && s->bitmap){
  SetAPen(s->window->RPort,s->pens[0]);
  if(s->top){RectFill(s->window->RPort,0,0,s->screen->Width-1,s->top-1);RectFill(s->window->RPort,0,s->top+s->height,s->screen->Width-1,s->screen->Height-1);}
  if(s->left){RectFill(s->window->RPort,0,0,s->left-1,s->screen->Height-1);RectFill(s->window->RPort,s->left+s->width,0,s->screen->Width-1,s->screen->Height-1);}
  BltBitMapRastPort(s->bitmap,0,0,s->window->RPort,s->left,s->top,s->width,s->height,0xc0);
 }
}
static void saver_paint(struct NDSaverWindow *s,int pattern,int lights)
{
 if(!s->window)return;
 nd_saver_scene(s,saver_rect,s->frame,pattern,lights);saver_blit(s);
}
static int saver_open(struct NDSaverWindow *s,int pattern,int lights)
{
 unsigned long rgb;LONG pen;int i;
 if(s->window){WindowToFront(s->window);ActivateWindow(s->window);return 1;}
 if(!IntuitionBase || !GfxBase)return 0;
 s->screen=LockPubScreen(NULL);if(!s->screen)return 0;
 s->width=s->screen->Width;s->height=s->width/2;
 if(s->height>s->screen->Height){s->height=s->screen->Height;s->width=s->height*2;}
 if(s->width<160 || s->height<80)goto failed;
 s->left=(s->screen->Width-s->width)/2;s->top=(s->screen->Height-s->height)/2;
 s->bitmap=AllocBitMap(s->width,s->height,
  GetBitMapAttr(s->screen->RastPort.BitMap,BMA_DEPTH),BMF_CLEAR,s->screen->RastPort.BitMap);
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
  else if(kind==IDCMP_ACTIVEWINDOW || kind==IDCMP_INACTIVEWINDOW)s->ticks=0;
 }
}
