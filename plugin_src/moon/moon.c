#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_task.h"
#include "nd_option_runtime.h"
#include "nd_plugin_versions.h"
#include "nd_moon.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct Library *MathIeeeDoubBasBase=NULL;
const char nd_version[]=ND_PLUGIN_VER_STRING(MOON,"Moon");long __stack=65536;
struct MoonJob {long days;unsigned long seconds;char latitude[96],longitude[96],offset[96];struct NDMoon moon;};
static int work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{struct MoonJob *j=data;long lat,lon;int offset,ok;(void)parent;if(action!=1 || bytes!=sizeof(*j) || !memchr(j->latitude,0,96)||!memchr(j->longitude,0,96)||!memchr(j->offset,0,96))return 0;
 if(!nd_moon_coordinate(j->latitude,90,&lat)||!nd_moon_coordinate(j->longitude,180,&lon)||!nd_moon_offset(j->offset,&offset))return -2;
 MathIeeeDoubBasBase=OpenLibrary("mathieeedoubbas.library",0);if(!MathIeeeDoubBasBase)return -3;ok=nd_moon_calculate(j->days,j->seconds,offset,lat,lon,&j->moon);CloseLibrary(MathIeeeDoubBasBase);MathIeeeDoubBasBase=NULL;return ok;}
static void request(struct NDTask *task,const char *program,const char *id,struct MoonJob *j,char label[32],unsigned long *interval)
{struct NDOptionString f[5];struct DateStamp d;char refresh[16];const char *keys[]={"LABEL","LATITUDE","LONGITUDE","UTC_OFFSET","REFRESH_MINUTES"},*fallback[]={"MOON","0","0","+00:00","60"};char *out[5];unsigned int limits[]={32,96,96,96,16},i;char guard[64];out[0]=label;out[1]=j->latitude;out[2]=j->longitude;out[3]=j->offset;out[4]=refresh;for(i=0;i<5;++i){f[i].key=keys[i];f[i].fallback=fallback[i];f[i].out=out[i];f[i].limit=limits[i];}nd_plugin_option_strings(id,f,5);*interval=(unsigned long)atoi(refresh)*3000UL;if(*interval<45000UL || *interval>1080000UL)*interval=180000UL;DateStamp(&d);j->days=d.ds_Days;j->seconds=(unsigned long)d.ds_Minute*60UL+d.ds_Tick/50;sprintf(guard,"NovaDock.Moon.%.19s",id);nd_task_start(task,program,guard,1,j,sizeof(*j));}
static struct NDPanel *details(struct NDPanel *panel,struct MoonJob *j,int ready)
{struct NDPanelField f[8];const char *b[]={"Refresh","Close"};char lit[32],age[32],rise[48],set[48];if(ready){sprintf(lit,"%u%% (estimated)",j->moon.illumination);sprintf(age,"%u.%02u days",j->moon.age100/100,j->moon.age100%100);nd_moon_time(j->moon.sunrise,j->moon.rise_day,j->moon.polar,rise);nd_moon_time(j->moon.sunset,j->moon.set_day,j->moon.polar,set);}else{strcpy(lit,"Calculating...");age[0]=rise[0]=set[0]=0;}
 f[0].label="Phase";f[0].value=ready ? nd_moon_phase(j->moon.frame):"Waiting";f[1].label="Illuminated";f[1].value=lit;f[2].label="Moon age";f[2].value=age;f[3].label="Sunrise";f[3].value=rise;f[4].label="Sunset";f[4].value=set;f[5].label="Local date";f[5].value=j->moon.date;f[6].label="UTC offset";f[6].value=j->offset;f[7].label="Accuracy";f[7].value="Mean lunar cycle; approximate solar times";{int i;for(i=0;i<8;++i)f[i].limit=0;}if(panel){int i;if(ready)nd_panel_title(panel,"Moon - approximate phase and solar times");for(i=0;i<8;++i)nd_panel_set(panel,i,f[i].value);return panel;}return nd_panel_open("Moon - approximate phase and solar times",f,8,b,2);}
int main(int argc,char **argv)
{struct NDClient dock;struct NDTask task;struct NDTaskPacket *reply;struct MoonJob j;struct NDPanel *panel=NULL;unsigned int event,elapsed=0;unsigned long wait=0,interval=180000;int ready=0,action,refresh=1;char label[32]="MOON",value[96]="Calculating...";
 if(argc==3&&!strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],work);
 if(argc==2&&!strcmp(argv[1],"SELFTEST")){memset(&j,0,sizeof(j));j.days=8040;j.seconds=18*3600+14*60;strcpy(j.latitude,"0");strcpy(j.longitude,"0");strcpy(j.offset,"+00:00");return work(1,&j,sizeof(j),NULL)==1 && j.moon.frame==0 && j.moon.illumination==0 ? 0:10;}
 if(argc!=2||!nd_client_init(&dock,argv[1]))return 20;memset(&task,0,sizeof(task));memset(&j,0,sizeof(j));((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 for(;;){if(refresh&&!task.port){request(&task,argv[0],argv[1],&j,label,&interval);refresh=0;wait=interval;if(!task.port){strcpy(value,"Worker unavailable; retry shortly");wait=250;}}
  reply=nd_task_poll(&task);if(reply){if(reply->result==1 && reply->bytes==sizeof(j)){j=*(struct MoonJob *)reply->data;ready=1;sprintf(value,"%s - %u%%",nd_moon_phase(j.moon.frame),j.moon.illumination);if(panel){panel=details(panel,&j,ready);}}else{ready=0;strcpy(value,reply->result==-2 ? "Set valid location and UTC offset":"Calculation failed");if(panel){panel=details(panel,&j,0);nd_panel_set(panel,7,reply->result==-2 ? "Latitude -90..90; longitude -180..180; offset +/-HH:MM":value);nd_panel_title(panel,value);}}FreeVec(reply);}
  event=nd_client_update_art_quiet(&dock,label,value,"moon",ready ? j.moon.frame:0,elapsed,500);if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS){nd_task_close(&task);refresh=1;ready=0;}
  if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=details(panel,&j,ready);}
  action=nd_panel_event(panel);if(action==-1||action==101){nd_panel_close(panel);panel=NULL;}else if(action==100){refresh=1;if(panel)nd_panel_title(panel,"Refreshing Moon estimates...");}
  if(wait>elapsed)wait-=elapsed;else{wait=0;refresh=1;}elapsed=nd_client_wait_signals(&dock,task.port ? 10:250,nd_panel_signals(panel)|nd_task_signals(&task));
 }
 nd_task_close(&task);nd_panel_close(panel);nd_client_close(&dock);return 0;
}
