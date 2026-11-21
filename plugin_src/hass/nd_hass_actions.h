#ifndef ND_HASS_ACTIONS_H
#define ND_HASS_ACTIONS_H
#include "nd_hass.h"
#include <string.h>
/* Menus reflect confirmed bridge metadata, never inferred lock/door capabilities. */
static void nd_hass_action_choices(const char *entity,const struct AHControls *c,int known,char out[128])
{char path[96],body[180];strcpy(out,"Show controls");if(!entity)return;
 if(!strncmp(entity,"lock.",5)){if(known&&c){if(c->lock)strcat(out,",Lock");if(c->unlock)strcat(out,",Unlock");if(c->lock_open)strcat(out,",Open lock");}return;}
 if(!strncmp(entity,"cover.",6)){if(known&&c){if(c->cover_open)strcat(out,",Open cover");if(c->cover_close)strcat(out,",Close cover");if(c->cover_stop)strcat(out,",Stop cover");}return;}
 if(!strncmp(entity,"media_player.",13)){if(known&&c){if(c->media_play)strcat(out,",Play");if(c->media_pause)strcat(out,",Pause");if(c->media_previous)strcat(out,",Previous");if(c->media_next)strcat(out,",Next");}return;}
 if(nd_hass_action(entity,"Turn off",path,body))strcpy(out,"Toggle,Turn on,Turn off,Show controls");else if(nd_hass_action(entity,"Activate",path,body))strcpy(out,"Activate,Show controls");}
static int nd_hass_control_buttons(const char *entity,const struct AHControls *c,int read_only,const char *buttons[5])
{int n=0;char path[96],body[180];if(read_only){buttons[n++]="Refresh";buttons[n++]="Close";return n;}
 if(!strncmp(entity,"lock.",5)){if(c->lock)buttons[n++]="Lock";if(c->unlock)buttons[n++]="Unlock";if(c->lock_open)buttons[n++]="Open lock";}
 else if(!strncmp(entity,"cover.",6)){if(c->cover_open)buttons[n++]="Open cover";if(c->cover_close)buttons[n++]="Close cover";if(c->cover_stop)buttons[n++]="Stop cover";}
 else if(!strncmp(entity,"media_player.",13)){if(c->media_previous)buttons[n++]="Previous";if(c->media_play)buttons[n++]="Play";if(c->media_pause)buttons[n++]="Pause";if(c->media_next)buttons[n++]="Next";}
 else if(nd_hass_action(entity,"Turn off",path,body)){buttons[n++]="Turn on";buttons[n++]="Turn off";buttons[n++]="Toggle";}
 else if(nd_hass_action(entity,"Activate",path,body)){buttons[n++]="Activate";buttons[n++]="Refresh";}
 if(!n)buttons[n++]="Refresh";buttons[n++]="Close";return n;}
static int nd_hass_action_sensitive(const struct NDHassEntity *e,const struct AHControls *c,const char *action)
{long value;if(!e||!action)return 0;if(!strncmp(e->id,"lock.",5))return !strcmp(action,"Unlock")||!strcmp(action,"Open lock");
 if(strncmp(e->id,"cover.",6))return 0;if(!strcmp(action,"Open cover"))return 1;
 return !strncmp(action,"Position ",9)&&amihass_number(action+9,&value)&&value>0&&(!c||!c->position_known||value>(long)c->position_value*100L);}
/* A confirmation belongs to one entity/action and expires after 30 seconds.
 * The UI owns this state; SDK requests are sent only after accept succeeds. */
struct NDHassApproval {char entity[96],action[20];unsigned int age;};
static void nd_hass_approval_cancel(struct NDHassApproval *p){memset(p,0,sizeof(*p));}
static int nd_hass_approval_begin(struct NDHassApproval *p,const struct NDHassEntity *e,const struct AHControls *c,const char *action)
{if(!e||!action||strlen(e->id)>=sizeof(p->entity)||strlen(action)>=sizeof(p->action)||!nd_hass_available(e)||!amihass_control_allowed(c,action))return 0;nd_hass_approval_cancel(p);strcpy(p->entity,e->id);strcpy(p->action,action);return 1;}
static int nd_hass_approval_tick(struct NDHassApproval *p,const struct NDHassEntity *e,const struct AHControls *c,unsigned int elapsed,int ready,int read_only)
{if(!p->entity[0])return 0;if(!ready||read_only||strcmp(p->entity,e->id)||!nd_hass_available(e)||!amihass_control_allowed(c,p->action)||elapsed>=1500||p->age>=1500-elapsed){nd_hass_approval_cancel(p);return 0;}p->age+=elapsed;return 1;}
static int nd_hass_approval_accept(struct NDHassApproval *p,const struct NDHassEntity *e,const struct AHControls *c,int ready,int read_only,char action[20])
{if(!nd_hass_approval_tick(p,e,c,0,ready,read_only))return 0;strcpy(action,p->action);nd_hass_approval_cancel(p);return 1;}
#endif
