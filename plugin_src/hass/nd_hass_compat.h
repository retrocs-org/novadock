#ifndef ND_HASS_COMPAT_H
#define ND_HASS_COMPAT_H
#include "nd_hass_native.h"
#include <string.h>
#define ND_HASS_FEATURE_AREAS 1U
#define ND_HASS_FEATURE_FILTERS 2U
#define ND_HASS_FEATURE_SUBSCRIPTIONS 4U
#define ND_HASS_FEATURE_CURRENT 7U
static unsigned int nd_hass_features(const struct NDHassMessage *m)
{
 unsigned int flags=0;int i;if(m->kind!=AMIHASS_STATUS || m->result!=AMIHASS_OK || m->count<0 || m->count>AMIHASS_PAGE)return 0;
 for(i=0;i<m->count;++i){const struct NDHassEntity *e=&m->items[i];if(strncmp(e->state,"1",sizeof(e->state)))continue;
  if(!strncmp(e->id,AMIHASS_CAP_AREAS,sizeof(e->id)))flags|=ND_HASS_FEATURE_AREAS;
  else if(!strncmp(e->id,AMIHASS_CAP_FILTERS,sizeof(e->id)))flags|=ND_HASS_FEATURE_FILTERS;
  else if(!strncmp(e->id,AMIHASS_CAP_SUBSCRIPTIONS,sizeof(e->id)))flags|=ND_HASS_FEATURE_SUBSCRIPTIONS;
 }return flags;
}
#endif
