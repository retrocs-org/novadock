/* Fresh test guest only. Measures real windows and dock diagnostic counters. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include "nd_reply.h"
#include "nd_control_client.h"
struct IntuitionBase *IntuitionBase=NULL;long __stack=65536;
static ULONG ticks(void){struct DateStamp d;DateStamp(&d);return (ULONG)d.ds_Days*4320000UL+(ULONG)d.ds_Minute*3000UL+d.ds_Tick;}
static char last_launch[80];
static int closed(const char *path)
{int i;BPTR file;for(i=0;i<1500;++i){file=Open((STRPTR)path,MODE_READWRITE);if(file){Close(file);Delay(15);return 1;}Delay(1);}return 0;}
static int start(const char *command){static unsigned int serial;char path[80];BPTR in,out;LONG r;sprintf(path,"QATests:launch-%02u.log",serial++);strcpy(last_launch,path);in=Open("NIL:",MODE_OLDFILE);out=Open(path,MODE_NEWFILE);if(!in||!out)return 0;r=SystemTags((STRPTR)command,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(r<0){Close(in);Close(out);}return r>=0;}
/* A newly linked Intuition window may not have its user port yet. Never
 * send the synthetic close event until that port and the opening task settle. */
static struct Window *find_window(struct Screen *screen,const char *title)
{struct Window *w;for(w=screen->FirstWindow;w;w=w->NextWindow)if(w->UserPort&&w->Title&&!strncmp(w->Title,title,strlen(title)))return w;return NULL;}
static int window_time(const char *side,const char *args,const char *title,const char *phase)
{struct Screen *s;struct Window *w;struct IntuiMessage *m;struct MsgPort *reply;char command[200],log[80];ULONG began;int i,found=0;reply=nd_reply_port();m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);s=LockPubScreen("Workbench");printf("PHASE: opening %s\n",phase);if(!reply||!m||!s)return 0;sprintf(command,"QATests:%s/NovaDockPrefs %s",side,args);began=ticks();if(!start(command))return 0;strcpy(log,last_launch);
 for(i=0;i<6000;++i){Forbid();w=find_window(s,title);if(w)found=1;Permit();if(found){printf("PERF,%s,%s,%lu\n",side,phase,ticks()-began);break;}Delay(1);}if(found){Delay(5);Forbid();w=find_window(s,title);if(w){m->ExecMessage.mn_Length=sizeof(*m);m->ExecMessage.mn_ReplyPort=reply;m->Class=IDCMP_CLOSEWINDOW;m->IDCMPWindow=w;PutMsg(w->UserPort,(struct Message *)m);}Permit();if(!w)return 0;printf("PHASE: close queued %s\n",phase);}if(!found){printf("FAIL: window never opened: %s\n",phase);return 0;}
 for(i=0;i<1500;++i){if(GetMsg(reply))break;Delay(1);}if(i==1500){printf("FAIL: close event not acknowledged: %s\n",phase);nd_reply_abandon(reply);UnlockPubScreen(NULL,s);return 0;}Delay(2);FreeVec(m);nd_reply_delete(reply);printf("PHASE: close replied %s\n",phase);
 for(i=0;i<1500;++i){Forbid();w=find_window(s,title);Permit();if(!w)break;Delay(1);}UnlockPubScreen(NULL,s);printf("PHASE: window gone %s\n",phase);return i<1500&&closed(log);}
