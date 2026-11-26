#ifndef ND_HASS_BRIDGE_H
#define ND_HASS_BRIDGE_H
#include <string.h>
#include "nd_hass_native.h"

/* Each tile leases only its selected entity. Credentials/networking stay in AmiHASS. */
struct NDHassBridge {
 struct NDHassClient client;
 struct NDHassEntity entity;
 struct AHControls controls;
 char target[96],owner[20],expected[20],previous_update[40],problem[96];
 unsigned int query_wait,start_wait,starting,interval,action_age;
 int result,legacy,read_only,action_stage,force;
};
/* action_stage: 0 idle, 1 sending, 2 awaiting readback, 3 confirmed, 4 failed. */
static int nd_hass_bridge_pending(const struct NDHassBridge *b)
{return b->action_stage==1 || b->action_stage==2;}
static const char *nd_hass_bridge_feedback(const struct NDHassBridge *b)
{return b->action_stage==1 ? "Sending":b->action_stage==2 ? "Waiting for confirmation":b->action_stage==3 ? "Confirmed":b->action_stage==4 ? (b->problem[0] ? b->problem:"Failed; no state confirmation"):"";}
static const char *nd_hass_bridge_problem(int result)
{
 switch(result){
 case 9:return "AmiHASS service is offline";
 case ND_HASS_FAILED:return "AmiHASS connection failed";
 case ND_HASS_STALE:return "AmiHASS data is stale";
 case ND_HASS_UNCONFIGURED:return "Configure AmiHASS";
 case ND_HASS_WAIT:return "Waiting for AmiHASS";
 case ND_HASS_UNAVAILABLE:return "Entity unavailable";
 case ND_HASS_BUSY:return "AmiHASS is busy";
 case ND_HASS_UNSUPPORTED:return "Action not supported";
 default:return "Connected";
 }
}
/* Service refusals are distinct from unsupported entity capabilities. Keep
 * them visible after the following state read; never replay the action. */
