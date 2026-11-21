/* Real plugin UI, fake dock host, separate AmiHASS SAMPLE service only. */
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "nd_protocol.h"
#include "nd_control.h"
#include "nd_hass_native.h"
struct IntuitionBase *IntuitionBase=NULL;long __stack=65536;
static const char *entity="lock.amihass_sample_front_door";
static struct MsgPort *host=NULL,*events=NULL;static int quit=0,clicked=0,goodbye=0;
static void pump(void)
{struct NDMessage *m;while((m=(struct NDMessage *)GetMsg(host))!=NULL){m->event=quit ? ND_EVENT_QUIT:0;if(!quit&&!clicked&&m->command==ND_MSG_UPDATE&&strstr(m->value,"Locked")){m->event|=ND_EVENT_CLICK;clicked=1;}if(m->command==ND_MSG_GOODBYE)goodbye=1;ReplyMsg((struct Message *)m);}}
static int stop_test_dock(void)
{struct NDControlMessage *m;struct MsgPort *reply,*port;int i,ok=0;reply=CreateMsgPort();m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!reply||!m)return 0;m->msg.mn_Length=sizeof(*m);m->msg.mn_ReplyPort=reply;m->magic=ND_CONTROL_MAGIC;m->version=ND_CONTROL_VERSION;m->action=ND_CONTROL_QUIT;
 Forbid();port=FindPort(ND_CONTROL_PORT);if(port)PutMsg(port,(struct Message *)m);Permit();if(!port){FreeVec(m);DeleteMsgPort(reply);return 0;}
 for(i=0;i<200;++i){if(GetMsg(reply)){ok=m->result==ND_CONTROL_OK;break;}Delay(2);}if(i==200)return 0;FreeVec(m);DeleteMsgPort(reply);if(!ok)return 0;
 for(i=0;i<200;++i){Forbid();port=FindPort(ND_PORT_NAME);Permit();if(!port)return 1;Delay(2);}return 0;}
static struct AmiHASSMessage *request(struct AmiHASSClient *c,int kind)
{struct AmiHASSMessage *m=NULL;int i;if(!amihass_send(c,kind,"HASS.LockUI",kind==AMIHASS_QUERY ? entity:NULL,NULL,0,AMIHASS_QUERY_ONCE,NULL,NULL))return NULL;for(i=0;i<250&&!m;++i){if(host)pump();m=amihass_reply(c);if(!m)Delay(2);}return m;}
static int query(struct AmiHASSClient *c,const char *state)
{struct AmiHASSMessage *m=request(c,AMIHASS_QUERY);int ok=m&&m->result==AMIHASS_OK&&!strcmp(m->item.id,entity)&&!strcmp(m->item.state,state);printf("Sample query wanted=%s result=%ld id=%s state=%s\n",state,m ? m->result:-99L,m ? m->item.id:"none",m ? m->item.state:"none");fflush(stdout);if(m)FreeVec(m);return ok;}
static struct Window *prompt(struct Screen *screen)
{struct Window *w;Forbid();for(w=screen->FirstWindow;w;w=w->NextWindow)if(w->Title&&(!strcmp(w->Title,"Confirm Home Assistant action")||!strncmp(w->Title,"Waiting for current state",25)))break;Permit();return w;}
static struct IntuiMessage *input(struct Window *w,int mode)
{struct IntuiMessage *m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);struct Gadget *g;if(!m)return NULL;m->ExecMessage.mn_Length=sizeof(*m);m->ExecMessage.mn_ReplyPort=events;m->IDCMPWindow=w;
 if(mode==1){m->Class=IDCMP_RAWKEY;m->Code=0x45;}else if(mode==2)m->Class=IDCMP_CLOSEWINDOW;else{for(g=w->FirstGadget;g;g=g->NextGadget)if(g->GadgetID==(mode==0 ? 101:100))break;if(!g){FreeVec(m);return NULL;}m->Class=IDCMP_GADGETUP;m->IAddress=g;}
 PutMsg(w->UserPort,(struct Message *)m);return m;}
