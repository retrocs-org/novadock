#include <stdio.h>
#include <string.h>
#include "nd_hass_summary.h"
void nd_hass_summary_clear(struct NDHassAggregate *s)
{memset(&s->data,0,sizeof(s->data));memset(s->slots,0,sizeof(s->slots));}
int nd_hass_summary_add(struct NDHassAggregate *s,const struct NDHassEntity *e)
{
 unsigned int slot,n,j;unsigned long key=2166136261UL;const unsigned char *p;int light,contact,open=-1;char name[64];
 if(!s || !e || !memchr(e->id,0,sizeof(e->id)) || !memchr(e->state,0,sizeof(e->state)) || !memchr(e->device_class,0,sizeof(e->device_class)) || !memchr(e->name,0,sizeof(e->name)) || !nd_hass_entity_id(e->id) || s->data.count.entities>=ND_HASS_SUMMARY_MAX)return 0;
 for(p=(const unsigned char *)e->id;*p;++p)key=((key^*p)*16777619UL)&0xffffffffUL;
 slot=(unsigned int)((key^(key>>16))&(ND_HASS_SUMMARY_MAX*2-1));
 for(j=0;s->slots[slot];++j){if(j>=ND_HASS_SUMMARY_MAX*2)return 0;n=s->slots[slot]-1;if(s->keys[n]==key&&!strcmp(s->ids[n],e->id))return 0;slot=(slot+1)&(ND_HASS_SUMMARY_MAX*2-1);}
 n=s->data.count.entities++;strcpy(s->ids[n],e->id);s->keys[n]=key;s->slots[slot]=(unsigned short)(n+1);
 for(j=0;j<63 && e->name[j];++j)name[j]=(unsigned char)e->name[j]<32 ? ' ':e->name[j];name[j]=0;if(!name[0])strncpy(name,e->id,63);name[63]=0;
 light=!strncmp(e->id,"light.",6);contact=!strncmp(e->id,"cover.",6) || (!strncmp(e->id,"binary_sensor.",14) && (!strcmp(e->device_class,"door") || !strcmp(e->device_class,"window") || !strcmp(e->device_class,"opening") || !strcmp(e->device_class,"garage_door")));
 if(light){++s->data.count.lights;if(!strcmp(e->state,"on"))++s->data.count.on;else if(!strcmp(e->state,"off"))++s->data.count.off;else ++s->data.count.unavailable;
  if(s->data.light_rows<ND_HASS_SUMMARY_ROWS){char *row=s->data.lights[s->data.light_rows++];sprintf(row,"%.63s: %.39s",name,e->state);}}
 else if(contact){++s->data.count.contacts;if(!strcmp(e->state,"on")||!strcmp(e->state,"open")||!strcmp(e->state,"opening")||!strcmp(e->state,"closing"))open=1;else if(!strcmp(e->state,"off")||!strcmp(e->state,"closed"))open=0;
  if(open>0){++s->data.count.open;if(s->data.open_rows<ND_HASS_SUMMARY_ROWS)sprintf(s->data.open[s->data.open_rows++],"%.63s: %.39s",name,e->state);}
  else if(!open)++s->data.count.closed;else ++s->data.count.unavailable;
 }else if(!strcmp(e->state,"unavailable")||!strcmp(e->state,"unknown"))++s->data.count.unavailable;
 return 1;
}
