/* Optional release checks and download UI, run in separately loaded processes. */
#define IntuitionBase NDUpdateIntuitionBase
#define AslBase NDUpdateAslBase
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_update.h"
#include "nd_update_hash.h"
#include "nd_config.h"
#include "nd_version.h"
#include "nd_protocol.h"
#include "nd_interaction.h"
#include "nd_panel.h"
struct IntuitionBase *IntuitionBase=NULL;
struct Library *AslBase=NULL;
#define UPDATE_GUARD "NovaDock.UpdateCheck"
#define FETCH_GUARD "NovaDock.Update.Fetch"
#define FETCH_PREFIX "T:NovaDockUpdate-"
#define FETCH_DIR_VALID(path) nd_update_staging_dir(path)
#include "../plugin_src/weather/nd_fetch_worker.h"
struct UpdateJob {struct MsgPort *port;char name[80],directory[160],command[1800];ULONG started;int transferred,completed;};
static unsigned long seconds(void)
{struct DateStamp d;DateStamp(&d);return (unsigned long)d.ds_Days*86400UL+(unsigned long)d.ds_Minute*60UL+(unsigned long)d.ds_Tick/50UL;}
static int online(void)
{int found;Forbid();found=FindName(&SysBase->LibList,"bsdsocket.library")!=NULL;Permit();return found;}
static int guard_present(const char *name)
{int found;Forbid();found=FindPort((STRPTR)name)!=NULL;Permit();return found;}
static int enabled(int reason,unsigned int *hours)
{
 struct NDConfig *config;int result=0;
 if(reason==3)return 1;
 if(!guard_present(ND_PORT_NAME))return 0;
 config=AllocVec(sizeof(*config),MEMF_PUBLIC|MEMF_CLEAR);if(!config)return 0;
 nd_load_effective(config);result=reason==1 ? config->check_updates:config->check_updates_running;
 if(hours)*hours=(unsigned int)config->update_interval_hours;FreeVec(config);return result;
}
static void state_read(struct NDUpdateState *state)
{
 FILE *f;char version[32],format[16],latest[32];unsigned long next,notice,checked;int result;memset(state,0,sizeof(*state));
 f=fopen("ENVARC:NovaDock/UpdateState","r");if(!f)return;
 if(fscanf(f,"%15s %lu %lu %31s",format,&next,&notice,version)==4 &&
    (!strcmp(format,"NDUPDATE1") || !strcmp(format,"NDUPDATE2")) &&
    (!strcmp(version,"-") || nd_update_compare(version,version)==0)){
  state->next_attempt=next;state->last_notice=notice;if(strcmp(version,"-"))strcpy(state->noticed,version);
  if(!strcmp(format,"NDUPDATE2") && fscanf(f,"%lu %d %31s",&checked,&result,latest)==3 && result>=0 && result<=5 && (!strcmp(latest,"-") || nd_update_compare(latest,latest)==0)){
   state->checked=checked;state->result=result;if(strcmp(latest,"-"))strcpy(state->latest,latest);
  }
 }
 fclose(f);
 /* A bad or reset system clock must not postpone checks indefinitely. */
 next=seconds();if(state->next_attempt>next+ND_UPDATE_DAY)state->next_attempt=0;
}
static void state_write(const struct NDUpdateState *state)
{
 BPTR lock,file;char body[128];LONG n,written;
 lock=Lock("ENVARC:NovaDock",ACCESS_READ);if(!lock)lock=CreateDir("ENVARC:NovaDock");if(!lock)return;UnLock(lock);
 n=sprintf(body,"NDUPDATE2 %lu %lu %s\n%lu %d %s\n",state->next_attempt,state->last_notice,state->noticed[0] ? state->noticed:"-",state->checked,state->result,state->latest[0] ? state->latest:"-");
 /* Truncated state is rejected on read; a failed state write never changes
  * the dock's saved opt-in or release files. One check owns the guard. */
 file=Open("ENVARC:NovaDock/UpdateState",MODE_NEWFILE);if(!file)return;
 written=Write(file,body,n);if(!Close(file) || written!=n)DeleteFile("ENVARC:NovaDock/UpdateState");
}
void nd_update_status(struct NDUpdateStatus *status)
{
 struct NDUpdateState state;if(!status)return;state_read(&state);
 memset(status,0,sizeof(*status));status->checked=state.checked;status->result=state.result;
 strcpy(status->version,state.latest);status->busy=guard_present(UPDATE_GUARD);status->checking=status->busy && state.result==5;
}
void nd_update_tick(struct NDUpdateSchedule *s,int startup,int running,unsigned int hours,const char *directory,unsigned long elapsed)
{
 struct NDUpdateState state;char path[520],command[1200];BPTR input,output;int busy,net,reason,mode=(startup ? 1:0)|(running ? 2:0);unsigned long now;LONG rc;
 if(!mode){nd_update_schedule(s,0,0,hours,0,0,0,0);return;}
 /* Reuse the housekeeping timer, but inspect ports/network/state at most
  * once a minute. Disabled checks perform no disk or network work. */
 if(s->enabled==mode && s->hours==hours && s->ticks+elapsed<3000UL){s->ticks+=elapsed;return;}
 now=seconds();busy=guard_present(UPDATE_GUARD) || guard_present(FETCH_GUARD);
 if(s->waiting && !busy){state_read(&state);s->next_due=state.next_attempt ? state.next_attempt:now+ND_UPDATE_RETRY;s->waiting=0;}
 net=online();reason=nd_update_schedule(s,startup,running,hours,net,busy,now,elapsed);if(!reason)return;
 if(!directory || strlen(directory)>=sizeof(path))return;strcpy(path,directory);
 if(!AddPart(path,"NovaDock",sizeof(path)) || !nd_quote_path(command,sizeof(command)-64,path))return;
 strcat(command,reason==1 ? " --ND-UPDATE-CHECK STARTUP":" --ND-UPDATE-CHECK RUNNING");
 input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(!input || !output){if(input)Close(input);if(output)Close(output);return;}
 rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
 if(rc<0){Close(input);Close(output);s->waiting=0;}
}
static void close_job(struct UpdateJob *job)
{
 if(job->port)fetch_close_port(job->port);
 if((!job->transferred || job->completed) && job->directory[0])fetch_clean(job->directory);
 memset(job,0,sizeof(*job));
}
static int start_job(struct UpdateJob *job,const char *program,const char *url,unsigned long limit,const char *parent)
{
 BPTR lock,input,output;char self[520],quoted[1100],target[200],tool[260],qtool[540],qtarget[420],qurl[560],command[1300];LONG rc;int serial;
 if(guard_present(FETCH_GUARD))return 0;
 lock=Lock("C:curl",ACCESS_READ);if(!lock)return 0;rc=NameFromLock(lock,tool,sizeof(tool));UnLock(lock);if(!rc)return 0;
 memset(job,0,sizeof(*job));job->started=weather_ticks();
 for(serial=0;serial<20;++serial){sprintf(job->directory,FETCH_PREFIX"%08lx-%08lx-%d",(ULONG)FindTask(NULL),job->started,serial);
  if(parent){if(strlen(parent)>100)goto failed;strcpy(job->directory,parent);sprintf(target,"NovaDockUpdate-%08lx-%08lx-%d",(ULONG)FindTask(NULL),job->started,serial);if(!AddPart(job->directory,target,sizeof(job->directory)))goto failed;}
  lock=CreateDir(job->directory);if(lock){UnLock(lock);break;}}
 if(serial==20){job->directory[0]=0;return 0;}
 sprintf(target,"%s/response",job->directory);
 if(!nd_quote_path(qtarget,sizeof(qtarget),target) || !nd_quote_path(qtool,sizeof(qtool),tool) || !nd_quote_path(qurl,sizeof(qurl),url))goto failed;
 sprintf(job->command,"%s --disable --fail --silent --show-error --location --max-redirs 5 --proto =https --proto-redir =https --connect-timeout 30 --max-time %lu --max-filesize %lu --user-agent NovaDock/%s --header \"Accept: application/vnd.github+json\" --output %s -- %s",qtool,limit<=ND_UPDATE_JSON_LIMIT ? 120UL:600UL,limit,ND_VERSION,qtarget,qurl);
 if(strlen(job->command)>=sizeof(job->command)-1)goto failed;
 job->port=CreateMsgPort();if(!job->port)goto failed;
 sprintf(job->name,"NovaDock.Update.%08lx.%08lx",(ULONG)FindTask(NULL),job->started);job->port->mp_Node.ln_Name=job->name;AddPort(job->port);
 if(!GetProgramDir() || !NameFromLock(GetProgramDir(),self,sizeof(self)) || !AddPart(self,FilePart((STRPTR)program),sizeof(self)) || !nd_quote_path(quoted,sizeof(quoted),self))goto failed;
 sprintf(command,"%s --ND-UPDATE-WORK %s 1",quoted,job->name);
 input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(!input || !output){if(input)Close(input);if(output)Close(output);goto failed;}
 rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
 if(rc<0){Close(input);Close(output);goto failed;}return 1;
failed:close_job(job);return 0;
}
/* Completion doesn't transfer the directory's ownership: caller verifies or
 * moves response before close_job removes the private staging files. */
