/* Native Aminet browser; downloads are explicit and never executed. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/icon.h>
#include <workbench/workbench.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_interaction.h"
#include "nd_download.h"
#include "nd_aminet.h"
#include "nd_aminet_http.h"
#include "nd_aminet_progress.h"
#include "nd_aminet_ui.h"
#define FETCH_PROGRESS_FILE "progress"
#define FETCH_STACK_SIZE 32768UL
#define FETCH_GUARD "NovaDock.Aminet.Fetch"
#define FETCH_PREFIX "T:NovaDockAminet-"
#define FETCH_DIR_VALID(path) (!strncmp(path,FETCH_PREFIX,sizeof(FETCH_PREFIX)-1)||nd_download_staging_dir(path))
#include "../weather/nd_fetch_worker.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
const char nd_version[]=ND_PLUGIN_VER_STRING(AMINET,"Aminet");long __stack=32768;
struct AMJob {struct MsgPort *port;char name[80],directory[160],command[1800],target[240];ULONG started,received,total;int transferred,kind;};
struct Library *IconBase=NULL;
static struct AMJob job;static struct NDAminetResults results,categories;
static char status[128]="Click to browse Aminet",drawer[96]="RAM:",self_path[240],category[48],query[81],package[ND_AMINET_PATH];
static int view,listing,page=1; /* view: packages, root, categories, details/readme */
static int am_exists(const char *path){BPTR lock=Lock((STRPTR)path,ACCESS_READ);if(!lock)return 0;UnLock(lock);return 1;}
static void am_job_close(void)
{if(job.port)fetch_close_port(job.port);if(!job.transferred&&job.directory[0])fetch_clean(job.directory);memset(&job,0,sizeof(job));am_download_state(0,0,0);am_status(status);}
static void am_set_status(const char *text){strncpy(status,text,127);status[127]=0;am_status(status);}
static int am_start(const char *remote,int kind,const char *target)
{
 BPTR lock,input,output;char parent[240],part[200],quoted_self[500],quoted_remote[840],quoted_part[420],command[680];LONG rc;int serial,busy;
 if(job.port){am_set_status("A transfer is already running.");return 0;}
 Forbid();busy=FindPort(FETCH_GUARD)!=NULL;Permit();if(busy){am_set_status("Previous transfer is finishing; try again shortly.");return 0;}
 memset(&job,0,sizeof(job));job.started=weather_ticks();job.kind=kind;
 if(target){strncpy(job.target,target,239);job.target[239]=0;if(am_exists(target)){am_set_status("Destination exists; choose another filename.");return 0;}strcpy(parent,target);*FilePart(parent)=0;}
 else strcpy(parent,"T:");
 if(strlen(parent)>=sizeof(job.directory))goto failed;
 for(serial=0;serial<20;++serial){strcpy(job.directory,parent);sprintf(part, target ? "NovaDockDownload-%08lx-%08lx-%d":"NovaDockAminet-%08lx-%08lx-%d",(ULONG)FindTask(NULL),job.started,serial);if(!AddPart(job.directory,part,sizeof(job.directory)))goto failed;lock=CreateDir(job.directory);if(lock){UnLock(lock);break;}}
 if(serial==20)goto failed;sprintf(part,"%s/response",job.directory);
 if(!nd_quote_path(quoted_self,sizeof(quoted_self),self_path)||!nd_quote_path(quoted_remote,sizeof(quoted_remote),remote)||!nd_quote_path(quoted_part,sizeof(quoted_part),part))goto failed;
 sprintf(job.command,"%s FETCH %s %s %d",quoted_self,quoted_remote,quoted_part,target ? 1:0);
 job.port=CreateMsgPort();if(!job.port)goto failed;sprintf(job.name,"NovaDock.Aminet.%08lx.%08lx",(ULONG)FindTask(NULL),job.started);job.port->mp_Node.ln_Name=job.name;Forbid();AddPort(job.port);Permit();
 sprintf(command,"%s WORK %s",quoted_self,job.name);input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(!input||!output){if(input)Close(input);if(output)Close(output);goto failed;}
 rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);
 if(rc==-1){Close(input);Close(output);goto failed;}am_download_state(kind==4,0,0);am_set_status(kind==4 ? "Connecting to download package...":"Loading Aminet...");return 1;