static const char *nd_hass_bridge_status(const struct NDHassBridge *b)
{return b->problem[0] ? b->problem:nd_hass_bridge_problem(b->result);}
static const char *nd_hass_reply_problem(const struct NDHassMessage *m)
{
 if(m->result==ND_HASS_BUSY&&!strcmp(m->item.state,AMIHASS_STATE_SELECTION_FULL))return "AmiHASS selection limit reached";
 if(m->result!=ND_HASS_UNSUPPORTED)return NULL;
 if(!strcmp(m->item.state,AMIHASS_STATE_CONFIRMATION))return "Enable Script doors in AmiHASS Preferences to allow this action";
 if(!strcmp(m->item.state,AMIHASS_STATE_CODE_REQUIRED))return "Use AmiHASS Dashboard for code-protected alarms";
 return NULL;
}
static unsigned int nd_hass_remaining(unsigned int wait,unsigned int elapsed)
{return wait>elapsed ? wait-elapsed:0;}
static void nd_hass_bridge_close(struct NDHassBridge *b)
{nd_hass_client_close(&b->client);memset(b,0,sizeof(*b));b->result=ND_HASS_UNCONFIGURED;}
static void nd_hass_bridge_options(struct NDHassBridge *b,unsigned int interval,int read_only)
{b->interval=interval>=5 && interval<=3600 ? interval:0;b->read_only=read_only!=0;}
static unsigned int nd_hass_bridge_delay(const struct NDHassBridge *b)
{return nd_hass_bridge_pending(b) ? 25U:b->interval ? (b->interval>60 ? 60:b->interval)*50U:250U;}
static void nd_hass_bridge_refresh(struct NDHassBridge *b)
{if(!nd_hass_bridge_pending(b)){b->force=1;b->query_wait=0;b->problem[0]=0;if(b->action_stage==4)b->action_stage=0;}}
static void nd_hass_bridge_fail(struct NDHassBridge *b)
{if(nd_hass_bridge_pending(b))b->action_stage=4;}
static int nd_hass_bridge_match(const struct NDHassBridge *b)
{
 if(!strcmp(b->expected,"Turn on"))return !strcmp(b->entity.state,"on");
 if(!strcmp(b->expected,"Turn off"))return !strcmp(b->entity.state,"off");
 if(!strcmp(b->expected,"Next")||!strcmp(b->expected,"Previous"))return b->entity.updated[0]&&strcmp(b->entity.updated,b->previous_update);
 if(!strcmp(b->expected,"Activate"))return b->entity.updated[0] && strcmp(b->entity.updated,b->previous_update);
 return amihass_control_matches(&b->controls,b->entity.state,b->expected);
}
static void nd_hass_bridge_poll(struct NDHassBridge *b,const char *owner,const char *entity,unsigned int elapsed)
{
 struct NDHassMessage *reply;const char *problem;int pending,reading,kind,refresh;unsigned int interval;int read_only;
 if(strcmp(b->target,entity ? entity:"") || strcmp(b->owner,owner ? owner:"")){
  interval=b->interval;read_only=b->read_only;nd_hass_bridge_close(b);nd_hass_bridge_options(b,interval,read_only);
  if(entity)strncpy(b->target,entity,95);if(owner)strncpy(b->owner,owner,19);
 }
 b->query_wait=nd_hass_remaining(b->query_wait,elapsed);b->start_wait=nd_hass_remaining(b->start_wait,elapsed);b->starting=nd_hass_remaining(b->starting,elapsed);
 if(nd_hass_bridge_pending(b)){if(elapsed>=450U || b->action_age>=450U-elapsed)b->action_stage=4;else b->action_age+=elapsed;}
 if(!nd_hass_entity_id(entity)){if(b->client.pending)nd_hass_client_close(&b->client);memset(&b->entity,0,sizeof(b->entity));b->result=ND_HASS_UNCONFIGURED;return;}
 pending=b->client.pending!=NULL;reply=nd_hass_reply(&b->client);
 if(reply){
  reading=reply->kind==ND_HASS_QUERY || reply->kind==AMIHASS_SUBSCRIBE;
  b->result=reply->result;problem=nd_hass_reply_problem(reply);if(problem){strncpy(b->problem,problem,sizeof(b->problem)-1);b->problem[sizeof(b->problem)-1]=0;}
  if(reply->kind==AMIHASS_SUBSCRIBE && reply->result==AMIHASS_UNSUPPORTED){b->legacy=1;b->query_wait=0;FreeVec(reply);reply=NULL;}
  else {
   if(reply->result<ND_HASS_OK || reply->result>ND_HASS_UNAVAILABLE || (!reading && reply->kind!=ND_HASS_ACTION)){
    memset(&b->entity,0,sizeof(b->entity));b->result=ND_HASS_FAILED;b->query_wait=250U;nd_hass_bridge_fail(b);FreeVec(reply);return;
   }
   if(reading){
    b->entity=reply->item;memset(&b->controls,0,sizeof(b->controls));
    b->entity.id[95]=b->entity.name[63]=b->entity.state[39]=b->entity.unit[15]=b->entity.device_class[23]=b->entity.updated[39]=0;
    if(reply->result==ND_HASS_OK && strcmp(b->entity.id,entity)){memset(&b->entity,0,sizeof(b->entity));b->result=ND_HASS_FAILED;}
    if(b->result==ND_HASS_OK && nd_hass_available(&b->entity)){
     if(b->action_stage!=4)b->problem[0]=0;
     amihass_controls_items(reply->items,(unsigned int)reply->count,&b->controls);
     if(b->action_stage==2 && nd_hass_bridge_match(b))b->action_stage=3;
    }else if(b->result!=ND_HASS_WAIT && b->result!=ND_HASS_BUSY)nd_hass_bridge_fail(b);
    b->query_wait=problem&&reply->result==ND_HASS_BUSY ? 1500U:reply->result==ND_HASS_WAIT ? 25U:nd_hass_bridge_delay(b);
   }else {
    if(reply->result==ND_HASS_WAIT || reply->result==ND_HASS_OK){if(b->action_stage==1)b->action_stage=2;}
    else nd_hass_bridge_fail(b);
    b->query_wait=problem&&reply->result==ND_HASS_BUSY ? 1500U:0; /* Follow acceptance with a read, never an optimistic state. */
   }
   b->starting=0;FreeVec(reply);
  }
 }else if(pending && !b->client.pending){memset(&b->entity,0,sizeof(b->entity));memset(&b->controls,0,sizeof(b->controls));b->result=ND_HASS_FAILED;b->query_wait=250U;nd_hass_bridge_fail(b);}
 if(b->query_wait || b->client.pending)return;
 kind=b->legacy || b->force ? ND_HASS_QUERY:AMIHASS_SUBSCRIBE;refresh=AMIHASS_QUERY_INTERVAL(b->interval);if(b->force)refresh|=AMIHASS_QUERY_ONCE|AMIHASS_QUERY_FRESH;
 if(nd_hass_send(&b->client,kind,owner,entity,NULL,0,refresh,NULL,NULL)){
  b->force=0;b->query_wait=nd_hass_bridge_delay(b);if(!b->entity.id[0])b->result=ND_HASS_WAIT;return;
 }
 memset(&b->entity,0,sizeof(b->entity));memset(&b->controls,0,sizeof(b->controls));nd_hass_bridge_fail(b);
 if(!b->start_wait){b->start_wait=1500U;b->starting=nd_hass_start(NULL) ? 500U:0;}
 b->result=b->starting ? ND_HASS_WAIT:9;b->query_wait=50U;
}
static void nd_hass_bridge_action(struct NDHassBridge *b,const char *owner,const char *entity,const char *action)
{
 if(b->read_only || nd_hass_bridge_pending(b))return;
 if(b->client.pending){b->result=ND_HASS_BUSY;return;}
 if(b->result!=ND_HASS_OK || strcmp(b->entity.id,entity) || !nd_hass_available(&b->entity))return;
 if(!amihass_control_allowed(&b->controls,action)){b->result=ND_HASS_UNSUPPORTED;return;}
 if(nd_hass_send(&b->client,ND_HASS_ACTION,owner,entity,action,0,0,NULL,NULL)){
  b->problem[0]=0;
  strncpy(b->expected,!strcmp(action,"Toggle") ? (!strcmp(b->entity.state,"on") ? "Turn off":"Turn on"):action,19);b->expected[19]=0;
  strncpy(b->previous_update,b->entity.updated,39);b->previous_update[39]=0;b->action_age=0;b->action_stage=1;b->result=ND_HASS_WAIT;b->query_wait=0;
 }else {memset(&b->entity,0,sizeof(b->entity));memset(&b->controls,0,sizeof(b->controls));b->result=9;b->action_stage=4;}
}
#endif
