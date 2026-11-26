/* Native compatibility, privacy, allocation and opt-in CPU sampling checks. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#include <proto/intuition.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_client.h"
#include "nd_control_client.h"
#include "nd_plugin_resources.h"
long __stack=65536;
static int failures;
static void check(int ok,const char *label){printf("%s %s\n",ok?"PASS":"FAIL",label);if(!ok)++failures;}
static int launch(const char *text){BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc=SystemTags((STRPTR)text,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(rc<0){if(in)Close(in);if(out)Close(out);}return rc>=0;}
static int worker(int busy)
{
 struct NDClient c;unsigned int event;char *a,*b;int i;volatile ULONG sum=0;
 if(!nd_client_init(&c,"RESOURCE"))return 20;
 a=malloc(4096);b=calloc(64,16);if(!a||!b)return 20;memset(a,7,4096);a=realloc(a,8192);if(!a)return 20;
 for(;;){nd_client_activity(&c,ND_PLUGIN_OP_SCAN,2,129,0);event=nd_client_update(&c,"SECRET-DO-NOT-LOG","PRIVATE-DO-NOT-LOG",ND_NO_METER);if(event&ND_EVENT_QUIT)break;
  if(busy)for(i=0;i<300000;++i)sum=sum*1664525UL+(ULONG)i;
  else nd_client_wait(&c,10);
 }free(a);free(b);nd_client_close(&c);return sum==0x13579UL?1:0;
}
static void protocol(int old)
{
 struct MsgPort *port=CreateMsgPort();struct NDMessage *m;ULONG ticks=0;int started=0,off=0,logging=0,trace=0,goodbye=0,mode,cadence=1;ULONG last=0;
 if(!port){++failures;return;}port->mp_Node.ln_Name=ND_PORT_NAME;port->mp_Node.ln_Pri=old?0:ND_HOST_ABI;AddPort(port);
 if(!launch("QATests:Regression WORK")){++failures;goto done;}
 for(ticks=0;ticks<650 && !goodbye;++ticks){mode=ticks<100?0:ticks<350?1:2;
  while((m=(struct NDMessage *)GetMsg(port))!=NULL){
   m->event=(UWORD)(ticks>=550?ND_EVENT_QUIT:0);
   if(m->version==ND_EXT_ABI_VERSION){struct NDExtMessage *e=(struct NDExtMessage *)m;e->event2=ND_EVENT2_HOST_SUPPORT|(mode==1?ND_EVENT2_SUPPORT_LOGGING:mode==2?ND_EVENT2_SUPPORT_TRACE:0);}
   if(m->command==ND_MSG_UPDATE)started=1;
   if(m->command==ND_MSG_SUPPORT){const struct NDPluginSupportStats *s=&((struct NDPluginSupportMessage *)m)->stats;
    struct DateStamp stamp;ULONG now;DateStamp(&stamp);now=((ULONG)stamp.ds_Days*1440UL+(ULONG)stamp.ds_Minute)*3000UL+(ULONG)stamp.ds_Tick;
    if(last && now-last<(mode==1?245UL:45UL))cadence=0;last=now;
    if(old || !mode)++off;else if(mode==1)++logging;else ++trace;
    check(s->version==1&&s->heap==9216&&s->peak>=9216&&s->allocations==3&&s->frees==1&&s->operation==ND_PLUGIN_OP_SCAN&&s->detail_a==129,"numeric support snapshot tracks realloc, RAM and scan count");
    check(!m->caption[0]&&!m->value[0],"support packet excludes private update text");
   }
   if(m->command==ND_MSG_GOODBYE)goodbye=1;ReplyMsg((struct Message *)m);
  }Delay(1);
 }
 check(started&&goodbye,"client remains responsive and closes cleanly");check(!off,"Off/older hosts receive no support-only traffic");if(!old)check(logging>=1&&trace>=1&&cadence,"Logging and Detailed trace use bounded snapshot cadence");
 printf("Support counts old=%d off=%d logging=%d trace=%d cadence=%d\n",old,off,logging,trace,cadence);
 done:RemPort(port);DeleteMsgPort(port);Delay(10);
}
static int ports(void){struct Node *p;int n=0;extern struct ExecBase *SysBase;Forbid();for(p=SysBase->PortList.lh_Head;p->ln_Succ;p=p->ln_Succ)++n;Permit();return n;}
static void activity_ui(void)
{
 struct IntuitionBase *IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);struct Screen *screen;struct Window *w=NULL;struct Gadget *g;struct IntuiMessage *m;struct MsgPort *reply;struct NDControlPluginResourcesMessage q;int i;
 if(!IntuitionBase){check(0,"activity Intuition base");return;}
 check(launch("QATests:Dock/SupportReport ACTIVITY"),"Support plugin activity helper launched");
 for(i=0;i<500 && !w;++i){ULONG key=LockIBase(0);for(screen=IntuitionBase->FirstScreen;screen&&!w;screen=screen->NextScreen)for(w=screen->FirstWindow;w;w=w->NextWindow)if(w->Title&&!strcmp((char *)w->Title,"NovaDock - Plugin activity"))break;UnlockIBase(key);if(!w)Delay(1);}
 check(w!=NULL,"native resource list opens");if(w){
  for(g=w->FirstGadget;g&&g->GadgetID!=102;g=g->NextGadget){}check(g && (g->Flags&GFLG_DISABLED),"Sample button is disabled during measurement");
  for(g=w->FirstGadget;g&&g->GadgetID!=104;g=g->NextGadget){}reply=CreateMsgPort();m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);
  if(g&&reply&&m){m->ExecMessage.mn_Length=sizeof(*m);m->ExecMessage.mn_ReplyPort=reply;m->IDCMPWindow=w;m->Class=IDCMP_GADGETUP;m->IAddress=g;PutMsg(w->UserPort,(struct Message *)m);for(i=0;i<300;++i){if(GetMsg(reply))break;Delay(1);}check(i<300,"activity list closes normally");if(i<300){FreeVec(m);DeleteMsgPort(reply);}}
 }
 memset(&q,0,sizeof(q));for(i=0;i<100;++i){Delay(5);nd_control_plugin_resources(&q);if(!q.sample_remaining)break;}check(i<100,"closing activity view leaves a bounded sample");CloseLibrary((struct Library *)IntuitionBase);
}
static void cpu(void)
{
 struct NDControlPluginResourcesMessage q;FILE *f;int i,j,base_ports;ULONG baseline;
 SystemTags("MakeDir RAM:ResourceENV RAM:ResourceENV/NovaDock",TAG_DONE);SystemTags("Assign ENV: RAM:ResourceENV",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f){++failures;return;}
 fputs("DRAG_BAR=NO\nREMOVABLE_MEDIA=NO\nHOVER_MAGNIFICATION=0\nPLUGIN=Resource probe|QATests:Regression|RESOURCE|METER|OFF\n",f);fclose(f);
 check(launch("QATests:Dock/NovaDock"),"normal host for CPU sampler launched");for(i=0;i<300;++i){memset(&q,0,sizeof(q));if(nd_control_plugin_resources(&q)==ND_CONTROL_OK)break;Delay(1);}check(i<300&&!q.sample_remaining,"resource listing leaves CPU sampler off");
 /* Probe is started explicitly; it is not an executable configured for autostart. */
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(f){fputs("DRAG_BAR=NO\nREMOVABLE_MEDIA=NO\nHOVER_MAGNIFICATION=0\nPLUGIN=Resource probe|QATests:Regression|RESOURCE|METER|ON\n",f);fclose(f);}nd_control_reload();
 /* The dock-launched test receives RESOURCE as argv[1] and enters busy mode. */
 Delay(100);base_ports=ports();baseline=AvailMem(MEMF_ANY);
 for(j=0;j<3;++j){memset(&q,0,sizeof(q));q.operation=1;check(nd_control_plugin_resources(&q)==ND_CONTROL_OK,"opt-in CPU sampler starts");
  for(i=0;i<100;++i){Delay(5);q.operation=0;if(nd_control_plugin_resources(&q)!=ND_CONTROL_OK)break;if(!q.sample_remaining&&q.rows[0].cpu_valid)break;}
  check(i<100&&q.count==1&&q.rows[0].cpu_samples>=10&&q.rows[0].cpu_hits>0&&q.rows[0].cpu_hits<=q.rows[0].cpu_samples,"jittered native CPU sample detects busy plugin task");check(q.rows[0].stats.version==1&&q.rows[0].stats.heap>=9216,"sample requests resource metadata with logging Off");
  Delay(20);check(ports()==base_ports,"CPU sample closes its temporary message port");printf("CPU pass=%d free=%lu baseline=%lu hits=%lu samples=%lu\n",j,AvailMem(MEMF_ANY),baseline,q.rows[0].cpu_hits,q.rows[0].cpu_samples);
 }
 activity_ui();nd_control_send(ND_CONTROL_QUIT);Delay(60);
}
int main(int argc,char **argv)
{
 BPTR guard;if(argc>1 && !strcmp(argv[1],"WORK"))return worker(0);if(argc>1 && !strcmp(argv[1],"RESOURCE"))return worker(1);
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 protocol(1);protocol(0);cpu();return failures?10:0;
}
