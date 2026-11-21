#!/usr/bin/env python3
"""Exercise the dock adapter with the real AmiHASS ABI and a fake local bridge."""
from pathlib import Path
import hashlib,json,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
program=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "nd_hass_native.h"
#include "nd_hass_art.h"
static int online,start_ok,starts,queries,actions,timeout,closed;static const char *target="light.desk";static char action_sent[20];
static struct AmiHASSMessage packet,*incoming;
static void FreeVec(void *p){assert(p==&packet);}
void amihass_client_close(struct AmiHASSClient *c){++closed;c->pending=NULL;incoming=NULL;}
int amihass_start(const char *program){assert(!program);++starts;return start_ok;}
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,
 const char *action,int offset,int refresh,const char *domain,const char *filter){
 assert(owner&&!strcmp(owner,"HASS_2"));assert(!strcmp(entity,target));
 (void)offset;(void)refresh;(void)domain;(void)filter;
 if(!online)return 0;assert(!c->pending);
 if(kind==AMIHASS_ACTION){assert(action);strcpy(action_sent,action);++actions;}
 else {assert((kind==AMIHASS_QUERY||kind==AMIHASS_SUBSCRIBE)&&!action);++queries;}
 c->pending=&packet;return 1;
}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c){
 struct AmiHASSMessage *m=incoming;incoming=NULL;
 if(timeout){timeout=0;c->pending=NULL;return NULL;}
 if(m)c->pending=NULL;return m;
}
#include "nd_hass_bridge.h"
#include "nd_hass_level.h"
static void answer(int kind,int result,const char *entity){memset(&packet,0,sizeof(packet));
 packet.kind=kind;packet.result=result;strcpy(packet.item.id,entity);
 strcpy(packet.item.state,"on");strcpy(packet.item.unit,"K");incoming=&packet;}
