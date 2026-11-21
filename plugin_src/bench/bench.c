#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/timer.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <string.h>
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_list.h"
#include "nd_task.h"
#include "nd_text.h"
#include "nd_bench.h"
#include "nd_plugin_versions.h"
struct Device *TimerBase=NULL;struct GfxBase *GfxBase=NULL;struct IntuitionBase *IntuitionBase=NULL;struct Library *GadToolsBase=NULL;
const char nd_version[]=ND_PLUGIN_VER_STRING(BENCH,"Bench");long __stack=65536;
#define HISTORY ND_BENCH_HISTORY

static int alive(const char *parent){int ok;if(!parent)return 1;Forbid();ok=FindPort((STRPTR)parent)!=NULL;Permit();return ok;}
static unsigned long clock_now(void){struct EClockVal t;ReadEClock(&t);return t.ev_lo;}
static void date(char out[32]){struct DateStamp d;long day;int y=1978,m=1,n;static const int months[]={31,28,31,30,31,30,31,31,30,31,30,31};DateStamp(&d);day=d.ds_Days;while(day>=(n=365+(!(y%4)&&((y%100)||!(y%400))))){day-=n;++y;}while(m<12 && day>=(n=months[m-1]+(m==2&&!(y%4)&&((y%100)||!(y%400))))){day-=n;++m;}sprintf(out,"%04d-%02d-%02ld %02ld:%02ld:%02ld",y,m,day+1,d.ds_Minute/60,d.ds_Minute%60,d.ds_Tick/50);}
static unsigned long milliseconds(unsigned long ticks,unsigned long hz){unsigned long divider=hz/1000;return divider ? ticks/divider:0;}
/* Save the retained report without starting timers or repeating measurements. */
static int save_report(struct NDBenchJob *j,const char *parent)
{BPTR lock;struct NDBenchResult check;if(!memchr(j->path,0,sizeof(j->path))||!memchr(j->report,0,sizeof(j->report))||!nd_bench_parse(j->report,&check))return 0;if(!alive(parent))return -2;
 if(parent){lock=CreateDir("ENVARC:NovaDock");if(lock)UnLock(lock);lock=CreateDir("ENVARC:NovaDock/Bench");if(lock)UnLock(lock);}
 j->save_result=nd_text_export(j->path,j->report,strlen(j->report));return j->save_result==1 ? 1:2;}
