/* HASS: independent tiles backed by one shared Home Assistant REST scheduler. */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_hass_native.h"
#include "nd_hass_bridge.h"
#include "nd_hass_source.h"
#include "nd_hass_features.h"
#include "nd_hass_level.h"
#include "nd_hass_art.h"
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_panel.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(HASS,"HASS");
long __stack=65536;
struct Settings {char label[32],entity[96],action[20],display[24];unsigned int interval;int read_only,timestamp;};
static void settings(const char *id,struct Settings *s)
{
 struct NDOptionString f[7];char read_only[8],interval[16],timestamp[8];const char *keys[]={"LABEL","ENTITY","ACTION","READONLY","REFRESH","DISPLAY","TIMESTAMP"};const char *fallback[]={"HASS","","Toggle","NO","Default","State","YES"};
 char *outputs[7];unsigned int limits[7],i;outputs[0]=s->label;outputs[1]=s->entity;outputs[2]=s->action;outputs[3]=read_only;outputs[4]=interval;outputs[5]=s->display;outputs[6]=timestamp;
 limits[0]=sizeof(s->label);limits[1]=sizeof(s->entity);limits[2]=sizeof(s->action);limits[3]=sizeof(read_only);limits[4]=sizeof(interval);limits[5]=sizeof(s->display);limits[6]=sizeof(timestamp);
 for(i=0;i<7;++i){f[i].key=keys[i];f[i].fallback=fallback[i];f[i].out=outputs[i];f[i].limit=limits[i];}nd_plugin_option_strings(id,f,7);
 if(!s->label[0])strcpy(s->label,"HASS");s->read_only=!strcmp(read_only,"YES");s->timestamp=strcmp(timestamp,"NO")!=0;i=(unsigned int)atoi(interval);s->interval=i>=5 && i<=3600 ? i:0;
}
struct ControlMap {int percent,color,temperature;unsigned long signature;};
static unsigned long signature(const struct AHControls *c)
{return (unsigned long)nd_hass_percent_kind(c) | (c->color ? 8UL:0) | (c->temperature ? 16UL:0) | (c->cover_open ? 32UL:0) | (c->cover_close ? 64UL:0) | (c->cover_stop ? 128UL:0);}
static struct NDPanel *controls(const struct Settings *s,const struct NDHassEntity *e,const char *status,const char *buttons[4],int *count,const struct AHControls *cap,struct ControlMap *map)
{
 struct NDPanelField f[9];char capabilities[120],path[96],body[180],percent[8],color[16],target[40],unit[40],range[140],low[40],high[40],step[40];unsigned int n=5;int kind=nd_hass_percent_kind(cap);
 map->percent=map->color=map->temperature=-1;map->signature=signature(cap);*count=0;

 if(s->read_only){buttons[0]="Refresh";buttons[1]="Close";*count=2;}
 else if(!strncmp(s->entity,"cover.",6)){
  if(cap->cover_open)buttons[(*count)++]="Open cover";if(cap->cover_close)buttons[(*count)++]="Close cover";if(cap->cover_stop)buttons[(*count)++]="Stop cover";if(!*count)buttons[(*count)++]="Refresh";buttons[(*count)++]="Close";
 }else if(nd_hass_action(s->entity,"Turn off",path,body)){buttons[0]="Turn on";buttons[1]="Turn off";buttons[2]="Toggle";buttons[3]="Close";*count=4;}
 else if(nd_hass_action(s->entity,"Activate",path,body)){buttons[0]="Activate";buttons[1]="Refresh";buttons[2]="Close";*count=3;}
 else{buttons[0]="Refresh";buttons[1]="Close";*count=2;}
 f[0].label="Device";f[0].value=e->name[0] ? e->name:s->entity;f[0].limit=0;f[1].label="State";f[1].value=e->state;f[1].limit=0;f[2].label="Updated";f[2].value=s->timestamp ? e->updated:"Hidden";f[2].limit=0;f[3].label="Status";f[3].value=status;f[3].limit=0;

