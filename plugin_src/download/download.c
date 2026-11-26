/* Serial HTTP(S) download queue. Payloads are staged on the target volume. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_panel.h"
#include "nd_interaction.h"
#include "nd_download.h"
#define FETCH_GUARD "NovaDock.Download.Fetch"
#define FETCH_PREFIX "T:NovaDockDownload-"
#define FETCH_DIR_VALID(path) nd_download_staging_dir(path)
#include "../weather/nd_fetch_worker.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
const char nd_version[]=ND_PLUGIN_VER_STRING(DOWNLOAD,"Download");
long __stack=32768;
struct Item {char url[ND_DOWNLOAD_PATH],target[ND_DOWNLOAD_PATH];};
struct Job {struct MsgPort *port;char name[80],directory[160],command[1800];ULONG started;int transferred;};
static struct Item queue[ND_DOWNLOAD_MAX];static int count;
static struct Job job;static char status[128]="Ready",tool[96],drawer[96];
static int exists(const char *path){BPTR lock=Lock((STRPTR)path,ACCESS_READ);if(!lock)return 0;UnLock(lock);return 1;}
static void read_settings(const char *id)
{struct NDOptionString fields[2];fields[0].key="PROGRAM";fields[0].fallback="C:curl";fields[0].out=tool;fields[0].limit=sizeof(tool);fields[1].key="DRAWER";fields[1].fallback="RAM:";fields[1].out=drawer;fields[1].limit=sizeof(drawer);nd_plugin_option_strings(id,fields,2);}
static void close_job(void)
{if(job.port)fetch_close_port(job.port);if(!job.transferred && job.directory[0])fetch_clean(job.directory);memset(&job,0,sizeof(job));}
static int start_job(const char *program)
{
 BPTR lock,input,output;char executable[180],parent[160],part[180],quoted_tool[370],quoted_url[340],quoted_part[370],self[500],command[600];LONG rc;int serial;
 if(!count || job.port)return 0;
 Forbid();rc=FindPort(FETCH_GUARD)!=NULL;Permit();if(rc){strcpy(status,"Waiting for another download");return 0;}
 if(exists(queue[0].target)){strcpy(status,"Destination already exists");return -1;}
 lock=Lock(tool,ACCESS_READ);if(!lock){strcpy(status,"curl not found; check Edit settings");return -1;}
 rc=NameFromLock(lock,executable,sizeof(executable));UnLock(lock);if(!rc)return -1;
 strcpy(parent,queue[0].target);*FilePart(parent)=0;
 if(!parent[0] || strlen(parent)>100){strcpy(status,"Use a full destination path");return -1;}
 memset(&job,0,sizeof(job));job.started=weather_ticks();
 for(serial=0;serial<20;++serial){strcpy(job.directory,parent);sprintf(part,"NovaDockDownload-%08lx-%08lx-%d",(ULONG)FindTask(NULL),job.started,serial);if(!AddPart(job.directory,part,sizeof(job.directory)))return -1;lock=CreateDir(job.directory);if(lock){UnLock(lock);break;}}
 if(serial==20){job.directory[0]=0;strcpy(status,"Cannot create staging drawer");return -1;}
 sprintf(part,"%s/response",job.directory);
 if(!nd_quote_path(quoted_tool,sizeof(quoted_tool),executable)||!nd_quote_path(quoted_url,sizeof(quoted_url),queue[0].url)||!nd_quote_path(quoted_part,sizeof(quoted_part),part))goto failed;
 sprintf(job.command,"%s --disable --fail --location --proto =http,https --proto-redir =http,https --connect-timeout 30 --max-time 600 --progress-bar --output %s -- %s",quoted_tool,quoted_part,quoted_url);
 job.port=CreateMsgPort();if(!job.port)goto failed;
 sprintf(job.name,"NovaDock.Download.%08lx.%08lx",(ULONG)FindTask(NULL),job.started);job.port->mp_Node.ln_Name=job.name;Forbid();AddPort(job.port);Permit();
 if(!nd_quote_path(self,sizeof(self),program))goto failed;sprintf(command,"%s WORK %s",self,job.name);
 input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(!input||!output){if(input)Close(input);if(output)Close(output);goto failed;}
 rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);
 if(rc==-1){Close(input);Close(output);goto failed;}strcpy(status,"Downloading");return 1;
failed:
 close_job();strcpy(status,"Could not start downloader");return -1;
}
static void pop_queue(void){int i;for(i=1;i<count;++i)queue[i-1]=queue[i];if(count)--count;}
static unsigned int job_events(struct NDClient *client)
{
 struct FetchMessage *m;char part[180];int done=0;
 if(!job.port)return 0;
 while((m=(struct FetchMessage *)GetMsg(job.port))!=NULL){
  if(m->magic!=FETCH_MAGIC || m->msg.mn_Length!=sizeof(*m)){if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;}
  if(m->kind==1){if(job.transferred)m->kind=-1;else{strcpy(m->command,job.command);strcpy(m->directory,job.directory);job.transferred=1;}ReplyMsg((struct Message *)m);}
  else if(m->kind==2){
   sprintf(part,"%s/response",m->directory);
   if(m->result) sprintf(status,"Download failed (curl %ld)",m->result);
   else if(exists(queue[0].target))strcpy(status,"Not saved: destination already exists");
   else if(Rename(part,queue[0].target))sprintf(status,"Saved: %.100s",queue[0].target);
   else strcpy(status,"Could not save completed download");
   fetch_clean(m->directory);FreeVec(m);done=1;break;
  }
 }
 if(done){unsigned int event=nd_client_notify(client,!strncmp(status,"Saved:",6) ? "Download saved":"Download failed",status,!strncmp(status,"Saved:",6) ? 0:2);close_job();pop_queue();return event;}
 else if((!job.transferred && (ULONG)(weather_ticks()-job.started)>200UL) || (ULONG)(weather_ticks()-job.started)>650UL*50UL){strcpy(status,"Download timed out");close_job();pop_queue();return nd_client_notify(client,"Download timed out",status,2);}
 return 0;
}
static unsigned int progress(char *bytes)
{
 char path[180],tail[256],*p,*end;FILE *file;long size;int value=-1;BPTR lock;struct FileInfoBlock *info;
 strcpy(bytes,"0 bytes");if(!job.port)return ND_NO_METER;
 sprintf(path,"%s/response",job.directory);lock=Lock(path,ACCESS_READ);
 if(lock){info=AllocDosObject(DOS_FIB,NULL);if(info){if(Examine(lock,info))sprintf(bytes,"%ld bytes",info->fib_Size);FreeDosObject(DOS_FIB,info);}UnLock(lock);}
 sprintf(path,"%s/fetch.log",job.directory);file=fopen(path,"rb");if(!file)return ND_NO_METER;
 if(!fseek(file,0,SEEK_END)){size=ftell(file);if(size>0 && !fseek(file,size>255 ? size-255:0,SEEK_SET)){size=(long)fread(tail,1,255,file);tail[size]=0;for(p=tail;*p;++p)if(*p=='%'){end=p;while(end>tail && ((end[-1]>='0'&&end[-1]<='9')||end[-1]=='.'))--end;if(end<p)value=atoi(end);}}}
 fclose(file);return value<0 ? ND_NO_METER:(unsigned int)(value>99 ? 99:value);
}
static struct NDPanel *open_panel(void)
{
 struct NDPanelField fields[4];static const char *buttons[]={"Queue URL","Clear waiting","Close"};char target[160];
 strcpy(target,drawer);if(!AddPart(target,"download",sizeof(target)))strcpy(target,"RAM:download");
 fields[0].label="URL";fields[0].value="";fields[0].limit=160;
 fields[1].label="Save as";fields[1].value=target;fields[1].limit=160;
 fields[2].label="Status";fields[2].value=status;fields[2].limit=0;
 fields[3].label="Active";fields[3].value=count ? queue[0].target:"None";fields[3].limit=0;
 return nd_panel_open("NovaDock Download",fields,4,buttons,3);
}
int main(int argc,char **argv)
{
 struct NDClient client;struct NDPanel *panel=NULL;unsigned int event,elapsed=0,meter;int reread=1,action,rc;char bytes[48],value[32],badge[12];const char *url,*target;
 if(argc==3&&!strcmp(argv[1],"WORK"))return fetch_worker(argv[2]);
 if(argc<2)return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 if(!nd_client_init(&client,argv[1]))return 20;
 for(;;){
  if(reread){read_settings(argv[1]);reread=0;}
  event=job_events(&client);if(count&&!job.port){rc=start_job(argv[0]);if(rc<0)pop_queue();}
  meter=progress(bytes);sprintf(value,"%.31s",job.port ? bytes:status);sprintf(badge,"%d",count);
  event|=nd_client_set_badge(&client,count ? badge:"");
  event|=nd_client_update_art_meter_quiet(&client,"DOWNLOAD",value,"download",job.port ? 1U:strstr(status,"failed") || strstr(status,"timed out") ? 2U:0U,meter,elapsed,500U);
  if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
  if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=open_panel();}
  if(panel){nd_panel_set(panel,2,status);nd_panel_set(panel,3,count ? queue[0].target:"None");}
  action=nd_panel_event(panel);
  if(action==100){
   url=nd_panel_get(panel,0);target=nd_panel_get(panel,1);
   if(count>=ND_DOWNLOAD_MAX)nd_panel_title(panel,"Download queue is full");
   else if(!nd_download_url(url))nd_panel_title(panel,"Enter an HTTP or HTTPS URL without credentials");
   else if(!target || !*target || !strchr(target,':') || strlen(target)>=160 || !*FilePart((STRPTR)target))nd_panel_title(panel,"Enter a full destination filename");
   else if(exists(target))nd_panel_title(panel,"Destination already exists; choose another filename");
   else{strcpy(queue[count].url,url);strcpy(queue[count].target,target);++count;nd_panel_set(panel,0,"");nd_panel_title(panel,"NovaDock Download - URL queued");}
  }else if(action==101){count=job.port ? 1:0;nd_panel_title(panel,"NovaDock Download - waiting queue cleared");}
  else if(action==-1 || action==102){nd_panel_close(panel);panel=NULL;}
  elapsed=nd_client_wait_signals(&client,job.port ? 50U:500U,nd_panel_signals(panel));
 }
 nd_panel_close(panel);close_job();nd_client_close(&client);return 0;
}
