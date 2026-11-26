/* Read-only room snapshots through AmiHASS's public Exec-port SDK only. */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_hass_native.h"
#include "nd_hass_compat.h"
#include "nd_hass_summary.h"
#include "nd_hass_room_native.h"
#include "nd_option_runtime.h"
#include "nd_list.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct RoomCache;
struct Room {struct NDHassClient client;struct NDHassAggregate *work;struct RoomCache *shared;unsigned long generation,published,revision,since_refresh,shared_age,requests,heartbeat;int shared_slot;unsigned int poll_interval;struct NDHassRoomData shown;char area[96],label[32],name[96],source[32],problem[96];unsigned long wait,age;unsigned int interval;int active,checked,total,offset,result,valid,dirty,fresh,sample;};
static void begin(struct Room *r)
{nd_hass_client_close(&r->client);if(r->work)nd_hass_summary_clear(r->work);++r->generation;r->checked=0;r->total=-1;r->offset=0;r->age=0;r->active=1;r->fresh=1;r->wait=0;r->result=ND_HASS_WAIT;r->dirty=1;r->problem[0]=0;}
static void failed(struct Room *r,int result,const char *why)
{++r->generation;nd_hass_client_close(&r->client);r->result=result;r->active=0;r->wait=1500;r->dirty=1;strncpy(r->problem,why,95);r->problem[95]=0;}
static void step(struct Room *r,const char *owner,unsigned int elapsed)
{
 struct NDHassMessage *m;int i,sent;
 if(!r->area[0]){failed(r,ND_HASS_UNCONFIGURED,"Select a room in HASS settings");return;}
 if(r->wait>elapsed)r->wait-=elapsed;else r->wait=0;
 if(!r->active){if(r->wait)return;begin(r);}
 if(r->age>1500){failed(r,ND_HASS_STALE,"Room snapshot timed out; previous counts are stale");return;}r->age+=elapsed;
 if(r->client.pending){m=nd_hass_reply(&r->client);if(m){
  if(!r->checked){if(m->kind==AMIHASS_STATUS && m->result==AMIHASS_OK){if((nd_hass_features(m)&3U)!=3U){FreeVec(m);failed(r,ND_HASS_UNSUPPORTED,"Update to AmiHASS 0.2+: rooms/filters required");return;}r->checked=1;m->item.state[39]=0;r->sample=!strcmp(m->item.state,AMIHASS_SERVER_SAMPLE);strcpy(r->source,r->sample ? "Sample data":"Home Assistant");}
   else if(m->result!=AMIHASS_WAIT&&m->result!=AMIHASS_BUSY){int result=m->result==AMIHASS_OK ? ND_HASS_FAILED:m->result;FreeVec(m);failed(r,result,"AmiHASS connection is unavailable");return;}}
  else if(m->result==AMIHASS_OK){
   if(m->kind!=AMIHASS_LIST||m->offset!=r->offset||m->total<0||m->total>ND_HASS_SUMMARY_MAX||m->count<0||m->count>AMIHASS_PAGE||m->offset+m->count>m->total||(!m->count&&r->offset<m->total)||(r->total>=0&&r->total!=m->total)){FreeVec(m);failed(r,ND_HASS_FAILED,"Room changed or exceeds 1024 entities; refresh/narrow scope");return;}
   r->total=m->total;for(i=0;i<m->count;++i)if(!nd_hass_summary_add(r->work,&m->items[i])){FreeVec(m);failed(r,ND_HASS_FAILED,"Duplicate or invalid room entity; refresh again");return;}
   r->offset+=m->count;if(r->offset==r->total){++r->generation;r->since_refresh=0;r->shown=r->work->data;r->valid=1;r->active=0;r->result=ND_HASS_OK;r->wait=(unsigned long)(r->poll_interval ? r->poll_interval:r->interval)*50;r->dirty=1;}
  }else if(m->result!=AMIHASS_WAIT&&m->result!=AMIHASS_BUSY){int result=m->result;FreeVec(m);failed(r,result,"Room snapshot unavailable; previous counts are stale");return;}
  if(m->result==AMIHASS_WAIT||m->result==AMIHASS_BUSY)r->wait=10;FreeVec(m);
 }else if(!r->client.pending){failed(r,ND_HASS_FAILED,"AmiHASS did not reply; previous counts are stale");return;}}
 if(!r->active||r->wait||r->client.pending)return;
 if(!r->checked)sent=nd_hass_send(&r->client,AMIHASS_STATUS,owner,NULL,NULL,0,0,NULL,NULL);
 else{sent=amihass_send_area(&r->client,AMIHASS_LIST,owner,"",r->offset,r->fresh,"","",r->area);if(sent){r->fresh=0;++r->requests;}}
 if(!sent)failed(r,9,"Install/start AmiHASS service");
}
/* Shared snapshots are private to NovaDock, anchored by a named Exec port.
 * Only the elected tile queries AmiHASS. Readers/writers never wait while
 * holding Forbid: reserve access atomically, then copy with scheduling enabled. */
