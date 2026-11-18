#ifndef ND_HASS_PICKER_H
#define ND_HASS_PICKER_H
#include "nd_hass_native.h"
#include <string.h>
#include "nd_hass_selection.h"
#include "nd_hass_compat.h"
#define ND_HASS_PICKER_MAX 64
static int nd_hass_picker_rows(int height,int font_height)
{int rows=(height-4)/(font_height>0 ? font_height:8);if(rows<1)rows=1;if(rows>ND_HASS_PICKER_MAX)rows=ND_HASS_PICKER_MAX;return rows;}
struct NDHassPicker {
 struct NDHassClient client;
 struct NDHassEntity items[ND_HASS_PICKER_MAX];
 char device[96],domain[24],filter[64],area[96];
 int devices,offset,count,total,selected,result,request,wait,dirty,capacity,page_refresh,checked;
};
static void nd_hass_picker_request(struct NDHassPicker *p,const char *filter,int offset,int refresh)
{
 char copy[64];strncpy(copy,filter ? filter:"",63);copy[63]=0;
 /* Abandon the old reply port before changing scope, page or filter. */
 nd_hass_client_close(&p->client);strcpy(p->filter,copy);
 p->offset=offset;p->count=p->total=0;p->selected=-1;
 p->checked=0;p->page_refresh=refresh;p->result=ND_HASS_WAIT;p->request=refresh ? 2:1;p->wait=0;p->dirty=1;
}
static int nd_hass_picker_init(struct NDHassPicker *p,int devices,const char *device,const char *domain)
{
 memset(p,0,sizeof(*p));p->devices=devices;p->capacity=ND_HASS_PAGE;
 if(!devices && device && *device && !nd_hass_global_type(domain) && !nd_hass_picker_device_id(device))return 0;
 if(!devices && device && !nd_hass_global_type(domain))strcpy(p->device,device);
 if(domain && strlen(domain)<sizeof(p->domain))strcpy(p->domain,domain);
 nd_hass_picker_request(p,"",0,0);return 1;
}
static int nd_hass_picker_scope(struct NDHassPicker *p,const char *area,const char *filter)
{if(!area)area="";if(!amihass_area_valid(area))return 0;strcpy(p->area,area);nd_hass_picker_request(p,filter,0,0);return 1;}
/* Called every five ticks; browsing never blocks the Preferences event loop. */
static void nd_hass_picker_step(struct NDHassPicker *p)
{
 struct NDHassMessage *m;int i,take,kind=p->devices==2 ? AMIHASS_AREAS:p->devices ? ND_HASS_DEVICES:ND_HASS_LIST;unsigned int required;
 if(p->client.pending){
  m=nd_hass_reply(&p->client);
  if(m && !p->checked){
   required=ND_HASS_FEATURE_FILTERS|(p->devices==2 || p->area[0] ? ND_HASS_FEATURE_AREAS:0);
   p->result=m->result;p->dirty=1;
   if(m->kind!=AMIHASS_STATUS)p->result=ND_HASS_FAILED;
   else if(m->result==ND_HASS_OK){if((nd_hass_features(m)&required)==required)p->checked=1;else p->result=ND_HASS_UNSUPPORTED;}
   FreeVec(m);m=NULL;
   if(p->checked){p->request=p->page_refresh ? 2:1;p->result=ND_HASS_WAIT;}
   else if(p->result==ND_HASS_WAIT || p->result==ND_HASS_BUSY){p->request=1;p->wait=10;}
   else{p->count=p->total=0;p->request=0;}
  }
  else if(m){
   p->selected=-1;p->result=m->result;p->dirty=1;m->entity[95]=0;
   if(m->kind!=kind || m->offset!=p->offset+p->count || (!p->devices && strcmp(m->entity,p->device)))p->result=ND_HASS_FAILED;
   if(p->result==ND_HASS_OK){
    if(m->count<0 || m->count>ND_HASS_PAGE || m->total<m->count || m->total>100000)p->result=ND_HASS_FAILED;
    else {
     for(i=0;i<m->count;++i){
      m->items[i].id[95]=0;m->items[i].name[63]=0;
      if(!(p->devices==2 ? (m->items[i].id[0] && amihass_area_valid(m->items[i].id)):p->devices ? nd_hass_picker_device_id(m->items[i].id):(nd_hass_entity_id(m->items[i].id) && nd_hass_type_matches(p->domain,m->items[i].id)))){p->result=ND_HASS_FAILED;break;}
     }
     if(p->result==ND_HASS_OK){
      p->total=m->total;take=m->count;if(take>p->capacity-p->count)take=p->capacity-p->count;
      if(take<0 || (!take && p->offset+p->count<p->total))p->result=ND_HASS_FAILED;
      else {for(i=0;i<take;++i)p->items[p->count+i]=m->items[i];p->count+=take;
       if(p->count<p->capacity && p->offset+p->count<p->total){p->request=p->page_refresh ? 2:1;p->result=ND_HASS_WAIT;}
      }
     }
    }
   }
   FreeVec(m);
   if(p->result==ND_HASS_WAIT || p->result==ND_HASS_BUSY){if(!p->request)p->request=1;p->wait=10;}
   else if(p->result!=ND_HASS_OK){p->count=p->total=0;}
  }else if(!p->client.pending){p->result=ND_HASS_FAILED;p->count=p->total=0;p->dirty=1;}
 }
 if(p->wait){--p->wait;return;}
 if(p->request && !p->client.pending){
  if(!p->checked ? nd_hass_send(&p->client,AMIHASS_STATUS,"NovaDock.Picker",NULL,NULL,0,0,NULL,NULL):p->devices==2 ? nd_hass_send(&p->client,AMIHASS_AREAS,"NovaDock.Picker",NULL,NULL,p->offset+p->count,p->request==2,NULL,NULL):amihass_send_area(&p->client,kind,"NovaDock.Picker",p->devices ? NULL:p->device,p->offset+p->count,p->request==2,p->domain,p->filter,p->area))p->request=0;
  else {p->result=9;p->dirty=1;p->wait=10;}
 }
}
#endif
