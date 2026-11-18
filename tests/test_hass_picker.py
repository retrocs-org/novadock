#!/usr/bin/env python3
"""Exercise real device-scoped picker state, cancellation and light action policy."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
program=r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nd_hass_native.h"
static struct AmiHASSMessage packet,*incoming;
static int sends,closes,kind_sent,refresh_sent,offset_sent,online=1;
static char scope[96],domain_sent[24],filter_sent[64],area_sent[96];
static void FreeVec(void *p){assert(p==&packet);}
void amihass_client_close(struct AmiHASSClient *c){++closes;c->pending=NULL;incoming=NULL;}
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,const char *action,int offset,int refresh,const char *domain,const char *filter){
 assert(owner&&!strcmp(owner,"NovaDock.Picker")&&!action&&!c->pending);if(!online)return 0;
 ++sends;kind_sent=kind;offset_sent=offset;refresh_sent=refresh;
 strcpy(scope,entity ? entity:"");strcpy(domain_sent,domain ? domain:"");strcpy(filter_sent,filter ? filter:"");c->pending=&packet;if(kind==AMIHASS_STATUS){memset(&packet,0,sizeof(packet));packet.kind=kind;packet.result=AMIHASS_OK;packet.count=3;strcpy(packet.items[0].id,AMIHASS_CAP_AREAS);strcpy(packet.items[1].id,AMIHASS_CAP_FILTERS);strcpy(packet.items[2].id,AMIHASS_CAP_SUBSCRIPTIONS);strcpy(packet.items[0].state,"1");strcpy(packet.items[1].state,"1");strcpy(packet.items[2].state,"1");incoming=&packet;}return 1;
}
int amihass_send_area(struct AmiHASSClient *c,int kind,const char *owner,const char *device,int offset,int refresh,const char *domain,const char *filter,const char *area){strcpy(area_sent,area ? area:"");return amihass_send(c,kind,owner,device,NULL,offset,refresh,domain,filter);}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c){struct AmiHASSMessage *m=incoming;incoming=NULL;if(m)c->pending=NULL;return m;}
#define nd_hass_picker_step nd_hass_picker_step_raw
#include "nd_hass_picker.h"
#undef nd_hass_picker_step
static void nd_hass_picker_step(struct NDHassPicker *p){nd_hass_picker_step_raw(p);if(p->client.pending && incoming && incoming->kind==AMIHASS_STATUS)nd_hass_picker_step_raw(p);}
static void answer(int result,int count,int total,const char *id){
 memset(&packet,0,sizeof(packet));packet.kind=kind_sent;packet.offset=offset_sent;strcpy(packet.entity,scope);
 packet.result=result;packet.count=count;packet.total=total;if(id){strcpy(packet.items[0].id,id);strcpy(packet.items[0].name,"Desk lamp");}incoming=&packet;
}
static void batch(int total){int i,n=total-offset_sent;if(n>16)n=16;answer(AMIHASS_OK,n,total,NULL);for(i=0;i<n;++i){sprintf(packet.items[i].id,"light.row_%d",offset_sent+i);sprintf(packet.items[i].name,"Light %d",offset_sent+i);}}
static void tick_request(struct NDHassPicker *p){int i;for(i=0;i<12&&!p->client.pending;++i)nd_hass_picker_step(p);assert(p->client.pending);}
int main(void){struct NDHassPicker p;int i,n;char name[64];
 const char *device="0123456789abcdef0123456789abcdef";
 nd_hass_device_name(name,"Desk | lamp\n");assert(!strcmp(name,"Desk   lamp "));
 assert(nd_hass_global_type("scene") && nd_hass_global_type("script") && nd_hass_global_type("group") && !nd_hass_global_type("light"));
 assert(!strcmp(nd_hass_type_choices("scene"),"Activate,Show controls"));
 assert(nd_hass_picker_init(&p,0,"","scene"));nd_hass_picker_step(&p);assert(kind_sent==AMIHASS_LIST&&!scope[0]&&!strcmp(domain_sent,"scene"));
 answer(AMIHASS_OK,1,1,"scene.evening");nd_hass_picker_step(&p);assert(p.result==AMIHASS_OK&&p.count==1);
 nd_hass_picker_request(&p,"",0,0);nd_hass_picker_step(&p);answer(AMIHASS_OK,1,1,"light.wrong");nd_hass_picker_step(&p);assert(p.result==AMIHASS_FAILED&&!p.count);
 nd_hass_client_close(&p.client);sends=0;
 assert(nd_hass_picker_device_id(device));assert(!nd_hass_picker_device_id("bad"));
 assert(nd_hass_picker_init(&p,0,"","light"));assert(!sends);
 assert(nd_hass_picker_scope(&p,"Study","desk"));nd_hass_picker_step(&p);assert(!strcmp(area_sent,"Study")&&!strcmp(domain_sent,"light")&&!strcmp(filter_sent,"desk")&&!scope[0]);
 n=closes;assert(nd_hass_picker_scope(&p,"Kitchen",""));assert(closes==n+1&&!p.count&&!p.client.pending);nd_hass_picker_step(&p);assert(!strcmp(area_sent,"Kitchen"));
 assert(!nd_hass_picker_scope(&p,"bad\nroom",""));assert(!strcmp(p.area,"Kitchen"));nd_hass_client_close(&p.client);
 assert(nd_hass_picker_init(&p,1,NULL,"light"));nd_hass_picker_step(&p);
 assert(kind_sent==AMIHASS_DEVICES&&!scope[0]&&!strcmp(domain_sent,"light"));answer(AMIHASS_OK,1,18,device);nd_hass_picker_step(&p);
 assert(p.count==1&&p.total==18&&!strcmp(p.items[0].name,"Desk lamp"));
 nd_hass_picker_request(&p,"Kitchen",16,1);nd_hass_picker_step(&p);
 assert(offset_sent==16&&refresh_sent&&!strcmp(filter_sent,"Kitchen")&&!strcmp(domain_sent,"light"));
 /* Changing a pending filter abandons the old reply before selecting. */
 n=closes;nd_hass_picker_request(&p,"Desk",0,0);assert(closes==n+1&&p.selected==-1&&!p.count&&!p.client.pending);
 nd_hass_picker_step(&p);assert(!strcmp(filter_sent,"Desk"));answer(AMIHASS_OK,1,1,device);nd_hass_picker_step(&p);
 nd_hass_client_close(&p.client);
 assert(nd_hass_picker_init(&p,0,device,"light"));nd_hass_picker_step(&p);
 assert(kind_sent==AMIHASS_LIST&&!strcmp(scope,device)&&!strcmp(domain_sent,"light"));
 answer(AMIHASS_WAIT,0,0,NULL);nd_hass_picker_step(&p);n=sends;for(i=0;i<9;++i)nd_hass_picker_step(&p);assert(sends==n);
 nd_hass_picker_step(&p);assert(sends==n+1);answer(AMIHASS_OK,1,1,"light.desk");nd_hass_picker_step(&p);assert(p.count==1&&p.result==AMIHASS_OK);
 /* Foreign device replies cannot populate the selected device's list. */
 nd_hass_picker_request(&p,"",0,0);nd_hass_picker_step(&p);answer(AMIHASS_OK,1,1,"light.other");packet.entity[0]=0;nd_hass_picker_step(&p);assert(!p.count&&p.result==AMIHASS_FAILED);
 nd_hass_picker_request(&p,"",0,0);nd_hass_picker_step(&p);answer(AMIHASS_OK,17,17,NULL);nd_hass_picker_step(&p);assert(!p.count&&p.result==AMIHASS_FAILED);
 /* Old services fail clearly; device browsing never falls back to all entities. */
 nd_hass_picker_request(&p,"",0,0);nd_hass_picker_step(&p);answer(AMIHASS_UNSUPPORTED,0,0,NULL);nd_hass_picker_step(&p);assert(p.result==AMIHASS_UNSUPPORTED&&!p.count&&!p.request);
 online=0;nd_hass_picker_request(&p,"",0,0);nd_hass_picker_step(&p);assert(p.result==9&&!p.count);
 assert(!strcmp(nd_hass_click_choices("light.desk"),"Toggle,Turn on,Turn off,Show controls"));
 assert(!strcmp(nd_hass_click_choices("scene.evening"),"Activate,Show controls"));
 assert(!strcmp(nd_hass_click_choices("sensor.temperature"),"Show controls"));
 assert(nd_hass_picker_rows(228,8)==28);assert(nd_hass_picker_rows(4,8)==1);assert(nd_hass_picker_rows(1000,8)==64);
 online=1;nd_hass_client_close(&p.client);assert(nd_hass_picker_init(&p,0,device,"light"));p.capacity=28;
 nd_hass_picker_request(&p,"",0,1);tick_request(&p);assert(refresh_sent);batch(61);nd_hass_picker_step(&p);
 assert(p.count==16&&p.result==AMIHASS_WAIT);tick_request(&p);assert(offset_sent==16&&refresh_sent);
 batch(61);nd_hass_picker_step(&p);assert(p.count==28&&p.result==AMIHASS_OK&&!strcmp(p.items[27].id,"light.row_27"));
 nd_hass_picker_request(&p,"",28,0);tick_request(&p);assert(offset_sent==28);batch(61);nd_hass_picker_step(&p);
 tick_request(&p);assert(offset_sent==44);batch(61);nd_hass_picker_step(&p);assert(p.count==28&&!strcmp(p.items[0].id,"light.row_28"));
 nd_hass_picker_request(&p,"",56,0);tick_request(&p);batch(61);nd_hass_picker_step(&p);assert(p.count==5&&p.result==AMIHASS_OK&&!strcmp(p.items[4].id,"light.row_60"));
 nd_hass_picker_request(&p,"",28,0);tick_request(&p);batch(61);nd_hass_picker_step(&p);assert(p.count==16);
 nd_hass_picker_request(&p,"Bedroom",0,0);assert(!p.count&&!p.client.pending&&p.selected==-1);tick_request(&p);assert(offset_sent==0&&!strcmp(filter_sent,"Bedroom"));
 nd_hass_client_close(&p.client);return 0;}
'''
# Calling Select and Devices... must retain the type; scenes bypass devices.
wrappers=(ROOT/'plugin_src/hass/nd_hass_ui.c').read_text()
wrappers=wrappers[wrappers.index('int nd_hass_choose_device('):wrappers.index('int nd_hass_picker_selftest(')]
program=program.replace('int main(void)', r'''
static const char *expected_choose_domain;
static int choose_devices,choose_entities,return_to_devices;
static int choose(int devices,const char *device,const char *domain,char area[96],struct NDHassEntity *chosen){
 assert(domain && !strcmp(domain,expected_choose_domain));assert(amihass_area_valid(area));
 memset(chosen,0,sizeof(*chosen));
 if(devices){assert(!device);++choose_devices;strcpy(chosen->id,"0123456789abcdef0123456789abcdef");strcpy(chosen->name,"Desk");}
 else {++choose_entities;if(nd_hass_global_type(domain))assert(device && !*device);else assert(device && nd_hass_picker_device_id(device));
  if(return_to_devices){return_to_devices=0;return 2;}
  sprintf(chosen->id,"%s.sample",!strcmp(domain,"All") ? "light":domain);}
 return 1;
}
'''+wrappers+'\nint main(void)')
program=program.replace('nd_hass_client_close(&p.client);return 0;', r'''
 {const char *domains[]={"light","switch","All","scene","script","group"};char selected_device[96],selected_name[64];struct NDHassEntity selected;int d;
 for(d=0;d<6;++d){expected_choose_domain=domains[d];choose_devices=choose_entities=0;selected_device[0]=selected_name[0]=0;return_to_devices=d==0;
  assert(nd_hass_choose_target(domains[d],selected_device,selected_name,&selected));
  assert(choose_devices==(nd_hass_global_type(domains[d]) ? 0:d==0 ? 2:1));
  assert(choose_entities==(d==0 ? 2:1));assert(nd_hass_type_matches(domains[d],selected.id));
 }}
 nd_hass_client_close(&p.client);return 0;''')
with tempfile.TemporaryDirectory() as t:
    p=Path(t);(p/'exec').mkdir()
    (p/'exec/ports.h').write_text('#include <stdint.h>\ntypedef uint32_t ULONG;typedef int32_t LONG;struct MsgPort;struct Message{unsigned short mn_Length;struct MsgPort *mn_ReplyPort;};\n')
    (p/'test.c').write_text(program)
    command=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(p),'-I'+str(ROOT/'plugin_src/hass'),str(p/'test.c'),str(ROOT/'plugin_src/hass/nd_hass.c'),'-o',str(p/'test')]
    if os.environ.get('SANITIZE')=='1':command+=['-fsanitize=address,undefined']
    subprocess.run(command,check=True);subprocess.run([str(p/'test')],check=True)
print('HASS device discovery, scoped entities, filters/paging, cancellation, old-service handling and light actions passed.')