static int side(const char *name)
{char command[200],log[80];ULONG began,base_used,used;struct NDDiagnosticsSnapshot a,b;int i;base_used=AvailMem(MEMF_ANY|MEMF_TOTAL)-AvailMem(MEMF_ANY);printf("PHASE: %s\n",name);fflush(stdout);sprintf(command,"Assign NovaDock: QATests:%s",name);if(SystemTags(command,TAG_DONE))return 0;sprintf(command,"Copy QATests:%s/novadock.cfg ENV:NovaDock/novadock.cfg",name);if(SystemTags(command,TAG_DONE))return 0;sprintf(command,"QATests:%s/NovaDock",name);began=ticks();if(!start(command))return 0;strcpy(log,last_launch);
 for(i=0;i<1500;++i){if(nd_control_diagnostics(&a)==ND_CONTROL_OK&&a.plugin_count==8&&a.tile_count>=10)break;Delay(1);}if(i==1500){printf("FAIL: dock startup timed out; plugin count %u; per-tile paints %lu\n",(unsigned int)a.plugin_count,a.tile_count);return 0;}printf("PERF,%s,startup_ticks,%lu\n",name,ticks()-began);Delay(100);
 if(nd_control_diagnostics(&a)!=ND_CONTROL_OK)return 0;Delay(250);if(nd_control_diagnostics(&b)!=ND_CONTROL_OK)return 0;
 printf("PERF,%s,idle_wakes,%lu\n",name,b.wake_count-a.wake_count);printf("PERF,%s,idle_paints,%lu\n",name,b.paint_count-a.paint_count);
 if(!window_time(name,"","NovaDock Preferences - General","prefs_general_ticks")||!window_time(name,"PAGE=PLUGINS","NovaDock Preferences - Plugins","prefs_plugins_cold_ticks")||!window_time(name,"PAGE=PLUGINS","NovaDock Preferences - Plugins","prefs_plugins_warm_ticks")||!window_time(name,"PAGE=PLUGIN PLUGIN=QR","QR settings","prefs_qr_ticks"))return 0;
 used=AvailMem(MEMF_ANY|MEMF_TOTAL)-AvailMem(MEMF_ANY);printf("MEMORY,%s,baseline,%lu,final,%lu\n",name,base_used,used);printf("PERF,%s,used_bytes,%lu\n",name,used>base_used ? used-base_used:0);
 {BPTR lock=Lock("T:",ACCESS_READ);struct FileInfoBlock *fib=AllocDosObject(DOS_FIB,NULL);ULONG bytes=0,count=0;if(lock&&fib&&Examine(lock,fib))while(ExNext(lock,fib))if(fib->fib_DirEntryType<0&&!strncmp(fib->fib_FileName,"NDC4-",5)){bytes+=fib->fib_Size;++count;}if(fib)FreeDosObject(DOS_FIB,fib);if(lock)UnLock(lock);printf("CACHE_FILES,%s,%lu,%lu\n",name,count,bytes);}
 if(nd_control_send(ND_CONTROL_QUIT)!=ND_CONTROL_OK)return 0;if(!closed(log))return 0;Delay(100);return 1;}
int main(int argc,char **argv)
{BPTR marker;int ok;setvbuf(stdout,NULL,_IONBF,0);puts("PHASE: performance main");((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;marker=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!marker)return 10;UnLock(marker);{char guard[80];BPTR seen;snprintf(guard,sizeof(guard),"QATests:perf-%s.started",argc==2 ? argv[1]:"paired");seen=Lock(guard,ACCESS_READ);if(seen){UnLock(seen);puts("FAIL: test guest restarted during performance measurements.");return 10;}seen=Open(guard,MODE_NEWFILE);if(!seen)return 10;Close(seen);}IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(!IntuitionBase)return 10;Delay(250);SystemTags("QATests:Before/NovaDock QUIT",TAG_DONE);Delay(100);
 {BPTR d=CreateDir("RAM:NDPerfEnv");if(d)UnLock(d);d=CreateDir("RAM:NDPerfEnv/NovaDock");if(d)UnLock(d);d=CreateDir("RAM:NDPerfArchive");if(d)UnLock(d);}
 if(SystemTags("Assign ENV: RAM:NDPerfEnv",TAG_DONE)||SystemTags("Assign ENVARC: RAM:NDPerfArchive",TAG_DONE))return 10;ok=argc==2 ? (!strcmp(argv[1],"Before") ? side("Before"):!strcmp(argv[1],"After") ? side("After"):0):side("Before")&&side("After");CloseLibrary((struct Library *)IntuitionBase);if(!ok)puts("FAIL: native performance probe did not complete.");return ok ? 0:10;}
