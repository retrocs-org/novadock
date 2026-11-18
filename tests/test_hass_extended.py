#!/usr/bin/env python3
"""Capability metadata, action ranges and service payloads through the SDK."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
code=r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_hass.h"
int main(void){struct AHControls c,d;struct AmiHASSEntity items[16];char path[96],body[180];unsigned int n;
 amihass_controls("{\"entity_id\":\"fan.bedroom\",\"state\":\"on\",\"attributes\":{\"percentage\":40,\"supported_features\":1}}",&c);assert(c.fan_speed&&c.speed_known&&c.speed==40);assert(amihass_control_allowed(&c,"Fan speed 75"));assert(!amihass_control_allowed(&c,"Fan speed 101"));assert(!amihass_control_allowed(&c,"Fan speed 12.5"));assert(!amihass_control_allowed(&c,"Position 30"));assert(amihass_control_matches(&c,"on","Fan speed 40"));
 n=amihass_control_items(&c,items,16);amihass_controls_items(items,n,&d);assert(d.fan_speed&&d.speed==40&&d.speed_known);assert(amihass_action("fan.bedroom","Fan speed 75",path,body)&&!strcmp(path,"/api/services/fan/set_percentage")&&strstr(body,"\"percentage\":75"));assert(!amihass_action("light.room","Fan speed 75",path,body));
 amihass_controls("{\"entity_id\":\"fan.simple\",\"state\":\"on\",\"attributes\":{\"percentage\":40,\"supported_features\":0}}",&c);assert(!c.fan_speed&&!amihass_control_allowed(&c,"Fan speed 75"));
 amihass_controls("{\"entity_id\":\"cover.shade\",\"state\":\"open\",\"attributes\":{\"current_position\":35,\"supported_features\":15}}",&c);assert(c.position&&c.position_value==35&&c.cover_open&&c.cover_close&&c.cover_stop);assert(amihass_control_allowed(&c,"Position 70"));assert(!amihass_control_allowed(&c,"Position -1"));assert(amihass_control_allowed(&c,"Stop cover"));n=amihass_control_items(&c,items,16);amihass_controls_items(items,n,&d);assert(d.position&&d.position_known&&d.position_value==35&&d.cover_stop&&d.cover_open&&d.cover_close);assert(amihass_action("cover.shade","Position 70",path,body)&&!strcmp(path,"/api/services/cover/set_cover_position")&&strstr(body,"\"position\":70"));assert(amihass_action("cover.shade","Close cover",path,body)&&!strcmp(path,"/api/services/cover/close_cover"));assert(!amihass_action("cover.shade","Position 101",path,body));assert(!amihass_action("lock.front","Open cover",path,body));
 amihass_controls("{\"entity_id\":\"cover.readonly\",\"state\":\"closed\",\"attributes\":{\"current_position\":0}}",&c);assert(!c.position&&!c.cover_open&&!c.cover_close&&!c.cover_stop&&!amihass_control_allowed(&c,"Position 50"));
 amihass_controls("{\"entity_id\":\"cover.partial\",\"state\":\"closed\",\"attributes\":{\"supported_features\":1}}",&c);assert(c.cover_open&&!c.cover_close&&!c.cover_stop&&!c.position);
 memset(items,0,sizeof(items));strcpy(items[0].id,"control.position");strcpy(items[0].state,"corrupt");amihass_controls_items(items,1,&d);assert(!d.position);
 amihass_controls("{\"entity_id\":\"climate.room\",\"state\":\"heat\",\"attributes\":{\"temperature\":21,\"min_temp\":7,\"max_temp\":35,\"target_temp_step\":0.5,\"temperature_unit\":\"C\"}}",&c);assert(c.temperature&&amihass_control_allowed(&c,"Temperature 22.5")&&!amihass_control_allowed(&c,"Temperature 22.25"));
 puts("HASS fan/cover metadata, read-only gating, ranges, payloads and climate steps passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'plugin_src/hass'),str(p/'check.c'),str(ROOT/'plugin_src/hass/nd_hass.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-g','-O1','-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)
