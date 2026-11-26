/* Private DOS handler deliberately stalls Info() to exercise real worker waits. */
#include "native_qa_ui.h"
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include "nd_volume_worker.h"
#include "nd_async_io.h"
#include "nd_task.h"
struct Library *IconBase;
long __stack=65536;
static int write_text(const char *path,const char *text)
{FILE *f=fopen(path,"w");int ok;if(!f)return 0;ok=fputs(text,f)>=0;return fclose(f)==0&&ok;}
static struct FileLock *locks[16];
static BPTR new_lock(struct MsgPort *port)
{int i;struct FileLock *l;for(i=0;i<16&&locks[i];++i){}if(i==16)return 0;l=AllocMem(sizeof(*l),MEMF_PUBLIC|MEMF_CLEAR);if(!l)return 0;l->fl_Task=port;l->fl_Access=ACCESS_READ;locks[i]=l;return MKBADDR(l);}
static void free_lock(BPTR lock)
{int i;for(i=0;i<16;++i)if(locks[i]==BADDR(lock)){FreeMem(locks[i],sizeof(*locks[i]));locks[i]=NULL;return;}}
static int slow_handler(void)
{
 struct MsgPort *port=CreateMsgPort();struct Message *m;FILE *f;BPTR root;int done=0;
 if(!port)return 20;port->mp_Node.ln_Name="NovaDock.QA.SlowFS";AddPort(port);root=new_lock(port);
 f=fopen("QATests:slow-root","w");if(!f)return 20;fprintf(f,"%lu",(ULONG)root);fclose(f);
 while(!done){ULONG signals=Wait((1UL<<port->mp_SigBit)|SIGBREAKF_CTRL_C);if(signals&SIGBREAKF_CTRL_C)done=1;
  while((m=GetMsg(port))!=NULL){struct DosPacket *p=(struct DosPacket *)m->mn_Node.ln_Name;LONG result=DOSFALSE,error=0;unsigned stall=0,kind=1;
   /* FREE_LOCK may arrive while AssignLock holds the DOS list for writing.
    * Do not resolve another assign from that handler path. */
   if(p->dp_Type==ACTION_INFO || p->dp_Type==ACTION_DISK_INFO || p->dp_Type==ACTION_FINDINPUT || p->dp_Type==ACTION_LOCATE_OBJECT){f=fopen("QATests:slow-delay","r");if(f){(void)fscanf(f,"%u %u",&stall,&kind);fclose(f);}}
   if(stall && ((kind==2 && p->dp_Type==ACTION_FINDINPUT) || ((kind==3 || kind==4) && p->dp_Type==ACTION_LOCATE_OBJECT))){write_text("QATests:slow-entered","entered");Delay(stall);}
   if((kind==3 || kind==4) && p->dp_Type==ACTION_LOCATE_OBJECT){result=DOSFALSE;error=ERROR_OBJECT_NOT_FOUND;}
   else if(p->dp_Type==ACTION_LOCATE_OBJECT || p->dp_Type==ACTION_COPY_DIR)result=(LONG)new_lock(port);
   else if(p->dp_Type==ACTION_FREE_LOCK){free_lock((BPTR)p->dp_Arg1);result=DOSTRUE;}
   else if(p->dp_Type==ACTION_IS_FILESYSTEM)result=DOSTRUE;
   else if(p->dp_Type==ACTION_EXAMINE_OBJECT){struct FileInfoBlock *fib=(struct FileInfoBlock *)BADDR(p->dp_Arg2);memset(fib,0,sizeof(*fib));fib->fib_DirEntryType=2;strcpy(fib->fib_FileName,"Slow");result=DOSTRUE;}
   else if(p->dp_Type==ACTION_EXAMINE_NEXT){struct FileInfoBlock *fib=(struct FileInfoBlock *)BADDR(p->dp_Arg2);if(!fib->fib_DiskKey){fib->fib_DiskKey=1;fib->fib_DirEntryType=-3;strcpy(fib->fib_FileName,"Icon");fib->fib_Size=42;result=DOSTRUE;}else error=ERROR_NO_MORE_ENTRIES;}
   else if(p->dp_Type==ACTION_INFO || p->dp_Type==ACTION_DISK_INFO){
    struct InfoData *info=(struct InfoData *)BADDR(p->dp_Type==ACTION_INFO ? p->dp_Arg2:p->dp_Arg1);unsigned delay=0;
    if(kind==1)delay=stall;if(delay){write_text("QATests:slow-entered","entered");Delay(delay);}
    memset(info,0,sizeof(*info));info->id_NumBlocks=100;info->id_NumBlocksUsed=25;info->id_BytesPerBlock=512;result=DOSTRUE;
   }else error=ERROR_ACTION_NOT_KNOWN;
   ReplyPkt(p,result,error);
  }
 }
 RemPort(port);DeleteMsgPort(port);{int i;for(i=0;i<16;++i)if(locks[i])FreeMem(locks[i],sizeof(*locks[i]));}return 0;
}
static int exists(const char *path){BPTR l=Lock((STRPTR)path,ACCESS_READ);if(!l)return 0;UnLock(l);return 1;}
static int wait_guard_gone(void){int i;for(i=0;i<700;++i){if(!qa_port("NovaDock.VolumeMeters"))return 1;Delay(1);}return 0;}
static struct Window *popup_window(void)
{struct Screen *screen;struct Window *w,*found=NULL;ULONG key=LockIBase(0);for(screen=IntuitionBase->FirstScreen;screen&&!found;screen=screen->NextScreen)for(w=screen->FirstWindow;w;w=w->NextWindow)if(w->UserPort && (!w->Title || !*w->Title) && w->Width>=240 && w->Width<=400 && w->Height>=35 && !(w->Flags&WFLG_BACKDROP)){found=w;break;}UnlockIBase(key);return found;}
static int query_folder(const char *path,unsigned int count,int more,int success)
{struct NDTask task;struct NDFolderBatch *request;struct NDTaskPacket *m=NULL;int i,ok=0;memset(&task,0,sizeof(task));request=AllocVec(sizeof(*request),MEMF_PUBLIC|MEMF_CLEAR);if(!request)return 0;strcpy(request->path,path);
 i=nd_task_start(&task,"QATests:Dock/SupportLog","NovaDock.QA.Folder",ND_IO_FOLDER,request,sizeof(*request));FreeVec(request);if(!i)return 0;
 for(i=0;i<500;++i){m=nd_task_poll(&task);if(m)break;Delay(1);}if(m){struct NDFolderBatch *r=(struct NDFolderBatch *)m->data;ok=success ? m->result==1 && r->count==count && r->more==more:m->result!=1 && r->error;
 if(ok && success){for(i=0;i<(int)count;++i)if(!nd_folder_visible_name(r->entries[i].name))ok=0;}
 FreeVec(m);}nd_task_close(&task);return ok;}

