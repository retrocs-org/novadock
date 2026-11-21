/* Isolated device-first integration check against AmiHASS's sample provider. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_hass_picker.h"
#include "nd_hass_bridge.h"
#include "nd_hass_source.h"
long __stack=65536;
static int running(void)
{int live;Forbid();live=FindPort(AMIHASS_PORT)!=NULL;Permit();return live;}
static int exists(const char *path)
{BPTR l=Lock((STRPTR)path,ACCESS_READ);if(!l)return 0;UnLock(l);return 1;}
static struct AmiHASSMessage *exchange(struct AmiHASSClient *c,int kind,int offset)
{struct AmiHASSMessage *m;int i;if(!amihass_send(c,kind,"NovaDock.RoomQA",NULL,NULL,offset,0,NULL,NULL))return NULL;for(i=0;i<120;++i){m=amihass_reply(c);if(m)return m;Delay(5);}return NULL;}
static int page(struct NDHassPicker *p)
{int i;for(i=0;i<160;++i){nd_hass_picker_step(p);if(p->result==AMIHASS_OK)return 1;if(p->result!=AMIHASS_WAIT&&p->result!=AMIHASS_BUSY)return 0;Delay(5);}return 0;}
static int state(struct NDHassBridge *tile,const char *entity,const char *wanted)
{int i;for(i=0;i<160;++i){nd_hass_bridge_poll(tile,"HASS_QA",entity,5);if(tile->result==AMIHASS_OK&&!tile->client.pending&&(!wanted||!strcmp(tile->entity.state,wanted)))return 1;Delay(5);}return 0;}
static int start_service(void)
{BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;int i;
 if(!in||!out){if(in)Close(in);if(out)Close(out);return 0;}
 rc=SystemTags("Bridge:AmiHASSService SERVICE",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
 if(rc<0){Close(in);Close(out);return 0;}for(i=0;i<100&&!running();++i)Delay(5);return running();}
static int brightness(struct NDHassBridge *tile,const char *entity,int level)
{char action[20];sprintf(action,"Brightness %d",level);nd_hass_bridge_action(tile,"HASS_QA",entity,action);
 return state(tile,entity,level ? "on":"off")&&tile->controls.brightness&&tile->controls.level_known&&tile->controls.level==level;}
static int native_ui(const char *command)
{BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("QATests:hass-ui.log",MODE_NEWFILE);LONG result;if(!in||!out)return 0;result=SystemTags((STRPTR)command,SYS_Input,in,SYS_Output,out,NP_StackSize,65536UL,TAG_DONE);Close(in);Close(out);return result==0;}
static int leases(struct AmiHASSClient *client,int expected)
{struct AmiHASSMessage *m;int i,n;if(!amihass_send(client,AMIHASS_DIAGNOSTICS,"NovaDock.RoomQA",NULL,NULL,0,0,"memory",NULL))return 0;m=NULL;for(n=0;n<120&&!m;++n){m=amihass_reply(client);if(!m)Delay(5);}if(!m)return 0;n=-1;for(i=0;i<m->count;++i)if(!strcmp(m->items[i].id,"leases.active"))n=atoi(m->items[i].state);FreeVec(m);return n==expected;}
int main(void)
{
 struct NDHassPicker picker;struct NDHassBridge tile;struct AmiHASSClient observer;
 struct AmiHASSMessage *m;FILE *f;BPTR lock;int i,cycle,result=10,owned=0,configured=0;char device[96],entity[96];LONG rc;
 memset(&picker,0,sizeof(picker));memset(&tile,0,sizeof(tile));memset(&observer,0,sizeof(observer));
 if(running()||exists("ENVARC:AmiHASS/AmiHASS.cfg")||exists("ENVARC:AmiHASS/Private/connection")){puts("FAIL: requires an isolated guest without a service or credentials.");return 10;}
 lock=CreateDir("ENVARC:AmiHASS");if(lock)UnLock(lock);
 f=fopen("ENVARC:AmiHASS/AmiHASS.cfg","wb");if(!f)return 10;configured=1;
 rc=fputs("AMIHASSCFG1\n\n30 0 0 1 60 0 1\n\nBridge:\nSYS:Prefs\nSYS:WBStartup\n",f);if(fclose(f)||rc<0)goto finished;
 {struct NDHassSource missing;memset(&missing,0,sizeof(missing));nd_hass_source_poll(&missing,"HASS.RoomQA",0);if(missing.client.pending||!strstr(nd_hass_source_label(&missing),"Start AmiHASS 0.2"))goto finished;nd_hass_source_close(&missing);puts("PASS: missing bridge gives AmiHASS 0.2+ startup guidance without sending an action.");}
 if(!start_service())goto finished;owned=1;
 m=exchange(&observer,AMIHASS_STATUS,0);if(!m)goto finished;
 rc=m->result==AMIHASS_OK&&!strcmp(m->item.state,AMIHASS_SERVER_SAMPLE);FreeVec(m);if(!rc){puts("FAIL: only sample controls are permitted.");goto finished;}
 /* Room/type/search combine on the service; unknown rooms must return no rows. */
 if(!nd_hass_picker_init(&picker,1,NULL,"light")||!nd_hass_picker_scope(&picker,"Study","desk")||!page(&picker)||picker.total!=1||strcmp(picker.items[0].name,"Sample desk"))goto finished;
 strcpy(device,picker.items[0].id);nd_hass_client_close(&picker.client);
 if(!nd_hass_picker_init(&picker,0,device,"light")||!nd_hass_picker_scope(&picker,"Study","")||!page(&picker)||picker.total!=1||strcmp(picker.items[0].id,"light.amihass_sample_desk"))goto finished;
 if(!nd_hass_picker_scope(&picker,"No such room","")||!page(&picker)||picker.count)goto finished;nd_hass_client_close(&picker.client);
 if(!nd_hass_picker_init(&picker,0,"","scene")||!nd_hass_picker_scope(&picker,"House","")||!page(&picker)||picker.count<1)goto finished;
 for(i=0;i<picker.count;++i)if(strncmp(picker.items[i].id,"scene.",6))goto finished;nd_hass_client_close(&picker.client);
 puts("PASS: room, type, device and search scopes combine; unknown room is empty; scenes stay filtered.");
 if(!state(&tile,"media_player.amihass_sample_living_room","playing")||!tile.controls.media_volume||strcmp(tile.controls.media_title,"Amiga radio"))goto finished;
 {const char *commands[]={"Pause","Play","Volume 43","Next","Previous"};const char *states[]={"paused","playing","playing","playing","playing"};for(i=0;i<5;++i){nd_hass_bridge_action(&tile,"HASS_QA","media_player.amihass_sample_living_room",commands[i]);if(!state(&tile,"media_player.amihass_sample_living_room",states[i]))goto finished;if(i==2&&tile.controls.volume!=43)goto finished;if(i==3&&strcmp(tile.controls.media_title,"Next sample track"))goto finished;if(i==4&&strcmp(tile.controls.media_title,"Previous sample track"))goto finished;}}
 nd_hass_bridge_close(&tile);puts("PASS: media title/artist, Play/Pause/Previous/Next and volume 43 use the independent sample service through Exec SDK.");

 if(!native_ui("QATests:LockUICheck"))goto finished;
 if(!state(&tile,"lock.amihass_sample_front_door","unlocked")||!tile.controls.lock||!tile.controls.unlock||!tile.controls.lock_open)goto finished;
 {const char *commands[]={"Lock","Unlock","Open lock"};const char *states[]={"locked","unlocked","open"};for(i=0;i<3;++i){nd_hass_bridge_action(&tile,"HASS_QA","lock.amihass_sample_front_door",commands[i]);if(!state(&tile,"lock.amihass_sample_front_door",states[i])||tile.action_stage!=3)goto finished;}}
 nd_hass_bridge_close(&tile);
 if(!state(&tile,"cover.amihass_sample_garage","closed"))goto finished;
 {const char *commands[]={"Open cover","Stop cover","Close cover"};const char *states[]={"open","open","closed"};for(i=0;i<3;++i){nd_hass_bridge_action(&tile,"HASS_QA","cover.amihass_sample_garage",commands[i]);if(!state(&tile,"cover.amihass_sample_garage",states[i])||tile.action_stage!=3)goto finished;}}
 nd_hass_bridge_close(&tile);puts("PASS: sample lock/unlock/latch and garage open/stop/close confirm readback through the independent bridge.");
 if(!native_ui("QATests:HASS PICKER-SELFTEST")||!native_ui("QATests:HASS CONTROL-SELFTEST"))goto finished;
 if(!native_ui("QATests:HASS ROOM-SELFTEST")||!native_ui("QATests:HASS ROOM-PICKER-SELFTEST")||!native_ui("QATests:HASS ROOM-SHARE-SELFTEST"))goto finished;
 puts("PASS: room summary and room-only chooser use sample service through Exec SDK.");
 puts("PASS: native room picker and light/climate/fan/cover/read-only panels open correctly.");
 if(!leases(&observer,0))goto finished;
 if(!nd_hass_picker_init(&picker,1,NULL,NULL)||!page(&picker)||picker.count<1)goto finished;
 strcpy(device,picker.items[0].id);puts("PASS: real AmiHASS service returns sample devices.");nd_hass_client_close(&picker.client);
 if(!nd_hass_picker_init(&picker,0,device,"light")||!page(&picker)||picker.count<1)goto finished;
 strcpy(entity,picker.items[0].id);if(strncmp(entity,"light.",6)||!strstr(entity,".amihass_sample_"))goto finished;
 puts("PASS: selected device returns only its light entities.");
 nd_hass_bridge_options(&tile,5,1);if(!state(&tile,entity,NULL)||!leases(&observer,1))goto finished;
 nd_hass_bridge_action(&tile,"HASS_QA",entity,"Turn off");if(tile.client.pending||tile.action_stage)goto finished;
 nd_hass_bridge_close(&tile);if(!leases(&observer,0))goto finished;
 puts("PASS: read-only tile sends no action and closing releases its selected-entity lease.");
 if(!state(&tile,entity,NULL))goto finished;
 nd_hass_bridge_action(&tile,"HASS_QA",entity,"Turn on");if(!state(&tile,entity,"on"))goto finished;
 nd_hass_bridge_action(&tile,"HASS_QA",entity,"Turn off");if(!state(&tile,entity,"off"))goto finished;
 nd_hass_bridge_action(&tile,"HASS_QA",entity,"Toggle");if(!state(&tile,entity,"on"))goto finished;
 if(tile.action_stage!=3)goto finished;
 puts("PASS: on/off/toggle read back the simulated bulb's state through AmiHASS.");
 if(!brightness(&tile,entity,25)||!brightness(&tile,entity,100)||!brightness(&tile,entity,0))goto finished;
 puts("PASS: brightness 25/100/0 returns confirmed levels, with zero turning the sample bulb off.");
 if(!amihass_send(&observer,AMIHASS_ACTION,"External.QA",entity,"Brightness 65",0,0,NULL,NULL))goto finished;
 m=NULL;for(i=0;i<120&&!m;++i){m=amihass_reply(&observer);if(!m)Delay(5);}if(!m)goto finished;
 rc=m->result==AMIHASS_WAIT||m->result==AMIHASS_OK;FreeVec(m);if(!rc)goto finished;
 if(!state(&tile,entity,"on")||tile.controls.level!=65)goto finished;
 puts("PASS: a second application's action refreshes the tile's state and brightness.");
 for(cycle=0;cycle<3;++cycle){
  if(!brightness(&tile,entity,0))goto finished;
  m=exchange(&observer,AMIHASS_STOP,0);if(!m)goto finished;FreeVec(m);
  for(i=0;i<100&&running();++i)Delay(5);if(running())goto finished;
  /* Suppress automatic startup only while observing this deliberate outage. */
  tile.start_wait=1500;tile.starting=0;tile.query_wait=0;
  nd_hass_bridge_poll(&tile,"HASS_QA",entity,5);
  if(tile.result==AMIHASS_OK||tile.controls.brightness)goto finished;
  nd_hass_bridge_action(&tile,"HASS_QA",entity,"Brightness 90");
  if(tile.client.pending)goto finished;
  if(!start_service()||!state(&tile,entity,"on")||tile.controls.level==90)goto finished;
 }
 puts("PASS: three service outages/restarts recover without replaying offline brightness actions.");
 m=exchange(&observer,AMIHASS_DIAGNOSTICS,0);if(!m)goto finished;rc=1;
 for(i=0;i<m->count;++i)if(!strncmp(m->items[i].id,"http.",5)&&strcmp(m->items[i].state,"0"))rc=0;
 FreeVec(m);if(!rc)goto finished;puts("PASS: no HTTP requests or real Home Assistant controls occurred.");result=0;
finished:
 nd_hass_client_close(&picker.client);nd_hass_bridge_close(&tile);
 if(owned&&running()){m=exchange(&observer,AMIHASS_STOP,0);if(m)FreeVec(m);for(i=0;i<100&&running();++i)Delay(5);if(running())result=10;}
 amihass_client_close(&observer);if(configured)DeleteFile("ENVARC:AmiHASS/AmiHASS.cfg");
 if(result)puts("FAIL: device-first sample integration check did not finish.");return result;
}