int main(void)
{struct AmiHASSClient observer;struct AmiHASSMessage *m;struct Screen *screen=NULL;struct Window *w;struct IntuiMessage *sent=NULL;FILE *f=NULL;BPTR dir,in,out;LONG rc;int i,k=-1,ok=0,configured=0,backup=0,started=0,published=0;memset(&observer,0,sizeof(observer));
 dir=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!dir){puts("FAIL: isolated QA marker required.");return 10;}UnLock(dir);
 m=request(&observer,AMIHASS_STATUS);if(!m)return 10;i=m->result==AMIHASS_OK&&!strcmp(m->item.state,AMIHASS_SERVER_SAMPLE);FreeVec(m);if(!i){puts("FAIL: sample service required; no controls sent.");return 10;}
 if(!query(&observer,"locked"))goto done;
 Forbid();i=FindPort(ND_PORT_NAME)!=NULL;Permit();printf("Existing dock port=%d\n",i);fflush(stdout);if(i&&!stop_test_dock())goto done;k=-10;host=CreateMsgPort();events=CreateMsgPort();if(!host||!events)goto done;host->mp_Node.ln_Name=ND_PORT_NAME;host->mp_Node.ln_Pri=ND_HOST_ABI;AddPort(host);published=1;
 k=-11;IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(!IntuitionBase)goto done;screen=LockPubScreen("Workbench");if(!screen)goto done;
 k=-12;dir=CreateDir("ENV:NovaDock");if(dir)UnLock(dir);backup=rename("ENV:NovaDock/novadock.cfg","ENV:NovaDock/lock-ui-qa.backup")==0;
 k=-13;f=fopen("ENV:NovaDock/novadock.cfg","wb");if(!f)goto done;configured=1;fputs("OPTION.HASS_LOCK.ENTITY=lock.amihass_sample_front_door\nOPTION.HASS_LOCK.ACTION=Unlock\nOPTION.HASS_LOCK.TYPE=lock\nOPTION.HASS_LOCK.READONLY=NO\n",f);fclose(f);f=NULL;
 k=-14;in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(!in||!out)goto done;rc=SystemTags("QATests:HASS HASS_LOCK",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(rc<0){Close(in);Close(out);goto done;}started=1;
 for(k=0;k<4;++k){clicked=0;w=NULL;for(i=0;i<600&&!w;++i){pump();w=prompt(screen);if(!w)Delay(2);}if(!w)goto done;printf("Confirmation scenario %d at %dx%d\n",k,screen->Width,screen->Height);fflush(stdout);
  if(w->TopEdge+w->Height>screen->Height)goto done;sent=input(w,k);if(!sent)goto done;
  for(i=0;i<200;++i){pump();if(GetMsg(events)){FreeVec(sent);sent=NULL;break;}Delay(2);}if(sent)goto done;printf("Scenario %d input replied\n",k);fflush(stdout);
  for(i=0;i<300&&prompt(screen);++i){pump();if(k==3&&i&&i%25==0){sent=input(prompt(screen),3);if(!sent)goto done;while(!GetMsg(events)){pump();Delay(2);}FreeVec(sent);sent=NULL;}Delay(2);}if(prompt(screen))goto done;printf("Scenario %d window closed\n",k);fflush(stdout);
  for(i=0;i<50;++i){pump();Delay(2);}if(!query(&observer,k==3 ? "unlocked":"locked"))goto done;printf("Scenario %d sample state confirmed\n",k);fflush(stdout);
 }
 puts("PASS: actual HASS popup Cancel, Escape and window-close leave the sample lock unchanged; Confirm alone unlocks it through AmiHASS.");ok=1;
 done:printf("Lock UI cleanup: success=%d, stage=%d\n",ok,k);fflush(stdout);if(f)fclose(f);if(started&&host){quit=1;for(i=0;i<400&&!goodbye;++i){pump();Delay(2);}if(!goodbye)ok=0;printf("Lock UI plugin goodbye=%d\n",goodbye);fflush(stdout);}if(host){if(published)RemPort(host);while((m=(struct AmiHASSMessage *)GetMsg(host))!=NULL)ReplyMsg((struct Message *)m);DeleteMsgPort(host);}if(events&&!sent)DeleteMsgPort(events);if(screen)UnlockPubScreen(NULL,screen);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);if(configured)DeleteFile("ENV:NovaDock/novadock.cfg");if(backup)rename("ENV:NovaDock/lock-ui-qa.backup","ENV:NovaDock/novadock.cfg");amihass_client_close(&observer);if(!ok)puts("FAIL: sample lock UI check did not finish.");return ok ? 0:10;}
