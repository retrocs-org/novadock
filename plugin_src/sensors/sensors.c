/* SENSOR: read-only hardware readings; vendor diagnostics run off the dock task. */
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
#include "nd_interaction.h"
#include "nd_panel.h"
#include "nd_sensor_native.h"
#define FETCH_GUARD "NovaDock.Sensors.Fetch"
#define FETCH_PREFIX "T:NovaDockSensors-"
#include "../weather/nd_fetch_worker.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(SENSORS,"Sensors");
long __stack=32768;
struct Settings {unsigned long selected;unsigned int interval,rtc_age;char warp[180],label[32];};
struct Job {struct MsgPort *port;char name[80],directory[160],command[1800];unsigned long started;int transferred;};
static struct NDSensors readings;
static struct Job job;
static unsigned long clock_ticks(void){return weather_ticks();}
static int apply(void *context,const char *key,const char *value)
{
 struct Settings *s=context;int i=nd_sensor_index(key);unsigned long n;
 if(!strcmp(key,"LABEL")){if(!*value || strlen(value)>31)return 0;strcpy(s->label,value);}
 else if(i>=0){if(!strcmp(value,"YES"))s->selected|=1UL<<i;else if(!strcmp(value,"NO"))s->selected&=~(1UL<<i);else return 0;}
 else if(!strcmp(key,"INTERVAL")){n=strtoul(value,NULL,10);if(n>=2 && n<=60)s->interval=(unsigned int)n;}
 else if(!strcmp(key,"RTC_MAX_AGE")){n=strtoul(value,NULL,10);if(n>=30 && n<=86400)s->rtc_age=(unsigned int)n;}
 else if(!strcmp(key,"WARPTOOL")){if(strlen(value)>=sizeof(s->warp))return 0;strcpy(s->warp,value);}
 return 1;
}
static void read_settings(const char *id,struct Settings *s)
{memset(s,0,sizeof(*s));s->selected=ND_SENSOR_ALL;strcpy(s->label,"SENSOR");s->interval=5;s->rtc_age=120;strcpy(s->warp,"C:WarpTool");nd_plugin_options_effective(id,apply,s);}
static void job_close(void)
{fetch_close_port(job.port);memset(&job,0,sizeof(job));}
static int start_warp(const char *program,const char *tool)
{
 BPTR lock,input,output;LONG result;char absolute[180],quoted[370],self[370],command[600];int serial;
 if(job.port || !*tool)return 0;
 Forbid();result=FindPort(FETCH_GUARD)!=NULL;Permit();if(result)return 0;
 lock=Lock((STRPTR)tool,ACCESS_READ);if(!lock)return 0;result=NameFromLock(lock,absolute,sizeof(absolute));UnLock(lock);if(!result)return 0;
 memset(&job,0,sizeof(job));job.started=clock_ticks();
 for(serial=0;serial<20;++serial){sprintf(job.directory,FETCH_PREFIX "%08lx-%08lx-%d",(ULONG)FindTask(NULL),job.started,serial);lock=CreateDir(job.directory);if(lock){UnLock(lock);break;}}
 if(serial==20){job.directory[0]=0;return 0;}
 if(!nd_quote_path(quoted,sizeof(quoted),absolute))goto fail;
 sprintf(job.command,"%s Diag",quoted);
 job.port=CreateMsgPort();if(!job.port)goto fail;
 sprintf(job.name,"NovaDock.Sensors.%08lx.%08lx.%d",(ULONG)FindTask(NULL),job.started,serial);job.port->mp_Node.ln_Name=job.name;
 Forbid();AddPort(job.port);Permit();
 if(!nd_quote_path(self,sizeof(self),program))goto fail;sprintf(command,"%s WORK %s",self,job.name);
 input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(!input || !output){if(input)Close(input);if(output)Close(output);goto fail;}
 result=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);
 if(result==-1){Close(input);Close(output);goto fail;}return 1;
