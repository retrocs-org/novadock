#ifndef ND_HASS_FEATURES_H
#define ND_HASS_FEATURES_H
#include <stdio.h>
#include <string.h>
#include "nd_hass_native.h"
static void nd_hass_capabilities(const struct AHControls *c,char out[120])
{
 out[0]=0;
 if(c->brightness)strcat(out,"Brightness; ");if(c->color)strcat(out,"RGB; ");if(c->temperature)strcat(out,"Thermostat target; ");
 if(c->fan_speed)strcat(out,"Fan speed; ");if(c->position)strcat(out,"Cover position; ");if(c->cover_open)strcat(out,"Open; ");if(c->cover_close)strcat(out,"Close; ");if(c->cover_stop)strcat(out,"Stop; ");
 if(out[0])out[strlen(out)-2]=0;else strcpy(out,"No extra controls advertised");
}
static void nd_hass_display(const struct NDHassEntity *e,const char *mode,char out[96])
{
 if(!strcmp(mode,"Name"))snprintf(out,96,"%.63s",e->name[0] ? e->name:e->id);
 else if(!strcmp(mode,"Name and state"))snprintf(out,96,"%.35s: %.30s %.15s",e->name[0] ? e->name:e->id,e->state,e->unit);
 else snprintf(out,96,"%.39s %.15s",e->state,e->unit);
}
#endif
