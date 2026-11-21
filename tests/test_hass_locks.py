"""Offline lock/door controls and confirmation policy; no transport or devices."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
code=r"""
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_hass_actions.h"
#include "nd_hass_features.h"
#include "nd_hass_art.h"
int main(void){struct AHControls c,copy;struct AmiHASSEntity items[16],e,other;struct NDHassApproval approval;char choices[128],text[120],action[20],path[96],body[180];const char *buttons[5];unsigned int n;
 memset(&e,0,sizeof(e));strcpy(e.id,"lock.amihass_sample_front_door");strcpy(e.name,"Sample front door");strcpy(e.state,"locked");
 amihass_controls("{\"entity_id\":\"lock.amihass_sample_front_door\",\"state\":\"locked\",\"attributes\":{\"supported_features\":1,\"code_format\":null}}",&c);assert(c.lock&&c.unlock&&c.lock_open);n=amihass_control_items(&c,items,16);amihass_controls_items(items,n,&copy);assert(copy.lock&&copy.unlock&&copy.lock_open);
 nd_hass_action_choices(e.id,&c,0,choices);assert(!strcmp(choices,"Show controls"));nd_hass_action_choices(e.id,&c,1,choices);assert(!strcmp(choices,"Show controls,Lock,Unlock,Open lock"));assert(nd_hass_control_buttons(e.id,&c,0,buttons)==4&&!strcmp(buttons[0],"Lock")&&!strcmp(buttons[2],"Open lock"));assert(nd_hass_control_buttons(e.id,&c,1,buttons)==2&&!strcmp(buttons[0],"Refresh"));
 assert(nd_hass_action(e.id,"Unlock",path,body)&&!strcmp(path,"/api/services/lock/unlock"));assert(!nd_hass_action(e.id,"Toggle",path,body));assert(nd_hass_action_sensitive(&e,&c,"Unlock")&&nd_hass_action_sensitive(&e,&c,"Open lock")&&!nd_hass_action_sensitive(&e,&c,"Lock"));
 nd_hass_approval_cancel(&approval);assert(!nd_hass_approval_accept(&approval,&e,&c,1,0,action));assert(nd_hass_approval_begin(&approval,&e,&c,"Unlock"));nd_hass_approval_cancel(&approval);assert(!nd_hass_approval_accept(&approval,&e,&c,1,0,action));
 assert(nd_hass_approval_begin(&approval,&e,&c,"Unlock"));assert(nd_hass_approval_accept(&approval,&e,&c,1,0,action)&&!strcmp(action,"Unlock"));assert(!nd_hass_approval_accept(&approval,&e,&c,1,0,action));
 assert(nd_hass_approval_begin(&approval,&e,&c,"Unlock"));assert(!nd_hass_approval_tick(&approval,&e,&c,10,0,0)&&!approval.entity[0]);
 assert(nd_hass_approval_begin(&approval,&e,&c,"Unlock"));assert(!nd_hass_approval_accept(&approval,&e,&c,1,1,action));
 other=e;strcpy(other.id,"lock.amihass_sample_other");assert(nd_hass_approval_begin(&approval,&e,&c,"Unlock"));assert(!nd_hass_approval_accept(&approval,&other,&c,1,0,action));
 assert(nd_hass_approval_begin(&approval,&e,&c,"Open lock"));assert(nd_hass_approval_tick(&approval,&e,&c,1499,1,0));assert(!nd_hass_approval_tick(&approval,&e,&c,1,1,0));
 assert(nd_hass_approval_begin(&approval,&e,&c,"Unlock"));copy=c;copy.unlock=0;assert(!nd_hass_approval_accept(&approval,&e,&copy,1,0,action));
 assert(!strcmp(amihass_display_state(&e),"Locked")&&!strcmp(e.state,"locked"));strcpy(e.state,"unlocked");nd_hass_display(&e,"State",text);assert(!strcmp(text,"Unlocked "));assert(nd_hass_visual_state(e.id,e.state)==ND_HASS_ICON_ON);strcpy(e.state,"jammed");assert(!amihass_control_matches(&c,e.state,"Unlock")&&nd_hass_visual_state(e.id,e.state)==ND_HASS_ICON_UNKNOWN);
 amihass_controls("{\"entity_id\":\"lock.amihass_sample_pin\",\"state\":\"locked\",\"attributes\":{\"supported_features\":1,\"code_format\":\"[0-9]+\"}}",&c);assert(!c.lock&&!c.unlock&&!c.lock_open);nd_hass_action_choices(e.id,&c,1,choices);assert(!strcmp(choices,"Show controls"));assert(!nd_hass_approval_begin(&approval,&e,&c,"Unlock"));
 strcpy(e.id,"binary_sensor.amihass_sample_front_door");strcpy(e.device_class,"door");strcpy(e.state,"off");nd_hass_display(&e,"State",text);assert(!strcmp(text,"Closed ")&&!strcmp(e.state,"off"));strcpy(e.state,"on");assert(!strcmp(amihass_display_state(&e),"Open"));assert(!nd_hass_action(e.id,"Unlock",path,body));
 memset(&c,0,sizeof(c));c.cover_open=c.cover_close=c.cover_stop=c.position=c.position_known=1;c.position_value=10;strcpy(e.id,"cover.amihass_sample_garage");strcpy(e.state,"closed");nd_hass_action_choices(e.id,&c,1,choices);assert(strstr(choices,"Open cover")&&strstr(choices,"Stop cover"));assert(nd_hass_action_sensitive(&e,&c,"Open cover")&&nd_hass_action_sensitive(&e,&c,"Position 30")&&!nd_hass_action_sensitive(&e,&c,"Position 5")&&!nd_hass_action_sensitive(&e,&c,"Position 0"));
 assert(!amihass_control_matches(&c,"unknown","Stop cover")&&!amihass_control_matches(&c,"jammed","Stop cover")&&!amihass_control_matches(&c,"opening","Stop cover"));assert(amihass_control_matches(&c,"open","Stop cover")&&amihass_control_matches(&c,"closed","Stop cover"));
 memset(&c,1,sizeof(c));nd_hass_capabilities(&c,text);assert(strlen(text)<120);
 puts("HASS locks/doors: capability filtering, PIN read-only, explicit actions, friendly raw-state-preserving labels, one-shot confirmation, cancellation, expiry and stale/changed-target rejection passed.");return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'exec').mkdir();(p/'exec/ports.h').write_text('typedef unsigned long ULONG;typedef long LONG;struct MsgPort;struct Message{unsigned short mn_Length;struct MsgPort *mn_ReplyPort;};\n');(p/'check.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(p),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'plugin_src/hass'),str(p/'check.c'),str(ROOT/'plugin_src/hass/nd_hass.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)