fail:
 fetch_clean(job.directory);job_close();return 0;
}
static int job_events(void)
{
 struct FetchMessage *m;char path[200],line[512];FILE *file;unsigned long now;int done=0,lines=0;
 if(!job.port)return 0;
 while((m=(struct FetchMessage *)GetMsg(job.port))!=NULL){
  if(m->magic!=FETCH_MAGIC || m->msg.mn_Length!=sizeof(*m)){if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;}
  if(m->kind==1){
   if(job.transferred)m->kind=-1;
   else{strcpy(m->command,job.command);strcpy(m->directory,job.directory);job.transferred=1;}
   ReplyMsg((struct Message *)m);
  }else if(m->kind==2){
   readings.available&=0x7fUL;now=nd_sensor_now();
   sprintf(path,"%s/fetch.log",m->directory);file=m->result==0 ? fopen(path,"r"):NULL;
   if(file){while(lines++<256 && fgets(line,sizeof(line),file)){if(strchr(line,'\n'))nd_sensor_warp_line(&readings,line,now);else{int c;while((c=fgetc(file))!=EOF && c!='\n'){} }}fclose(file);}
   nd_sensor_publish_warp(&readings,now);fetch_clean(m->directory);FreeVec(m);done=1;break;
  }
 }
 if(done || (unsigned long)(clock_ticks()-job.started)>1500UL){job_close();return 1;}return 0;
}
static unsigned long visible(const struct Settings *s){return readings.available&s->selected;}
static void panel_rows(const struct Settings *s,int *page,char text[4][128],char labels[4][20])
{
 int i,n=0,at=0,total=0;char value[32];unsigned long mask=visible(s);
 for(i=0;i<ND_SENSOR_COUNT;++i)if(mask&(1UL<<i))++total;
 if(*page*4>=total)*page=0;
 for(i=0;i<4;++i){labels[i][0]=text[i][0]=0;}
 if(!total){strcpy(labels[0],"Sensors");strcpy(text[0],readings.available ? "No readings selected in Edit settings.":"No sensors detected.");strcpy(text[1],"RTC: start AK_Sensors; Warp: install WarpTool.");return;}
 for(i=0;i<ND_SENSOR_COUNT;++i)if(mask&(1UL<<i)){
  if(n++<*page*4)continue;if(at>=4)break;
  nd_sensor_format(i,readings.value[i],value);sprintf(labels[at],"%d",n);sprintf(text[at],"%s: %s",nd_sensor_defs[i].label,value);++at;
 }
}
static struct NDPanel *open_panel(const struct Settings *s,int *page)
{
 static const char *buttons[]={"Previous","Next","Refresh","Close"};struct NDPanelField fields[4];char text[4][128],labels[4][20];int i;
 panel_rows(s,page,text,labels);for(i=0;i<4;++i){fields[i].label=labels[i];fields[i].value=text[i];fields[i].limit=0;}
 return nd_panel_open("NovaDock Sensors",fields,4,buttons,4);
}
static void update_panel(struct NDPanel *p,const struct Settings *s,int *page)
{char text[4][128],labels[4][20];int i;panel_rows(s,page,text,labels);for(i=0;i<4;++i){nd_panel_set_label(p,i,labels[i]);nd_panel_set(p,i,text[i]);}}
int main(int argc,char **argv)
{
 struct NDClient client;struct NDPanel *panel=NULL;struct Settings settings;unsigned int event,elapsed=0;
 unsigned long now,sampled=0,warped=0,rotated=0,mask,extra;int reread=1,force=1,page=0,current=-1,i,action,fresh_warp;
 char value[32],caption[32],status_path[100];
 if(argc==3 && !strcmp(argv[1],"WORK"))return fetch_worker(argv[2]);
 if(argc<2)return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 if(!nd_client_init(&client,argv[1]))return 20;
 for(;;){
  now=nd_sensor_now();if(reread){read_settings(argv[1],&settings);reread=0;force=1;}
  if(force || now<sampled || now-sampled>=settings.interval){
   nd_sensor_native_sample(&readings,now,settings.rtc_age);sampled=now;
   for(i=7;i<ND_SENSOR_COUNT;++i)if(now<readings.stamp[i] || now-readings.stamp[i]>90)readings.available&=~(1UL<<i);
   fresh_warp=nd_sensor_import_warp(&readings,now);
   if(!fresh_warp && (force || now<warped || now-warped>=30)){start_warp(argv[0],settings.warp);warped=now;}
   nd_sensor_publish(argv[1],&readings,now);if(panel)update_panel(panel,&settings,&page);force=0;
  }
  if(job_events()){nd_sensor_publish(argv[1],&readings,now);if(panel)update_panel(panel,&settings,&page);}
  mask=visible(&settings);strcpy(caption,settings.label);strcpy(value,readings.available ? "NO SELECTION":"NO SENSORS");
  if(mask){
   if(current<0 || !(mask&(1UL<<current)) || now<rotated || now-rotated>=5){
    for(i=1;i<=ND_SENSOR_COUNT;++i)if(mask&(1UL<<((current+i+ND_SENSOR_COUNT)%ND_SENSOR_COUNT))){current=(current+i+ND_SENSOR_COUNT)%ND_SENSOR_COUNT;break;}rotated=now;
   }
   strcpy(caption,strcmp(settings.label,"SENSOR") ? settings.label:nd_sensor_defs[current].short_label);nd_sensor_format(current,readings.value[current],value);
  }
  event=nd_client_update_quiet(&client,caption,value,ND_NO_METER,elapsed,500U);
  if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
  if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=open_panel(&settings,&page);force=1;}
  while(panel && (action=nd_panel_event(panel))!=0){
   if(action==-1 || action==103){nd_panel_close(panel);panel=NULL;}
   else{if(action==100 && page>0)--page;if(action==101)++page;if(action==102)force=1;update_panel(panel,&settings,&page);}
  }
  extra=nd_panel_signals(panel)|(job.port ? 1UL<<job.port->mp_SigBit:0);
  elapsed=nd_client_wait_signals(&client,job.port ? 25U:100U,extra);
 }
 job_close();nd_panel_close(panel);if(nd_sensor_status_path(argv[1],status_path,sizeof(status_path)))DeleteFile(status_path);nd_client_close(&client);return 0;
}