#define ROOM_CACHE_MAGIC 0x4e445235UL
struct RoomCache {struct MsgPort port;unsigned long magic,revision,heartbeat;char port_name[64],area[96];struct Room *owner,*clients[48];unsigned int intervals[48],readers;int writing,refresh,valid,result,sample;struct NDHassRoomData shown;char source[32],problem[96];};
static unsigned long room_hash(const char *s)
{unsigned long h=2166136261UL;while(*s)h=((h^(unsigned char)*s++)*16777619UL)&0xffffffffUL;return h;}
static int shared_attach(struct Room *r)
{struct RoomCache *cache,*candidate;int i;candidate=AllocVec(sizeof(*candidate),MEMF_PUBLIC|MEMF_CLEAR);if(!candidate)return 0;sprintf(candidate->port_name,"NovaDock.RoomCache.v1.%08lx.%lu",room_hash(r->area),(unsigned long)strlen(r->area));candidate->port.mp_Node.ln_Name=candidate->port_name;candidate->port.mp_Node.ln_Type=NT_MSGPORT;candidate->port.mp_Node.ln_Pri=-99;candidate->port.mp_Flags=PA_IGNORE;NewList(&candidate->port.mp_MsgList);candidate->magic=ROOM_CACHE_MAGIC;strcpy(candidate->area,r->area);
 Forbid();cache=(struct RoomCache *)FindPort(candidate->port_name);if(cache&&(cache->port.mp_Node.ln_Pri!=-99||cache->magic!=ROOM_CACHE_MAGIC||strcmp(cache->area,r->area))){Permit();FreeVec(candidate);return 0;}if(!cache){cache=candidate;AddPort(&cache->port);candidate=NULL;}
 for(i=0;i<48&&cache->clients[i];++i){}if(i<48){cache->clients[i]=r;cache->intervals[i]=r->interval;r->shared=cache;r->shared_slot=i;}Permit();if(candidate)FreeVec(candidate);return i<48;
}
static void shared_close(struct Room *r)
{struct RoomCache *cache=r->shared;int i,used=0;nd_hass_client_close(&r->client);if(cache){Forbid();if(cache->owner==r)cache->owner=NULL;if(cache->clients[r->shared_slot]==r)cache->clients[r->shared_slot]=NULL;for(i=0;i<48;++i)if(cache->clients[i])++used;if(!used)RemPort(&cache->port);Permit();if(!used)FreeVec(cache);}r->shared=NULL;if(r->work)FreeVec(r->work);r->work=NULL;}
static void shared_refresh(struct Room *r)
{if(r->shared){Forbid();r->shared->refresh=1;Permit();}}
static void shared_step(struct Room *r,const char *owner,unsigned int elapsed)
{struct RoomCache *cache;unsigned int interval;int leader=0,i,copy=0,publish=0;
 if(!r->area[0]){if(r->result!=ND_HASS_UNCONFIGURED)failed(r,ND_HASS_UNCONFIGURED,"Select a room in HASS settings");return;}
 if(!r->shared&&!shared_attach(r)){failed(r,ND_HASS_FAILED,"Room cache unavailable; retry or restart tile");return;}cache=r->shared;
 Forbid();if(!cache->owner)cache->owner=r;leader=cache->owner==r;interval=r->interval;for(i=0;i<48;++i)if(cache->clients[i]&&cache->intervals[i]<interval)interval=cache->intervals[i];Permit();
 if(leader){Forbid();++cache->heartbeat;if(r->active)cache->refresh=0;Permit();if(!r->work){r->work=AllocVec(sizeof(*r->work),MEMF_PUBLIC|MEMF_CLEAR);if(!r->work){failed(r,ND_HASS_FAILED,"Not enough memory for room aggregation");}else{begin(r);r->wait=room_hash(r->area)%76;}}
  if(r->since_refresh<60000UL)r->since_refresh+=elapsed;
  if(r->work){if(r->wait>(unsigned long)interval*50)r->wait=(unsigned long)interval*50;Forbid();if(cache->refresh&&!r->active&&r->since_refresh>=250){cache->refresh=0;copy=1;}Permit();if(copy){begin(r);r->since_refresh=0;}r->poll_interval=interval;step(r,owner,elapsed);}
  if(r->published!=r->generation){Forbid();if(!cache->readers&&!cache->writing){cache->writing=1;publish=1;}Permit();if(publish){cache->shown=r->shown;cache->valid=r->valid;cache->result=r->result;cache->sample=r->sample;strcpy(cache->source,r->source);strcpy(cache->problem,r->problem);Forbid();++cache->revision;cache->writing=0;Permit();r->published=r->generation;r->revision=cache->revision;}}
 }else{Forbid();if(!cache->writing&&cache->revision!=r->revision){++cache->readers;copy=1;}else copy=0;Permit();if(copy){r->shown=cache->shown;r->valid=cache->valid;r->result=cache->result;r->sample=cache->sample;strcpy(r->source,cache->source);strcpy(r->problem,cache->problem);r->revision=cache->revision;Forbid();--cache->readers;Permit();r->dirty=1;r->shared_age=0;}else{unsigned long beat;Forbid();beat=cache->heartbeat;Permit();if(beat!=r->heartbeat){r->heartbeat=beat;r->shared_age=0;}else r->shared_age+=elapsed;if(r->shared_age>1500&&r->result!=ND_HASS_STALE){r->result=ND_HASS_STALE;r->dirty=1;strcpy(r->problem,"Room owner has not replied; restart its tile");}}}
}