static int run(struct NDBenchJob *j,const char *parent)
{
 struct MsgPort *port=NULL;struct timerequest *timer=NULL;struct BitMap *bitmap=NULL;struct RastPort rp;struct Screen *screen=NULL;unsigned char *memory=NULL;struct EClockVal start;unsigned long hz,t,elapsed,units,seed=1,target;int stage,i,ok=0,opened=0;
 port=CreateMsgPort();if(port)timer=(struct timerequest *)CreateIORequest(port,sizeof(*timer));if(!timer || OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)timer,0))goto done;opened=1;TimerBase=timer->tr_node.io_Device;
 GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(!GfxBase || !IntuitionBase)goto done;
 memory=AllocVec(65536,MEMF_PUBLIC);bitmap=AllocBitMap(320,128,2,BMF_CLEAR|BMF_DISPLAYABLE,NULL);if(!memory||!bitmap)goto done;memset(memory,0x5a,65536);InitRastPort(&rp);rp.BitMap=bitmap;
 memset(&j->result,0,sizeof(j->result));date(j->result.date);j->result.cpu_flags=SysBase->AttnFlags;j->result.chip=AvailMem(MEMF_CHIP);j->result.fast=AvailMem(MEMF_FAST);strcpy(j->result.memory_kind,TypeOfMem(memory)&MEMF_FAST ? "Fast":"Chip/other");screen=LockPubScreen("Workbench");if(screen){j->result.screen_width=screen->Width;j->result.screen_height=screen->Height;j->result.screen_depth=GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH);UnlockPubScreen(NULL,screen);screen=NULL;}
 hz=ReadEClock(&start);j->result.clock_hz=hz;if(!hz)goto done;target=(hz/1000)*(j->duration ? j->duration:1000);if(!target)goto done;
 for(stage=0;stage<3;++stage){nd_task_stage(parent,(unsigned int)stage+1);t=clock_now();units=0;do{
   if(!alive(parent)){ok=-2;goto done;}
   if(stage==0){seed=nd_bench_kernel(seed,1024);units+=1024;}
   else if(stage==1){nd_bench_copy(memory+32768,memory,32768);nd_bench_copy(memory,memory+32768,32768);units+=64;}
   else{for(i=0;i<16;++i){SetAPen(&rp,(units+i)&3);RectFill(&rp,(i*13)%240,(i*7)%80,(i*13)%240+63,(i*7)%80+31);}WaitBlit();units+=16;}
   elapsed=clock_now()-t;
  }while(elapsed<target && units<0xff000000UL);
  if(stage==0)j->result.cpu=nd_bench_rate(units,milliseconds(elapsed,hz));else if(stage==1)j->result.memory=nd_bench_rate(units,milliseconds(elapsed,hz));else j->result.graphics=nd_bench_rate(units,milliseconds(elapsed,hz));
 }
 if(!seed)j->result.cpu+=0; /* Kernel result is consumed, not optimized away. */
 if(!nd_bench_report(&j->result,j->report,sizeof(j->report)))goto done;
 if(!parent){sprintf(j->path,"T:NovaDock-Bench-selftest-%08lx.txt",clock_now());}else{sprintf(j->path,"ENVARC:NovaDock/Bench/bench-%.4s%.2s%.2s-%.2s%.2s%.2s-%04lx.txt",j->result.date,j->result.date+5,j->result.date+8,j->result.date+11,j->result.date+14,j->result.date+17,clock_now()&0xffffUL);}
 ok=save_report(j,parent);
 done:if(screen)UnlockPubScreen(NULL,screen);if(bitmap){WaitBlit();FreeBitMap(bitmap);}if(memory)FreeVec(memory);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);IntuitionBase=NULL;GfxBase=NULL;if(opened)CloseDevice((struct IORequest *)timer);TimerBase=NULL;if(timer)DeleteIORequest((struct IORequest *)timer);if(port)DeleteMsgPort(port);return ok;
}
static int history(struct NDBenchJob *j,const char *parent)
{
 BPTR lock;struct FileInfoBlock *fib;unsigned int scanned=0,i,k;char path[320],text[2048];FILE *f;size_t n;j->count=0;lock=Lock("ENVARC:NovaDock/Bench",ACCESS_READ);if(!lock)return 1;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return 0;}
 if(Examine(lock,fib))while(ExNext(lock,fib)){
  if(!alive(parent)){FreeDosObject(DOS_FIB,fib);UnLock(lock);return -2;}if(++scanned>4096){FreeDosObject(DOS_FIB,fib);UnLock(lock);return -4;}if(fib->fib_DirEntryType>=0 || strncmp(fib->fib_FileName,"bench-",6) || strlen(fib->fib_FileName)>95)continue;
  for(i=0;i<j->count && strcmp(j->names[i],fib->fib_FileName)>0;++i){}if(i>=HISTORY)continue;
  sprintf(path,"ENVARC:NovaDock/Bench/%s",fib->fib_FileName);f=fopen(path,"rb");if(!f)continue;n=fread(text,1,sizeof(text)-1,f);text[n]=0;if(ferror(f)||fgetc(f)!=EOF){fclose(f);continue;}fclose(f);{struct NDBenchResult r;if(!nd_bench_parse(text,&r))continue;for(k=j->count<HISTORY ? j->count:HISTORY-1;k>i;--k){j->history[k]=j->history[k-1];strcpy(j->names[k],j->names[k-1]);}j->history[i]=r;strcpy(j->names[i],fib->fib_FileName);if(j->count<HISTORY)++j->count;}
 }FreeDosObject(DOS_FIB,fib);UnLock(lock);return 1;
}
static int work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{struct NDBenchJob *j=data;if(bytes!=sizeof(*j))return 0;return action==1 ? run(j,parent):action==2 ? history(j,parent):action==3 ? save_report(j,parent):0;}
static void controls(struct NDPanel *panel,const struct NDBenchJob *j,int ready,unsigned int pending)
{nd_panel_button(panel,0,"Run",!pending);nd_panel_button(panel,1,"Stop",pending&&pending!=3);nd_panel_button(panel,2,"Retry save",!pending&&ready&&j->save_result!=1&&j->report[0]);nd_panel_button(panel,3,"History",!pending);}
static struct NDPanel *details(struct NDPanel *panel,struct NDBenchJob *j,int ready,const struct NDBenchResult *compare)
{struct NDPanelField f[7];const char *b[]={"Run","Stop","Retry save","History","Close"};char cpu[96],mem[96],gfx[96],env[128],delta[24];int i;if(ready){sprintf(cpu,"%lu ops/s",j->result.cpu);sprintf(mem,"%lu KiB/s (%s)",j->result.memory,j->result.memory_kind);sprintf(gfx,"%lu fills/s (320x128x2 planar)",j->result.graphics);}else{strcpy(cpu,"Not measured");mem[0]=gfx[0]=0;}if(compare){nd_bench_change(j->result.cpu,compare->cpu,delta);sprintf(cpu,"%s (%lu / %lu ops/s)",delta,j->result.cpu,compare->cpu);nd_bench_change(j->result.memory,compare->memory,delta);sprintf(mem,"%s (%lu / %lu KiB/s)",delta,j->result.memory,compare->memory);nd_bench_change(j->result.graphics,compare->graphics,delta);sprintf(gfx,"%s (%lu / %lu fills/s)",delta,j->result.graphics,compare->graphics);}sprintf(env,"CPU flags %lx; Chip %lu KiB; Fast %lu KiB",j->result.cpu_flags,j->result.chip/1024,j->result.fast/1024);
 f[0].label="CPU";f[0].value=cpu;f[1].label="Memory copy";f[1].value=mem;f[2].label="Off-screen fill";f[2].value=gfx;f[3].label="Configuration";f[3].value=env;f[4].label="Amiga local time";f[4].value=compare ? compare->date:j->result.date;f[5].label="Saved result";f[5].value=ready&&j->save_result!=1 ? nd_text_export_problem(j->save_result):j->path;f[6].label="Test";f[6].value="About 3 seconds; other tasks affect measured rates";for(i=0;i<7;++i)f[i].limit=0;if(panel){nd_panel_title(panel,compare ? "BENCH - saved comparison":"BENCH - results");for(i=0;i<7;++i)nd_panel_set(panel,i,f[i].value);return panel;}return nd_panel_open("BENCH - comparable local workloads",f,7,b,5);}
