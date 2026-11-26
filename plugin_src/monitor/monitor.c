/* WATCH and LOGS: a responsive resident UI, bounded models and one DOS worker.
 * The worker owns its message until hand-off; late results are freed there if
 * the resident has gone away. Monitored paths are accessed only by workers. */
#include <exec/types.h>
#include <exec/memory.h>
#include <devices/clipboard.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/wb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_alerts.h"
#include "nd_client.h"
#include "nd_reply.h"
#include "nd_wait.h"
#include "nd_list.h"
#include "nd_panel.h"
#include "nd_text.h"
#include "nd_option_runtime.h"
#include "nd_interaction.h"
#include "nd_monitor.h"
#include "../clipboard/nd_clipboard.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
#ifdef ND_MONITOR_LOGS
#define MONITOR_ID "LOGS"
#define MONITOR_ART "logs"
#define PATH_KEY "FILE"

#else
#define MONITOR_ID "WATCH"
#define MONITOR_ART "watch"
#define PATH_KEY "DRAWER"

#endif
long __stack=32768;
struct Library *WorkbenchBase=NULL;
#define MONITOR_MAGIC 0x4e444d4fUL
#define JOB_SCAN 1
#define JOB_OPEN 2
#define JOB_COPY 3
#define JOB_EXPORT 4
#define LOG_EXPORT 10241
struct MonitorMessage {
 struct Message message;ULONG magic;int kind,action,result,reset,skip;
 char id[20],path[ND_MONITOR_PATH],copy[ND_MONITOR_TEXT];
#ifndef ND_MONITOR_LOGS
 struct NDWatchSnapshot snapshot;
#endif
 struct NDLogCursor cursor;
 unsigned int length;
#ifdef ND_MONITOR_LOGS
 unsigned char data[ND_LOG_CHUNK];char output[LOG_EXPORT];
#endif
};
struct MonitorSettings {char path[ND_MONITOR_PATH],label[32],include[128],exclude[128];unsigned int interval,quiet;int alerts;};
static struct NDMonitorHistory history;
#ifndef ND_MONITOR_LOGS
static struct NDWatchSnapshot snapshot;
#endif
static struct NDLogCursor cursor;
static struct NDLogParser parser;
static struct MonitorSettings settings;
static struct MsgPort *job_port;
static char job_name[80],status[100];
static unsigned long job_started,generation=1,job_generation,serial;
static int degraded=1,paused;
static unsigned int notice_count;static unsigned long notice_first,notice_last;
struct MonitorRequest {int action;char id[20],path[ND_MONITOR_PATH],copy[ND_MONITOR_TEXT];struct NDLogCursor cursor;
#ifdef ND_MONITOR_LOGS
 unsigned int length;char output[LOG_EXPORT];
#endif
};
static struct MonitorRequest request;
static unsigned long ticks(void)
{struct DateStamp d;DateStamp(&d);return ((unsigned long)d.ds_Days*1440UL+(unsigned long)d.ds_Minute)*3000UL+(unsigned long)d.ds_Tick;}
static int send_to(const char *name,struct MonitorMessage *m)
{struct MsgPort *p;Forbid();p=FindPort((STRPTR)name);if(p)PutMsg(p,(struct Message *)m);Permit();return p!=NULL;}
static void close_job(void)
{
 struct MonitorMessage *m;if(!job_port)return;
 Forbid();RemPort(job_port);Permit();
 while((m=(struct MonitorMessage *)GetMsg(job_port))!=NULL){if(m->kind==1){m->kind=-1;nd_reply_finish((struct Message *)m);}else FreeVec(m);}
 DeleteMsgPort(job_port);job_port=NULL;
}
static int option(void *ctx,const char *key,const char *value)
{
 struct MonitorSettings *s=ctx;unsigned long n;
 if(!strcmp(key,"LABEL")){if(!*value || strlen(value)>=sizeof(s->label))return 0;strcpy(s->label,value);}
 else if(!strcmp(key,PATH_KEY)){if(strlen(value)>=sizeof(s->path))return 0;strcpy(s->path,value);}
 else if(!strcmp(key,"INTERVAL")){n=strtoul(value,NULL,10);if(n<2 || n>300)return 0;s->interval=(unsigned int)n;}
 else if(!strcmp(key,"INCLUDE") || !strcmp(key,"EXCLUDE")){if(strlen(value)>127)return 0;strcpy(!strcmp(key,"INCLUDE") ? s->include:s->exclude,value);}
 else if(!strcmp(key,"QUIET")){n=strtoul(value,NULL,10);if(n<1 || n>30)return 0;s->quiet=(unsigned int)n;}
 else if(!strcmp(key,"ALERTS"))s->alerts=!strcmp(value,"YES");
 return 1;
}
static void load(const char *id)
{
 memset(&settings,0,sizeof(settings));strcpy(settings.label,MONITOR_ID);settings.interval=10;settings.alerts=1;settings.quiet=3;strcpy(settings.include,"#?");nd_plugin_options_effective(id,option,&settings);
 memset(&history,0,sizeof(history));
#ifndef ND_MONITOR_LOGS
 snapshot.count=snapshot.valid=0;
#endif
 memset(&cursor,0,sizeof(cursor));nd_log_reset(&parser,0);++generation;degraded=1;notice_count=0;
 strcpy(status,*settings.path ? "Waiting for first check":"Choose a " PATH_KEY " in Edit settings");
}
static int start_job(const char *program,const char *id,int action,const char *path,const char *copy)
{
 char self[530],command[640],guard_name[64];struct MsgPort *guard;BPTR input,output;LONG rc;
 if(job_port)return 0;
 sprintf(guard_name,"NovaDock.%s.Work.%s",MONITOR_ID,id);Forbid();guard=FindPort(guard_name);Permit();
 if(guard){strcpy(status,"Previous worker still busy");return 0;}
 if(!nd_quote_path(self,sizeof(self),program))return 0;
 job_port=CreateMsgPort();if(!job_port){strcpy(status,"Not enough memory for check");return 0;}
 job_started=ticks();job_generation=generation;
 sprintf(job_name,"NovaDock.%s.%08lx.%08lx.%08lx",MONITOR_ID,(ULONG)FindTask(NULL),job_started,++serial);job_port->mp_Node.ln_Name=job_name;
 Forbid();AddPort(job_port);Permit();
 /* Request is prepared locally, copied into the worker's owned wire packet. */
 {memset(&request,0,sizeof(request));request.action=action;strcpy(request.id,id);strncpy(request.path,path,sizeof(request.path)-1);if(copy){
#ifdef ND_MONITOR_LOGS
 if(action==JOB_EXPORT){request.length=(unsigned int)strlen(copy);if(request.length>=LOG_EXPORT){close_job();return 0;}memcpy(request.output,copy,request.length+1);}else
#endif
 strncpy(request.copy,copy,sizeof(request.copy)-1);}request.cursor=cursor;
 }
 sprintf(command,"%s WORK %s",self,job_name);input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(!input || !output){if(input)Close(input);if(output)Close(output);close_job();strcpy(status,"Could not start worker");return 0;}
 rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_Priority,-1L,NP_StackSize,32768UL,TAG_DONE);
 if(rc==-1){Close(input);Close(output);close_job();strcpy(status,"Could not start worker");return 0;}
 strcpy(status,action==JOB_SCAN ? "Checking...":action==JOB_COPY ? "Copying...":"Opening...");return 1;
}
static int open_target(const char *path)
{
 char quoted[530],command[550];BPTR input,output;LONG rc;
 WorkbenchBase=OpenLibrary("workbench.library",36);
 if(WorkbenchBase){int opened=WorkbenchBase->lib_Version>=44 && OpenWorkbenchObjectA((STRPTR)path,NULL);CloseLibrary(WorkbenchBase);WorkbenchBase=NULL;if(opened)return 1;}
 if(!nd_quote_path(quoted,sizeof(quoted),path))return 0;sprintf(command,"C:WBRUN %s",quoted);
 input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);if(!input || !output){if(input)Close(input);if(output)Close(output);return 0;}
 rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,TAG_DONE);if(rc==-1){Close(input);Close(output);}return rc!=-1;
}
#ifndef ND_MONITOR_LOGS
static void scan_drawer(struct MonitorMessage *m)
{
 BPTR lock=Lock(m->path,ACCESS_READ);struct FileInfoBlock *fib;struct NDWatchEntry *e;LONG error=0;m->result=0;
 if(!lock)return;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return;}
 if(Examine(lock,fib) && fib->fib_DirEntryType>0){
  while(ExNext(lock,fib)){
   if(m->snapshot.count==ND_WATCH_MAX){m->result=-2;break;}
   e=&m->snapshot.entries[m->snapshot.count++];strncpy(e->name,fib->fib_FileName,sizeof(e->name)-1);e->size=fib->fib_Size;e->days=fib->fib_Date.ds_Days;e->minutes=fib->fib_Date.ds_Minute;e->ticks=fib->fib_Date.ds_Tick;e->key=fib->fib_DiskKey;e->type=fib->fib_DirEntryType;
  }
  if(m->result!=-2){error=IoErr();if(error==ERROR_NO_MORE_ENTRIES){m->result=1;m->snapshot.valid=1;}}
 }
 FreeDosObject(DOS_FIB,fib);UnLock(lock);
}
#else
static void read_log(struct MonitorMessage *m)
{
 BPTR f=Open(m->path,MODE_OLDFILE);struct FileInfoBlock *fib;long size,start,got;unsigned char prefix[32];unsigned int length;int reset;
 m->result=0;if(!f)return;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){Close(f);return;}
 if(!ExamineFH(f,fib) || fib->fib_DirEntryType>=0 || fib->fib_Size<0)goto done;
 size=fib->fib_Size;length=(unsigned int)(size<32 ? size:32);got=Read(f,prefix,length);if(got!=(long)length)goto done;
 reset=nd_log_replaced(&m->cursor,fib->fib_DiskKey,size,prefix,length);m->reset=reset;
 start=(!m->cursor.valid || reset) ? (size>ND_LOG_CHUNK ? size-ND_LOG_CHUNK:0):m->cursor.offset;
 m->skip=(!m->cursor.valid || reset) && start>0;
 if(Seek(f,start,OFFSET_BEGINNING)==-1)goto done;
 got=Read(f,m->data,ND_LOG_CHUNK);if(got<0)goto done;m->length=(unsigned int)got;
 m->cursor.offset=start+got;m->cursor.key=fib->fib_DiskKey;m->cursor.valid=1;
 m->cursor.prefix_length=length;memcpy(m->cursor.prefix,prefix,length);m->result=1;
