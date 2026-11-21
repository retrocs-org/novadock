#ifndef ND_HASS_FEATURES_H
#define ND_HASS_FEATURES_H
#include <stdio.h>
#include <string.h>
#include "nd_hass_native.h"
static void nd_hass_capability(char out[120],const char *text)
{size_t n=strlen(out),m=strlen(text);if(n+m<120)memcpy(out+n,text,m+1);}
static void nd_hass_capabilities(const struct AHControls *c,char out[120])
{
 out[0]=0;
 if(c->lock)nd_hass_capability(out,"Lock; ");if(c->unlock)nd_hass_capability(out,"Unlock; ");if(c->lock_open)nd_hass_capability(out,"Open latch; ");
 if(c->brightness)nd_hass_capability(out,"Brightness; ");if(c->color)nd_hass_capability(out,"RGB; ");if(c->temperature)nd_hass_capability(out,"Thermostat target; ");
 if(c->fan_speed)nd_hass_capability(out,"Fan speed; ");if(c->position)nd_hass_capability(out,"Cover position; ");if(c->cover_open)nd_hass_capability(out,"Open; ");if(c->cover_close)nd_hass_capability(out,"Close; ");if(c->cover_stop)nd_hass_capability(out,"Stop; ");
 if(c->media_play||c->media_pause||c->media_next||c->media_previous)nd_hass_capability(out,"Playback; ");if(c->media_volume)nd_hass_capability(out,"Volume; ");
 if(out[0])out[strlen(out)-2]=0;else strcpy(out,"No extra controls advertised");
}
static void nd_hass_display(const struct NDHassEntity *e,const char *mode,char out[96])
{
 if(!strcmp(mode,"Name"))snprintf(out,96,"%.63s",e->name[0] ? e->name:e->id);
 else if(!strcmp(mode,"Name and state"))snprintf(out,96,"%.35s: %.30s %.15s",e->name[0] ? e->name:e->id,amihass_display_state(e),e->unit);
 else snprintf(out,96,"%.39s %.15s",amihass_display_state(e),e->unit);
}
#endif
