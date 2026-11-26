/* Run only on disposable QATests/System volumes. Real plugin programs, real
 * dock IPC, measured CPU/resource snapshots and native owned-window checks. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "nd_control_client.h"
#include "nd_support.h"
#include "nd_protocol.h"
int nd_native_rexx_save_barrier(void);
long __stack=65536;
struct IntuitionBase *IntuitionBase;
#define BATCH 6
#define MAX_CASES 64
struct Case {char id[20],program[64];int panel,budget;};
static struct Case cases[MAX_CASES];static int count,failures;
static ULONG now(void)
{struct DateStamp d;DateStamp(&d);return ((ULONG)d.ds_Days*1440UL+(ULONG)d.ds_Minute)*3000UL+(ULONG)d.ds_Tick;}
static int check(int good,const char *id,const char *what)
{printf("%s %s %s\n",good?"PASS":"FAIL",id,what);if(!good)++failures;return good;}
static int present(const char *name)
{int found;Forbid();found=FindPort((STRPTR)name)!=NULL;Permit();return found;}
static struct Task *owner(const char *id)
{char name[64];struct MsgPort *p;struct Task *task=NULL;strcpy(name,ND_WAKE_PORT_PREFIX);strcat(name,id);Forbid();p=FindPort(name);if(p)task=p->mp_SigTask;Permit();return task;}
static struct Window *owned_window(struct Task *task)
{struct Screen *s;struct Window *w,*found=NULL;ULONG key;if(!task)return NULL;key=LockIBase(0);
 for(s=IntuitionBase->FirstScreen;s&&!found;s=s->NextScreen)for(w=s->FirstWindow;w;w=w->NextWindow)
  if(w->UserPort&&w->UserPort->mp_SigTask==task){found=w;break;}
 UnlockIBase(key);return found;}
static int close_owned(struct Window *w)
{struct MsgPort *reply;struct IntuiMessage *m;unsigned int i;
 reply=CreateMsgPort();m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!reply||!m){if(reply)DeleteMsgPort(reply);if(m)FreeVec(m);return 0;}
 m->ExecMessage.mn_Length=sizeof(*m);m->ExecMessage.mn_ReplyPort=reply;m->IDCMPWindow=w;m->Class=IDCMP_CLOSEWINDOW;
 PutMsg(w->UserPort,(struct Message *)m);
 for(i=0;i<250;++i){if(GetMsg(reply)){FreeVec(m);DeleteMsgPort(reply);return 1;}Delay(1);}
 /* Keep a live message/reply port allocated if its recipient stopped replying. */
 return 0;}
static int click(unsigned int index)
{FILE *f=fopen("QATests:qa-click.rexx","w");if(!f)return 0;
 fprintf(f,"/* NovaDock disposable plugin QA */\naddress NOVADOCK 'CLICK %u'\nexit rc\n",index+1);fclose(f);
 return SystemTags("SYS:Rexxc/RX QATests:qa-click.rexx",TAG_DONE)==0;}
static int config(unsigned int first,unsigned int end)
{FILE *f=fopen("ENV:NovaDock/novadock.cfg","w");unsigned int i;if(!f)return 0;
 fputs("SIDE=BOTTOM\nROWS=1\nDRAG_BAR=NO\nHOVER_MAGNIFICATION=0\nAUTO_HIDE=NO\nREMOVABLE_MEDIA=NO\nRUNNING_APPS=NO\nRUNNING_SECTION=NO\nTOOLTIPS=NO\nTHEME=Nord\n",f);
 for(i=first;i<end;++i)fprintf(f,"PLUGIN=%s|%s|%s|METER|ON\n",cases[i].id,cases[i].program,cases[i].id);
 /* Never use external user libraries, devices, network services or launchers. */
 fputs("OPTION.SENSORS.I2C=NO\nOPTION.SENSORS.WARPTOOL=QATests:unavailable\nOPTION.HASS.READONLY=YES\nOPTION.ACTIONS.CONFIRM=YES\nOPTION.WHDLOAD.DRAWER=QATests:Empty\nOPTION.MUSICLIB.DRAWER=QATests:Empty\nOPTION.WATCH.DRAWER=QATests:Empty\nOPTION.LOGS.FILE=QATests:sample.log\n",f);
 return fclose(f)==0;}
static ULONG updates(const struct NDDiagnosticsSnapshot *s,const char *id)
{unsigned int i;for(i=0;i<s->listed_plugins;++i)if(!strcmp(s->plugins[i].id,id))return s->plugins[i].updates;return 0;}
static int sample(struct NDControlPluginResourcesMessage *q)
{unsigned int i;memset(q,0,sizeof(*q));q->operation=1;if(nd_control_plugin_resources(q)!=ND_CONTROL_OK)return 0;
 for(i=0;i<120;++i){Delay(5);q->operation=0;if(nd_control_plugin_resources(q)!=ND_CONTROL_OK)return 0;if(!q->sample_remaining&&q->sample_age!=ND_DIAGNOSTIC_AGE_UNKNOWN)return 1;}return 0;}