 nd_hass_capabilities(cap,capabilities);f[4].label="Controls";f[4].value=capabilities;f[4].limit=0;
 if(!s->read_only && kind){map->percent=(int)n;sprintf(percent,"%d",nd_hass_percent_value(cap)<0 ? 0:nd_hass_percent_value(cap));f[n].label=kind==1 ? "Brightness":kind==2 ? "Fan speed":"Position";f[n].value=percent;f[n++].limit=ND_PANEL_PERCENT;}
 if(!s->read_only && cap->color){map->color=(int)n;sprintf(color,"#%06lx",cap->rgb);f[n].label="RGB (Return)";f[n].value=color;f[n++].limit=8;}
 if(!s->read_only && cap->temperature){map->temperature=(int)n;sprintf(unit,"Target %s (Return)",cap->unit);amihass_number_text(cap->target,target);f[n].label=unit;f[n].value=target;f[n++].limit=20;
  amihass_number_text(cap->minimum,low);amihass_number_text(cap->maximum,high);amihass_number_text(cap->step,step);sprintf(range,"%s to %s %s; step %s",low,high,cap->unit,step);f[n].label="Range";f[n].value=range;f[n++].limit=0;}

 return nd_panel_open("Home Assistant",f,n,buttons,(unsigned int)*count);
}
static int control_check(void)
{struct Settings s;struct NDHassEntity e;struct AHControls c;struct ControlMap map;struct NDPanel *p;const char *b[4];int n,k,tick,ok=1;memset(&s,0,sizeof(s));memset(&e,0,sizeof(e));strcpy(e.name,"Sample control");strcpy(e.state,"on");
 for(k=0;k<4;++k){memset(&c,0,sizeof(c));if(k==0){strcpy(s.entity,"light.amihass_sample_living_room");c.brightness=c.level_known=c.color=c.color_known=1;c.level=37;c.rgb=0x4080ff;}if(k==1){strcpy(s.entity,"climate.amihass_sample_hallway");c.temperature=1;c.target=2300;c.minimum=700;c.maximum=3500;c.step=50;strcpy(c.unit,"C");}if(k==2){strcpy(s.entity,"fan.amihass_sample_bedroom");c.fan_speed=c.speed_known=1;c.speed=40;}if(k==3){strcpy(s.entity,"cover.amihass_sample_garage");c.position=c.position_known=c.cover_open=c.cover_close=c.cover_stop=1;c.position_value=70;}
  p=controls(&s,&e,"Sample data",b,&n,&c,&map);if(!p)return 10;/* Dispatch refreshes before teardown, as the real event loop does. */for(tick=0;tick<5;++tick){Delay(2);nd_panel_event(p);}if(map.percent>=0)ok=ok && nd_panel_percent_get(p,(unsigned int)map.percent)==(k==0 ? 37:k==2 ? 40:70);if(map.color>=0)ok=ok && !strcmp(nd_panel_get(p,(unsigned int)map.color),"#4080ff");if(map.temperature>=0)ok=ok && !strcmp(nd_panel_get(p,(unsigned int)map.temperature),"23");nd_panel_close(p);Delay(5);}
 s.read_only=1;p=controls(&s,&e,"Sample data",b,&n,&c,&map);if(!p)return 10;/* Dispatch refreshes before teardown, as the real event loop does. */for(tick=0;tick<5;++tick){Delay(2);nd_panel_event(p);}ok=ok && n==2 && !strcmp(b[0],"Refresh") && map.percent<0 && map.color<0 && map.temperature<0;nd_panel_close(p);
 puts(ok ? "PASS: native HASS capability-driven light, climate, fan and cover inputs.":"FAIL: native HASS controls.");return ok ? 0:10;}
