/* Isolated device-first integration check against AmiHASS's sample provider. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_hass_picker.h"
#include "nd_hass_bridge.h"
long __stack=65536;
static int running(void)
{int live;Forbid();live=FindPort(AMIHASS_PORT)!=NULL;Permit();return live;}
static int exists(const char *path)
{BPTR l=Lock((STRPTR)path,ACCESS_READ);if(!l)return 0;UnLock(l);return 1;}
static struct AmiHASSMessage *exchange(struct AmiHASSClient *c,int kind,int offset)
{struct AmiHASSMessage *m;int i;if(!amihass_send(c,kind,"NovaDock.QA",NULL,NULL,offset,0,NULL,NULL))return NULL;for(i=0;i<120;++i){m=amihass_reply(c);if(m)return m;Delay(5);}return NULL;}
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
int main(void)
{
 struct NDHassPicker picker;struct NDHassBridge tile;struct AmiHASSClient observer;
 struct AmiHASSMessage *m;FILE *f;BPTR lock;int i,cycle,result=10,owned=0,configured=0;char device[96],entity[96];LONG rc;
 memset(&picker,0,sizeof(picker));memset(&tile,0,sizeof(tile));memset(&observer,0,sizeof(observer));
 if(running()||exists("ENVARC:AmiHASS/AmiHASS.cfg")||exists("ENVARC:AmiHASS/Private/connection")){puts("FAIL: requires an isolated guest without a service or credentials.");return 10;}
 lock=CreateDir("ENVARC:AmiHASS");if(lock)UnLock(lock);
 f=fopen("ENVARC:AmiHASS/AmiHASS.cfg","wb");if(!f)return 10;configured=1;
 rc=fputs("AMIHASSCFG1\n\n30 0 0 1 60 0 1\n\nBridge:\nSYS:Prefs\nSYS:WBStartup\n",f);if(fclose(f)||rc<0)goto finished;
 if(!start_service())goto finished;owned=1;
 m=exchange(&observer,AMIHASS_STATUS,0);if(!m)goto finished;
 rc=m->result==AMIHASS_OK&&!strcmp(m->item.state,AMIHASS_SERVER_SAMPLE);FreeVec(m);if(!rc){puts("FAIL: only sample controls are permitted.");goto finished;}
 if(!nd_hass_picker_init(&picker,1,NULL,NULL)||!page(&picker)||picker.count<1)goto finished;
 strcpy(device,picker.items[0].id);puts("PASS: real AmiHASS service returns sample devices.");nd_hass_client_close(&picker.client);
 if(!nd_hass_picker_init(&picker,0,device,"light")||!page(&picker)||picker.count<1)goto finished;
 strcpy(entity,picker.items[0].id);if(strncmp(entity,"light.",6)||!strstr(entity,".amihass_sample_"))goto finished;
 puts("PASS: selected device returns only its light entities.");
 if(!state(&tile,entity,NULL))goto finished;
 nd_hass_bridge_action(&tile,"HASS_QA",entity,"Turn on");if(!state(&tile,entity,"on"))goto finished;
 nd_hass_bridge_action(&tile,"HASS_QA",entity,"Turn off");if(!state(&tile,entity,"off"))goto finished;
 nd_hass_bridge_action(&tile,"HASS_QA",entity,"Toggle");if(!state(&tile,entity,"on"))goto finished;
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
