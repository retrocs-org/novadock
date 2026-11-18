#ifndef ND_HASS_SOURCE_H
#define ND_HASS_SOURCE_H
#include "nd_hass_bridge.h"
#include "nd_hass_compat.h"
/* Independent read-only status client: never blocks a tile's control/readback. */
struct NDHassSource {struct NDHassClient client;unsigned int wait,features;int known,sample,connected,changed;char state[40],hint[96];unsigned long age;};
static void nd_hass_source_close(struct NDHassSource *s)
{nd_hass_client_close(&s->client);memset(s,0,sizeof(*s));}
static void nd_hass_source_poll(struct NDHassSource *s,const char *owner,unsigned int elapsed)
{
 struct NDHassMessage *m;int sample;s->changed=0;s->wait=nd_hass_remaining(s->wait,elapsed);
 if(s->client.pending){m=nd_hass_reply(&s->client);if(m){
  if(m->kind==AMIHASS_STATUS && m->result==AMIHASS_OK){sample=!strcmp(m->item.state,AMIHASS_SERVER_SAMPLE);s->changed=s->known && sample!=s->sample;s->known=1;s->features=nd_hass_features(m);s->sample=sample;s->connected=m->status.connected!=0;s->age=m->age;strncpy(s->state,m->item.state,39);s->state[39]=0;strncpy(s->hint,m->item.name,95);s->hint[95]=0;}
  else s->connected=0;FreeVec(m);s->wait=250U;
 }else if(!s->client.pending){s->connected=0;s->wait=250U;}}
 if(!s->client.pending && !s->wait){if(!nd_hass_send(&s->client,AMIHASS_STATUS,owner,NULL,NULL,0,0,NULL,NULL)){s->connected=0;s->wait=250U;}}
}
static const char *nd_hass_source_label(const struct NDHassSource *s)
{return s->known && s->features!=ND_HASS_FEATURE_CURRENT ? "Update AmiHASS service":s->known ? (s->sample ? "Sample data":s->connected ? "Home Assistant":s->state):"Checking bridge";}
#endif