/* Native QA uses GadTools' public disabled-state API, not private gadget text. */
static int enabled(struct Gadget *g,struct Window *w)
{ULONG disabled=1;return GT_GetGadgetAttrs(g,w,NULL,GA_Disabled,(ULONG)&disabled,TAG_DONE)==1&&!disabled;}
static int panel_test(struct NDBenchJob *j)
{struct NDPanel *p=NULL;struct Screen *s=NULL;struct Window *w;struct Gadget *g,*retry=NULL,*stop=NULL;int ok=0,count=0;const char *phase="libraries";
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);if(!IntuitionBase||!GadToolsBase)goto done;s=LockPubScreen("Workbench");if(!s)goto done;
 j->result.cpu=123;j->result.memory=456;j->result.graphics=789;strcpy(j->result.date,"2026-11-21 12:00:00");strcpy(j->result.memory_kind,"Fast");strcpy(j->report,"Retained report");j->save_result=ND_TEXT_PROTECTED;
 phase="open panel";p=details(NULL,j,1,NULL);if(!p)goto done;Delay(2);w=s->FirstWindow;phase="window bounds";if(!w||w->TopEdge+w->Height>s->Height||w->LeftEdge+w->Width>s->Width)goto done;
 phase="button bounds";for(g=w->FirstGadget;g;g=g->NextGadget)if(g->GadgetID>=100&&g->GadgetID<105){++count;if(g->GadgetID==102)retry=g;if(g->GadgetID==101)stop=g;if(g->LeftEdge<0||g->TopEdge<0||g->LeftEdge+g->Width>w->Width||g->TopEdge+g->Height>w->Height||g->Width<80)goto done;}if(!retry||!stop||count!=5)goto done;
 phase="retry enabled";controls(p,j,1,0);if(!enabled(retry,w)||enabled(stop,w))goto done;
 phase="empty disabled";controls(p,j,0,0);if(enabled(retry,w)||enabled(stop,w))goto done;
 phase="saved disabled";j->save_result=1;controls(p,j,1,0);if(enabled(retry,w))goto done;j->save_result=ND_TEXT_PROTECTED;
 phase="stop enabled";controls(p,j,1,1);if(enabled(retry,w)||!enabled(stop,w))goto done;
 phase="saving disabled";controls(p,j,1,3);if(enabled(retry,w)||enabled(stop,w))goto done;
 if(w!=s->FirstWindow)goto done;printf("PASS: BENCH retry/stop/saving controls and five-button bounds on %dx%d Workbench.\n",s->Width,s->Height);ok=1;
 done:if(!ok)printf("FAIL: BENCH UI phase %s\n",phase);nd_panel_close(p);if(s)UnlockPubScreen(NULL,s);if(GadToolsBase)CloseLibrary(GadToolsBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);GadToolsBase=NULL;IntuitionBase=NULL;return ok;}
