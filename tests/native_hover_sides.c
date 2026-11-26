/* Isolated AmigaOS 3.2.3 pixel regression: Animate off must snap,
 * independent of pointer distance. Expects the normal dock in QATests:Dock. */
#include <exec/types.h>
#include <exec/memory.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <graphics/clip.h>
#include <dos/dostags.h>
#include <stdio.h>
#include <string.h>
#include "nd_control_client.h"
#include "nd_rtg_pixels.h"
struct IntuitionBase *IntuitionBase;struct GfxBase *GfxBase;
struct Library *LayersBase;
struct Library *NDGlassCyberGfxBase;long __stack=65536;
static int pointer(struct Screen *screen,int x,int y)
{
 struct MsgPort *port;struct IOStdReq *io;struct InputEvent event;struct IEPointerPixel pixel;int ok=0;
 port=CreateMsgPort();if(!port)return 0;io=(struct IOStdReq *)CreateIORequest(port,sizeof(*io));if(!io){DeleteMsgPort(port);return 0;}
 if(!OpenDevice("input.device",0,(struct IORequest *)io,0)){
  memset(&event,0,sizeof(event));memset(&pixel,0,sizeof(pixel));pixel.iepp_Screen=screen;pixel.iepp_Position.X=x;pixel.iepp_Position.Y=y;
  event.ie_Class=IECLASS_NEWPOINTERPOS;event.ie_SubClass=IESUBCLASS_PIXEL;event.ie_EventAddress=&pixel;
  io->io_Command=IND_WRITEEVENT;io->io_Data=&event;io->io_Length=sizeof(event);ok=DoIO((struct IORequest *)io)==0;CloseDevice((struct IORequest *)io);
 }DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return ok;
}
static int make_test_icon(void)
{
 static const UBYTE header[]={70,79,82,77,0,0,0,182,73,76,66,77,66,77,72,68,0,0,0,20,0,32,0,32,0,0,0,0,1,2,0,0,0,0,10,10,0,32,0,32,67,77,65,80,0,0,0,6,0,0,0,255,0,255,66,79,68,89,0,0,0,128};
 FILE *f=fopen("QATests:Dock/Icons/Snap.iff","wb");int i,ok;
 if(!f)return 0;ok=fwrite(header,1,sizeof(header),f)==sizeof(header);
 for(i=0;i<128;++i)if(fputc(255,f)==EOF)ok=0;
 if(fclose(f))ok=0;return ok;
}
static int pattern_pen(int x,int y)
{unsigned int n=(unsigned int)(x/16)*5U+(unsigned int)(y/8)*13U;n^=(unsigned int)(y/8)>>1;return 1+(int)(n%3U);}
static int pixel_matches(struct Screen *s,int x,int y,int pen)
{ULONG rgb[3];UBYTE p[3];GetRGB32(s->ViewPort.ColorMap,pen,1,rgb);WaitBlit();if(__nd_glass_read(NDGlassCyberGfxBase,p,0,0,3,&s->RastPort,x,y,1,1,0)!=1)return 0;return p[0]==(rgb[0]>>24)&&p[1]==(rgb[1]>>24)&&p[2]==(rgb[2]>>24);}

