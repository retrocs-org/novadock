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
static struct Window *find_dock(struct Screen *screen,struct Window *under)
{struct Window *w,*found=NULL;ULONG key=LockIBase(0);for(w=screen->FirstWindow;w;w=w->NextWindow)if(w!=under&&(w->Flags&WFLG_BORDERLESS)&&w->Width>100&&w->TopEdge>screen->Height/2){found=w;break;}UnlockIBase(key);return found;}

static int make_test_icon(void)
{
 static const UBYTE header[]={70,79,82,77,0,0,0,182,73,76,66,77,66,77,72,68,0,0,0,20,0,32,0,32,0,0,0,0,1,2,0,0,0,0,10,10,0,32,0,32,67,77,65,80,0,0,0,6,0,0,0,255,0,255,66,79,68,89,0,0,0,128};
 FILE *f=fopen("QATests:Dock/Icons/Snap.iff","wb");int i,ok;
 if(!f)return 0;ok=fwrite(header,1,sizeof(header),f)==sizeof(header);
 for(i=0;i<128;++i)if(fputc(255,f)==EOF)ok=0;
 if(fclose(f))ok=0;return ok;
}
static int icon_size(struct Screen *screen,int centre,int *width,int *height)
{
 UBYTE pixels[420];int x,y,minx=10000,miny=10000,maxx=-1,maxy=-1;
 WaitBlit();for(y=screen->Height-180;y<screen->Height-30;++y){
  if(__nd_glass_read(NDGlassCyberGfxBase,pixels,0,0,420,&screen->RastPort,centre-70,y,140,1,0)!=140)return 0;
  for(x=0;x<140;++x)if(pixels[3*x]>190&&pixels[3*x+1]<80&&pixels[3*x+2]>190){
   if(x<minx)minx=x;if(x>maxx)maxx=x;if(y<miny)miny=y;if(y>maxy)maxy=y;
  }
 }if(maxx<0)return 0;*width=maxx-minx+1;*height=maxy-miny+1;return 1;
}
int main(void)
{
 struct Screen *screen=NULL;struct Window *dock;BPTR guard,in,out;FILE *f;int i,rc=10,cx,cy,rest_w,rest_h,peak_w,peak_h,w,h,offset;
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);
 if(!IntuitionBase||!GfxBase||!NDGlassCyberGfxBase)goto done;screen=LockPubScreen("Workbench");if(!screen)goto done;
 SystemTags("MakeDir RAM:SnapEnv RAM:SnapEnv/NovaDock",TAG_DONE);SystemTags("Assign ENV: RAM:SnapEnv",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 if(!make_test_icon())goto done;
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto done;
 fputs("SIDE=BOTTOM\nALWAYS_ON_TOP=YES\nAUTO_HIDE=NO\nTOOLTIPS=NO\nREMOVABLE_MEDIA=NO\nTHEME=Nord\nDOCK_BACKGROUND=SOLID\nICON_COLOURS=COLOURFUL\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=NO\n",f);
 for(i=0;i<4;++i)fprintf(f,"LAUNCH=Tile%d|SYS:System/Shell|NovaDock:Icons/Snap.iff|ON|AUTO||||NO|NO||||10|NO\n",i);fclose(f);
 pointer(screen,10,30);in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
 if(SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)goto done;
 for(i=0;i<300;++i){dock=find_dock(screen,NULL);if(dock)break;Delay(1);}if(i==300)goto stop;Delay(200);dock=find_dock(screen,NULL);
 cx=dock->LeftEdge+7+43;cy=dock->TopEdge+35;
 if(!icon_size(screen,cx,&rest_w,&rest_h)){puts("FAIL resting icon pixels not found");goto stop;}
 pointer(screen,cx+17,cy);Delay(25);
 if(!icon_size(screen,cx,&peak_w,&peak_h)||peak_w!=rest_w*11/4||peak_h!=rest_h*11/4){puts("FAIL hover did not magnify");goto stop;}
 printf("Resting icon %dx%d; immediate full hover %dx%d\n",rest_w,rest_h,peak_w,peak_h);
 for(offset=-28;offset<=28;offset+=7){pointer(screen,cx+offset,cy);Delay(12);
  if(!icon_size(screen,cx,&w,&h))goto stop;
  printf("Pointer offset %d: rendered icon %dx%d\n",offset,w,h);
  if(w!=peak_w||h!=peak_h){puts("FAIL Animate off still changes size within one tile");goto stop;}
 }
 pointer(screen,10,30);Delay(20);if(!icon_size(screen,cx,&w,&h)||w!=rest_w||h!=rest_h){puts("FAIL leaving did not restore normal size");goto stop;}
 puts("PASS Animate off immediately magnifies one tile and keeps rendered size fixed across pointer movement");rc=0;
 stop:nd_control_send(ND_CONTROL_QUIT);Delay(10);
 done:if(screen)UnlockPubScreen(NULL,screen);if(NDGlassCyberGfxBase)CloseLibrary(NDGlassCyberGfxBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return rc;
}
