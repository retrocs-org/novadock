"""AmiHASS 0.3 policy markers and updated local climate validation."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
code=r"""
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_hass_native.h"
static struct AmiHASSMessage packet,*incoming;
static int kind,sends,actions;static char target[96];
static void FreeVec(void *p){assert(p==&packet);}
void amihass_client_close(struct AmiHASSClient *c){c->pending=NULL;incoming=NULL;}
int amihass_start(const char *p){(void)p;return 0;}
int amihass_send(struct AmiHASSClient *c,int k,const char *o,const char *e,const char *a,int off,int refresh,const char *d,const char *f){assert(!c->pending);(void)o;(void)a;(void)off;(void)refresh;(void)d;(void)f;kind=k;strcpy(target,e?e:"");++sends;if(k==AMIHASS_ACTION)++actions;c->pending=&packet;return 1;}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c){struct AmiHASSMessage *m=incoming;if(m){incoming=NULL;c->pending=NULL;}return m;}
#include "nd_hass_source.h"
static void answer(int result,const char *state){memset(&packet,0,sizeof(packet));packet.kind=kind;packet.result=result;strcpy(packet.item.id,target);strcpy(packet.item.state,state);incoming=&packet;}
int main(void){struct NDHassBridge b;struct NDHassSource s;struct AHControls c;int n;unsigned int i;
 assert(sizeof(struct AmiHASSEntity)==280&&sizeof(struct AmiHASSStatus)==180&&sizeof(struct AmiHASSMessage)==5220);
 memset(&c,0,sizeof(c));c.temperature=1;c.minimum=4460;c.maximum=8600;c.step=100;strcpy(c.unit,"F");assert(amihass_control_allowed(&c,"Temperature 72"));assert(!amihass_control_allowed(&c,"Temperature 72.5"));
 memset(&b,0,sizeof(b));nd_hass_bridge_poll(&b,"HASS.QA","light.desk",0);answer(AMIHASS_BUSY,AMIHASS_STATE_SELECTION_FULL);n=sends;nd_hass_bridge_poll(&b,"HASS.QA","light.desk",0);assert(b.result==AMIHASS_BUSY&&b.query_wait==1500&&strstr(nd_hass_bridge_status(&b),"selection limit"));
 nd_hass_bridge_poll(&b,"HASS.QA","light.desk",1499);assert(sends==n);nd_hass_bridge_poll(&b,"HASS.QA","light.desk",1);assert(sends==n+1);answer(AMIHASS_OK,"on");nd_hass_bridge_poll(&b,"HASS.QA","light.desk",0);assert(!b.problem[0]);
 nd_hass_bridge_close(&b);strcpy(b.entity.id,"lock.front");strcpy(b.entity.state,"locked");strcpy(b.target,"lock.front");strcpy(b.owner,"HASS.QA");b.result=AMIHASS_OK;b.controls.unlock=1;
 nd_hass_bridge_action(&b,"HASS.QA","lock.front","Unlock");assert(actions==1);answer(AMIHASS_UNSUPPORTED,AMIHASS_STATE_CONFIRMATION);nd_hass_bridge_poll(&b,"HASS.QA","lock.front",0);assert(b.action_stage==4&&strstr(nd_hass_bridge_feedback(&b),"Script doors"));
 answer(AMIHASS_OK,"locked");nd_hass_bridge_poll(&b,"HASS.QA","lock.front",0);assert(actions==1&&b.action_stage==4&&strstr(nd_hass_bridge_feedback(&b),"Script doors"));assert(strstr(nd_hass_bridge_status(&b),"Script doors"));nd_hass_bridge_refresh(&b);assert(!b.problem[0]&&!b.action_stage);nd_hass_bridge_close(&b);
 memset(&packet,0,sizeof(packet));packet.result=AMIHASS_UNSUPPORTED;strcpy(packet.item.state,AMIHASS_STATE_CODE_REQUIRED);assert(strstr(nd_hass_reply_problem(&packet),"code-protected"));packet.result=AMIHASS_OK;assert(!nd_hass_reply_problem(&packet));packet.result=AMIHASS_BUSY;strcpy(packet.item.state,AMIHASS_STATE_CONFIRMATION);assert(!nd_hass_reply_problem(&packet));
 memset(&s,0,sizeof(s));s.attempted=s.available=s.known=1;s.features=ND_HASS_FEATURE_CURRENT;strcpy(s.state,AMIHASS_SERVER_AUTH_REJECTED);assert(!strcmp(nd_hass_source_label(&s),"Check AmiHASS credentials"));strcpy(s.state,AMIHASS_SERVER_RETRY_WAIT);assert(strstr(nd_hass_source_label(&s),"reconnect"));
 /* Extra 0.3 capability rows must not make a 0.2-compatible service fail. */
 memset(&packet,0,sizeof(packet));packet.kind=AMIHASS_STATUS;packet.result=AMIHASS_OK;packet.count=AMIHASS_PAGE;strcpy(packet.items[0].id,AMIHASS_CAP_AREAS);strcpy(packet.items[1].id,AMIHASS_CAP_FILTERS);strcpy(packet.items[2].id,AMIHASS_CAP_SUBSCRIPTIONS);for(i=0;i<AMIHASS_PAGE;++i)strcpy(packet.items[i].state,"1");strcpy(packet.items[3].id,AMIHASS_CAP_QUERY_MANY);strcpy(packet.items[4].id,AMIHASS_CAP_HISTORY);assert(nd_hass_features(&packet)==ND_HASS_FEATURE_CURRENT);
 puts("AmiHASS 0.3: unchanged wire sizes, Fahrenheit grid, capacity backoff/recovery, explicit door-policy refusal without action replay, authentication hints and additive capabilities passed.");return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'exec').mkdir();(p/'exec/ports.h').write_text('#include <stdint.h>\ntypedef uint32_t ULONG;typedef int32_t LONG;struct MsgPort;struct Message{char wire[20];};\n');(p/'check.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(p),'-I'+str(ROOT/'plugin_src/hass'),str(p/'check.c'),str(ROOT/'plugin_src/hass/nd_hass.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)
