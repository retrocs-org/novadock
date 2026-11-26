#ifndef ND_NATIVE_QA_UI_H
#define ND_NATIVE_QA_UI_H
/* Helpers for disposable native QA only; no installed user settings. */
#include <exec/types.h>
#include <exec/execbase.h>
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
#include <proto/graphics.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_control_client.h"
#include "nd_protocol.h"
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *GadToolsBase;
static ULONG qa_ticks(void){struct DateStamp d;DateStamp(&d);return ((ULONG)d.ds_Days*1440UL+(ULONG)d.ds_Minute)*3000UL+(ULONG)d.ds_Tick;}
static int qa_port(const char *name){int found;Forbid();found=FindPort((STRPTR)name)!=NULL;Permit();return found;}
static int qa_start(const char *command){BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;if(!in||!out){if(in)Close(in);if(out)Close(out);return 0;}rc=SystemTags((STRPTR)command,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(rc<0){Close(in);Close(out);}return rc>=0;}
static struct Window *qa_window(const char *prefix){struct Screen *s;struct Window *w,*found=NULL;ULONG key=LockIBase(0);for(s=IntuitionBase->FirstScreen;s&&!found;s=s->NextScreen)for(w=s->FirstWindow;w;w=w->NextWindow)if(w->UserPort&&w->Title&&(!strncmp(w->Title,prefix,strlen(prefix))||(!strcmp(prefix,"NovaDock Preferences")&&!strncmp(w->Title,"New plugin",10)))){found=w;break;}UnlockIBase(key);return found;}
static struct Window *qa_wait_window(const char *title){int i;struct Window *w;for(i=0;i<1500;++i){w=qa_window(title);if(w){Delay(3);return qa_window(title);}Delay(1);}return NULL;}
static int qa_gone(const char *title){int i;for(i=0;i<300;++i){if(!qa_window(title))return 1;Delay(1);}return 0;}
static struct Gadget *qa_gadget(struct Window *w,UWORD id){struct Gadget *g;for(g=w->FirstGadget;g;g=g->NextGadget)if(g->GadgetID==id)return g;return NULL;}
static int qa_post(struct Window *w,ULONG cls,UWORD code,UWORD id){struct MsgPort *reply;struct IntuiMessage *m;int i;struct Gadget *g=id?qa_gadget(w,id):NULL;if(id&&!g)return 0;reply=CreateMsgPort();m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!reply||!m){if(reply)DeleteMsgPort(reply);if(m)FreeVec(m);return 0;}m->ExecMessage.mn_Length=sizeof(*m);m->ExecMessage.mn_ReplyPort=reply;m->Class=cls;m->Code=code;m->IDCMPWindow=w;m->IAddress=g;PutMsg(w->UserPort,(struct Message *)m);for(i=0;i<250;++i){if(GetMsg(reply)){FreeVec(m);DeleteMsgPort(reply);return 1;}Delay(1);}return 0;}
static int qa_close(const char *title){struct Window *w=qa_window(title);return !w || (qa_post(w,IDCMP_CLOSEWINDOW,0,0)&&qa_gone(title));}
static int qa_input(UWORD cls,UWORD code,UWORD qualifier){struct MsgPort *p=CreateMsgPort();struct IOStdReq *io;struct InputEvent e[2];int ok=0;if(!p)return 0;io=(struct IOStdReq *)CreateIORequest(p,sizeof(*io));if(!io){DeleteMsgPort(p);return 0;}if(!OpenDevice("input.device",0,(struct IORequest *)io,0)){memset(e,0,sizeof(e));e[0].ie_Class=cls;e[0].ie_Code=code;e[0].ie_Qualifier=qualifier;e[1]=e[0];e[1].ie_Code|=IECODE_UP_PREFIX;e[0].ie_NextEvent=&e[1];io->io_Command=IND_WRITEEVENT;io->io_Data=e;io->io_Length=sizeof(e);ok=DoIO((struct IORequest *)io)==0;CloseDevice((struct IORequest *)io);}DeleteIORequest((struct IORequest *)io);DeleteMsgPort(p);Delay(5);return ok;}
static int qa_key(UWORD code,UWORD qualifier){return qa_input(IECLASS_RAWKEY,code,qualifier);}
static int qa_pointer(struct Screen *s,int x,int y){struct MsgPort *p=CreateMsgPort();struct IOStdReq *io;struct InputEvent e;struct IEPointerPixel pixel;int ok=0;if(!p)return 0;io=(struct IOStdReq *)CreateIORequest(p,sizeof(*io));if(!io){DeleteMsgPort(p);return 0;}if(!OpenDevice("input.device",0,(struct IORequest *)io,0)){memset(&e,0,sizeof(e));memset(&pixel,0,sizeof(pixel));pixel.iepp_Screen=s;pixel.iepp_Position.X=x;pixel.iepp_Position.Y=y;e.ie_Class=IECLASS_NEWPOINTERPOS;e.ie_SubClass=IESUBCLASS_PIXEL;e.ie_EventAddress=&pixel;io->io_Command=IND_WRITEEVENT;io->io_Data=&e;io->io_Length=sizeof(e);ok=DoIO((struct IORequest *)io)==0;CloseDevice((struct IORequest *)io);}DeleteIORequest((struct IORequest *)io);DeleteMsgPort(p);return ok;}
static int qa_rexx(const char *command){FILE *f=fopen("QATests:qa-action.rexx","w");if(!f)return 0;fprintf(f,"/* disposable QA */\noptions results\naddress NOVADOCK '%s'\nsay result\nexit rc\n",command);fclose(f);return SystemTags("SYS:Rexxc/RX QATests:qa-action.rexx",TAG_DONE)==0;}
static int qa_stop(void){int i;(void)nd_control_send(ND_CONTROL_QUIT);for(i=0;i<400;++i){if(!qa_port(ND_CONTROL_PORT))return 1;Delay(1);}return 0;}
static int qa_bounds(struct Window *w){struct Gadget *g;int left,top,width,height;if(!w||w->LeftEdge<0||w->TopEdge<w->WScreen->BarHeight||w->LeftEdge+w->Width>w->WScreen->Width||w->TopEdge+w->Height>w->WScreen->Height)return 0;for(g=w->FirstGadget;g;g=g->NextGadget){if((g->GadgetType&GTYP_SYSGADGET)||!g->GadgetID)continue;left=g->LeftEdge;top=g->TopEdge;width=g->Width;height=g->Height;if(g->Flags&GFLG_RELRIGHT)left+=w->Width-1;if(g->Flags&GFLG_RELBOTTOM)top+=w->Height-1;if(g->Flags&GFLG_RELWIDTH)width+=w->Width;if(g->Flags&GFLG_RELHEIGHT)height+=w->Height;if(left<0||top<0||width<0||height<0||left+width>w->Width||top+height>w->Height){printf("BOUNDS gadget=%u rect=%d,%d %dx%d window=%dx%d title=%s\n",g->GadgetID,left,top,width,height,w->Width,w->Height,w->Title);return 0;}}return 1;}
static int qa_init(void){BPTR guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 0;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);return IntuitionBase&&GfxBase&&GadToolsBase;}
static void qa_finish(void){if(GadToolsBase)CloseLibrary(GadToolsBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);}
#endif
