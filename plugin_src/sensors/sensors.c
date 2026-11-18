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
#include "nd_i2c_cache.h"
#include "nd_sensor_status.h"
#define FETCH_GUARD "NovaDock.Sensors.Fetch"
#define FETCH_PREFIX "T:NovaDockSensors-"
#include "../weather/nd_fetch_worker.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(SENSORS,"Sensors");
long __stack=32768;
static struct NDI2CCache i2c;
struct Settings {unsigned long selected,i2c_selected;int i2c_enabled;unsigned int interval,rtc_age;char warp[180],label[32];};
struct Job {struct MsgPort *port;char name[80],directory[160],command[1800];unsigned long started;int transferred;};
static struct NDSensors readings;
static struct Job job;
static unsigned long clock_ticks(void){return weather_ticks();}
static int apply(void *context,const char *key,const char *value)
{
 struct Settings *s=context;int i=nd_sensor_index(key);unsigned long n;
 if(nd_i2c_key(key)){char option[16];unsigned int j;for(j=0;j<i2c.count;++j){nd_i2c_option(i2c.readings[j].id,option);if(!strcmp(option,key)){if(!strcmp(value,"NO"))s->i2c_selected&=~(1UL<<j);else if(!strcmp(value,"YES"))s->i2c_selected|=1UL<<j;else return 0;}}}
 else if(!strcmp(key,"I2C")){s->i2c_enabled=!strcmp(value,"YES");}
 else if(!strcmp(key,"LABEL")){if(!*value || strlen(value)>31)return 0;strcpy(s->label,value);}
 else if(i>=0){if(!strcmp(value,"YES"))s->selected|=1UL<<i;else if(!strcmp(value,"NO"))s->selected&=~(1UL<<i);else return 0;}
 else if(!strcmp(key,"INTERVAL")){n=strtoul(value,NULL,10);if(n>=2 && n<=60)s->interval=(unsigned int)n;}
 else if(!strcmp(key,"RTC_MAX_AGE")){n=strtoul(value,NULL,10);if(n>=30 && n<=86400)s->rtc_age=(unsigned int)n;}
 else if(!strcmp(key,"WARPTOOL")){if(strlen(value)>=sizeof(s->warp))return 0;strcpy(s->warp,value);}
 return 1;
}
static void read_settings(const char *id,struct Settings *s)
{memset(s,0,sizeof(*s));s->selected=ND_SENSOR_ALL;s->i2c_selected=0xffffffffUL;s->i2c_enabled=1;strcpy(s->label,"SENSOR");s->interval=5;s->rtc_age=120;strcpy(s->warp,"C:WarpTool");nd_plugin_options_effective(id,apply,s);}
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
   if(file){while(lines++<256 && fgets(line,sizeof(line),file)){if(strchr(line,'\n') || feof(file))nd_sensor_warp_line(&readings,line,now);else{int c;while((c=fgetc(file))!=EOF && c!='\n'){} }}fclose(file);}
   nd_sensor_publish_warp(&readings,now);fetch_clean(m->directory);FreeVec(m);done=1;break;
  }
 }
 if(done || (unsigned long)(clock_ticks()-job.started)>1500UL){job_close();return 1;}return 0;
}
static void start_i2c(const char *program)
{
 char path[260],quoted[530];BPTR in,out;LONG rc;struct MsgPort *guard;
 Forbid();guard=FindPort("NovaDock.Sensors.I2C");Permit();if(guard || strlen(program)>240)return;
 strcpy(path,program);strcpy(FilePart(path),"SensorsI2C");if(!nd_quote_path(quoted,sizeof(quoted),path))return;
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(!in || !out){if(in)Close(in);if(out)Close(out);return;}
 rc=SystemTags(quoted,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);if(rc==-1){Close(in);Close(out);}
}
static int i2c_visible(const struct Settings *s,unsigned int i)
{return s->i2c_enabled && i<i2c.count && (s->i2c_selected&(1UL<<i)) && i2c.readings[i].available && i2c.provider && nd_i2c_fresh(&i2c,nd_sensor_now(),60) && i2c.readings[i].stamp<=nd_sensor_now() && nd_sensor_now()-i2c.readings[i].stamp<=60;}
static unsigned long visible(const struct Settings *s){return readings.available&s->selected;}
static void panel_rows(const struct Settings *s,int *page,char text[4][128],char labels[4][20])
{
 int i,n=0,at=0,total=0;char value[32];unsigned long now=nd_sensor_now(),age;const char *state;
 for(i=0;i<ND_SENSOR_COUNT;++i)if((readings.available&(1UL<<i)) || readings.stamp[i])++total;
 total+=(int)i2c.count;
 if(*page*4>=total)*page=0;for(i=0;i<4;++i){labels[i][0]=text[i][0]=0;}
 if(!total){strcpy(labels[0],"Status");strcpy(text[0],nd_sensor_summary(&readings,&i2c,s->selected,s->i2c_selected,s->i2c_enabled,now));strcpy(text[1],"Native: ZZ9000, AK_Sensors ENV and WarpTool Diag.");strcpy(text[2],s->i2c_enabled ? (i2c.stamp && !i2c.provider ? "Optional i2csensors.library v2 is not installed.":"Waiting for optional I2C provider."):"Optional I2C provider is disabled.");strcpy(text[3],"Use Edit settings to choose channels or the WarpTool path.");return;}
 for(i=0;i<ND_SENSOR_COUNT;++i)if((readings.available&(1UL<<i)) || readings.stamp[i]){
  if(n++<*page*4)continue;if(at>=4)break;age=i<3 ? s->interval*3UL:i<7 ? s->rtc_age:90;
  state=!(s->selected&(1UL<<i)) ? "Not selected":nd_sensor_reading_status((readings.available&(1UL<<i))!=0,readings.stamp[i],now,age);
  if(!strcmp(state,"Ready"))nd_sensor_format(i,readings.value[i],value);else strcpy(value,state);
  sprintf(labels[at],"%d",n);sprintf(text[at],"%s: %s",nd_sensor_defs[i].label,value);++at;
 }
 for(i=0;i<(int)i2c.count;++i){
  if(n++<*page*4)continue;if(at>=4)break;
  state=!s->i2c_enabled ? "Provider disabled":!(s->i2c_selected&(1UL<<i)) ? "Not selected":!i2c.provider ? "Library missing":nd_sensor_reading_status(i2c_visible(s,(unsigned int)i),i2c.readings[i].stamp,now,60);
  if(!strcmp(state,"Ready"))nd_i2c_format(&i2c.readings[i],value);else strcpy(value,state);
  sprintf(labels[at],"%d",n);sprintf(text[at],"%.63s: %.31s",i2c.readings[i].name,value);++at;
 }
}
static void publish_health(const char *id,const struct Settings *s,unsigned long now)
{
 char path[120];FILE *f;int i;const char *state;if(!nd_sensor_status_path(id,path,sizeof(path)-8))return;strcat(path,".health");f=fopen(path,"w");if(f){fprintf(f,"NDSHEALTH1 %lu\n%s\n",now,nd_sensor_summary(&readings,&i2c,s->selected,s->i2c_selected,s->i2c_enabled,now));
  for(i=0;i<ND_SENSOR_COUNT;++i)if(readings.stamp[i] || (readings.available&(1UL<<i))){state=nd_sensor_reading_status((readings.available&(1UL<<i))!=0,readings.stamp[i],now,i<3 ? s->interval*3UL:i<7 ? s->rtc_age:90);fprintf(f,"%s %s\n",nd_sensor_defs[i].key,state);}
  fclose(f);}
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
 unsigned long now,sampled=0,warped=0,rotated=0,mask,extra,i2c_started=0;int reread=1,force=1,page=0,current=-1,i,action,fresh_warp;
 char value[32],caption[32],status_path[100];
 if(argc==3 && !strcmp(argv[1],"WORK"))return fetch_worker(argv[2]);
 if(argc<2)return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 if(!nd_client_init(&client,argv[1]))return 20;
 for(;;){
  now=nd_sensor_now();if(reread){(void)nd_i2c_load(&i2c);read_settings(argv[1],&settings);reread=0;force=1;}
  if(force || now<sampled || now-sampled>=settings.interval){
   if(settings.i2c_enabled){
    if(nd_i2c_load(&i2c))read_settings(argv[1],&settings);
    if((!i2c.stamp || now<i2c.stamp || now-i2c.stamp>=(i2c.provider ? 5UL:30UL)) && (!i2c_started || now<i2c_started || now-i2c_started>=5UL)){start_i2c(argv[0]);i2c_started=now;}
   }
   nd_sensor_native_sample(&readings,now,settings.rtc_age);sampled=now;
   for(i=7;i<ND_SENSOR_COUNT;++i)if(now<readings.stamp[i] || now-readings.stamp[i]>90)readings.available&=~(1UL<<i);
   fresh_warp=nd_sensor_import_warp(&readings,now);
   if(!fresh_warp && (force || now<warped || now-warped>=30)){start_warp(argv[0],settings.warp);warped=now;}
   nd_sensor_publish(argv[1],&readings,now);publish_health(argv[1],&settings,now);if(panel)update_panel(panel,&settings,&page);force=0;
  }
  if(job_events()){nd_sensor_publish(argv[1],&readings,now);publish_health(argv[1],&settings,now);if(panel)update_panel(panel,&settings,&page);}
  mask=visible(&settings);strcpy(caption,settings.label);strcpy(value,nd_sensor_summary(&readings,&i2c,settings.selected,settings.i2c_selected,settings.i2c_enabled,now));
  if(current<0 || current>=ND_SENSOR_COUNT+(int)i2c.count ||
     (current<ND_SENSOR_COUNT ? !(mask&(1UL<<current)):!i2c_visible(&settings,(unsigned int)(current-ND_SENSOR_COUNT))) || now<rotated || now-rotated>=5){
   int total=ND_SENSOR_COUNT+(int)i2c.count,next;current=current<0 ? total-1:current;
   for(i=1;i<=total;++i){next=(current+i)%total;if(next<ND_SENSOR_COUNT ? (mask&(1UL<<next))!=0:i2c_visible(&settings,(unsigned int)(next-ND_SENSOR_COUNT))){current=next;break;}}rotated=now;
  }
  if(current>=0 && current<ND_SENSOR_COUNT && (mask&(1UL<<current))){
   strcpy(caption,strcmp(settings.label,"SENSOR") ? settings.label:nd_sensor_defs[current].short_label);nd_sensor_format(current,readings.value[current],value);
  }else if(current>=ND_SENSOR_COUNT && i2c_visible(&settings,(unsigned int)(current-ND_SENSOR_COUNT))){nd_i2c_format(&i2c.readings[current-ND_SENSOR_COUNT],value);mask=1;}
  if(!mask)event=nd_client_update_art_quiet(&client,caption,value,"SadSensor",0,elapsed,500U);
  else event=nd_client_update_quiet(&client,caption,value,ND_NO_METER,elapsed,500U);
  if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
  if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=open_panel(&settings,&page);force=1;}
  while(panel && (action=nd_panel_event(panel))!=0){
   if(action==-1 || action==103){nd_panel_close(panel);panel=NULL;}
   else{if(action==100 && page>0)--page;if(action==101)++page;if(action==102)force=1;update_panel(panel,&settings,&page);}
  }
  extra=nd_panel_signals(panel)|(job.port ? 1UL<<job.port->mp_SigBit:0);
  elapsed=nd_client_wait_signals(&client,job.port ? 25U:100U,extra);
 }
 job_close();nd_panel_close(panel);if(nd_sensor_status_path(argv[1],status_path,sizeof(status_path)-8)){DeleteFile(status_path);strcat(status_path,".health");DeleteFile(status_path);}nd_client_close(&client);return 0;
}
