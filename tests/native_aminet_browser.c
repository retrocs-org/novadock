/* Online, disposable native Aminet browser search + ASL download regression. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <string.h>
#include "nd_protocol.h"
#include "nd_aminet.h"
struct IntuitionBase *IntuitionBase;struct Library *GadToolsBase;long __stack=65536;
static struct MsgPort *host;static char last_status[32];static int click_pending=1;
static void pump(void)
{
 struct NDMessage *m;while((m=(struct NDMessage *)GetMsg(host))!=NULL){m->event=0;
  if(m->command==ND_MSG_UPDATE){strcpy(last_status,m->value);if(click_pending){m->event=ND_EVENT_CLICK;click_pending=0;}}
  /* Deliberately old-host capabilities: also exercise modern client's fallback. */
  if(m->version==5)((struct NDExtMessage *)m)->event2=0;ReplyMsg((struct Message *)m);
 }
}
static struct Window *find_window(const char *title)
{struct Screen *s;struct Window *w,*found=NULL;ULONG key=LockIBase(0);for(s=IntuitionBase->FirstScreen;s&&!found;s=s->NextScreen)for(w=s->FirstWindow;w;w=w->NextWindow)if(w->Title&&!strcmp((char *)w->Title,title)){found=w;break;}UnlockIBase(key);return found;}
static int post_ui(struct Window *w,int id,int code)
{
 struct Gadget *g;struct MsgPort *reply;struct IntuiMessage *m;int i;
 for(g=w->FirstGadget;g&&g->GadgetID!=id;g=g->NextGadget){}if(!g)return 0;reply=CreateMsgPort();m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!reply||!m)return 0;
 m->ExecMessage.mn_Length=sizeof(*m);m->ExecMessage.mn_ReplyPort=reply;m->IDCMPWindow=w;m->Class=IDCMP_GADGETUP;m->Code=(UWORD)code;m->IAddress=g;PutMsg(w->UserPort,(struct Message *)m);
 for(i=0;i<500;++i){pump();if(GetMsg(reply)){FreeVec(m);DeleteMsgPort(reply);return 1;}Delay(1);}return 0;
}
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
static int click_named(struct Window *w,const char *name)
{
    struct Gadget *g,fallback;struct IntuiText *t;struct MsgPort *port;struct IOStdReq *io;struct InputEvent e;int up;
    for(g=w->FirstGadget;g;g=g->NextGadget){for(t=g->GadgetText;t;t=t->NextText)
        if(t->IText&&!strcmp((char *)t->IText,name))break;if(t)break;}
    if(!g){
        /* ASL's layout object owns its bottom-row child buttons. */
        memset(&fallback,0,sizeof(fallback));fallback.LeftEdge=!strcmp(name,"Cancel")?w->Width-78:10;
        fallback.TopEdge=w->Height-26;fallback.Width=66;fallback.Height=14;g=&fallback;
    }
    ActivateWindow(w);Delay(3);
    if(!pointer(w->WScreen,w->LeftEdge+g->LeftEdge+(g->Flags&GFLG_RELRIGHT?w->Width:0)+(g->Width+(g->Flags&GFLG_RELWIDTH?w->Width:0))/2,w->TopEdge+g->TopEdge+(g->Flags&GFLG_RELBOTTOM?w->Height:0)+(g->Height+(g->Flags&GFLG_RELHEIGHT?w->Height:0))/2))return 0;
    Delay(3);port=CreateMsgPort();if(!port)return 0;io=(struct IOStdReq *)CreateIORequest(port,sizeof(*io));if(!io)return 0;
    if(OpenDevice("input.device",0,(struct IORequest *)io,0)){DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return 0;}
    for(up=0;up<2;++up){memset(&e,0,sizeof(e));e.ie_Class=IECLASS_RAWMOUSE;
        e.ie_Code=up?IECODE_LBUTTON|IECODE_UP_PREFIX:IECODE_LBUTTON;e.ie_Qualifier=up?0:IEQUALIFIER_LEFTBUTTON;
        io->io_Command=IND_WRITEEVENT;io->io_Data=&e;io->io_Length=sizeof(e);DoIO((struct IORequest *)io);Delay(3);}
    CloseDevice((struct IORequest *)io);DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return 1;
}