static int job_event(struct UpdateJob *job)
{
 struct FetchMessage *m;int result=0;
 while((m=(struct FetchMessage *)GetMsg(job->port))!=NULL){
  if(m->magic!=FETCH_MAGIC || m->msg.mn_Length!=sizeof(*m)){if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;}
  if(m->kind==1){if(job->transferred)m->kind=-1;else{strcpy(m->command,job->command);strcpy(m->directory,job->directory);job->transferred=1;}ReplyMsg((struct Message *)m);}
  else if(m->kind==2){result=m->result==0 ? 1:-1;job->completed=1;FreeVec(m);break;}
 }
 if(!result && (ULONG)(weather_ticks()-job->started)>650UL*50UL)result=-1;
 return result;
}
static int wait_job(struct UpdateJob *job)
{
 int result=0;unsigned int ticks=0;
 while(!result){result=job_event(job);if(result)break;if(!(ticks++%50) && !guard_present(ND_PORT_NAME))return -1;Delay(2);}return result;
}
static char *response(struct UpdateJob *job,unsigned int limit)
{
 FILE *f;char path[200],*data;size_t n;
 sprintf(path,"%s/response",job->directory);f=fopen(path,"rb");if(!f)return NULL;
 data=AllocVec(limit+1,MEMF_PUBLIC|MEMF_CLEAR);if(!data){fclose(f);return NULL;}
 n=fread(data,1,limit,f);if(ferror(f) || fgetc(f)!=EOF || memchr(data,0,n)){FreeVec(data);data=NULL;}else data[n]=0;
 fclose(f);return data;
}
static int select_target(const char *filename,char out[520])
{
 struct FileRequester *request;int result=0;
 AslBase=OpenLibrary("asl.library",39);if(!AslBase)return 0;
 request=AllocAslRequestTags(ASL_FileRequest,TAG_DONE);
 if(request){
  if(AslRequestTags(request,ASLFR_TitleText,(ULONG)"Save NovaDock update",ASLFR_DoSaveMode,TRUE,ASLFR_InitialDrawer,(ULONG)"RAM:",ASLFR_InitialFile,(ULONG)filename,TAG_DONE)){
   if(strlen(request->fr_Drawer)<500){strcpy(out,request->fr_Drawer);result=AddPart(out,request->fr_File,520) && request->fr_File[0] && strchr(out,':')!=NULL;}
  }
  FreeAslRequest(request);
 }
 CloseLibrary(AslBase);AslBase=NULL;return result;
}
static int file_exists(const char *path)
{BPTR lock=Lock((STRPTR)path,ACCESS_READ);if(lock)UnLock(lock);return lock!=0;}
static int verify(const char *path,const struct NDUpdateRelease *release,const char *expected,struct NDPanel *panel)
{
 FILE *f;struct NDUpdateHash hash;unsigned char buffer[4096];char digest[65];unsigned long total=0;size_t n;int valid=0,action;unsigned int blocks=0;
 f=fopen(path,"rb");if(!f)return 0;nd_update_hash_init(&hash);
 while((n=fread(buffer,1,sizeof(buffer),f))!=0){total+=(unsigned long)n;if(total>release->size)goto finished;nd_update_hash_add(&hash,buffer,(unsigned int)n);
  if(!(++blocks%8)){action=nd_panel_event(panel);if(action==-1 || action==100){valid=-1;goto finished;}Delay(1);}
 }
 if(ferror(f) || total!=release->size)goto finished;nd_update_hash_end(&hash,digest);valid=!strcmp(digest,expected) && (!release->digest[0] || !strcmp(digest,release->digest));
finished:if(fclose(f))valid=0;return valid;
}
static void notice(const char *message)
{
 struct NDPanelField field;struct NDPanel *panel;int action;
 static const char *buttons[]={"Close"};
 field.label="Status";field.value=message;field.limit=0;
 panel=nd_panel_open("NovaDock update",&field,1,buttons,1);if(!panel)return;
 while((action=nd_panel_event(panel))!=-1 && action!=100)Delay(5);
 nd_panel_close(panel);
}
static void download(const char *program,const struct NDUpdateRelease *release,const char *expected)
{
 struct UpdateJob job;struct NDPanel *panel=NULL;struct NDPanelField fields[3];static const char *buttons[]={"Cancel"};
 char target[520],parent[520],part[200],progress[80],message[128];struct FileInfoBlock *info;BPTR lock;unsigned long bytes=0;int result=0,action,verified;unsigned int polls=0;
 if(!select_target(release->file,target))return;
 if(file_exists(target)){notice("That file already exists. Choose another name.");return;}
 strcpy(parent,target);*FilePart(parent)=0;
 memset(&job,0,sizeof(job));if(!start_job(&job,program,release->url,release->size,parent)){notice("Could not start download. Check C:curl and destination.");return;}
 fields[0].label="Version";fields[0].value=release->version;fields[0].limit=0;
 fields[1].label="Received";fields[1].value="Starting...";fields[1].limit=0;
 fields[2].label="Status";fields[2].value="Downloading securely";fields[2].limit=0;
 panel=nd_panel_open("NovaDock update",fields,3,buttons,1);if(!panel){close_job(&job);return;}
 sprintf(part,"%s/response",job.directory);
 while(1){
  result=job_event(&job);action=nd_panel_event(panel);
  if(action==-1 || action==100){result=0;break;}
  if(result)break;
  if(!(polls++%5)){lock=Lock(part,ACCESS_READ);if(lock){info=AllocDosObject(DOS_FIB,NULL);if(info){if(Examine(lock,info) && info->fib_Size>=0)bytes=(unsigned long)info->fib_Size;FreeDosObject(DOS_FIB,info);}UnLock(lock);}
  sprintf(progress,"%lu / %lu bytes",bytes,release->size);nd_panel_set(panel,1,progress);}Delay(10);
 }
 if(result==1){
  nd_panel_set(panel,2,"Verifying SHA-256...");
  verified=verify(part,release,expected,panel);
  if(verified<0){result=0;strcpy(message,"Cancelled.");}
  else if(!verified)strcpy(message,"Verification failed; archive discarded.");
  else if(file_exists(target) || !Rename(part,target))strcpy(message,"Could not save; destination may already exist.");
  else strcpy(message,"Saved. Unpack and run Install-NovaDock.");
 }else strcpy(message,"Download failed; no archive was kept.");
 close_job(&job);
 nd_panel_close(panel);panel=NULL;
 if(result){static const char *close_buttons[]={"Close"};fields[1].value=release->file;fields[2].value=message;
  panel=nd_panel_open("NovaDock update",fields,3,close_buttons,1);
  if(panel){while((action=nd_panel_event(panel))!=-1 && action!=100)Delay(5);nd_panel_close(panel);}
 }
}
static int check(const char *program,int reason)
{
 unsigned int hours=4;
 struct MsgPort *guard;struct NDUpdateState state;struct NDUpdateRelease release;struct UpdateJob job;
 struct NDPanel *panel=NULL;struct NDPanelField fields[3];static const char *buttons[]={"Download...","Later"};
 char *data=NULL,checksum[65];unsigned long now;int result=0,action=0,owned=0;unsigned int poll=0;
 ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 guard=CreateMsgPort();if(!guard)return 20;
 Forbid();if(!FindPort(UPDATE_GUARD)){guard->mp_Node.ln_Name=UPDATE_GUARD;AddPort(guard);owned=1;}Permit();
 if(!owned){DeleteMsgPort(guard);return 0;}
 memset(&state,0,sizeof(state));memset(&job,0,sizeof(job));if(!enabled(reason,&hours))goto finished;
 now=seconds();state_read(&state);state.checked=now;state.result=5;state.next_attempt=now+ND_UPDATE_RETRY;
 if(!online()){state.result=4;state_write(&state);goto finished;}state_write(&state);
 if(!start_job(&job,program,ND_UPDATE_API,ND_UPDATE_JSON_LIMIT,NULL) || wait_job(&job)!=1)goto finished;
 data=response(&job,ND_UPDATE_JSON_LIMIT);if(!data || !nd_update_release(data,&release))goto finished;
 FreeVec(data);data=NULL;close_job(&job);state.next_attempt=seconds()+nd_update_interval(hours);state.result=nd_update_compare(release.version,ND_VERSION)==1 ? 2:1;strcpy(state.latest,release.version);state.checked=seconds();state_write(&state);
 if(nd_update_compare(release.version,ND_VERSION)!=1 || (reason!=3 && !nd_update_notice_due(&state,release.version,now)) || !enabled(reason,NULL))goto finished;
 if(!start_job(&job,program,release.checksum_url,160,NULL) || wait_job(&job)!=1)goto finished;
 data=response(&job,160);if(!data || !nd_update_checksum(data,release.file,checksum) || (release.digest[0] && strcmp(checksum,release.digest)))goto finished;
 FreeVec(data);data=NULL;close_job(&job);if(!enabled(reason,NULL))goto finished;
 fields[0].label="Installed";fields[0].value=ND_VERSION;fields[0].limit=0;
 fields[1].label="Available";fields[1].value=release.version;fields[1].limit=0;
 fields[2].label="Package";fields[2].value=release.file;fields[2].limit=0;
 panel=nd_panel_open("A new NovaDock release is available",fields,3,buttons,2);if(!panel)goto finished;
 strcpy(state.noticed,release.version);state.last_notice=seconds();state_write(&state);
 while(!action){action=nd_panel_event(panel);if(!(poll++%100) && !enabled(reason,NULL)){action=-1;break;}Delay(5);}
 nd_panel_close(panel);panel=NULL;
 if(action==100 && enabled(reason,NULL))download(program,&release,checksum);result=0;
finished:
 if(state.result==5){state.result=3;state_write(&state);}
 if(data)FreeVec(data);close_job(&job);if(panel)nd_panel_close(panel);
 Forbid();RemPort(guard);Permit();DeleteMsgPort(guard);return result;
}
int nd_update_helper_main(int argc,char **argv)
{
 if(argc>1 && !strcmp(argv[1],"--ND-UPDATE-CHECK"))return argc==3 && (!strcmp(argv[2],"STARTUP") || !strcmp(argv[2],"RUNNING") || !strcmp(argv[2],"MANUAL")) ? check(argv[0],!strcmp(argv[2],"MANUAL") ? 3:(!strcmp(argv[2],"STARTUP") ? 1:2)):20;
 if(argc>1 && !strcmp(argv[1],"--ND-UPDATE-WORK"))return argc==4 && !strcmp(argv[3],"1") ? fetch_worker(argv[2]):20;
 return -1;
}