static struct NDPluginResourceRow *row(struct NDControlPluginResourcesMessage *q,const char *id)
{unsigned int i;for(i=0;i<q->count;++i)if(!strcmp(q->rows[i].id,id))return &q->rows[i];return NULL;}
static void batch(unsigned int first,unsigned int end)
{struct NDSupportSettings settings;struct NDControlPluginResourcesMessage q;
 struct NDDiagnosticsSnapshot before,after;struct NDPluginResourceRow *r;
 BPTR in,out;unsigned int i,j;ULONG start,packets;int good,base,ready;
 if(!config(first,end)){check(0,"batch","configuration");return;}
 settings.level=0;settings.persist=0;if(!nd_support_write(ND_SUPPORT_ENV,&settings)){check(0,"batch","capture settings");return;}
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
 if(!in||!out||SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE)<0){check(0,"batch","dock startup");return;}
 start=now();memset(&q,0,sizeof(q));
 for(j=0;j<200;++j){ready=nd_control_plugin_resources(&q)==ND_CONTROL_OK&&q.count==end-first;
  for(i=first;i<end&&ready;++i){r=row(&q,cases[i].id);if(!r||!r->online||!owner(cases[i].id))ready=0;}
  if(ready)break;Delay(5);}
 printf("BATCH first=%u count=%u actual=%u startup_ticks=%lu\n",first,end-first,q.count,now()-start);
 memset(&before,0,sizeof(before));memset(&after,0,sizeof(after));nd_control_diagnostics(&before);Delay(250);nd_control_diagnostics(&after);
 check(sample(&q),"batch","five-second CPU/resource sample with capture Off");
 for(i=first;i<end;++i){r=row(&q,cases[i].id);packets=updates(&after,cases[i].id)-updates(&before,cases[i].id);good=r&&r->online&&!r->stale&&r->stats.version==1&&r->cpu_valid&&r->cpu_samples>=10&&r->cpu_hits<=r->cpu_samples&&owner(cases[i].id)!=NULL;
  if(packets>(ULONG)cases[i].budget)good=0;
  printf("METRIC|%s|%d|%lu|%lu|%lu|%lu|%lu|%lu|%lu|%lu|%lu\n",cases[i].id,good,packets,r?r->stats.heap:0,r?r->stats.peak:0,r?r->cpu_hits:0,r?r->cpu_samples:0,r?r->stats.opens:0,r?r->stats.reads:0,r?r->stats.writes:0,r?r->stats.events:0);
  check(good,cases[i].id,"startup, heartbeat, bounded idle packets, tracked RAM and valid CPU sample");}
 settings.level=1;nd_support_write(ND_SUPPORT_ENV,&settings);nd_control_send(ND_CONTROL_SUPPORT_FLUSH);
 nd_control_diagnostics(&after);check(after.support_level==1,"batch","Logging policy applied");
 settings.level=2;nd_support_write(ND_SUPPORT_ENV,&settings);nd_control_send(ND_CONTROL_SUPPORT_FLUSH);
 check(sample(&q),"batch","Detailed trace metadata sample");nd_control_send(ND_CONTROL_SUPPORT_FLUSH);
 for(i=first;i<end;++i){base=failures;r=row(&q,cases[i].id);check(r&&r->stats.version==1&&r->online,cases[i].id,"Detailed trace metadata available");
  if(cases[i].panel){struct Task *task=owner(cases[i].id);struct Window *w=NULL;
   good=click(i-first);for(j=0;j<250&&good;++j){w=owned_window(task);if(w)break;Delay(1);}
   if(w)printf("WINDOW|%s|%d|%d|%d|%d|%d|%d|%d\n",cases[i].id,w->LeftEdge,w->TopEdge,w->Width,w->Height,w->WScreen->Width,w->WScreen->Height,w->WScreen->BarHeight);
   good=good&&w&&w->LeftEdge>=0&&w->TopEdge>=w->WScreen->BarHeight&&w->LeftEdge+w->Width<=w->WScreen->Width&&w->TopEdge+w->Height<=w->WScreen->Height;
   check(good,cases[i].id,"click opens a native window within screen bounds");if(w)check(close_owned(w),cases[i].id,"native close replies");
  }
  printf("CASE|%s|%s\n",cases[i].id,failures==base?"passed":"failed");}
 nd_control_send(ND_CONTROL_QUIT);
 for(j=0;j<400;++j){ready=!present(ND_CONTROL_PORT);for(i=first;i<end&&ready;++i)if(owner(cases[i].id))ready=0;if(ready)break;Delay(1);}
 for(i=first;i<end;++i)check(!owner(cases[i].id),cases[i].id,"shutdown removes the plugin wake port");
 check(!present(ND_CONTROL_PORT),"batch","dock shutdown complete");Delay(10);
}
int main(void)
{BPTR guard,dir;FILE *f;unsigned int first,end;
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(!IntuitionBase)return 20;
 SystemTags("MakeDir RAM:AllPluginENV RAM:AllPluginENV/NovaDock RAM:AllPluginENVArc",TAG_DONE);SystemTags("Assign ENV: RAM:AllPluginENV",TAG_DONE);SystemTags("Assign ENVARC: RAM:AllPluginENVArc",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 dir=CreateDir("QATests:Empty");if(dir)UnLock(dir);f=fopen("QATests:sample.log","w");if(f){fputs("Native plugin QA sample\n",f);fclose(f);}
 f=fopen("QATests:qa-cases.tsv","r");if(!f)return 20;
 while(count<MAX_CASES&&fscanf(f,"%19s %63s %d %d",cases[count].id,cases[count].program,&cases[count].panel,&cases[count].budget)==4)++count;fclose(f);if(!count)return 20;
 for(first=0;first<(unsigned int)count;first=end){end=first+BATCH;if(end>(unsigned int)count)end=(unsigned int)count;batch(first,end);}
 check(nd_native_rexx_save_barrier(),"settings","pending ARexx edit does not override external Preferences save (8 attempts)");
 printf("SUMMARY cases=%d failures=%d\n",count,failures);CloseLibrary((struct Library *)IntuitionBase);return failures?10:0;
}