failed:am_job_close();am_set_status("Could not start transfer; check destination/free space.");return 0;
}
static void am_packages(void)
{
 int i;char row[160];if(!ui)return;am_clear();for(i=0;i<results.count;++i){sprintf(row,"%-25.25s %.100s",FilePart(results.rows[i].path),results.rows[i].title);am_row(row);}am_attach();view=0;package[0]=0;
}
static void am_roots(void)
{
 static const char *titles[]={"Business","Communication","Demos","Development","Disk tools","Documentation","Drivers","Games","Graphics","Information","Magazines","Miscellaneous","Modules","Music","Pictures","Text","Utilities"};
 static const char *roots[]={"biz","comm","demo","dev","disk","docs","driver","game","gfx","info","mags","misc","mods","mus","pix","text","util"};int i;
 if(!ui)return;package[0]=0;am_clear();memset(&categories,0,sizeof(categories));for(i=0;i<17;++i){strcpy(categories.rows[i].path,roots[i]);am_row(titles[i]);}categories.count=17;am_attach();view=1;am_set_status("Select a category, then Open. No package runs automatically.");
}
static int am_listing(void)
{
 char remote[400],encoded[241];if(listing==0)strcpy(remote,"/recent");else if(listing==1)sprintf(remote,"/%s?page=%d",category,page);
 else{if(!nd_aminet_encode_query(query,encoded,sizeof(encoded))){am_set_status("Enter 2-80 search characters.");return 0;}sprintf(remote,"/search?query=%s&page=%d",encoded,page);}
 return am_start(remote,1,NULL);
}
static void am_readme(int full)
{char remote[120];if(!package[0]){am_set_status("Select a package first.");return;}if(nd_aminet_readme_path(package,remote,sizeof(remote)))am_start(remote,full ? 3:2,NULL);}
static void am_response(const char *path,int kind)
{
 FILE *f;int ok=0,i;char row[200];struct NDAminetDetails detail;struct NDAminetResults *next=NULL;
 f=fopen(path,"rb");if(!f){am_set_status("Cannot read downloaded metadata.");return;}
 if(kind==1 || kind==5){next=AllocVec(sizeof(*next),MEMF_PUBLIC|MEMF_CLEAR);if(!next){fclose(f);am_set_status("Not enough memory to load this page.");return;}}
 if(kind==1){ok=nd_aminet_search(f,listing==0 ? (page-1)*50:0,next);if(ok){results=*next;am_packages();sprintf(row,"Page %d: %d packages. Open for details; Get to download.",page,results.count);am_set_status(row);}}
 else if(kind==5){ok=nd_aminet_categories(f,category,next);if(ok)categories=*next;if(ok&&ui){am_clear();for(i=0;i<categories.count;++i)am_row(categories.rows[i].path);am_attach();view=2;package[0]=0;am_set_status("Select a subcategory, then Open.");}}
 else if(kind==2){ok=nd_aminet_details(f,&detail);if(ok&&ui){am_clear();am_row(package);am_row(detail.summary);sprintf(row,"Version: %.80s",detail.version);am_row(row);sprintf(row,"Architecture: %.120s",detail.architecture);am_row(row);sprintf(row,"Requires: %.120s",detail.requires);am_row(row);sprintf(row,"Author: %.120s",detail.author);am_row(row);am_attach();view=3;am_set_status("Readme shows the full text. Get chooses where to save.");}}
 else if(kind==3){ok=1;if(ui){am_clear();while(ui->count<AM_ROWS&&fgets(row,160,f)){char *p;for(p=row;*p;++p){if(*p=='\n'||*p=='\r'){*p=0;break;}if((unsigned char)*p<32||*p==127)*p=' ';}am_row(row);}am_attach();view=4;am_set_status(ui->count==AM_ROWS ? "Readme: first 512 lines. Back returns to packages.":"Readme. Back returns to packages.");}}
 if(next)FreeVec(next);fclose(f);if(!ok)am_set_status("Cannot read Aminet response; try another category or search.");
}
static void am_job_events(void)
{
 struct FetchMessage *m;char path[200],error[128];FILE *f;int kind;unsigned long bytes=0;
 if(!job.port)return;
 while((m=(struct FetchMessage *)GetMsg(job.port))!=NULL){
  if(m->magic!=FETCH_MAGIC||m->msg.mn_Length!=sizeof(*m)){if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;}
  if(m->kind==1){if(job.transferred)m->kind=-1;else{strcpy(m->command,job.command);strcpy(m->directory,job.directory);job.transferred=1;}ReplyMsg((struct Message *)m);}
  else if(m->kind==2){kind=job.kind;sprintf(path,"%s/response",m->directory);
   f=fopen(path,"rb");if(f){if(!fseek(f,0,SEEK_END)){long length=ftell(f);if(length>0)bytes=(unsigned long)length;}fclose(f);}
   if(m->result){sprintf(path,"%s/fetch.log",m->directory);f=fopen(path,"rb");error[0]=0;if(f){if(!fgets(error,sizeof(error),f))error[0]=0;fclose(f);}error[strcspn(error,"\r\n")]=0;am_set_status(error[0] ? error:"Aminet transfer failed; check the network.");}
   else if(kind==4){if(am_exists(job.target))am_set_status("Not saved: destination already exists.");else if(Rename(path,job.target)){struct DiskObject *icon;char icon_path[246];sprintf(icon_path,"%s.info",job.target);if(IconBase&&!am_exists(icon_path)&&(icon=GetDefDiskObject(WBPROJECT))!=NULL){PutDiskObject(job.target,icon);FreeDiskObject(icon);}sprintf(error,"Saved: %.110s",job.target);am_set_status(error);}else am_set_status("Cannot save completed download; check free space.");}
   else am_response(path,kind);
   nd_client_activity(NULL,ND_PLUGIN_OP_NETWORK,(m->result || (kind==4 && strncmp(status,"Saved:",6))) ? 3:2,kind,(LONG)bytes);
   fetch_clean(m->directory);FreeVec(m);am_job_close();break;
  }
 }
 if(job.port&&(ULONG)(weather_ticks()-job.started)>(job.kind==4 ? 330UL:45UL)*50UL){am_set_status("Transfer timed out; retry after the background transfer ends.");am_job_close();}
}
/* Worker snapshots are small and written at most twice per second. Do not
 * rely on a live file's directory size: some filesystems update it only at close. */
