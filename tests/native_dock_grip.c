/* Isolated AmigaOS 3.2.3 pixel regression: Animate off must snap,
 * independent of pointer distance. Expects the normal dock in QATests:Dock. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
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
#include "nd_layout.h"
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
static ULONG qa_count(struct List *l)
{struct Node *n;ULONG count=0;for(n=l->lh_Head;n->ln_Succ;n=n->ln_Succ)++count;return count;}
static void qa_resources(ULONG *ports,ULONG *tasks)
{Forbid();*ports=qa_count(&SysBase->PortList);*tasks=qa_count(&SysBase->TaskReady)+qa_count(&SysBase->TaskWait);Permit();}
static int main_menu_open(struct Screen *screen)
{
 struct Window *dock=dock_window(screen),*w;int found=0;ULONG key=LockIBase(0);
 for(w=screen->FirstWindow;w;w=w->NextWindow)if(w!=dock && (w->Flags&WFLG_BORDERLESS) &&
    (w->Flags&WFLG_RMBTRAP) && w->Width>=175 && w->Height>=122){found=1;break;}
 UnlockIBase(key);return found;
}
static int shelf_length(struct Screen *screen,int side)
{
 static struct NDConfig cfg;struct NDLayout fit;
 if(nd_load_effective(&cfg)>0 || !nd_compact_enabled(&cfg) || !nd_layout_fit(&cfg,screen->Width,screen->Height,screen->BarHeight,&fit))return -1;
 return side<2?fit.width:fit.height;
}
static int qa_top=1,qa_artwork=0;
static int config(int side,int grip,int magnify)
{
 static const char *const sides[]={"BOTTOM","TOP","LEFT","RIGHT"};FILE *f;int i;
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)return 0;
 fprintf(f,"SIDE=%s\nPOSITION=CENTER\nDRAG_BAR=%s\nALWAYS_ON_TOP=%s\nAUTO_HIDE=NO\nTOOLTIPS=NO\nREMOVABLE_MEDIA=NO\nTHEME=Nord\nDOCK_BACKGROUND=%s\nHOVER_MAGNIFICATION=%d\nHOVER_ANIMATION=YES\n",sides[side],grip?"YES":"NO",qa_top?"YES":"NO",qa_artwork?"ARTWORK":"SOLID",magnify);
 for(i=0;i<3;++i)fprintf(f,"LAUNCH=Tile%d|NewShell||ON|AUTO||||NO|NO||||10|NO\n",i);
 return fclose(f)==0;
}
static int saved_position(const char *expected)
{FILE *f=fopen("ENV:NovaDock/novadock.cfg","r");char line[256];int found=0;if(!f)return 0;while(fgets(line,sizeof(line),f))if(strstr(line,expected))found=1;fclose(f);return found;}
int main(int argc,char **argv)
{
 struct Screen *screen=NULL;struct Window *w;BPTR guard,in,out;ULONG memory_baseline=0,ports_baseline=0,tasks_baseline=0,ports,tasks,free_mem;int pass;int rc=10,side,magnify,target,x,y,i,before,normal,wide;
 if(argc>2 && !strcmp(argv[2],"BACKDROP"))qa_top=0;
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
 if(!IntuitionBase||!GfxBase)goto done;screen=LockPubScreen("Workbench");if(!screen)goto done;
 SystemTags("MakeDir RAM:GripEnv RAM:GripEnv/NovaDock",TAG_DONE);SystemTags("Assign ENV: RAM:GripEnv",TAG_DONE);SystemTags("Assign ENVARC: RAM:GripEnv",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 if(argc>1){FILE *trace=fopen("ENV:NovaDock/support.cfg","w");if(trace){fputs("MODE=TRACE\nSAVE_TO_DISK=NO\n",trace);fclose(trace);}}
 if(!config(0,1,0))goto done;pointer(screen,20,30);in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
 if(SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)goto done;
 for(i=0;i<300;++i){w=dock_window(screen);if(w)break;Delay(1);}if(i==300)goto stop;Delay(150);
 for(pass=0;pass<(argc>1?1:3);++pass){
 for(magnify=argc>1?100:0;magnify<=100;magnify+=100)for(side=argc>1&&qa_top?1:0;side<4;++side){
  pointer(screen,20,30);if(!config(side,0,magnify)||nd_control_reload()!=ND_CONTROL_OK)goto stop;Delay(35);w=dock_window(screen);if(!w)goto stop;
  normal=shelf_length(screen,side);
  if(!config(side,1,magnify)||nd_control_reload()!=ND_CONTROL_OK)goto stop;Delay(35);w=dock_window(screen);if(!w)goto stop;
  wide=shelf_length(screen,side);if(wide-normal!=16){puts("FAIL grip did not reserve exactly 16 pixels");goto stop;}
  {static struct NDConfig cfg;struct NDLayout fitted;
   if(nd_load_effective(&cfg)>0 || !nd_compact_enabled(&cfg) || !nd_layout_fit(&cfg,screen->Width,screen->Height,screen->BarHeight,&fitted))goto stop;
   wide=side<2?fitted.width:fitted.height;
  }
  for(target=0;target<3;++target){
   w=dock_window(screen);if(!w)goto stop;if(!grip_point(&x,&y))goto stop;
   pointer(screen,x,y);Delay(8);button(IECODE_LBUTTON);Delay(4);
   if(side<2)x=target==0?13:target==1?(screen->Width-wide)/2+8:screen->Width-wide+3;
   else y=target==0?screen->BarHeight+13:target==1?screen->BarHeight+(screen->Height-screen->BarHeight-wide)/2+8:screen->Height-wide+3;
   pointer(screen,x,y);Delay(8);button(IECODE_LBUTTON|IECODE_UP_PREFIX);Delay(30);
   if(!nd_control_sync_settings()||!saved_position("POSITION=CUSTOM")){{struct NDDiagnosticsSnapshot d;char line[256];FILE *f;nd_control_diagnostics(&d);printf("FAIL grip drag/save side=%d mag=%d target=%d pointer=%d,%d box=%d,%d %dx%d save=%u starts=%lu failed=%lu\n",side,magnify,target,screen->MouseX,screen->MouseY,d.viewport_left,d.viewport_top,d.viewport_width,d.viewport_height,d.save_state,d.saves_started,d.saves_failed);f=fopen("ENV:NovaDock/novadock.cfg","r");if(f){while(fgets(line,sizeof(line),f))if(strstr(line,"POSITION=")||strstr(line,"DRAG_BAR="))fputs(line,stdout);fclose(f);}SystemTags("Copy ENV:NovaDock/novadock.cfg QATests:failed.cfg",TAG_DONE);}goto stop;}
   pointer(screen,screen->Width/2,screen->Height/2);Delay(90);
  }
  w=dock_window(screen);if(!w)goto stop;if(!grip_point(&x,&y))goto stop;
  pointer(screen,x,y);Delay(10);before=main_menu_open(screen);button(IECODE_RBUTTON);Delay(10);button(IECODE_RBUTTON|IECODE_UP_PREFIX);Delay(10);
  if(before || !main_menu_open(screen)){{struct NDDiagnosticsSnapshot d;nd_control_diagnostics(&d);printf("FAIL main menu side=%d mag=%d before=%d pointer=%d,%d grip=%d,%d %dx%d viewport=%d,%d %dx%d\n",side,magnify,before,screen->MouseX,screen->MouseY,d.grip_left,d.grip_top,d.grip_width,d.grip_height,d.viewport_left,d.viewport_top,d.viewport_width,d.viewport_height);capture_failure(screen);nd_control_send(ND_CONTROL_SUPPORT_FLUSH);SystemTags("Copy RAM:NovaDock-Trace/dock.log QATests:grip-trace.log",TAG_DONE);}goto stop;}
  pointer(screen,20,30);button(IECODE_LBUTTON);button(IECODE_LBUTTON|IECODE_UP_PREFIX);Delay(10);
  printf("PASS side=%d magnify=%d: grip toggle, continuous drag and persisted custom position, right-click menu\n",side,magnify);
 }
 pointer(screen,screen->Width/2,screen->Height/2);Delay(150);
 free_mem=AvailMem(MEMF_ANY);qa_resources(&ports,&tasks);
 if(!pass){memory_baseline=free_mem;ports_baseline=ports;tasks_baseline=tasks;}
 else if(free_mem+131072UL<memory_baseline || ports>ports_baseline+2UL || tasks>tasks_baseline+2UL){puts("FAIL continuing resource loss after warmed interaction pass");goto stop;}
 printf("PASS warmed pass %d free=%lu ports=%lu tasks=%lu (baseline %lu/%lu/%lu)\n",pass,free_mem,ports,tasks,memory_baseline,ports_baseline,tasks_baseline);
 }
 rc=0;puts("PASS optional drag grip native interaction and resource soak regression");
 stop:nd_control_send(ND_CONTROL_QUIT);Delay(20);
 done:if(screen)UnlockPubScreen(NULL,screen);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return rc;
}