int main(void){struct NDHassBridge b;
 assert(nd_hass_sad_status("Install/start AmiHASS service"));
 assert(nd_hass_sad_status("Stale; last reading is not current"));
 assert(nd_hass_sad_status(nd_hass_bridge_problem(ND_HASS_FAILED)));
 assert(strlen(nd_hass_bridge_problem(ND_HASS_FAILED))<32);
 assert(!nd_hass_sad_status("on K"));assert(!nd_hass_sad_status("SELECT ENTITY"));int i,n;memset(&b,0,sizeof(b));
 nd_hass_bridge_close(&b);nd_hass_bridge_poll(&b,"HASS_2","",0);
 assert(!queries&&!starts&&b.result==AMIHASS_UNCONFIGURED);
 nd_hass_bridge_poll(&b,"HASS_2","light.desk",0);assert(starts==1&&b.result==9);
 for(i=0;i<29;++i)nd_hass_bridge_poll(&b,"HASS_2","light.desk",50);
 assert(starts==1);start_ok=1;nd_hass_bridge_poll(&b,"HASS_2","light.desk",50);
 assert(starts==2&&b.result==AMIHASS_WAIT);
 online=1;nd_hass_bridge_poll(&b,"HASS_2","light.desk",50);assert(queries==1&&b.client.pending);
 answer(AMIHASS_QUERY,AMIHASS_OK,"light.desk");nd_hass_bridge_poll(&b,"HASS_2","light.desk",0);
 assert(b.result==AMIHASS_OK&&!strcmp(b.entity.state,"on"));
 /* Capability metadata is required; an older bridge cannot accidentally dim. */
 nd_hass_bridge_action(&b,"HASS_2",target,"Brightness 50");assert(!actions&&b.result==AMIHASS_UNSUPPORTED);
 nd_hass_bridge_poll(&b,"HASS_2",target,250);
 answer(AMIHASS_QUERY,AMIHASS_OK,target);packet.count=1;
 strcpy(packet.items[0].id,"control.brightness");strcpy(packet.items[0].state,"25");
 nd_hass_bridge_poll(&b,"HASS_2",target,0);assert(b.controls.brightness&&b.controls.level==25);
 nd_hass_bridge_action(&b,"HASS_2",target,"Brightness 50");assert(actions==1&&!strcmp(action_sent,"Brightness 50"));
 assert(b.controls.level==25); /* No optimistic percentage or state. */
 answer(AMIHASS_ACTION,AMIHASS_WAIT,target);nd_hass_bridge_poll(&b,"HASS_2",target,0);
 answer(AMIHASS_QUERY,AMIHASS_OK,target);packet.count=1;
 strcpy(packet.items[0].id,"control.brightness");strcpy(packet.items[0].state,"50");
 nd_hass_bridge_poll(&b,"HASS_2",target,0);assert(b.controls.level==50);
 {struct NDHassLevelEdit edit;int sent=actions;
  nd_hass_level_cancel(&edit);nd_hass_bridge_poll(&b,"HASS_2",target,250);assert(b.client.pending);
  nd_hass_level_offer(&edit,&b,37);nd_hass_level_poll(&edit,&b,10);nd_hass_level_send(&edit,&b,"HASS_2",target);
  assert(actions==sent&&edit.level==37&&b.controls.level==50);
  /* A later drag supersedes the queued draft without sending an old value. */
  nd_hass_level_offer(&edit,&b,42);answer(AMIHASS_QUERY,AMIHASS_OK,target);packet.count=1;
  strcpy(packet.items[0].id,"control.brightness");strcpy(packet.items[0].state,"50");nd_hass_bridge_poll(&b,"HASS_2",target,0);
  nd_hass_level_poll(&edit,&b,0);nd_hass_level_send(&edit,&b,"HASS_2",target);
  assert(actions==sent+1&&!strcmp(action_sent,"Brightness 42")&&edit.level<0&&b.controls.level==50);
  answer(AMIHASS_ACTION,AMIHASS_WAIT,target);nd_hass_bridge_poll(&b,"HASS_2",target,0);
  answer(AMIHASS_QUERY,AMIHASS_OK,target);packet.count=1;strcpy(packet.items[0].id,"control.brightness");strcpy(packet.items[0].state,"42");nd_hass_bridge_poll(&b,"HASS_2",target,0);assert(b.controls.level==42);
  nd_hass_level_offer(&edit,&b,0);nd_hass_level_cancel(&edit);nd_hass_level_send(&edit,&b,"HASS_2",target);assert(actions==sent+1);
  nd_hass_level_offer(&edit,&b,10);nd_hass_level_poll(&edit,&b,500);assert(edit.level<0);
  nd_hass_level_offer(&edit,&b,20);b.result=AMIHASS_FAILED;nd_hass_level_poll(&edit,&b,0);assert(edit.level<0);
  b.result=AMIHASS_OK;nd_hass_level_send(&edit,&b,"HASS_2",target);assert(actions==sent+1);
 }
 actions=0;queries=1;
 nd_hass_bridge_action(&b,"HASS_2","light.desk","Toggle");assert(actions==1&&b.result==AMIHASS_WAIT);
 answer(AMIHASS_ACTION,AMIHASS_WAIT,"light.desk");nd_hass_bridge_poll(&b,"HASS_2","light.desk",0);
 assert(queries==2&&b.client.pending); /* Server readback, no optimistic state. */
 answer(AMIHASS_QUERY,AMIHASS_STALE,"light.desk");nd_hass_bridge_poll(&b,"HASS_2","light.desk",0);
 nd_hass_bridge_action(&b,"HASS_2","light.desk","Toggle");assert(actions==1);
 nd_hass_bridge_poll(&b,"HASS_2","light.desk",250);
 answer(AMIHASS_QUERY,AMIHASS_OK,"light.wrong");nd_hass_bridge_poll(&b,"HASS_2","light.desk",0);
 assert(b.result==AMIHASS_FAILED&&!b.entity.id[0]);
 nd_hass_bridge_poll(&b,"HASS_2","light.desk",250);timeout=1;
 nd_hass_bridge_poll(&b,"HASS_2","light.desk",0);assert(b.result==AMIHASS_FAILED);
 online=0;nd_hass_bridge_poll(&b,"HASS_2","light.desk",500);assert(!b.entity.id[0]);
 n=actions;online=1;nd_hass_bridge_poll(&b,"HASS_2","light.desk",50);
 answer(AMIHASS_QUERY,AMIHASS_OK,"light.desk");nd_hass_bridge_poll(&b,"HASS_2","light.desk",0);
 assert(b.result==AMIHASS_OK&&actions==n); /* No action replay on reconnect. */
 /* External changes update the confirmed tile without a dock action. */
 n=actions;nd_hass_bridge_poll(&b,"HASS_2",target,250);
 answer(AMIHASS_QUERY,AMIHASS_OK,target);strcpy(packet.item.state,"off");
 nd_hass_bridge_poll(&b,"HASS_2",target,0);assert(!strcmp(b.entity.state,"off")&&actions==n);
 /* A selection change abandons an old pending request and its metadata. */
 nd_hass_bridge_poll(&b,"HASS_2",target,250);target="light.other";
 nd_hass_bridge_poll(&b,"HASS_2",target,0);assert(b.result==AMIHASS_WAIT&&!b.entity.id[0]&&!b.controls.brightness);
 answer(AMIHASS_QUERY,AMIHASS_OK,target);nd_hass_bridge_poll(&b,"HASS_2",target,0);
 for(i=0;i<30;++i){online=0;nd_hass_bridge_poll(&b,"HASS_2",target,500);
  assert(!b.entity.id[0]&&!b.controls.brightness);online=1;
  nd_hass_bridge_poll(&b,"HASS_2",target,50);answer(AMIHASS_QUERY,AMIHASS_OK,target);
  nd_hass_bridge_poll(&b,"HASS_2",target,0);assert(b.result==AMIHASS_OK&&actions==n);}
 nd_hass_bridge_poll(&b,"HASS_2",target,250);n=closed;
 nd_hass_bridge_close(&b);assert(!b.client.pending&&closed==n+1&&!b.entity.id[0]);
 return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'exec').mkdir();(p/'exec/ports.h').write_text('#include <stdint.h>\ntypedef uint32_t ULONG;typedef int32_t LONG;struct MsgPort;struct Message{unsigned short mn_Length;struct MsgPort *mn_ReplyPort;};\n')
 (p/'test.c').write_text(program)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(p),'-I'+str(ROOT/'plugin_src/hass'),'-I'+str(ROOT/'include'),str(p/'test.c'),str(ROOT/'plugin_src/hass/nd_hass.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],check=True)
print('HASS bridge ABI, deferred startup, rate limiting, query/action readback, timeout and reconnect passed.')

snapshot=json.loads((ROOT/'plugin_src/hass/service/sdk.json').read_text())
assert snapshot['version']=='0.2' and snapshot['abi']==1
for name,digest in snapshot['files'].items():
 if name=='amihass_client.c':
  data=(ROOT/'plugin_src/hass/nd_hass_client.c').read_text().replace('"nd_hass_native.h"','"amihass.h"').replace('"service/amihass_config.h"','"amihass_config.h"').encode()
 elif name=='core.c':data=(ROOT/'plugin_src/hass/nd_hass.c').read_text().replace('"nd_hass.h"','"amihass_core.h"').encode()
 elif name=='nd_reply.h':data=(ROOT/'include/nd_reply.h').read_bytes()
 else:data=(ROOT/'plugin_src/hass/service'/name).read_bytes()
 assert hashlib.sha256(data).hexdigest()==digest, 'AmiHASS SDK snapshot differs: '+name
print('Vendored AmiHASS SDK matches the recorded public bridge interface.')

import importlib.util
spec=importlib.util.spec_from_file_location('hass_art',ROOT/'tools/generate_hass_icons.py')
import sys
sys.path.insert(0,str(ROOT/'tools'))
art=importlib.util.module_from_spec(spec);spec.loader.exec_module(art)
pixels=art.artwork(True);decoded=[[0]*32 for _ in range(32)]
for x,y,w,h,color in art.rectangles(pixels):
 for yy in range(y,y+h):
  for xx in range(x,x+w):decoded[yy][xx]=color
assert decoded==pixels and pixels!=art.artwork(False)
subprocess.run([sys.executable,str(ROOT/'tools/generate_hass_icons.py'),'--check'],check=True)
print('Sad HASS status rules, recoverable artwork and runtime pixels match the generated icons.')