int main(int argc,char **argv)
{
 struct NDClient dock;struct NDHassLevelEdit level_edit;struct NDHassBridge bridge;struct NDHassSource source;struct Settings s;struct NDHassEntity e;struct NDPanel *panel=NULL;
 char value[96],detail[160],path[96],body[180];unsigned int event,elapsed=0;int mode,reread=1,state=ND_HASS_WAIT,button,button_count=0,was_connected=0,loss_notified=0;struct ControlMap panel_map;char pending_action[20]="";unsigned int pending_age=0;char brightness_action[20];long target;char number[40];const char *action=NULL,*panel_actions[4];
 if(argc==2&&!strcmp(argv[1],"CONTROL-SELFTEST"))return control_check();
 if(argc==2&&!strcmp(argv[1],"PICKER-SELFTEST"))return nd_hass_picker_selftest();
 mode=nd_hass_command(argc,argv);
 if(mode==3)return 20; /* Shared connection setup belongs to AmiHASSPrefs. */
 if(argc<2 || !nd_client_init(&dock,argv[1]))return 20;memset(&bridge,0,sizeof(bridge));memset(&source,0,sizeof(source));memset(&e,0,sizeof(e));value[0]=0;nd_hass_level_cancel(&level_edit);
 for(;;){
  if(reread){pending_action[0]=0;nd_hass_level_cancel(&level_edit);settings(argv[1],&s);nd_hass_bridge_close(&bridge);nd_hass_bridge_options(&bridge,s.interval,s.read_only);memset(&e,0,sizeof(e));state=ND_HASS_WAIT;reread=0;nd_panel_close(panel);panel=NULL;}
  if(s.entity[0])nd_hass_source_poll(&source,argv[1],elapsed);
  if(source.changed){pending_action[0]=0;nd_hass_level_cancel(&level_edit);nd_hass_bridge_close(&bridge);nd_hass_bridge_options(&bridge,s.interval,s.read_only);}
  /* Drain drag/release messages before applying any incoming percentage. */
  button=nd_panel_event(panel);
  if(panel && panel_map.percent>=0 && button==panel_map.percent+1)nd_hass_level_offer(&level_edit,&bridge,nd_panel_percent_get(panel,(unsigned int)panel_map.percent));
  nd_hass_bridge_poll(&bridge,argv[1],s.entity,elapsed);
  nd_hass_level_poll(&level_edit,&bridge,elapsed);
  state=bridge.result;e=bridge.entity;event=0;
  if(state==ND_HASS_OK){if(loss_notified)event|=nd_client_notify(&dock,"AmiHASS reconnected",s.label,0);was_connected=1;loss_notified=0;}
  else if(was_connected && !loss_notified && state!=ND_HASS_WAIT && state!=AMIHASS_BUSY){event|=nd_client_notify(&dock,"AmiHASS unavailable",s.label,1);loss_notified=1;}
  if(!s.entity[0]){int live;Forbid();live=FindPort(AMIHASS_PORT)!=NULL;Permit();if(!live)state=9;}
  if(!s.entity[0]){nd_hass_source_close(&source);}
  if(!s.entity[0] && state!=9)strcpy(value,"SELECT ENTITY");else if(state==ND_HASS_OK && nd_hass_available(&e))sprintf(value,"%.38s %.15s",e.state,e.unit);else strcpy(value,nd_hass_bridge_problem(state));
  if(panel && state==ND_HASS_OK && panel_map.signature!=signature(&bridge.controls)){
   pending_action[0]=0;nd_hass_level_cancel(&level_edit);
   nd_panel_close(panel);panel=controls(&s,&e,value,panel_actions,&button_count,&bridge.controls,&panel_map);
  }
  if(panel && panel_map.percent>=0)nd_panel_percent_set(panel,(unsigned int)panel_map.percent,
      level_edit.level>=0 ? level_edit.level:state==ND_HASS_OK && nd_hass_percent_kind(&bridge.controls) ? nd_hass_percent_value(&bridge.controls):-1,
      state==ND_HASS_OK && !s.read_only && !nd_hass_bridge_pending(&bridge) && nd_hass_percent_kind(&bridge.controls) && nd_hass_available(&e));
  if(panel && state==ND_HASS_OK && !pending_action[0]){char confirmed[40];if(panel_map.color>=0 && bridge.controls.color_known){sprintf(confirmed,"#%06lx",bridge.controls.rgb);nd_panel_sync_string(panel,(unsigned int)panel_map.color,confirmed);}if(panel_map.temperature>=0){amihass_number_text(bridge.controls.target,confirmed);nd_panel_sync_string(panel,(unsigned int)panel_map.temperature,confirmed);}}
  if(state==ND_HASS_OK)nd_hass_display(&e,s.display,value);
  if(panel){const char *feedback=nd_hass_bridge_feedback(&bridge);snprintf(detail,sizeof(detail),"%s%s%.70s",s.read_only ? "Read only; ":"",nd_hass_source_label(&source),feedback[0] ? "; ":"");if(feedback[0])strncat(detail,feedback,sizeof(detail)-strlen(detail)-1);else if(state!=ND_HASS_OK){strncat(detail,"; ",sizeof(detail)-strlen(detail)-1);strncat(detail,source.hint[0] ? source.hint:nd_hass_problem(state),sizeof(detail)-strlen(detail)-1);}
   {char reading[96];nd_hass_display(&e,"State",reading);nd_panel_set(panel,1,state==ND_HASS_OK ? reading:value);}nd_panel_set(panel,2,s.timestamp ? e.updated:"Hidden");nd_panel_set(panel,3,detail);
   if(panel_map.color>=0)nd_panel_enable(panel,(unsigned int)panel_map.color,state==ND_HASS_OK && !nd_hass_bridge_pending(&bridge));if(panel_map.temperature>=0)nd_panel_enable(panel,(unsigned int)panel_map.temperature,state==ND_HASS_OK && !nd_hass_bridge_pending(&bridge));
  }
  event|=nd_client_update_art_quiet(&dock,s.label,value,state!=ND_HASS_OK && nd_hass_sad_status(nd_hass_bridge_problem(state)) ? "SadHASS":nd_hass_auto_icon(s.entity,e.device_class,e.name),
       state!=ND_HASS_OK && nd_hass_sad_status(nd_hass_bridge_problem(state)) ? 0:state==ND_HASS_OK ? nd_hass_visual_state(s.entity,e.state):ND_HASS_ICON_UNKNOWN,elapsed,500U);if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS){reread=1;continue;}
  event|=nd_client_set_badge(&dock,state==ND_HASS_OK && (!strcmp(e.state,"on") || !strcmp(e.state,"open")) ? "*":"");
  action=NULL;
  if(event&ND_EVENT_CLICK){if(s.read_only || !s.entity[0] || !nd_hass_action(s.entity,s.action,path,body) || !strcmp(s.action,"Show controls")){if(panel)nd_panel_raise(panel);else {panel=controls(&s,&e,value,panel_actions,&button_count,&bridge.controls,&panel_map);}}else action=s.action;}
  if(panel && panel_map.color>=0 && button==panel_map.color+1 && state==ND_HASS_OK){snprintf(brightness_action,sizeof(brightness_action),"Color %.7s",nd_panel_get(panel,(unsigned int)panel_map.color));action=brightness_action;}
  if(panel && panel_map.temperature>=0 && button==panel_map.temperature+1 && state==ND_HASS_OK){if(amihass_number(nd_panel_get(panel,(unsigned int)panel_map.temperature),&target)){amihass_number_text(target,number);snprintf(brightness_action,sizeof(brightness_action),"Temperature %s",number);action=brightness_action;}else nd_panel_title(panel,"Enter a numeric temperature in the shown unit");}
  if(button>=100 && button<100+button_count){
   const char *chosen=panel_actions[button-100];
   if(!strcmp(chosen,"Close")){nd_hass_level_cancel(&level_edit);pending_action[0]=0;nd_panel_close(panel);panel=NULL;}
   else if(!strcmp(chosen,"Refresh"))nd_hass_bridge_refresh(&bridge);
   else action=chosen;
  }else if(button==-1){pending_action[0]=0;nd_hass_level_cancel(&level_edit);nd_panel_close(panel);panel=NULL;}
  if(action){nd_hass_level_cancel(&level_edit);if(s.read_only || nd_hass_bridge_pending(&bridge)){if(panel)nd_panel_title(panel,s.read_only ? "Read-only tile":"Wait for state confirmation");}
   else if(!amihass_control_allowed(&bridge.controls,action)||!nd_hass_action(s.entity,action,path,body)){if(panel)nd_panel_title(panel,"Unsupported value, range or temperature step");}else if(state==ND_HASS_OK){strncpy(pending_action,action,19);pending_action[19]=0;pending_age=0;if(panel && !strncmp(action,"Color ",6) && panel_map.color>=0)nd_panel_commit_string(panel,(unsigned int)panel_map.color);if(panel && !strncmp(action,"Temperature ",12) && panel_map.temperature>=0)nd_panel_commit_string(panel,(unsigned int)panel_map.temperature);}}
  if(pending_action[0]){if(elapsed>=500U || pending_age>=500U-elapsed || (state!=ND_HASS_OK && state!=ND_HASS_WAIT && state!=AMIHASS_BUSY)){pending_action[0]=0;if(panel)nd_panel_title(panel,"Control expired; no delayed action sent");}
   else{pending_age+=elapsed;if(state==ND_HASS_OK && !bridge.client.pending && !nd_hass_bridge_pending(&bridge)){nd_hass_bridge_action(&bridge,argv[1],s.entity,pending_action);pending_action[0]=0;}}}
  if(panel)nd_hass_level_send(&level_edit,&bridge,argv[1],s.entity);
  elapsed=nd_client_wait_signals(&dock,bridge.client.pending || panel || nd_hass_bridge_pending(&bridge) ? 10U:250U,nd_panel_signals(panel)|(bridge.client.port ? 1UL<<bridge.client.port->mp_SigBit:0)|(source.client.port ? 1UL<<source.client.port->mp_SigBit:0));
 }
 nd_panel_close(panel);nd_hass_bridge_close(&bridge);nd_hass_source_close(&source);nd_client_close(&dock);return 0;
}
