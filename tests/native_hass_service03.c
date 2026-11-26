/* Production NovaDock adapter against a real AmiHASS service, sample data only.
 * The host runner mounts only disposable drives and disables network access. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_hass_source.h"
long __stack=65536;
static struct AmiHASSClient observer;
static struct NDHassBridge tile;
static int owned=0;
static int running(void)
{int live;Forbid();live=FindPort(AMIHASS_PORT)!=NULL;Permit();return live;}
static struct AmiHASSMessage *exchange(int kind,const char *id,int offset)
{struct AmiHASSMessage *m;int i;
 if(!amihass_send(&observer,kind,"HASS.QA.Observer",id,NULL,offset,AMIHASS_QUERY_ONCE,NULL,NULL))return NULL;
 for(i=0;i<150;++i){m=amihass_reply(&observer);if(m)return m;Delay(2);}return NULL;}
static int assign_private(const char *name,const char *path)
{BPTR lock=CreateDir(path);if(lock)UnLock(lock);lock=Lock(path,ACCESS_READ);if(!lock)return 0;if(AssignLock((STRPTR)name,lock))return 1;UnLock(lock);return 0;}
static int config(int sample,int doors)
{FILE *f=fopen("ENVARC:AmiHASS/AmiHASS.cfg","wb");int ok;if(!f)return 0;
 ok=fprintf(f,"AMIHASSCFG1\n\n30 0 0 1 60 0 %d 0 0 %d\n\nQATests:Service\nQATests:Service\nQATests:Service\n",sample,doors)>0;
 if(fclose(f))ok=0;return ok;}
static int launch(int sample,int doors)
{BPTR in,out;LONG rc;int i;if(running()||!config(sample,doors))return 0;
 in=Open("NIL:",MODE_OLDFILE);out=Open("AutoRun:service.log",MODE_NEWFILE);
 if(!in||!out){if(in)Close(in);if(out)Close(out);return 0;}
 rc=SystemTags("QATests:Service/AmiHASSService SERVICE",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
 if(rc<0){Close(in);Close(out);return 0;}owned=1;
 for(i=0;i<100&&!running();++i)Delay(5);return running();}
static int stop(void)
{struct AmiHASSMessage *m;int i;nd_hass_bridge_close(&tile);
 if(owned&&running()){m=exchange(AMIHASS_STOP,NULL,0);if(m)FreeVec(m);}
 amihass_client_close(&observer);
 for(i=0;i<100&&running();++i)Delay(5);
 if(running())return 0;owned=0;return 1;}
static int status(const char *expected)
{struct AmiHASSMessage *m=exchange(AMIHASS_STATUS,NULL,0);int ok;
 if(!m)return 0;ok=m->result==AMIHASS_OK&&!strcmp(m->item.state,expected)&&nd_hass_features(m)==ND_HASS_FEATURE_CURRENT;
 printf("Service state %s, metadata rows %ld, required features %u.\n",m->item.state,m->count,nd_hass_features(m));FreeVec(m);return ok;}
static int diagnostics(void)
{struct AmiHASSMessage *m;int offset=0,total=-1,i,ok;
 do{m=exchange(AMIHASS_DIAGNOSTICS,NULL,offset);if(!m)return 0;
 ok=m->result==AMIHASS_OK&&m->offset==offset&&m->count>0&&m->count<=AMIHASS_PAGE&&m->total>=offset+m->count&&(total<0||m->total==total);
 if(total<0)total=m->total;
 for(i=0;i<m->count&&i<AMIHASS_PAGE;++i)if(!strncmp(m->items[i].id,"http.",5)&&strtoul(m->items[i].state,NULL,10))ok=0;
 offset+=m->count;FreeVec(m);if(!ok)return 0;
 }while(offset<total);
 printf("PASS: all %d diagnostic rows paged; sample run sent zero HTTP requests.\n",total);return 1;}
static int read_tile(const char *id)
{int i;nd_hass_bridge_close(&tile);
 for(i=0;i<250;++i){nd_hass_bridge_poll(&tile,"HASS.QA.Tile",id,2);if(tile.result==AMIHASS_OK&&!tile.client.pending)return 1;Delay(2);}return 0;}
static int control(const char *id,const char *action)
{struct AmiHASSMessage *m;struct AHControls caps;int i,ok;
 if(!read_tile(id))return 0;nd_hass_bridge_action(&tile,"HASS.QA.Tile",id,action);
 if(tile.action_stage!=1)return 0;
 for(i=0;i<250;++i){nd_hass_bridge_poll(&tile,"HASS.QA.Tile",id,2);if(tile.action_stage==3&&!tile.client.pending)break;Delay(2);}
 if(i==250)return 0;m=exchange(AMIHASS_QUERY,id,0);if(!m)return 0;amihass_controls_items(m->items,m->count,&caps);
 ok=m->result==AMIHASS_OK&&amihass_control_matches(&caps,m->item.state,action);
 printf("Control %s: %s; independently read state %s.\n",id,action,m->item.state);FreeVec(m);return ok;}
int main(void)
{BPTR lock;int ok=0;const char *stage="isolation";
 memset(&observer,0,sizeof(observer));memset(&tile,0,sizeof(tile));
 lock=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!lock||running()){if(lock)UnLock(lock);puts("FAIL: isolated fixture without an existing service required.");return 10;}UnLock(lock);
 printf("Wire sizes: entity %lu, status %lu, message %lu bytes.\n",(unsigned long)sizeof(struct AmiHASSEntity),(unsigned long)sizeof(struct AmiHASSStatus),(unsigned long)sizeof(struct AmiHASSMessage));
 if(sizeof(struct AmiHASSMessage)!=5220UL||sizeof(struct AmiHASSEntity)!=280UL||sizeof(struct AmiHASSStatus)!=180UL)goto done;
 lock=CreateDir("RAM:HASS-QA");if(lock)UnLock(lock);
 if(!assign_private("ENV","RAM:HASS-QA/Env")||!assign_private("ENVARC","RAM:HASS-QA/Archive"))goto done;
 lock=CreateDir("ENVARC:AmiHASS");if(!lock)goto done;UnLock(lock);
 stage="unconfigured";if(!launch(0,0)||!status(AMIHASS_SERVER_UNCONFIGURED))goto done;
 if(!diagnostics()||!stop())goto done;
 stage="sample controls";if(!launch(1,0)||!status(AMIHASS_SERVER_SAMPLE))goto done;
 if(!control("light.amihass_sample_living_room","Brightness 37")||!control("light.amihass_sample_living_room","Color #4080ff")||!control("climate.amihass_sample_hallway","Temperature 23"))goto done;
 /* AmiHASS intentionally bypasses live door policy for shared sample data.
  * Protocol refusal feedback is covered by test_hass_service03.py. */
 stage="sample lock";if(!control("lock.amihass_sample_front_door","Unlock")||!control("lock.amihass_sample_front_door","Lock")||!diagnostics())goto done;
 ok=1;
 done:if(!stop())ok=0;
 if(ok)puts("PASS: production NovaDock adapter interoperates with AmiHASS 0.3; sample controls, diagnostics and clean stop verified.");
 else printf("FAIL: HASS compatibility check at %s.\n",stage);
 return ok ? 0:10;}
