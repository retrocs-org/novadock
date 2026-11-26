/* Offline native regression: a newly opened window behind an expanding dock. */
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
static int pattern_pen(int x,int y)
{unsigned int n=(unsigned int)(x/16)*5U+(unsigned int)(y/8)*13U;n^=(unsigned int)(y/8)>>1;return 1+(int)(n%3U);}
static int pixel_matches(struct Screen *s,int x,int y,int pen)
{ULONG rgb[3];UBYTE p[3];GetRGB32(s->ViewPort.ColorMap,pen,1,rgb);WaitBlit();if(__nd_glass_read(NDGlassCyberGfxBase,p,0,0,3,&s->RastPort,x,y,1,1,0)!=1)return 0;return p[0]==(rgb[0]>>24)&&p[1]==(rgb[1]>>24)&&p[2]==(rgb[2]>>24);}
int main(int argc,char **argv)
{
 struct Screen *screen=NULL;struct Window *under=NULL,*dock;BPTR lock,in,out;FILE *f;int i,y,x,base_top,base_left,base_width,checked=0,bad=0,scrolled=0,outside=0,outside_bad=0,xx,yy,covered,rc=10;
 struct Window *cover;struct NDDiagnosticsSnapshot state;int ready;
 (void)argv;lock=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!lock)return 20;UnLock(lock);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);LayersBase=OpenLibrary("layers.library",39);if(!IntuitionBase||!GfxBase||!NDGlassCyberGfxBase||!LayersBase)goto done;
 screen=LockPubScreen("Workbench");if(!screen)goto done;
 SystemTags("MakeDir RAM:HoverEnv RAM:HoverEnv/NovaDock",TAG_DONE);SystemTags("Assign ENV: RAM:HoverEnv",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto done;
 fputs("SIDE=BOTTOM\nALWAYS_ON_TOP=YES\nAUTO_HIDE=NO\nTOOLTIPS=NO\nTHEME=Nord\nDOCK_BACKGROUND=SOLID\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=YES\n",f);
 fprintf(f,"BACKGROUND_TRANSPARENCY=%d\n",argc>1?50:0);
 for(i=0;i<4;++i)fprintf(f,"LAUNCH=Tile%d|SYS:System/Shell|NovaDock:Icons/Folder.iff|ON|AUTO||||NO|NO||||10|NO\n",i);fclose(f);
 pointer(screen,10,30);in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
 if(SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)goto done;
 for(i=0;i<200;++i){dock=find_dock(screen,NULL);if(dock)break;Delay(1);}if(i==200)goto stop;
 Delay(300);dock=find_dock(screen,NULL);if(!dock)goto stop;
 base_top=dock->TopEdge;base_left=dock->LeftEdge;base_width=dock->Width;printf("Base dock: %d,%d %d x %d; screen %d x %d\n",base_left,base_top,base_width,dock->Height,screen->Width,screen->Height);Delay(5);
 under=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Left,10,WA_Top,screen->Height-300,WA_Width,screen->Width-20,WA_Height,290,WA_Title,(ULONG)"Underlay QA",WA_DragBar,TRUE,WA_Flags,WFLG_SMART_REFRESH,TAG_DONE);if(!under)goto stop;
 for(y=24;y<under->Height-4;y+=8)for(x=16;x<under->Width-4;x+=16){SetAPen(under->RPort,pattern_pen(x,y));RectFill(under->RPort,x,y,x+15,y+7);}WaitBlit();WindowToBack(under);Delay(2);
 if(!pointer(screen,base_left+base_width/3,base_top+35))goto stop;
 for(i=0;i<65;++i){pointer(screen,i<20?base_left+base_width/3:base_left+base_width*(2+(i/4)%7)/10,base_top+35);Delay(2);memset(&state,0,sizeof(state));ready=nd_control_diagnostics(&state)==ND_CONTROL_OK&&state.viewport_version==1&&!state.viewport_pending;dock=find_dock(screen,under);if(!dock)continue;LockLayerInfo(&screen->LayerInfo);
  if(dock->WLayer->Scroll_X||dock->WLayer->Scroll_Y)++scrolled;
  if(i==0||i==20||i==64)printf("Frame %d dock %d,%d %d x %d; mouse %d,%d\n",i,dock->LeftEdge,dock->TopEdge,dock->Width,dock->Height,screen->MouseX,screen->MouseY);
  if(ready&&state.viewport_left==dock->LeftEdge&&state.viewport_top==dock->TopEdge&&state.viewport_width==dock->Width&&state.viewport_height==dock->Height&&i<20&&dock->TopEdge<base_top-8){x=dock->LeftEdge;y=dock->TopEdge;
   if(x>=under->LeftEdge+16&&x<under->LeftEdge+under->Width-4)for(;y<dock->TopEdge+1;++y){
    if(y>under->TopEdge+24){++checked;if(!pixel_matches(screen,x,y,pattern_pen(x-under->LeftEdge,y-under->TopEdge))){++bad;if(bad<8)printf("Stale sample: frame %d at %d,%d\n",i,x,y);}}
   }
  }
  /* A displaced backing-store copy can corrupt the exposed application,
     even while the dock itself looks correct. Sample outside every overlay. */
  for(yy=under->TopEdge+32;yy<base_top-4;yy+=13)for(xx=under->LeftEdge+20;xx<under->LeftEdge+under->Width-12;xx+=40){
   covered=0;for(cover=screen->FirstWindow;cover;cover=cover->NextWindow){
    if(cover==under||(cover->Flags&WFLG_BACKDROP))continue;
    if(xx>=cover->LeftEdge&&xx<cover->LeftEdge+cover->Width&&yy>=cover->TopEdge&&yy<cover->TopEdge+cover->Height){covered=1;break;}
   }
   if(!covered){++outside;if(!pixel_matches(screen,xx,yy,pattern_pen(xx-under->LeftEdge,yy-under->TopEdge)))++outside_bad;}
  }
  UnlockLayerInfo(&screen->LayerInfo);
 }
 printf("Underlay animation samples (transparency %d): %d checked, %d stale\n",argc>1?50:0,checked,bad);
 printf("Exposed application: %d checked, %d damaged; %d frames with nonzero layer scroll\n",outside,outside_bad,scrolled);
 if(checked>5&&!bad&&outside>100&&!outside_bad&&!scrolled){puts("PASS newly covered magnification headroom matches the live window behind it");rc=0;}
 stop:nd_control_send(ND_CONTROL_QUIT);Delay(5);
 done:if(under)CloseWindow(under);if(screen)UnlockPubScreen(NULL,screen);if(LayersBase)CloseLibrary(LayersBase);if(NDGlassCyberGfxBase)CloseLibrary(NDGlassCyberGfxBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return rc;
}
