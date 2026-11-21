#ifndef ND_HASS_LEVEL_H
#define ND_HASS_LEVEL_H
#include <stdio.h>
#include "nd_hass_bridge.h"
static int nd_hass_percent_kind(const struct AHControls *c)
{return c->brightness ? 1:c->fan_speed ? 2:c->position ? 3:c->media_volume ? 4:0;}
static int nd_hass_percent_value(const struct AHControls *c)
{return c->brightness ? (c->level_known ? c->level:-1):c->fan_speed ? (c->speed_known ? c->speed:-1):c->position ? (c->position_known ? c->position_value:-1):c->media_volume ? (c->volume_known ? c->volume:-1):-1;}
struct NDHassLevelEdit {int level;unsigned int age;};
static void nd_hass_level_cancel(struct NDHassLevelEdit *edit)
{edit->level=-1;edit->age=0;}
static void nd_hass_level_offer(struct NDHassLevelEdit *edit,const struct NDHassBridge *bridge,int level)
{if(!bridge->read_only && !nd_hass_bridge_pending(bridge) && level>=0 && level<=100 && bridge->result==ND_HASS_OK && nd_hass_percent_kind(&bridge->controls) && nd_hass_available(&bridge->entity)){edit->level=level;edit->age=0;}}
static void nd_hass_level_poll(struct NDHassLevelEdit *edit,const struct NDHassBridge *bridge,unsigned int elapsed)
{if(edit->level<0)return;if(elapsed>=500U || edit->age>=500U-elapsed ||
 (bridge->result!=ND_HASS_OK && bridge->result!=ND_HASS_WAIT && bridge->result!=ND_HASS_BUSY) ||
 (bridge->result==ND_HASS_OK && (!nd_hass_percent_kind(&bridge->controls) || !nd_hass_available(&bridge->entity))))nd_hass_level_cancel(edit);else edit->age+=elapsed;}
static void nd_hass_level_send(struct NDHassLevelEdit *edit,struct NDHassBridge *bridge,const char *owner,const char *entity)
{char action[20];if(edit->level<0 || bridge->read_only || nd_hass_bridge_pending(bridge) || bridge->client.pending || bridge->result!=ND_HASS_OK)return;
 sprintf(action,"%s %d",bridge->controls.brightness ? "Brightness":bridge->controls.fan_speed ? "Fan speed":bridge->controls.position ? "Position":"Volume",edit->level);nd_hass_bridge_action(bridge,owner,entity,action);nd_hass_level_cancel(edit);}
#endif