static void rows(struct Room *r,struct NDList *list,int lights)
{char storage[ND_LIST_MAX][160],title[112];const char *ptr[ND_LIST_MAX];unsigned int i,n=0,count,limit;struct NDHassCounts *c=&r->shown.count;
 sprintf(title,"%.45s - %s%s",r->name[0] ? r->name:r->area,r->source[0] ? r->source:"Checking bridge",r->result==ND_HASS_OK ? "":" (not current)");nd_list_title(list,title);
 sprintf(storage[n++],"Light entities: %u on, %u off (%u total)",c->on,c->off,c->lights);sprintf(storage[n++],"Contacts/covers: %u open, %u closed (%u total)",c->open,c->closed,c->contacts);sprintf(storage[n++],"Unavailable/unknown: %u; room entities: %u",c->unavailable,c->entities);
 if(r->result!=ND_HASS_OK)snprintf(storage[n++],160,"%s",r->problem[0] ? r->problem:"Updating room snapshot...");else sprintf(storage[n++],"Read-only snapshot; refresh every %u seconds",r->interval);
 count=lights ? r->shown.light_rows:r->shown.open_rows;limit=ND_LIST_MAX-n;if(count>limit)count=limit;for(i=0;i<count;++i)snprintf(storage[n++],160,"%s",lights ? r->shown.lights[i]:r->shown.open[i]);for(i=0;i<n;++i)ptr[i]=storage[i];nd_list_rows(list,ptr,n);
}
static void settings(struct Room *r,const char *owner)
{struct NDOptionString f[4];char refresh[16];const char *keys[]={"LABEL","AREA","ENTITY_NAME","REFRESH"},*fallback[]={"HASS","","","Default"};char *out[4];unsigned int limits[]={32,96,96,16},i;out[0]=r->label;out[1]=r->area;out[2]=r->name;out[3]=refresh;for(i=0;i<4;++i){f[i].key=keys[i];f[i].fallback=fallback[i];f[i].out=out[i];f[i].limit=limits[i];}nd_plugin_option_strings(owner,f,4);i=(unsigned int)atoi(refresh);r->interval=i>=5&&i<=3600 ? i:60;}
int nd_hass_room_run(struct NDClient *dock,const char *owner)
{struct Room *r=AllocVec(sizeof(*r),MEMF_PUBLIC|MEMF_CLEAR);struct NDList *list=NULL;unsigned int event,elapsed=0;int action,lights=0,result=0;char value[96],badge[8];const char *buttons[]={"Refresh","Lights","Open contacts","Close"};if(!r)return 0;settings(r,owner);(void)nd_hass_start(NULL);r->result=ND_HASS_WAIT;
 for(;;){shared_step(r,owner,elapsed);if(r->result==ND_HASS_OK)sprintf(value,"%u lights on; %u open",r->shown.count.on,r->shown.count.open);else strcpy(value,r->problem[0] ? r->problem:"Updating room snapshot...");
  event=nd_client_update_art_quiet(dock,r->label,value,r->result!=ND_HASS_OK&&r->result!=ND_HASS_WAIT ? "SadHASS":"HASS",0,elapsed,500);badge[0]=0;if(r->result==ND_HASS_OK&&r->shown.count.open)sprintf(badge,"%u",r->shown.count.open);event|=nd_client_set_badge(dock,badge);if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS){result=1;break;}
  if(event&ND_EVENT_CLICK){if(list)nd_list_raise(list);else{list=nd_list_open("HASS room - read-only snapshot",buttons,4);r->dirty=1;}}
  action=nd_list_event(list);if(action==-1||action==103){nd_list_close(list);list=NULL;}else if(action==100){shared_refresh(r);}else if(action==101||action==102){lights=action==101;r->dirty=1;}
  if(list&&r->dirty){rows(r,list,lights);r->dirty=0;}
  elapsed=nd_client_wait_signals(dock,r->client.pending||r->active ? 5:250,nd_list_signals(list)|(r->client.port ? 1UL<<r->client.port->mp_SigBit:0));
 }shared_close(r);nd_list_close(list);FreeVec(r);return result;
}
int nd_hass_room_selftest(void)
{struct Room *r=AllocVec(sizeof(*r),MEMF_PUBLIC|MEMF_CLEAR);int i,ok=0;if(!r)return 10;strcpy(r->area,"study");r->interval=60;r->result=ND_HASS_WAIT;for(i=0;i<500&&r->result==ND_HASS_WAIT;++i){shared_step(r,"HASS_ROOM_QA",5);Delay(5);}if(r->result==ND_HASS_OK&&r->sample&&r->shown.count.lights>=1&&r->shown.count.entities>=1){const char *b[]={"Refresh","Lights","Open contacts","Close"};struct NDList *list=nd_list_open("HASS room sample check",b,4);if(list){rows(r,list,0);Delay(10);nd_list_close(list);ok=1;}}shared_close(r);FreeVec(r);return ok ? 0:10;}