static void am_progress(void)
{
 FILE *file;unsigned long received,total;char path[200],text[128];
 if(!job.port||job.kind!=4||!job.transferred)return;
 sprintf(path,"%s/progress",job.directory);file=fopen(path,"r");if(!file)return;
 if(nd_aminet_progress_read(file,&received,&total)&&total&&
    (received!=job.received||total!=job.total)){
  job.received=received;job.total=total;am_download_state(1,received,total);
  if(total<1024UL)sprintf(text,"Downloading: %d%% - %lu / %lu bytes",nd_aminet_percent(received,total),received,total);
  else sprintf(text,"Downloading: %d%% - %lu.%lu / %lu.%lu KB",nd_aminet_percent(received,total),received/1024UL,(received%1024UL)*10UL/1024UL,total/1024UL,(total%1024UL)*10UL/1024UL);
  am_set_status(text);
 }fclose(file);
}
static char progress_path[200];
static int am_report_progress(unsigned long received,unsigned long total)
{
 static ULONG previous_tick,previous_done,previous_total;static int reported;
 ULONG now=weather_ticks();BPTR file;char text[64];LONG count;
 if(!progress_path[0])return 1;
 if(reported&&received==previous_done&&total==previous_total)return 1;
 if(reported&&total==previous_total&&received!=total&&(ULONG)(now-previous_tick)<25UL)return 1;
 count=sprintf(text,"%lu %lu\n",received,total);file=Open(progress_path,MODE_NEWFILE);
 if(file){if(Write(file,text,count)==count){previous_tick=now;previous_done=received;previous_total=total;reported=1;}Close(file);}
 return 1;
}
static void am_action(int action)
{
 int selected;char remote[400],target[240];
 if(action==33){am_close();return;}if(!ui)return;
 if(job.port){if(action)am_status("Transfer in progress. You can close this window.");return;}
 if(action==3){listing=0;page=1;am_listing();}
 else if(action==4)am_roots();
 else if(action==5)am_packages();
 else if(action==2){strncpy(query,((struct StringInfo *)ui->query->SpecialInfo)->Buffer,80);query[80]=0;listing=2;page=1;am_listing();}
 else if(action==20||action==21){if(view!=0){am_set_status("Back returns to packages.");return;}if(action==20){if(page==1)return;--page;}else{if(results.count<50||page>=1000)return;++page;}am_listing();}
 else if(action==10){selected=ui->selected;if(view==0&&selected>=0&&selected<results.count){strcpy(package,results.rows[selected].path);am_set_status(results.rows[selected].title);}}
 else if(action==30){selected=ui->selected;if((view==1||view==2)&&selected>=0&&selected<categories.count){strcpy(category,categories.rows[selected].path);page=1;if(strchr(category,'/')){listing=1;am_listing();}else{sprintf(remote,"/tree?path=%s",category);am_start(remote,5,NULL);}}
  else am_readme(0);}
 else if(action==31)am_readme(1);
 else if(action==32){if(!package[0]){am_set_status("Select a package first.");return;}if(am_save_path(drawer,FilePart(package),target,sizeof(target))){sprintf(remote,"/%s",package);am_start(remote,4,target);}}
}
int main(int argc,char **argv)
{
 struct NDClient client;unsigned int event,elapsed=0;char value[32],error[128];BPTR lock;int result=20,reread=1;
 if(argc==5&&!strcmp(argv[1],"FETCH")){int download=!strcmp(argv[4],"1");
  if(download&&strlen(argv[3])+10<sizeof(progress_path)){strcpy(progress_path,argv[3]);*FilePart(progress_path)=0;AddPart(progress_path,"progress",sizeof(progress_path));}
  if(nd_aminet_fetch(argv[2],argv[3],download ? ND_AMINET_DOWNLOAD_LIMIT:262144UL,download ? 300UL:30UL,download,download ? am_report_progress:NULL,error,sizeof(error)))return 0;
  printf("%s\n",error);return 20;}
 if(argc==3&&!strcmp(argv[1],"WORK"))return fetch_worker(argv[2]);
 if(argc<2)return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 lock=GetProgramDir();if(!lock||!NameFromLock(lock,self_path,sizeof(self_path))||!AddPart(self_path,"Aminet",sizeof(self_path)))return 20;
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);AslBase=OpenLibrary("asl.library",39);
 IconBase=OpenLibrary("icon.library",39);
 if(!IntuitionBase||!GfxBase||!GadToolsBase||!AslBase||!nd_client_init(&client,argv[1]))goto done;
 for(;;){int downloading=job.port && job.kind==4;event=0;if(reread){nd_plugin_option_string(argv[1],"DRAWER","RAM:",drawer,sizeof(drawer));reread=0;}am_job_events();if(downloading && !job.port)event|=nd_client_notify(&client,!strncmp(status,"Saved:",6) ? "Aminet download saved":"Aminet download failed",status,!strncmp(status,"Saved:",6) ? 0:2);am_progress();sprintf(value,"%.31s",job.port&&job.kind!=4 ? "Transferring...":status);
  if(job.port)nd_client_activity(&client,ND_PLUGIN_OP_NETWORK,1,job.kind,(LONG)job.received);
  event|=nd_client_update_quiet(&client,"AMINET",value,ND_NO_METER,elapsed,500U);if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
  if(event&ND_EVENT_CLICK){int new_window=!ui;if(am_open()){if(new_window){if(results.count)am_packages();else if(!job.port){listing=0;page=1;am_listing();}}am_download_state(job.port&&job.kind==4,job.received,job.total);am_status(status);}}
  am_action(am_event());elapsed=nd_client_wait_signals(&client,job.port ? 25U:500U,(ui ? 1UL<<ui->window->UserPort->mp_SigBit:0)|(job.port ? 1UL<<job.port->mp_SigBit:0));
 }am_job_close();nd_client_close(&client);result=0;
done:am_close();if(IconBase)CloseLibrary(IconBase);if(AslBase)CloseLibrary(AslBase);if(GadToolsBase)CloseLibrary(GadToolsBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return result;
}
