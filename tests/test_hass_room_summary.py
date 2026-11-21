"""Exercise production room paging/failure state machine with structured SDK replies."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'plugin_src/hass/nd_hass_room.c').read_text()
fragment=source[source.index('struct Room {'):source.index('/* Shared snapshots')]
code=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_hass_native.h"
#include "nd_hass_compat.h"
#include "nd_hass_summary.h"
static struct AmiHASSMessage packet,*incoming;
static int sent_kind,sent_refresh,sent_offset,online=1,actions;
static char sent_area[96];
static void FreeVec(void *p){assert(p==&packet);}
void amihass_client_close(struct AmiHASSClient *c){c->pending=NULL;incoming=NULL;}
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,const char *action,int offset,int refresh,const char *domain,const char *filter)
{assert(!c->pending&&owner);(void)entity;(void)action;(void)domain;(void)filter;if(!online)return 0;sent_kind=kind;sent_offset=offset;sent_refresh=refresh;if(kind==AMIHASS_ACTION)++actions;c->pending=&packet;return 1;}
int amihass_send_area(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,int offset,int refresh,const char *domain,const char *filter,const char *area)
{strcpy(sent_area,area);return amihass_send(c,kind,owner,entity,NULL,offset,refresh,domain,filter);}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c){struct AmiHASSMessage *m=incoming;if(m){incoming=NULL;c->pending=NULL;}return m;}
'''+fragment+r'''
static void answer(int result,int count,int total)
{memset(&packet,0,sizeof(packet));packet.kind=sent_kind;packet.result=result;packet.offset=sent_offset;packet.count=count;packet.total=total;incoming=&packet;}
static void supported(struct Room *r)
{begin(r);step(r,"QA",0);assert(sent_kind==AMIHASS_STATUS);answer(AMIHASS_OK,2,0);strcpy(packet.items[0].id,AMIHASS_CAP_AREAS);strcpy(packet.items[1].id,AMIHASS_CAP_FILTERS);strcpy(packet.items[0].state,"1");strcpy(packet.items[1].state,"1");strcpy(packet.item.state,AMIHASS_SERVER_SAMPLE);step(r,"QA",0);assert(sent_kind==AMIHASS_LIST&&sent_offset==0&&sent_refresh==1&&!strcmp(sent_area,r->area));}
int main(void)
{static struct Room r;static struct NDHassAggregate work;int n;r.work=&work;strcpy(r.area,"study");r.interval=60;supported(&r);
 answer(AMIHASS_WAIT,0,0);step(&r,"QA",0);step(&r,"QA",10);assert(sent_refresh==0&&sent_offset==0);
 answer(AMIHASS_OK,16,17);for(n=0;n<16;++n){sprintf(packet.items[n].id,"light.item%d",n);strcpy(packet.items[n].state,"on");}step(&r,"QA",0);assert(!r.valid&&sent_offset==16);
 answer(AMIHASS_OK,1,17);strcpy(packet.items[0].id,"binary_sensor.window");strcpy(packet.items[0].device_class,"window");strcpy(packet.items[0].state,"on");step(&r,"QA",0);assert(r.valid&&r.result==AMIHASS_OK&&r.shown.count.on==16&&r.shown.count.open==1&&r.sample);
 supported(&r);answer(AMIHASS_OK,1,2);strcpy(packet.items[0].id,"light.a");strcpy(packet.items[0].state,"off");step(&r,"QA",0);answer(AMIHASS_OK,1,3);strcpy(packet.items[0].id,"light.b");step(&r,"QA",0);assert(r.result==ND_HASS_FAILED&&r.shown.count.on==16); /* retain explicitly stale old snapshot */
 supported(&r);answer(AMIHASS_OK,2,2);strcpy(packet.items[0].id,"light.a");strcpy(packet.items[0].state,"on");packet.items[1]=packet.items[0];step(&r,"QA",0);assert(r.result==ND_HASS_FAILED);
 supported(&r);answer(AMIHASS_OK,0,1);step(&r,"QA",0);assert(r.result==ND_HASS_FAILED);
 supported(&r);answer(AMIHASS_OK,0,1025);step(&r,"QA",0);assert(r.result==ND_HASS_FAILED);
 supported(&r);r.age=1501;step(&r,"QA",0);assert(r.result==ND_HASS_STALE&&!r.client.pending);
 begin(&r);step(&r,"QA",0);answer(AMIHASS_OK,0,0);step(&r,"QA",0);assert(r.result==ND_HASS_UNSUPPORTED);
 online=0;begin(&r);step(&r,"QA",0);assert(!r.active);online=1;
 supported(&r);strcpy(r.area,"kitchen");begin(&r);assert(!r.client.pending&&r.work->data.count.entities==0);supported(&r);answer(AMIHASS_OK,0,0);step(&r,"QA",0);assert(r.result==ND_HASS_OK&&r.shown.count.entities==0&&!actions);
 puts("Room snapshots: paging, fresh-once, read-only enforcement, duplicates, scope changes, stale counts, limits and timeouts passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'exec').mkdir();(p/'exec/ports.h').write_text('#include <stdint.h>\ntypedef uint32_t ULONG;typedef int32_t LONG;struct MsgPort;struct Message{unsigned short mn_Length;struct MsgPort *mn_ReplyPort;};\n');(p/'check.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(p),'-I'+str(ROOT/'plugin_src/hass'),str(p/'check.c'),str(ROOT/'plugin_src/hass/nd_hass.c'),str(ROOT/'plugin_src/hass/nd_hass_summary.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)