int main(int argc,char **argv)
{struct NDClient dock;struct NDTask task;struct NDTaskPacket *reply;struct NDBenchJob *j;struct NDPanel *panel=NULL;struct NDList *list=NULL;unsigned int event,elapsed=0,i,pending=0;int action,selected,ready=0,shown_stage=0,ok;char value[64]="Click to benchmark",rows[HISTORY][128];const char *ptr[HISTORY],*buttons[]={"Compare","Close"};
 if(argc==3&&!strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],work);j=AllocVec(sizeof(*j),MEMF_PUBLIC|MEMF_CLEAR);if(!j)return 20;
 if(argc==2&&!strcmp(argv[1],"PANEL-SELFTEST")){ok=panel_test(j);FreeVec(j);return ok ? 0:10;}
 if(argc==2&&!strcmp(argv[1],"SELFTEST")){int ok;j->duration=200;ok=run(j,NULL);if(ok==1){DeleteFile(j->path);puts(j->report);}FreeVec(j);return ok==1 ? 0:10;}
 if(argc!=2||!nd_client_init(&dock,argv[1])){FreeVec(j);return 20;}memset(&task,0,sizeof(task));((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 for(;;){reply=nd_task_poll(&task);if(reply){pending=0;if(reply->bytes==sizeof(*j) && reply->result>0){unsigned int kind=reply->action;*j=*(struct NDBenchJob *)reply->data;if(kind==1||kind==3){ready=1;sprintf(value,"CPU %lu ops/s",j->result.cpu);if(panel){panel=details(panel,j,ready,NULL);if(reply->result==2)nd_panel_title(panel,"Result not saved - fix destination and Retry save");}}else{if(!list)list=nd_list_open("Saved BENCH results (latest 16)",buttons,2);for(i=0;i<j->count;++i){sprintf(rows[i],"%.31s - CPU %lu / RAM %lu / fill %lu",j->history[i].date,j->history[i].cpu,j->history[i].memory,j->history[i].graphics);ptr[i]=rows[i];}nd_list_rows(list,ptr,j->count);}}else if(panel)nd_panel_title(panel,"Benchmark/history failed; settings are unchanged");FreeVec(reply);}
  if(task.port && task.stage && (int)task.stage!=shown_stage){const char *stages[]={"","CPU (1/3)","Memory copy (2/3)","Off-screen drawing (3/3)"};if(task.stage<=3){char title[96];sprintf(title,"BENCH - %s; Stop cancels",stages[task.stage]);nd_panel_title(panel,title);shown_stage=task.stage;}}
  event=nd_client_update_art_quiet(&dock,"BENCH",task.port ? (pending==3 ? "Saving...":"Working..."):value,"bench",task.port ? 1:0,elapsed,250);if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else {panel=details(panel,j,ready,NULL);shown_stage=0;}}
  controls(panel,j,ready,pending);
  action=nd_panel_event(panel);if(action==-1||action==104){nd_panel_close(panel);panel=NULL;}else if(action==100 && !task.port){j->duration=1000;shown_stage=0;if(nd_task_start(&task,argv[0],"NovaDock.Bench.Worker",1,j,sizeof(*j))){pending=1;nd_panel_title(panel,"Benchmark running - Stop cancels between batches");}else nd_panel_title(panel,"Previous worker is still stopping; retry shortly");}
  else if(action==101&&task.port&&pending!=3){nd_task_close(&task);pending=0;nd_panel_title(panel,"Benchmark stopped; partial result not saved");}
  else if(action==102&&!task.port&&ready&&j->save_result!=1&&j->report[0]){if(nd_task_start(&task,argv[0],"NovaDock.Bench.Worker",3,j,sizeof(*j))){pending=3;nd_panel_title(panel,"Saving completed benchmark; measurements retained");}else nd_panel_title(panel,"Previous worker is still stopping; retry shortly");}
  else if(action==103 && !task.port){if(nd_task_start(&task,argv[0],"NovaDock.Bench.Worker",2,j,sizeof(*j)))pending=2;else nd_panel_title(panel,"History worker busy");}
  action=nd_list_event(list);selected=nd_list_selected(list);if(action==-1||action==101){nd_list_close(list);list=NULL;}else if(action==100 && selected>=0 && selected<(int)j->count){if(ready){panel=details(panel,j,ready,&j->history[selected]);}else{j->result=j->history[selected];j->save_result=1;ready=1;strcpy(j->path,j->names[selected]);panel=details(panel,j,ready,NULL);}}
  elapsed=nd_client_wait_signals(&dock,task.port ? 5:250,nd_panel_signals(panel)|nd_list_signals(list)|nd_task_signals(&task));
 }nd_task_close(&task);nd_list_close(list);nd_panel_close(panel);nd_client_close(&dock);FreeVec(j);return 0;
}