int nd_hass_room_shared_selftest(void)
{struct Room *a=AllocVec(sizeof(*a),MEMF_PUBLIC|MEMF_CLEAR),*b=AllocVec(sizeof(*b),MEMF_PUBLIC|MEMF_CLEAR);int i,ok=0;if(!a||!b)goto done;strcpy(a->area,"study");strcpy(b->area,"study");a->interval=b->interval=60;a->result=b->result=ND_HASS_WAIT;
 for(i=0;i<600;++i){shared_step(a,"HASS_ROOM_QA",5);shared_step(b,"HASS_ROOM_QB",5);if(a->result==ND_HASS_OK&&b->result==ND_HASS_OK)break;Delay(5);}
 if(!a->sample||!b->sample||!a->shown.count.lights||a->shared!=b->shared||!a->work||b->work||b->client.pending||a->requests<1||b->requests||a->shown.count.entities!=b->shown.count.entities)goto done;
 puts("PASS: two room tiles share one paged aggregation and only one duplicate-ID workspace.");shared_close(a);a->result=0;
 for(i=0;i<600;++i){shared_step(b,"HASS_ROOM_QB",5);if(b->work&&b->result==ND_HASS_OK&&b->requests)break;Delay(5);}if(!b->sample||!b->work||b->result!=ND_HASS_OK||!b->requests)goto done;
 puts("PASS: closing the owner promotes the remaining room tile and refreshes through AmiHASS.");ok=1;
 done:if(a){shared_close(a);FreeVec(a);}if(b){shared_close(b);FreeVec(b);}return ok ? 0:10;}
