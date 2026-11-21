"""Exercise subscriptions, refresh, source status and confirmation with real adapter code."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
code=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_hass_native.h"
static struct AmiHASSMessage packet,*incoming;
static int sent_kind,sent_refresh,actions,closed,sends,online=1;
static char sent_entity[96];
static void FreeVec(void *p){assert(p==&packet);}
void amihass_client_close(struct AmiHASSClient *c){++closed;c->pending=NULL;incoming=NULL;}
int amihass_start(const char *unused){(void)unused;return 0;}
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,const char *action,int offset,int refresh,const char *domain,const char *filter){
 assert(!c->pending&&owner);(void)action;(void)offset;(void)domain;(void)filter;if(!online)return 0;
 sent_kind=kind;sent_refresh=refresh;strcpy(sent_entity,entity ? entity:"");++sends;if(kind==AMIHASS_ACTION)++actions;c->pending=&packet;return 1;
}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c){struct AmiHASSMessage *m=incoming;incoming=NULL;if(m)c->pending=NULL;return m;}
#include "nd_hass_bridge.h"
#include "nd_hass_source.h"
#include "nd_hass_features.h"
static void answer(int result,const char *state){memset(&packet,0,sizeof(packet));packet.kind=sent_kind;packet.result=result;strcpy(packet.item.id,sent_entity);if(sent_kind==AMIHASS_STATUS){packet.count=3;strcpy(packet.items[0].id,AMIHASS_CAP_AREAS);strcpy(packet.items[1].id,AMIHASS_CAP_FILTERS);strcpy(packet.items[2].id,AMIHASS_CAP_SUBSCRIPTIONS);strcpy(packet.items[0].state,"1");strcpy(packet.items[1].state,"1");strcpy(packet.items[2].state,"1");}if(state)strcpy(packet.item.state,state);incoming=&packet;}
int main(void){struct NDHassBridge b;struct NDHassSource source;struct AHControls cap;struct AmiHASSEntity e;char text[120];int n;
 memset(&b,0,sizeof(b));nd_hass_bridge_options(&b,3600,1);nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);
 assert(sent_kind==AMIHASS_SUBSCRIBE&&AMIHASS_QUERY_MAX_AGE(sent_refresh)==3600);
 answer(AMIHASS_OK,"off");nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);assert(b.result==AMIHASS_OK&&b.query_wait==3000);
 n=actions;nd_hass_bridge_action(&b,"HASS.1","light.desk","Turn on");assert(actions==n); /* Read-only is enforced below UI. */
 nd_hass_bridge_poll(&b,"HASS.1","light.desk",2999);assert(!b.client.pending);
 nd_hass_bridge_poll(&b,"HASS.1","light.desk",1);assert(b.client.pending&&sent_kind==AMIHASS_SUBSCRIBE); /* Renew before 120s expiry. */
 answer(AMIHASS_OK,"off");nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);
 nd_hass_bridge_refresh(&b);nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);
 assert(sent_kind==AMIHASS_QUERY&&(sent_refresh&AMIHASS_QUERY_FRESH)&&(sent_refresh&AMIHASS_QUERY_ONCE));
 answer(AMIHASS_WAIT,NULL);nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);nd_hass_bridge_poll(&b,"HASS.1","light.desk",25);assert(sent_kind==AMIHASS_SUBSCRIBE&&!(sent_refresh&AMIHASS_QUERY_FRESH));
 answer(AMIHASS_OK,"off");nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);
 nd_hass_bridge_options(&b,5,0);nd_hass_bridge_action(&b,"HASS.1","light.desk","Toggle");assert(actions==n+1&&b.action_stage==1&&!strcmp(b.expected,"Turn on")&&!strcmp(b.entity.state,"off"));
 nd_hass_bridge_action(&b,"HASS.1","light.desk","Toggle");assert(actions==n+1);
 answer(AMIHASS_WAIT,NULL);nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);assert(b.action_stage==2&&b.client.pending);
 answer(AMIHASS_OK,"off");nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);assert(nd_hass_bridge_pending(&b));
 nd_hass_bridge_poll(&b,"HASS.1","light.desk",25);answer(AMIHASS_OK,"on");nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);assert(b.action_stage==3&&!nd_hass_bridge_pending(&b));
 nd_hass_bridge_action(&b,"HASS.1","light.desk","Turn off");answer(AMIHASS_WAIT,NULL);nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);
 answer(AMIHASS_OK,"on");nd_hass_bridge_poll(&b,"HASS.1","light.desk",0);nd_hass_bridge_poll(&b,"HASS.1","light.desk",450);assert(b.action_stage==4&&strstr(nd_hass_bridge_feedback(&b),"Failed"));
 n=closed;nd_hass_bridge_poll(&b,"HASS.1","light.other",0);assert(closed==n+1&&!b.expected[0]);answer(AMIHASS_UNSUPPORTED,NULL);nd_hass_bridge_poll(&b,"HASS.1","light.other",0);assert(b.legacy&&sent_kind==AMIHASS_QUERY); /* Explicit fallback for older service. */
 answer(AMIHASS_OK,"on");nd_hass_bridge_poll(&b,"HASS.1","light.other",0);nd_hass_bridge_close(&b);assert(!b.client.pending);
 memset(&source,0,sizeof(source));nd_hass_source_poll(&source,"HASS.1",0);assert(sent_kind==AMIHASS_STATUS&&!sent_entity[0]);answer(AMIHASS_OK,"sample");packet.status.connected=1;nd_hass_source_poll(&source,"HASS.1",0);assert(source.known&&source.sample&&!source.changed&&!strcmp(nd_hass_source_label(&source),"Sample data"));
 nd_hass_source_poll(&source,"HASS.1",250);answer(AMIHASS_OK,"connected");packet.status.connected=1;nd_hass_source_poll(&source,"HASS.1",0);assert(source.changed&&!source.sample&&source.connected);
 nd_hass_source_poll(&source,"HASS.1",250);answer(AMIHASS_OK,"retry_wait");strcpy(packet.item.name,"Check DNS settings");nd_hass_source_poll(&source,"HASS.1",0);assert(!source.connected&&!strcmp(source.hint,"Check DNS settings"));nd_hass_source_poll(&source,"HASS.1",250);answer(AMIHASS_OK,"connected");packet.count=1;nd_hass_source_poll(&source,"HASS.1",0);assert(strstr(nd_hass_source_label(&source),"AmiHASS 0.2"));
 nd_hass_source_poll(&source,"HASS.1",250);answer(AMIHASS_OK,"sample");packet.count=4;strcpy(packet.items[3].id,"feature.future_extension");strcpy(packet.items[3].state,"1");nd_hass_source_poll(&source,"HASS.1",0);assert(!strcmp(nd_hass_source_label(&source),"Sample data"));
 online=0;nd_hass_source_poll(&source,"HASS.1",250);assert(!source.connected&&!source.available&&strstr(nd_hass_source_label(&source),"Start AmiHASS 0.2"));online=1;
 nd_hass_source_close(&source);
 memset(&cap,0,sizeof(cap));cap.fan_speed=cap.cover_open=cap.cover_close=cap.cover_stop=cap.position=1;nd_hass_capabilities(&cap,text);assert(strstr(text,"Fan speed")&&strstr(text,"Cover position")&&strlen(text)<120);
 memset(&e,0,sizeof(e));strcpy(e.name,"Desk");strcpy(e.id,"sensor.desk");strcpy(e.state,"21");strcpy(e.unit,"C");nd_hass_display(&e,"State",text);assert(!strcmp(text,"21 C"));nd_hass_display(&e,"Name and state",text);assert(!strcmp(text,"Desk: 21 C"));nd_hass_display(&e,"Name",text);assert(!strcmp(text,"Desk"));
 puts("Subscriptions, bounded renewals, read-only enforcement, forced refresh, confirmation/timeout, legacy fallback and source status passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'exec').mkdir();(p/'exec/ports.h').write_text('#include <stdint.h>\ntypedef uint32_t ULONG;typedef int32_t LONG;struct MsgPort;struct Message{unsigned short mn_Length;struct MsgPort *mn_ReplyPort;};\n');(p/'test.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(p),'-I'+str(ROOT/'plugin_src/hass'),str(p/'test.c'),str(ROOT/'plugin_src/hass/nd_hass.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],check=True)