int main(int argc,char **argv)
{
 int i,ok=0,stage=1,assigned=0;FILE *f;unsigned long root=0;ULONG start;struct NDVolumeSpace space;struct NDDiagnosticsSnapshot d;
 if(argc==2&&!strcmp(argv[1],"HANDLER"))return slow_handler();
 if(!qa_init())return 20;if(qa_port(ND_CONTROL_PORT))goto done;
 if(!qa_start("QATests:VolumeQA HANDLER"))goto done;
 for(i=0;i<200;++i){f=fopen("QATests:slow-root","r");if(f){(void)fscanf(f,"%lu",&root);fclose(f);if(root)break;}Delay(1);}if(!root||!AssignLock("QASlow",(BPTR)root))goto stop;assigned=1;
 if(argc==2 && !strcmp(argv[1],"POPUP"))goto popup_setup;
 stage=2;nd_volume_meters_begin();nd_volume_meters_add("SYS:");nd_volume_meters_add("QASlow:");nd_volume_meters_add("QASlow:");if(!nd_volume_meters_submit("QATests:Dock/SupportLog"))goto stop;
 for(i=0;i<500;++i){(void)nd_volume_meters_poll();if(nd_volume_meters_cached("QASlow:",&space))break;Delay(1);}if(i==500||space.used!=25||!wait_guard_gone())goto stop;
 puts("PASS native volume worker startup, real DOS query and cached result");
 stage=3;write_text("QATests:slow-delay","250");DeleteFile("QATests:slow-entered");nd_volume_meters_begin();nd_volume_meters_add("SYS:");nd_volume_meters_add("QASlow:");if(!nd_volume_meters_submit("QATests:Dock/SupportLog"))goto stop;
 for(i=0;i<200;++i){(void)nd_volume_meters_poll();if(exists("QATests:slow-entered"))break;Delay(1);}if(i==200||!nd_volume_meters_cached("SYS:",&space))goto stop;
 start=qa_ticks();for(i=0;i<1000;++i)if(!nd_volume_meters_cached("QASlow:",&space)||space.used!=25)goto stop;
 if(qa_ticks()-start>25)goto stop;nd_volume_meters_close();if(nd_volume_meters_signals())goto stop;
 if(!wait_guard_gone())goto stop;puts("PASS last-good volume cache and nonblocking close while Info waits five seconds");
 popup_setup:
 stage=4;if(SystemTags("MakeDir RAM:VolumeEnv RAM:VolumeEnv/NovaDock RAM:VolumeArc RAM:VolumeArc/NovaDock",TAG_DONE)||SystemTags("Assign ENV: RAM:VolumeEnv",TAG_DONE)||SystemTags("Assign ENVARC: RAM:VolumeArc",TAG_DONE)||SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE))goto stop;
 if(!write_text("ENV:NovaDock/novadock.cfg","SIDE=BOTTOM\nHOVER_MAGNIFICATION=0\nREMOVABLE_MEDIA=NO\nDRAWER=Slow|QASlow:||ON|AUTO||||NO|YES\n"))goto stop;
 if(argc==2 && !strcmp(argv[1],"POPUP"))goto popup_tests;
 DeleteFile("QATests:slow-entered");if(!qa_start("QATests:Dock/NovaDock"))goto stop;
 for(i=0;i<250;++i){if(nd_control_diagnostics(&d)==ND_CONTROL_OK&&exists("QATests:slow-entered"))break;Delay(1);}if(i==250)goto stop;
 stage=51;start=qa_ticks();{LONG rc=nd_control_diagnostics(&d);printf("Diagnostics rc=%ld ticks=%lu\n",rc,qa_ticks()-start);if(rc!=ND_CONTROL_OK||qa_ticks()-start>50)goto stop;}
 stage=52;start=qa_ticks();{int stopped=qa_stop();printf("Quit stopped=%d ticks=%lu\n",stopped,qa_ticks()-start);/* Normal shutdown includes the existing 50-tick plugin drain.
    * Require exit well before the 250-tick Info wait, with the worker still alive. */
   if(!stopped||qa_ticks()-start>150||!qa_port("NovaDock.VolumeMeters"))goto stop;}
 if(!wait_guard_gone())goto stop;puts("PASS dock diagnostics and Quit stay responsive while filesystem Info waits");
 stage=6;write_text("QATests:slow-delay","250 2");DeleteFile("QATests:slow-entered");
 if(!write_text("ENV:NovaDock/novadock.cfg","SIDE=BOTTOM\nHOVER_MAGNIFICATION=0\nREMOVABLE_MEDIA=NO\nDRAWER=SlowIcon|QASlow:|QASlow:Disk|ON|AUTO||||NO|NO\nDRAWER=FastIcon|SYS:|QATests:Dock/Icons/Folder.iff|ON|AUTO||||NO|NO\n"))goto stop;
 if(!qa_start("QATests:Dock/NovaDock"))goto stop;
 for(i=0;i<300;++i){if(nd_control_diagnostics(&d)==ND_CONTROL_OK && exists("QATests:slow-entered") && d.icons_ready && d.icon_jobs)break;Delay(1);}if(i==300){printf("Icons ready=%u pending=%u\n",d.icons_ready,d.icon_jobs);goto stop;}
 start=qa_ticks();if(nd_control_diagnostics(&d)!=ND_CONTROL_OK || qa_ticks()-start>50)goto stop;
 if(!qa_rexx("SET GAP 7") || !qa_rexx("STATUS") || !qa_stop())goto stop;
 write_text("QATests:slow-delay","0");for(i=0;i<700 && (qa_port("NovaDock.Icon.0")||qa_port("NovaDock.Icon.1"));++i)Delay(1);if(i==700)goto stop;
 puts("PASS fast icon renders while slow icon waits; reload and Quit safely retire outstanding artwork");
 stage=7;write_text("QATests:slow-delay","250 3");DeleteFile("QATests:slow-entered");
 if(!write_text("ENV:NovaDock/novadock.cfg","SIDE=BOTTOM\nHOVER_MAGNIFICATION=0\nREMOVABLE_MEDIA=NO\nLAUNCH=SlowLaunch|C:True||ON|CLI||QASlow:MissingDir\n"))goto stop;
 if(!qa_start("QATests:Dock/NovaDock"))goto stop;for(i=0;i<300&&!qa_port(ND_CONTROL_PORT);++i)Delay(1);
 if(!qa_rexx("OPEN SlowLaunch"))goto stop;for(i=0;i<200&&!exists("QATests:slow-entered");++i)Delay(1);if(i==200)goto stop;
 start=qa_ticks();if(nd_control_diagnostics(&d)!=ND_CONTROL_OK || !d.launch_jobs || qa_ticks()-start>50)goto stop;
 if(!qa_stop() || !qa_port("NovaDock.Launch.0"))goto stop;write_text("QATests:slow-delay","0");
 for(i=0;i<700 && qa_port("NovaDock.Launch.0");++i)Delay(1);if(i==700)goto stop;
 puts("PASS blocked launcher lookup leaves diagnostics and Quit responsive with bounded independent ownership");
 popup_tests:
 stage=8;{BPTR dir=CreateDir("QATests:PopupFiles");char path[256];if(dir)UnLock(dir);dir=CreateDir("QATests:PopupEmpty");if(dir)UnLock(dir);
 for(i=0;i<70;++i){sprintf(path,"QATests:PopupFiles/File%02d",i);if(!write_text(path,"file"))goto stop;}
 write_text("QATests:PopupFiles/.hidden","hidden");write_text("QATests:PopupFiles/Icon.info","hidden");
 if(!query_folder("QATests:PopupFiles",64,1,1) || !query_folder("QATests:PopupEmpty",0,0,1) || !query_folder("QATests:PopupMissing",0,0,0))goto stop;}
 puts("PASS folder snapshots bound results to 64, hide metadata, distinguish empty and unavailable");
 stage=9;write_text("QATests:slow-delay","250 4");DeleteFile("QATests:slow-entered");
 if(!write_text("ENV:NovaDock/novadock.cfg","SIDE=BOTTOM\nHOVER_MAGNIFICATION=0\nREMOVABLE_MEDIA=NO\nGROUP=Popup|QASlow:|QATests:Dock/Icons/Folder.iff|ON|AUTO||||NO|NO\n"))goto stop;
 if(!qa_start("QATests:Dock/NovaDock"))goto stop;for(i=0;i<300&&!qa_port(ND_CONTROL_PORT);++i)Delay(1);
 if(!qa_rexx("OPEN Popup"))goto stop;for(i=0;i<200&&!exists("QATests:slow-entered");++i)Delay(1);printf("POPUP wait=%d window=%p guard=%d,%d\n",i,popup_window(),qa_port("NovaDock.Folder.0"),qa_port("NovaDock.Folder.1"));if(i==200 || !popup_window())goto stop;stage=91;
 start=qa_ticks();if(nd_control_diagnostics(&d)!=ND_CONTROL_OK || qa_ticks()-start>50)goto stop;
 stage=92;if(!qa_post(popup_window(),IDCMP_VANILLAKEY,27,0))goto stop;Delay(5);if(popup_window())goto stop;
 stage=93;if(!qa_stop() || (!qa_port("NovaDock.Folder.0") && !qa_port("NovaDock.Folder.1")))goto stop;
 write_text("QATests:slow-delay","0");for(i=0;i<700 && (qa_port("NovaDock.Folder.0")||qa_port("NovaDock.Folder.1"));++i)Delay(1);if(i==700)goto stop;
 puts("PASS loading folder cancels immediately; diagnostics and Quit do not wait for directory I/O");
 stage=10;write_text("QATests:slow-delay","250 2");DeleteFile("QATests:slow-entered");if(!qa_start("QATests:Dock/NovaDock"))goto stop;for(i=0;i<300&&!qa_port(ND_CONTROL_PORT);++i)Delay(1);
 if(!qa_rexx("OPEN Popup"))goto stop;for(i=0;i<300&&!exists("QATests:slow-entered");++i)Delay(1);if(i==300 || !popup_window())goto stop;
 start=qa_ticks();if(nd_control_diagnostics(&d)!=ND_CONTROL_OK || qa_ticks()-start>50)goto stop;
 if(!qa_post(popup_window(),IDCMP_VANILLAKEY,27,0) || !qa_stop())goto stop;
 write_text("QATests:slow-delay","0");for(i=0;i<700 && (qa_port("NovaDock.FolderIcon.0")||qa_port("NovaDock.FolderIcon.1"));++i)Delay(1);if(i==700)goto stop;
 puts("PASS popup icon reads are independent; closing safely discards delayed icons");ok=1;
 stop:if(!ok){struct Screen *sc;struct Window *w;printf("FAIL volume worker stage %d IoErr=%ld\n",stage,IoErr());for(sc=IntuitionBase->FirstScreen;sc;sc=sc->NextScreen)for(w=sc->FirstWindow;w;w=w->NextWindow)printf("WINDOW title=%s size=%dx%d flags=%lu\n",w->Title ? w->Title:"(null)",w->Width,w->Height,w->Flags);}nd_volume_meters_close();if(qa_port(ND_CONTROL_PORT))qa_stop();(void)wait_guard_gone();if(assigned)AssignLock("QASlow",0);
 {struct MsgPort *p;Forbid();p=FindPort("NovaDock.QA.SlowFS");if(p)Signal(p->mp_SigTask,SIGBREAKF_CTRL_C);Permit();}
 done:qa_finish();return ok ? 0:10;
}