static int wait_ready(int seconds)
{int i;for(i=0;i<seconds*50;++i){pump();if(!strncmp(last_status,"Page ",5))return 1;Delay(1);}printf("Last status: %s\n",last_status);return 0;}
int main(void)
{
 struct Window *w=NULL,*save=NULL;struct Gadget *g;struct List *labels=NULL;struct Node *n;int i,selected=-1,rc=10;BPTR guard,in,out;FILE *f;unsigned char header[32];long size;
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);if(!IntuitionBase||!GadToolsBase)return 20;
 SystemTags("MakeDir RAM:AminetEnv RAM:AminetEnv/NovaDock QATests:Downloads",TAG_DONE);SystemTags("Assign ENV: RAM:AminetEnv",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto done;fputs("OPTION.AMINET.DRAWER=QATests:Downloads\n",f);fclose(f);
 host=CreateMsgPort();if(!host)goto done;host->mp_Node.ln_Name=ND_PORT_NAME;host->mp_Node.ln_Pri=ND_HOST_ABI;AddPort(host);
 in=Open("NIL:",MODE_OLDFILE);out=Open("AutoRun:aminet-client.log",MODE_NEWFILE);
 if(SystemTags("QATests:Dock/Plugins/Aminet AMINET",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)goto done;
 for(i=0;i<500&&!w;++i){pump();w=find_window("NovaDock AMINET");Delay(1);}if(!w){puts("FAIL real Aminet window missing");goto done;}
 puts("PASS real Aminet browser opened");
 if(!wait_ready(45)){puts("FAIL initial live Aminet listing");goto done;}
 for(g=w->FirstGadget;g&&g->GadgetID!=1;g=g->NextGadget){}if(!g)goto done;
 GT_SetGadgetAttrs(g,w,NULL,GTST_String,(ULONG)"JustName",TAG_DONE);last_status[0]=0;if(!post_ui(w,2,0)||!wait_ready(45)){puts("FAIL live search");goto done;}
 for(g=w->FirstGadget;g&&g->GadgetID!=10;g=g->NextGadget){}if(!g)goto done;
 {int page=0,count=0;if(sscanf(last_status,"Page %d: %d",&page,&count)!=2 || page!=1 || count!=1){printf("FAIL expected one exact search result: %s\n",last_status);goto done;}}
 selected=0;puts("PASS live search populated real Aminet browser with one exact JustName result");
 if(!post_ui(w,10,selected)||!post_ui(w,32,0))goto done;
 for(i=0;i<500&&!save;++i){pump();save=find_window("Save Aminet package");Delay(1);}if(!save){puts("FAIL save requester missing");goto done;}
 if(!click_named(save,"Save"))goto done;
 for(i=0;i<3000;++i){pump();if(!strncmp(last_status,"Saved:",6))break;Delay(1);}if(i==3000){printf("FAIL download: %s\n",last_status);goto done;}
 f=fopen("QATests:Downloads/JustName.lha","rb");if(!f)goto done;fseek(f,0,SEEK_END);size=ftell(f);rewind(f);i=(int)fread(header,1,sizeof(header),f);fclose(f);
 if(size<=0||size>1048576L||!nd_aminet_archive_header("util/cli/JustName.lha",header,(unsigned int)i)){printf("FAIL downloaded header/size %ld\n",size);goto done;}
 printf("PASS browser Get/ASL saved complete LHA archive: %ld bytes; package was never executed\n",size);rc=0;
 done:if(host){for(i=0;i<600;++i){struct NDMessage *m;while((m=(struct NDMessage *)GetMsg(host))!=NULL){m->event=ND_EVENT_QUIT;if(m->version==5)((struct NDExtMessage *)m)->event2=0;ReplyMsg((struct Message *)m);}Delay(1);}RemPort(host);DeleteMsgPort(host);}if(GadToolsBase)CloseLibrary(GadToolsBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return rc;
}
