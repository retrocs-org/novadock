/* Disposable FS-UAE test: real sensor client, real dock, known ENV readings,
 * and 129 dummy game drawers. No sensor hardware or WHDLoad execution. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <string.h>
#include "nd_protocol.h"
#include "nd_artwork.h"
#include "nd_control_client.h"
#include "nd_rtg_pixels.h"
struct IntuitionBase *IntuitionBase;struct GfxBase *GfxBase;
struct Library *NDGlassCyberGfxBase;long __stack=65536;
static int failures;
static void check(int ok,const char *label){printf("%s %s\n",ok?"PASS":"FAIL",label);if(!ok)++failures;}
static int write_text(const char *path,const char *text){FILE *f=fopen(path,"w");if(!f)return 0;fputs(text,f);return fclose(f)==0;}
static int launch(const char *text){BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;if(!in||!out)return 0;rc=SystemTags((STRPTR)text,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(rc<0){Close(in);Close(out);}return rc>=0;}
static void config(int magnify,int labels)
{
 FILE *f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f){++failures;return;}
 fprintf(f,"SIDE=BOTTOM\nDRAG_BAR=NO\nALWAYS_ON_TOP=YES\nAUTO_HIDE=NO\nTOOLTIPS=NO\nREMOVABLE_MEDIA=NO\nTHEME=Nord\nFONT=WORKBENCH\nDOCK_BACKGROUND=SOLID\nICON_COLOURS=COLOURFUL\nHOVER_MAGNIFICATION=%d\nHOVER_ANIMATION=NO\nLABELS=%s\nPLUGIN=SENSOR|Plugins/Sensors|SENSORS|METER|ON|YES\nPLUGIN=WHDLoad|Plugins/Games|WHDLOAD|METER|ON|YES\nOPTION.SENSORS.I2C=NO\nOPTION.SENSORS.INTERVAL=2\nOPTION.SENSORS.ROTATE=0\nOPTION.SENSORS.CLICK=Next sensor\nOPTION.SENSORS.RTC_TEMP_TITLE=Board\nOPTION.SENSORS.RTC_CPU_TITLE=CPU probe\nOPTION.WHDLOAD.DRAWER=QATests:Games\nOPTION.WHDLOAD.ICON=WHDLoad\n",magnify,labels?"ALWAYS":"NEVER");fclose(f);
}
static void make_games(void)
{int i;char dir[80],file[100];BPTR lock;for(i=0;i<129;++i){sprintf(dir,"QATests:Games/Game%03d",i);lock=CreateDir(dir);if(lock)UnLock(lock);sprintf(file,"%s/Test.slave",dir);write_text(file,"Fixture only; never execute.\n");sprintf(file,"%s/Test.info",dir);write_text(file,"Fixture icon marker.\n");}}
static void reply(struct NDMessage *m,ULONG actions,UWORD event)
{m->event=event|ND_EVENT_HOST_BADGES;if(m->version==5 && m->exec_msg.mn_Length>=sizeof(struct NDExtMessage))((struct NDExtMessage *)m)->event2=ND_EVENT2_HOST_ARTWORK|ND_EVENT2_HOST_ARTWORK_META|actions;ReplyMsg((struct Message *)m);}
static void sensor_ipc(void)
{
 struct MsgPort *port=CreateMsgPort();struct NDMessage *m;int ticks,first=0,second=0,missing=0,quit=0;ULONG art;
 if(!port){++failures;return;}port->mp_Node.ln_Name=ND_PORT_NAME;port->mp_Node.ln_Pri=ND_HOST_ABI;AddPort(port);
 if(!launch("QATests:Dock/Plugins/Sensors SENSORS")){++failures;goto done;}
 for(ticks=0;ticks<1200 && !quit;++ticks){while((m=(struct NDMessage *)GetMsg(port))!=NULL){
  ULONG actions=0;UWORD event=0;art=m->version==5 ? ((struct NDExtMessage *)m)->reserved2[0]:0;
  if(m->command==ND_MSG_UPDATE && !strcmp(m->caption,"Board") && art==nd_artwork_key("SensorTemp")){
   check(!strcmp(m->value,"45.0 C"),"real SENSOR publishes named Board reading and temperature artwork");first=1;actions=ND_EVENT2_SENSOR_NEXT;
  }else if(first && m->command==ND_MSG_UPDATE && !strcmp(m->caption,"CPU probe") && art==nd_artwork_key("SensorCPU")){
   check(!strcmp(m->value,"45.0 C"),"manual Next changes name/artwork while identical value stays intact");second=1;
   DeleteFile("ENV:AK_Internal_temp_C");DeleteFile("ENV:AK_CPU_temp_C");actions=ND_EVENT2_REFRESH;
  }else if(second && m->command==ND_MSG_UPDATE && art==nd_artwork_key("SadSensor")){
   check(strcmp(m->value,"45.0 C")!=0,"unavailable readings never retain a stale current value");missing=1;event=ND_EVENT_QUIT;quit=1;
  }reply(m,actions,event);
 }Delay(1);}
 check(first&&second&&missing,"real sensor client negotiation, names, manual selection and offline state");
 done:for(ticks=0;ticks<120;++ticks){while((m=(struct NDMessage *)GetMsg(port))!=NULL)reply(m,0,ND_EVENT_QUIT);Delay(1);}RemPort(port);DeleteMsgPort(port);
}
static void games_ipc(void)
{
 struct MsgPort *port=CreateMsgPort();struct NDMessage *m;int ticks,found=0;
 if(!port){++failures;return;}port->mp_Node.ln_Name=ND_PORT_NAME;port->mp_Node.ln_Pri=ND_HOST_ABI;AddPort(port);
 if(!launch("QATests:Dock/Plugins/Games WHDLOAD")){++failures;goto done;}
 for(ticks=0;ticks<1500 && !found;++ticks){while((m=(struct NDMessage *)GetMsg(port))!=NULL){
  if(m->command==ND_MSG_BADGE && !strcmp(m->value,"129"))found=1;
  reply(m,0,found?ND_EVENT_QUIT:0);
 }Delay(1);}
 check(found,"real WHDLoad scanner sends full 129-object count");
 done:for(ticks=0;ticks<120;++ticks){while((m=(struct NDMessage *)GetMsg(port))!=NULL)reply(m,0,ND_EVENT_QUIT);Delay(1);}RemPort(port);DeleteMsgPort(port);
}
static struct Window *dock_window(struct Screen *screen)
{
 struct NDDiagnosticsSnapshot s;struct Window *w,*found=NULL;ULONG lock;
 if(nd_control_diagnostics(&s)!=ND_CONTROL_OK || s.viewport_pending)return NULL;
 lock=LockIBase(0);for(w=screen->FirstWindow;w;w=w->NextWindow)if((w->Flags&WFLG_BORDERLESS)&&w->LeftEdge==s.viewport_left&&w->TopEdge==s.viewport_top&&w->Width==s.viewport_width&&w->Height==s.viewport_height){found=w;break;}UnlockIBase(lock);return found;
}
static ULONG crc(struct Screen *screen,int x,int y,int w,int h)
{UBYTE row[1024*3];ULONG sum=2166136261UL;int i,j;WaitBlit();for(j=0;j<h;++j){if(__nd_glass_read(NDGlassCyberGfxBase,row,0,0,w*3,&screen->RastPort,x,y+j,w,1,0)!=w)return 0;for(i=0;i<w*3;++i){sum^=row[i];sum*=16777619UL;}}return sum;}
static int input(struct Screen *screen,int x,int y,int click)
{
 struct MsgPort *port=CreateMsgPort();struct IOStdReq *io;struct InputEvent e;struct IEPointerPixel pixel;int ok=0;
 if(!port)return 0;io=(struct IOStdReq *)CreateIORequest(port,sizeof(*io));if(!io){DeleteMsgPort(port);return 0;}
 if(!OpenDevice("input.device",0,(struct IORequest *)io,0)){
  memset(&e,0,sizeof(e));memset(&pixel,0,sizeof(pixel));pixel.iepp_Screen=screen;pixel.iepp_Position.X=x;pixel.iepp_Position.Y=y;e.ie_Class=IECLASS_NEWPOINTERPOS;e.ie_SubClass=IESUBCLASS_PIXEL;e.ie_EventAddress=&pixel;
  io->io_Command=IND_WRITEEVENT;io->io_Data=&e;io->io_Length=sizeof(e);ok=DoIO((struct IORequest *)io)==0;
  if(click){Delay(3);memset(&e,0,sizeof(e));e.ie_Class=IECLASS_RAWMOUSE;e.ie_Code=IECODE_LBUTTON;e.ie_Qualifier=IEQUALIFIER_LEFTBUTTON;DoIO((struct IORequest *)io);Delay(3);e.ie_Code=IECODE_LBUTTON|IECODE_UP_PREFIX;e.ie_Qualifier=0;DoIO((struct IORequest *)io);}
  CloseDevice((struct IORequest *)io);
 }DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return ok;
}
static int badge_129(struct Screen *screen,struct Window *dock)
{
 struct BitMap *bitmap;struct RastPort rp;int x,y,bx,by,width=TextLength(dock->RPort,"129",3)+6,height=dock->RPort->TxHeight+2,ok=1;LONG bg;ULONG a,b;
 /* Default two-tile layout: border 7, tile width 86, gap 5. */
 bx=dock->LeftEdge+7+91+86-width-3;by=dock->TopEdge+7+3;
 bitmap=AllocBitMap(width,height,1,BMF_CLEAR,NULL);if(!bitmap)return 0;InitRastPort(&rp);rp.BitMap=bitmap;SetFont(&rp,dock->RPort->Font);SetAPen(&rp,1);Move(&rp,3,1+rp.TxBaseline);Text(&rp,"129",3);
 bg=ReadPixel(&screen->RastPort,bx,by);
 for(y=0;y<height;++y)for(x=0;x<width;++x){a=(ULONG)ReadPixel(&rp,x,y);b=(ULONG)ReadPixel(&screen->RastPort,bx+x,by+y);if((a!=0)!=(b!=(ULONG)bg))ok=0;}
 FreeBitMap(bitmap);return ok;
}
static void screenshot(struct Screen *s,const char *path)
{FILE *f=fopen(path,"wb");UBYTE row[3072];int y;if(!f)return;fprintf(f,"P6\n%d %d\n255\n",s->Width,s->Height);for(y=0;y<s->Height;++y){__nd_glass_read(NDGlassCyberGfxBase,row,0,0,s->Width*3,&s->RastPort,0,y,s->Width,1,0);fwrite(row,3,s->Width,f);}fclose(f);}
int main(void)
{
 struct Screen *screen=NULL;struct Window *dock;BPTR guard;int i;ULONG heading,value,footer;
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);if(!IntuitionBase||!GfxBase||!NDGlassCyberGfxBase)return 20;screen=LockPubScreen("Workbench");if(!screen)return 20;
 SystemTags("MakeDir RAM:SensorQA RAM:SensorQA/NovaDock QATests:Games",TAG_DONE);SystemTags("Assign ENV: RAM:SensorQA",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);config(0,1);make_games();
 write_text("ENV:AK_Internal_temp_C","45.0\n");write_text("ENV:AK_CPU_temp_C","45.0\n");sensor_ipc();games_ipc();
 write_text("ENV:AK_Internal_temp_C","45.0\n");write_text("ENV:AK_CPU_temp_C","45.0\n");check(launch("QATests:Dock/NovaDock"),"normal dock launched");
 for(i=0;i<500;++i){dock=dock_window(screen);if(dock)break;Delay(1);}Delay(200);dock=dock_window(screen);if(!dock){check(0,"dock window exists");goto done;}
 for(i=0;i<100 && !badge_129(screen,dock);++i)Delay(10);
 check(badge_129(screen,dock),"rendered badge contains all three digits at tile upper-right");
 heading=crc(screen,dock->LeftEdge+7,dock->TopEdge+7+33,86,10);value=crc(screen,dock->LeftEdge+7,dock->TopEdge+7+46,86,10);footer=crc(screen,dock->LeftEdge+7,dock->TopEdge+7+59,86,12);
 screenshot(screen,"AutoRun:sensor-before.ppm");input(screen,dock->LeftEdge+48,dock->TopEdge+32,1);Delay(150);input(screen,screen->Width/2,screen->Height/2,0);Delay(15);dock=dock_window(screen);
 if(dock){check(heading!=crc(screen,dock->LeftEdge+7,dock->TopEdge+7+33,86,10),"rendered channel name changes after tile click");check(value==crc(screen,dock->LeftEdge+7,dock->TopEdge+7+46,86,10),"identical numeric reading remains unchanged");check(footer==crc(screen,dock->LeftEdge+7,dock->TopEdge+7+59,86,12),"instance footer stays independent of channel name");screenshot(screen,"AutoRun:sensor-after.ppm");}
 else check(0,"dock retained after sensor switch");
 done:nd_control_send(ND_CONTROL_QUIT);Delay(50);UnlockPubScreen(NULL,screen);CloseLibrary(NDGlassCyberGfxBase);CloseLibrary((struct Library *)GfxBase);CloseLibrary((struct Library *)IntuitionBase);return failures?10:0;
}