static struct Window *current_dock(struct Screen *screen)
{
 struct NDDiagnosticsSnapshot s;struct Window *w,*found=NULL;ULONG key;
 if(nd_control_diagnostics(&s)!=ND_CONTROL_OK||s.viewport_pending)return NULL;
 key=LockIBase(0);for(w=screen->FirstWindow;w;w=w->NextWindow)
 if((w->Flags&WFLG_BORDERLESS)&&w->LeftEdge==s.viewport_left&&w->TopEdge==s.viewport_top&&w->Width==s.viewport_width&&w->Height==s.viewport_height){found=w;break;}
 UnlockIBase(key);return found;
}
static int image_box(struct Screen *screen,struct Window *dock,int *width,int *height,int *cx,int *cy)
{
 static UBYTE pixels[6144];int x,y,minx=10000,miny=10000,maxx=-1,maxy=-1;
 if(dock->Width>2048)return 0;WaitBlit();
 for(y=dock->TopEdge;y<dock->TopEdge+dock->Height;++y){
  if(__nd_glass_read(NDGlassCyberGfxBase,pixels,0,0,3*dock->Width,&screen->RastPort,dock->LeftEdge,y,dock->Width,1,0)!=dock->Width)return 0;
  for(x=0;x<dock->Width;++x)if(pixels[3*x]>190&&pixels[3*x+1]<80&&pixels[3*x+2]>190){
   if(x<minx)minx=x;if(x>maxx)maxx=x;if(y<miny)miny=y;if(y>maxy)maxy=y;
  }
 }
 if(maxx<0)return 0;*width=maxx-minx+1;*height=maxy-miny+1;*cx=dock->LeftEdge+(minx+maxx)/2;*cy=(miny+maxy)/2;return 1;
}
static int outside_ok(struct Screen *screen,struct Window *under,struct Window *dock)
{
 int x,y;
 if(dock->WLayer->Scroll_X||dock->WLayer->Scroll_Y)return 0;
 for(y=under->TopEdge+32;y<under->TopEdge+under->Height-12;y+=29)
  for(x=under->LeftEdge+20;x<under->LeftEdge+under->Width-12;x+=47){
   if(x>=dock->LeftEdge&&x<dock->LeftEdge+dock->Width&&y>=dock->TopEdge&&y<dock->TopEdge+dock->Height)continue;
   if(!pixel_matches(screen,x,y,pattern_pen(x-under->LeftEdge,y-under->TopEdge)))return 0;
  }
 return 1;
}
static int write_config(int side,int animate)
{
 static const char *const sides[]={"BOTTOM","TOP","LEFT","RIGHT"};FILE *f=fopen("ENV:NovaDock/novadock.cfg","w");
 if(!f)return 0;
 fprintf(f,"SIDE=%s\nDRAG_BAR=YES\nALWAYS_ON_TOP=YES\nAUTO_HIDE=NO\nTOOLTIPS=NO\nREMOVABLE_MEDIA=NO\nTHEME=Nord\nDOCK_BACKGROUND=SOLID\nICON_COLOURS=COLOURFUL\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=%s\nHOVER_CACHE_KB=128\nLAUNCH=Tile|NewShell|NovaDock:Icons/Snap.iff|ON|AUTO||||NO|NO||||10|NO\n",sides[side],animate?"YES":"NO");
 return fclose(f)==0;
}
int main(void)
{
 struct Screen *screen=NULL;struct Window *dock,*under=NULL;BPTR guard,in,out;int side,animate,i,x,y,cx,cy,w,h,rest_w,rest_h,rc=10;
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);LayersBase=OpenLibrary("layers.library",39);NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);
 if(!IntuitionBase||!GfxBase||!LayersBase||!NDGlassCyberGfxBase)goto done;screen=LockPubScreen("Workbench");if(!screen)goto done;
 SystemTags("MakeDir RAM:SideEnv RAM:SideEnv/NovaDock",TAG_DONE);SystemTags("Assign ENV: RAM:SideEnv",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 if(!make_test_icon()||!write_config(0,0))goto done;pointer(screen,screen->Width/2,screen->Height/2);
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)goto done;
 for(i=0;i<300;++i){dock=current_dock(screen);if(dock)break;Delay(1);}if(i==300)goto stop;Delay(200);
 under=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Left,0,WA_Top,screen->BarHeight,WA_Width,screen->Width,WA_Height,screen->Height-screen->BarHeight,
  WA_Title,(ULONG)"All-side underlay QA",WA_DragBar,TRUE,WA_Flags,WFLG_SMART_REFRESH,TAG_DONE);if(!under)goto stop;
 for(y=24;y<under->Height-4;y+=8)for(x=16;x<under->Width-4;x+=16){SetAPen(under->RPort,pattern_pen(x,y));RectFill(under->RPort,x,y,x+15,y+7);}WaitBlit();WindowToBack(under);
 for(animate=0;animate<=1;++animate)for(side=0;side<4;++side){
  pointer(screen,screen->Width/2,screen->Height/2);if(!write_config(side,animate)||nd_control_reload()!=ND_CONTROL_OK)goto stop;Delay(60);
  dock=current_dock(screen);if(!dock||!image_box(screen,dock,&rest_w,&rest_h,&cx,&cy))goto stop;
  pointer(screen,cx,cy);Delay(animate?100:10);
  dock=current_dock(screen);if(!dock||!image_box(screen,dock,&w,&h,&cx,&cy)){puts("FAIL magnified pixels missing");goto stop;}
  if(w<=rest_w || h<=rest_h){printf("FAIL no magnification side=%d animate=%d %dx%d -> %dx%d\n",side,animate,rest_w,rest_h,w,h);goto stop;}
  for(i=0;i<36;++i){
   pointer(screen,cx+(side<2?((i%9)-4)*6:0),cy+(side<2?0:((i%9)-4)*6));Delay(2);
   dock=current_dock(screen);if(!dock)continue;
   if(!outside_ok(screen,under,dock)){printf("FAIL damaged underlay side=%d animate=%d frame=%d\n",side,animate,i);goto stop;}
   if(!animate){int sw,sh,px,py;if(!image_box(screen,dock,&sw,&sh,&px,&py)||sw!=w||sh!=h){puts("FAIL Animate-off pixel size changed within tile");goto stop;}}
  }
  pointer(screen,screen->Width/2,screen->Height/2);Delay(100);dock=current_dock(screen);
  if(!dock||!outside_ok(screen,under,dock)||!image_box(screen,dock,&w,&h,&cx,&cy)||w!=rest_w||h!=rest_h){printf("FAIL rest/underlay restoration side=%d animate=%d\n",side,animate);goto stop;}
  printf("PASS rendered pixels and foreign-window underlay side=%d animate=%d\n",side,animate);
 }
 rc=0;puts("PASS all-side rendered magnification and underlay regression");
 stop:nd_control_send(ND_CONTROL_QUIT);Delay(15);
 done:if(under)CloseWindow(under);if(screen)UnlockPubScreen(NULL,screen);if(LayersBase)CloseLibrary(LayersBase);if(NDGlassCyberGfxBase)CloseLibrary(NDGlassCyberGfxBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return rc;
}
