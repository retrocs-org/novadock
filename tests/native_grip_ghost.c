#include <exec/types.h>
#include <exec/memory.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuitionbase.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <string.h>
#include "nd_control_client.h"
#include "nd_rtg_pixels.h"
struct IntuitionBase *IntuitionBase;struct GfxBase *GfxBase;struct Library *NDGlassCyberGfxBase;long __stack=65536;
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

static int button(UWORD code)
{
 struct MsgPort *port;struct IOStdReq *io;struct InputEvent e;int ok=0;
 port=CreateMsgPort();if(!port)return 0;io=(struct IOStdReq *)CreateIORequest(port,sizeof(*io));if(!io){DeleteMsgPort(port);return 0;}
 if(!OpenDevice("input.device",0,(struct IORequest *)io,0)){
  memset(&e,0,sizeof(e));e.ie_Class=IECLASS_RAWMOUSE;e.ie_Code=code;
  e.ie_Qualifier=(code==IECODE_LBUTTON ? IEQUALIFIER_LEFTBUTTON:code==IECODE_RBUTTON ? IEQUALIFIER_RBUTTON:0);
  io->io_Command=IND_WRITEEVENT;io->io_Data=&e;io->io_Length=sizeof(e);ok=DoIO((struct IORequest *)io)==0;CloseDevice((struct IORequest *)io);
 }DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return ok;
}
static struct Window *dock_window(struct Screen *screen)
{
 struct Window *w,*found=NULL;struct NDDiagnosticsSnapshot d;ULONG key;
 if(nd_control_diagnostics(&d)!=ND_CONTROL_OK)return NULL;
 key=LockIBase(0);for(w=screen->FirstWindow;w;w=w->NextWindow)
  if((w->Flags&WFLG_BORDERLESS)&&w->LeftEdge==d.viewport_left&&w->TopEdge==d.viewport_top&&w->Width==d.viewport_width&&w->Height==d.viewport_height){found=w;break;}
 UnlockIBase(key);return found;
}
static int grip_point(int *x,int *y)
{
 struct NDDiagnosticsSnapshot d;
 if(nd_control_diagnostics(&d)!=ND_CONTROL_OK || d.grip_version!=1 || d.grip_width<=0 || d.grip_height<=0)return 0;
 *x=d.grip_left+d.grip_width/2;*y=d.grip_top+d.grip_height/2;return 1;
}
static void capture_failure(struct Screen *screen)
{
 struct Library *cyber=OpenLibrary("cybergraphics.library",41);UBYTE *row;FILE *f;int y;
 row=AllocVec((ULONG)screen->Width*3UL,MEMF_PUBLIC);f=fopen("QATests:grip-failure.ppm","wb");
 if(cyber&&row&&f){fprintf(f,"P6\n%d %d\n255\n",screen->Width,screen->Height);
  for(y=0;y<screen->Height;++y)if(__nd_glass_read(cyber,row,0,0,screen->Width*3,&screen->RastPort,0,y,screen->Width,1,0)==screen->Width)fwrite(row,3,screen->Width,f);}
 if(f)fclose(f);if(row)FreeVec(row);if(cyber)CloseLibrary(cyber);
}