done:FreeDosObject(DOS_FIB,fib);Close(f);
}
#endif
static int worker(const char *parent)
{
 struct MonitorMessage *m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);struct MsgPort *reply=nd_reply_port(),*guard=NULL;char name[64];int replied=0,owned=0;
 ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 if(!m || !reply){if(m)FreeVec(m);if(reply)nd_reply_delete(reply);return 20;}
 m->magic=MONITOR_MAGIC;m->kind=1;m->message.mn_Length=sizeof(*m);m->message.mn_ReplyPort=reply;
 if(!send_to(parent,m)){FreeVec(m);nd_reply_delete(reply);return 10;}
 if(nd_wait_port(NULL,reply,150UL) && GetMsg(reply))replied=1;
 /* Atomic with the parent: do not abandon an already delivered reply. */
 Forbid();if(!replied && GetMsg(reply))replied=1;if(!replied)nd_reply_abandon(reply);Permit();if(!replied)return 10;nd_reply_delete(reply);m->message.mn_ReplyPort=NULL;
 if(m->kind!=1){FreeVec(m);return 10;}
 guard=CreateMsgPort();sprintf(name,"NovaDock.%s.Work.%s",MONITOR_ID,m->id);
 if(guard){guard->mp_Node.ln_Name=name;Forbid();if(!FindPort(name)){AddPort(guard);owned=1;}Permit();}
 if(owned){
  if(m->action==JOB_OPEN)m->result=open_target(m->path);
  else if(m->action==JOB_COPY)m->result=nd_text_copy(m->copy);
#ifdef ND_MONITOR_LOGS
  else if(m->action==JOB_EXPORT)m->result=m->length<LOG_EXPORT ? nd_text_export(m->path,m->output,m->length):0;
#endif
#ifdef ND_MONITOR_LOGS
  else read_log(m);
#else
  else scan_drawer(m);
#endif
 }else m->result=-3;
 m->kind=2;if(send_to(parent,m))m=NULL;if(m)FreeVec(m);
 if(guard){if(owned){Forbid();RemPort(guard);Permit();}DeleteMsgPort(guard);}return 0;
}
static unsigned int receive_job(void)
{
 struct MonitorMessage *m;unsigned int changes=0;
#ifndef ND_MONITOR_LOGS
 struct DateStamp d;char time[16];
#endif
 if(!job_port)return 0;
 while((m=(struct MonitorMessage *)GetMsg(job_port))!=NULL){
  if(m->magic!=MONITOR_MAGIC || m->message.mn_Length!=sizeof(*m)){if(m->message.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;}
  if(m->kind==1){if(job_generation!=generation){m->kind=-1;nd_reply_finish((struct Message *)m);close_job();break;}else{m->action=request.action;strcpy(m->id,request.id);strcpy(m->path,request.path);strcpy(m->copy,request.copy);m->cursor=request.cursor;
#ifdef ND_MONITOR_LOGS
 if(m->action==JOB_EXPORT){m->length=request.length;memcpy(m->output,request.output,m->length+1);}
#endif
 }nd_reply_finish((struct Message *)m);}
  else{
   if(job_generation==generation){
    if(m->action!=JOB_SCAN)strcpy(status,m->result==1 ? (m->action==JOB_COPY ? "Copied to clipboard":m->action==JOB_EXPORT ? "Exported":"Opened"):m->result==-4 ? "Destination exists; choose another name":"Action failed");
    else if(m->result==1){
#ifdef ND_MONITOR_LOGS
     if(!cursor.valid || m->reset){nd_log_reset(&parser,m->skip);if(m->reset)nd_monitor_add(&history,"--- log replaced or truncated ---",NULL,0,0);}
     cursor=m->cursor;changes=nd_log_feed(&history,&parser,m->data,m->length);strcpy(status,"Following log; warnings/errors marked");
#else
     DateStamp(&d);sprintf(time,"%02ld:%02ld:%02ld",d.ds_Minute/60,d.ds_Minute%60,d.ds_Tick/50);changes=nd_watch_filtered(&history,&snapshot,&m->snapshot,settings.path,time,settings.include,settings.exclude);nd_watch_snapshot_copy(&snapshot,&m->snapshot);strcpy(status,"Watching immediate drawer contents");
#endif
     degraded=0;
    }else{degraded=1;
#ifndef ND_MONITOR_LOGS
     snapshot.valid=0;
#endif
     strcpy(status,m->result==-2 ? "Over 256 entries; choose a smaller drawer":m->result==-3 ? "Previous worker still busy":"Unavailable; will retry automatically");}
   }
   FreeVec(m);close_job();break;
  }
 }
 if(job_port && ticks()-job_started>750UL){
  struct MsgPort *guard;char name[64];sprintf(name,"NovaDock.%s.Work.%s",MONITOR_ID,request.id);
  Forbid();guard=FindPort(name);Permit();degraded=1;
  strcpy(status,guard ? "Check taking too long; worker still busy":"Worker did not finish; will retry");if(!guard)close_job();
 }
 return changes;
}
int main(int argc,char **argv)
{
 struct NDClient client;struct NDList *list=NULL;struct NDPanel *export_panel=NULL;
 unsigned int event,elapsed=0,changes,i,count,severity=0;unsigned long due,view_sequence=~0UL;int action,selected,view_paused=0;char badge[8],value[32],rows[ND_MONITOR_HISTORY][ND_MONITOR_TEXT];const char *pointers[ND_MONITOR_HISTORY];
 static struct NDMonitorHistory view;
#ifdef ND_MONITOR_LOGS
 static char output[LOG_EXPORT];
 static const char *buttons[]={"Copy line","Open file","Mark read","Clear history","Check now","Pause view","All lines","Export...","Close"};
#else
 static const char *buttons[]={"Open","Open drawer","Mark read","Clear history","Check now","Pause","Close"};
#endif
 if(argc==3 && !strcmp(argv[1],"WORK"))return worker(argv[2]);
 if(argc!=2 || !*argv[1] || strlen(argv[1])>19)return 20;
 ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 if(!nd_client_init(&client,argv[1]))return 20;load(argv[1]);due=ticks();
 for(;;){
  changes=receive_job();event=0;
  if(changes && settings.alerts){if(!notice_count)notice_first=ticks();notice_last=ticks();if(notice_count<65535U-changes)notice_count+=changes;else notice_count=65535;}
  if(notice_count && (ticks()-notice_last>=settings.quiet*50UL || ticks()-notice_first>=1500UL)){char notice[64];
#ifdef ND_MONITOR_LOGS
   sprintf(notice,"%u new warning/error lines",notice_count);event=nd_client_notify(&client,settings.label,notice,ND_ALERT_WARNING);
#else
   sprintf(notice,"%u new or changed files",notice_count);event=nd_client_notify(&client,settings.label,notice,ND_ALERT_INFO);
#endif
   notice_count=0;
  }
  if(!paused && !job_port && *settings.path && (long)(ticks()-due)>=0){start_job(argv[0],argv[1],JOB_SCAN,settings.path,NULL);due=ticks()+settings.interval*50UL;}
  if(history.unseen)sprintf(badge,"%u",history.unseen);else badge[0]=0;event|=nd_client_set_badge(&client,badge);
  if(paused){strcpy(value,"Paused");event|=nd_client_update_art_quiet(&client,settings.label,value,MONITOR_ART,0,elapsed,250);}
  else if(degraded){sprintf(value,"%.31s",status);event|=nd_client_update_art_quiet(&client,settings.label,value,MONITOR_ART,1,elapsed,250);}
  else{sprintf(value,"%u unread; %u recent",history.unseen,history.count);event|=nd_client_update_art_quiet(&client,settings.label,value,MONITOR_ART,0,elapsed,250);}
  if(event&ND_EVENT_QUIT)break;
  if(event&ND_EVENT_SETTINGS){load(argv[1]);due=ticks();view_sequence=~0UL;view_paused=0;}
  if(event&ND_EVENT_CLICK){if(list)nd_list_raise(list);else{
#ifdef ND_MONITOR_LOGS
   list=nd_list_open(settings.label,buttons,9);nd_list_follow(list,1);view_paused=0;
#else
   list=nd_list_open(settings.label,buttons,7);nd_list_button(list,5,paused ? "Resume":"Pause");
#endif
   view_sequence=~0UL;}}
  if(list && !view_paused && (view_sequence!=history.sequence || view.count!=history.count)){
   view.count=0;view.first=0;for(i=0;i<history.count;++i){const struct NDMonitorRow *r=nd_monitor_row(&history,i);if(!severity || r->severity>=severity)view.rows[view.count++]=*r;}
   view_sequence=history.sequence;count=view.count;for(i=0;i<count;++i){sprintf(rows[i],"%s%.157s",view.rows[i].unread ? "* ":"  ",view.rows[i].text);pointers[i]=rows[i];}nd_list_rows(list,pointers,count);
  }
  nd_list_title(list,paused ? "Paused; resume establishes a fresh baseline":view_paused ? "View paused; capture continues":status);
  action=nd_list_event(list);selected=nd_list_selected(list);
#ifdef ND_MONITOR_LOGS
  if(list && !view_paused && !nd_list_following(list)){view_paused=1;nd_list_button(list,5,"Resume view");}
  if(action==-1 || action==108){nd_list_close(list);list=NULL;}
  else if(action==105){view_paused=!view_paused;nd_list_follow(list,!view_paused);nd_list_button(list,5,view_paused ? "Resume view":"Pause view");if(!view_paused){view_sequence=~0UL;nd_list_select(list,-1);}}
  else if(action==106){severity=(severity+1)%3;view_paused=0;nd_list_select(list,-1);view_sequence=~0UL;nd_list_follow(list,1);nd_list_button(list,5,"Pause view");nd_list_button(list,6,severity==2 ? "Errors only":severity==1 ? "Warnings/errors":"All lines");}
  else if(action==107 && !export_panel){struct NDPanelField f={"New text file","T:NovaDock-LOGS.txt",240};static const char *b[]={"Export","Cancel"};export_panel=nd_panel_open("Export selected line, or all visible lines",&f,1,b,2);if(export_panel){view_paused=1;nd_list_follow(list,0);nd_list_button(list,5,"Resume view");}}
  else if(action==100 && selected>=0 && selected<(int)view.count){if(!start_job(argv[0],argv[1],JOB_COPY,settings.path,view.rows[selected].text))strcpy(status,"Busy; try Copy again after the check");}
  else if(action==101 && *settings.path)start_job(argv[0],argv[1],JOB_OPEN,settings.path,NULL);
#else
  if(action==-1 || action==106){nd_list_close(list);list=NULL;}
  else if(action==105){paused=!paused;++generation;notice_count=0;snapshot.valid=0;due=ticks();nd_list_button(list,5,paused ? "Resume":"Pause");strcpy(status,paused ? "Monitoring paused":"Resuming with a fresh baseline");}
  else if(action==100 && selected>=0 && selected<(int)view.count)start_job(argv[0],argv[1],JOB_OPEN,view.rows[selected].path,NULL);
  else if(action==101 && *settings.path)start_job(argv[0],argv[1],JOB_OPEN,settings.path,NULL);
#endif
  else if(action==102)nd_monitor_read(&history);
  else if(action==103){unsigned long sequence=history.sequence+1;memset(&history,0,sizeof(history));history.sequence=sequence;view_paused=0;nd_list_follow(list,1);
#ifdef ND_MONITOR_LOGS
   nd_list_button(list,5,"Pause view");
#endif
}
  else if(action==104){if(!paused)due=ticks();else strcpy(status,"Resume monitoring before checking");}
#ifdef ND_MONITOR_LOGS
  action=nd_panel_event(export_panel);if(action==-1 || action==101){nd_panel_close(export_panel);export_panel=NULL;}
  else if(action==100){unsigned int used=0;output[0]=0;selected=nd_list_selected(list);for(i=0;i<view.count;++i)if(selected<0 || i==(unsigned int)selected){unsigned int n=(unsigned int)strlen(view.rows[i].text);if(used+n+2>sizeof(output))break;memcpy(output+used,view.rows[i].text,n);used+=n;output[used++]='\n';output[used]=0;}
   if(!used)nd_panel_title(export_panel,"No visible lines to export");else if(start_job(argv[0],argv[1],JOB_EXPORT,nd_panel_get(export_panel,0),output)){nd_panel_close(export_panel);export_panel=NULL;}else nd_panel_title(export_panel,"Busy; try again after the current check");}
#endif
  elapsed=nd_client_wait_signals(&client,list || job_port || export_panel ? 10:50,nd_list_signals(list)|nd_panel_signals(export_panel)|(job_port ? 1UL<<job_port->mp_SigBit:0));
 }
 close_job();nd_list_close(list);nd_panel_close(export_panel);nd_client_close(&client);return 0;
}
