/* QA-only: acknowledge exactly the initial welcome of one generated test script.
 * Installer 47 shows this page even when script questions are substituted.
 * Never target a normal installer, a confirmation page or an error requester. */
#include <exec/memory.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
struct IntuitionBase *IntuitionBase=NULL;
long __stack=32768;
static int proceed(struct Screen *screen,int x,int y)
{
 struct MsgPort *port;struct IOStdReq *io;struct InputEvent event;struct IEPointerPixel pixel;int ok=0;
 port=CreateMsgPort();if(!port)return 0;io=(struct IOStdReq *)CreateIORequest(port,sizeof(*io));if(!io){DeleteMsgPort(port);return 0;}
 if(!OpenDevice("input.device",0,(struct IORequest *)io,0)){
  memset(&event,0,sizeof(event));memset(&pixel,0,sizeof(pixel));pixel.iepp_Screen=screen;pixel.iepp_Position.X=x;pixel.iepp_Position.Y=y;
  event.ie_Class=IECLASS_NEWPOINTERPOS;event.ie_SubClass=IESUBCLASS_PIXEL;event.ie_EventAddress=&pixel;
  io->io_Command=IND_WRITEEVENT;io->io_Data=&event;io->io_Length=sizeof(event);ok=DoIO((struct IORequest *)io)==0;Delay(3);
  memset(&event,0,sizeof(event));event.ie_Class=IECLASS_RAWMOUSE;event.ie_Code=IECODE_LBUTTON;event.ie_Qualifier=IEQUALIFIER_LEFTBUTTON;io->io_Data=&event;if(DoIO((struct IORequest *)io)!=0)ok=0;Delay(3);
  event.ie_Code=IECODE_LBUTTON|IECODE_UP_PREFIX;event.ie_Qualifier=0;io->io_Data=&event;if(DoIO((struct IORequest *)io)!=0)ok=0;
  CloseDevice((struct IORequest *)io);
 }
 DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return ok;
}
int main(int argc,char **argv)
{
 struct Screen *screen;struct Window *window;struct Gadget *g;int i,j,x=0,y=0,found=0,ok=0,buttons,radio;BPTR log;char line[180];
 {BPTR marker;int valid=0;const char *names[]={"QATests:Current-Install","QATests:Current-Keep","QATests:Current-Remove","QATests:Previous-Install","QATests:Previous-Keep","QATests:Previous-Remove"};unsigned int n;if(argc!=2)return 20;marker=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!marker)return 20;UnLock(marker);for(n=0;n<sizeof(names)/sizeof(names[0]);++n){size_t len=strlen(names[n]);if(!strncmp(argv[1],names[n],len)&&(!argv[1][len]||(argv[1][len]=='-'&&argv[1][len+1]>='0'&&argv[1][len+1]<='2'&&!argv[1][len+2])))valid=1;}if(!valid)return 20;}
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(!IntuitionBase)return 20;screen=LockPubScreen("Workbench");if(!screen){CloseLibrary((struct Library *)IntuitionBase);return 20;}
 for(i=0;i<150&&!found;++i){Forbid();
  for(window=screen->FirstWindow;window&&!found;window=window->NextWindow){if(!window->Title||strcmp((char *)window->Title,argv[1]))continue;buttons=radio=0;
   for(g=window->FirstGadget,j=0;g&&j<80;g=g->NextGadget,++j){if(g->GadgetID==90){buttons|=1;x=window->LeftEdge+g->LeftEdge+g->Width/2;y=window->TopEdge+g->TopEdge+g->Height/2;}if(g->GadgetID==91)buttons|=2;if(g->GadgetID==100)buttons|=4;if(g->GadgetID==1)buttons|=8;if(g->GadgetID>=2&&g->GadgetID<=4)++radio;}
   if(buttons==15&&radio)found=1;
  }Permit();if(!found)Delay(5);
 }
 if(found)ok=proceed(screen,x,y);
 log=Open("AutoRun:installer-welcome.log",MODE_READWRITE);if(log){Seek(log,0,OFFSET_END);sprintf(line,"%s: %s\n",argv[1],ok ? "initial welcome acknowledged":"no matching initial welcome");Write(log,line,strlen(line));Close(log);}
 UnlockPubScreen(NULL,screen);CloseLibrary((struct Library *)IntuitionBase);return ok ? 0:10;
}