static int count_grips(struct Screen *screen,UBYTE *pixels)
{
 struct NDDiagnosticsSnapshot d;int x,y,a,b,n=0,tx,ty,same;UBYTE pattern[6*14*3];
 if(nd_control_diagnostics(&d)!=ND_CONTROL_OK||d.viewport_pending||d.grip_width<=0)return -1;
 tx=d.grip_left+d.grip_width/2-3;ty=d.grip_top+d.grip_height/2-6;
 for(y=0;y<screen->Height;++y)if(nd_glass_rgb_line(pixels+y*screen->Width*3,screen->Width,&screen->RastPort,y)!=screen->Width)return -1;
 for(b=0;b<14;++b)memcpy(pattern+b*18,pixels+((ty+b)*screen->Width+tx)*3,18);
 for(y=d.viewport_top;y<=d.viewport_top+d.viewport_height-14;++y)for(x=d.viewport_left;x<=d.viewport_left+d.viewport_width-6;++x){
  same=1;for(b=0;b<14 && same;++b)for(a=0;a<18;++a)if(pattern[b*18+a]!=pixels[((y+b)*screen->Width+x)*3+a]){same=0;break;}
  if(same){++n;if(n<5)printf("grip pixels at %d,%d expected %d,%d\n",x,y,tx,ty);}
 }return n;
}
static int config(int top)
{FILE*f=fopen("ENV:NovaDock/novadock.cfg","w");int i;if(!f)return 0;
 fprintf(f,"DRAG_BAR=YES\nALWAYS_ON_TOP=%s\nAUTO_HIDE=NO\nREMOVABLE_MEDIA=NO\nTHEME=Nord\nDOCK_BACKGROUND=ARTWORK\nHOVER_MAGNIFICATION=100\nHOVER_ANIMATION=YES\n",top?"YES":"NO");
 fputs("DRAWER=System|SYS:|SYS:Disk|ON|AUTO||||NO|YES\nLAUNCH=Shell|NewShell|SYS:System/Shell|ON|AUTO\n",f);
 for(i=0;i<3;++i)fprintf(f,"LAUNCH=Tile%d|NewShell|NovaDock:Icons/Folder.iff|ON|AUTO||||NO|NO||||10|NO\n",i);
 fputs("PLUGIN=CPU|Plugins/Activity|CPU|METER|ON\nPLUGIN=RAM|Plugins/Memory|RAM|METER|ON\nPLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON\n",f);return fclose(f)==0;}
int main(void)
{
 struct Screen *screen;struct Window *w,*cover=NULL;UBYTE *pixels;BPTR lock,in,out;int top,i,x,y,n,tries,rc=10;
 lock=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!lock)return 20;UnLock(lock);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase*)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase*)OpenLibrary("graphics.library",39);NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);if(!IntuitionBase||!GfxBase||!NDGlassCyberGfxBase)return 20;
 screen=LockPubScreen("Workbench");if(!screen)return 20;pixels=AllocVec(screen->Width*screen->Height*3UL,MEMF_PUBLIC);if(!pixels)return 20;
 SystemTags("MakeDir RAM:GripGhost RAM:GripGhost/NovaDock",TAG_DONE);SystemTags("Assign ENV: RAM:GripGhost",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 {FILE *trace=fopen("ENV:NovaDock/support.cfg","w");if(trace){fputs("MODE=TRACE\nSAVE_TO_DISK=YES\n",trace);fclose(trace);}}
 config(0);pointer(screen,10,30);in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
 for(i=0;i<300;++i){if(dock_window(screen))break;Delay(1);}if(i==300)goto stop;Delay(200);
 printf("Native depth descriptor=%u actual=%lu\n",screen->RastPort.BitMap->Depth,GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH));
 cover=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Left,280,WA_Top,220,WA_Width,350,WA_Height,240,WA_Title,(ULONG)"Foreign window",WA_DragBar,TRUE,WA_DepthGadget,TRUE,TAG_DONE);
 Delay(170);
 for(top=0;top<2;++top){pointer(screen,10,30);config(top);nd_control_reload();Delay(100);
  for(i=0;i<16;++i){w=dock_window(screen);if(!w)goto stop;if(!grip_point(&x,&y))goto stop;
   pointer(screen,top ? x+35+(i%8)*85:120+(i%8)*95,screen->Height-45);Delay(70);
   if(i%4==3){pointer(screen,10,30);config(top);nd_control_reload();Delay(50);pointer(screen,120+(i%8)*95,screen->Height-45);Delay(60);}
   printf("largest=%lu\n",AvailMem(MEMF_LARGEST));
   for(tries=0;tries<100;++tries){n=count_grips(screen,pixels);if(n>=0)break;Delay(2);}
  printf("mode=%d frame=%d grips=%d\n",top,i,n);
   if(n!=1){puts("FAIL ghost grip");capture_failure(screen);goto stop;}
  }
 }rc=0;puts("PASS no ghost grips during magnification");
 stop:if(cover)CloseWindow(cover);nd_control_send(ND_CONTROL_QUIT);Delay(10);FreeVec(pixels);UnlockPubScreen(NULL,screen);CloseLibrary(NDGlassCyberGfxBase);CloseLibrary((struct Library*)GfxBase);CloseLibrary((struct Library*)IntuitionBase);return rc;
}
