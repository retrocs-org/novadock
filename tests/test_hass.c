#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_hass.h"
#include "nd_config.h"
#include "nd_options.h"
#include "nd_manifest.h"
static const char *state="{\"entity_id\":\"light.desk\",\"state\":\"on\",\"attributes\":{\"friendly_name\":\"Desk \\u00e5\",\"unit_of_measurement\":\"K\"},\"last_updated\":\"2026-11-17T12:00:00Z\"}";
int main(void)
{
 struct NDHassEntity e,items[2];struct NDHassConnection c;struct NDConfig cfg;
 char json[1200],copy[1200],token[1025],path[96],body[180],request[1700];unsigned int i;int index=-1;
 assert(nd_hass_url("https://homeassistant.local:8123",0));assert(!nd_hass_url("http://homeassistant.local:8123",0));assert(nd_hass_url("http://127.0.0.1:8123",1));
 assert(!nd_hass_url("https://name:secret@home",1));assert(!nd_hass_url("https://home/api/",1));assert(!nd_hass_url("https://home:65536",1));assert(!nd_hass_url("https://home\nheader=x",1));assert(!nd_hass_url("https://home?token=x",1));
 assert(nd_hass_entity_id("light.my_light"));assert(!nd_hass_entity_id("light.a\"}"));assert(!nd_hass_entity_id("light.a/b"));assert(!nd_hass_entity_id("light."));assert(!nd_hass_entity_id("light.a.b"));
 assert(nd_hass_token("test-only-token-for-local-fixture\n",token));assert(!strcmp(token,"test-only-token-for-local-fixture"));
 assert(!nd_hass_token("bad\nAuthorization: example",token));assert(!nd_hass_token("not enough",token));
 assert(nd_hass_state(state,&e));assert(!strcmp(e.id,"light.desk") && !strcmp(e.state,"on"));assert(!strcmp(e.name,"Desk \345"));assert(nd_hass_available(&e));assert(!strcmp(nd_hass_icon(&e),"Bulb"));
 assert(nd_hass_matches(&e,"light","DESK"));assert(!nd_hass_matches(&e,"switch","desk"));assert(nd_hass_matches(&e,"All","desk"));
 sprintf(json,"[%s,%s]",state,state);assert(nd_hass_catalog(json,items,2)==2);assert(nd_hass_catalog(json,items,1)==-1);
 assert(nd_hass_catalog("[]",items,2)==0);assert(nd_hass_catalog("[1]",items,2)==-1);
 for(i=0;i<strlen(state);++i){memcpy(copy,state,i);copy[i]=0;assert(!nd_hass_state(copy,&e));}
 for(i=0;i<strlen(state);++i){strcpy(copy,state);copy[i]='\\';(void)nd_hass_state(copy,&e);}
 assert(!nd_hass_state("{\"entity_id\":\"light.desk\",\"entity_id\":\"light.other\",\"state\":\"on\"}",&e));
 assert(!nd_hass_state("{\"entity_id\":\"light.desk\",\"entity_\\u0069d\":\"light.other\",\"state\":\"on\"}",&e));
 assert(nd_hass_action("light.desk","Toggle",path,body));assert(!strcmp(path,"/api/services/homeassistant/toggle"));assert(!strcmp(body,"{\"entity_id\":\"light.desk\"}"));
 assert(nd_hass_action("scene.amiga","Activate",path,body));assert(!strcmp(path,"/api/services/scene/turn_on"));assert(nd_hass_action("scene.amiga","Toggle",path,body));
 assert(nd_hass_action("group.desk","Turn off",path,body));assert(!nd_hass_action("sensor.room","Toggle",path,body));assert(!nd_hass_action("lock.front","Turn on",path,body));assert(!nd_hass_action("cover.garage","Toggle",path,body));
 strcpy(c.url,"https://homeassistant.local:8123");c.allow_http=0;c.poll=30;
 assert(nd_hass_request(&c,"test-only-token-for-local-fixture",path,"group.desk",request,sizeof(request)));assert(strstr(request,"Authorization: Bearer test-only-token-for-local-fixture\"\n"));assert(strstr(request,"data = \"{\\\"entity_id\\\":\\\"group.desk\\\"}\"\n"));
 assert(!nd_hass_request(&c,"test-only-token-for-local-fixture","/api/states\nheader",NULL,request,sizeof(request)));assert(!nd_hass_request(&c,"test-only-token-for-local-fixture","/api/states/../../",NULL,request,sizeof(request)));
 assert(!nd_hass_request(&c,"test-only-token-for-local-fixture",path,"group.desk",request,10));
 {int total;char page[2400],payload[4096];char *args[]={"HASS","HASS_2","CONNECTION"};
  assert(nd_hass_command(3,args)==3);
  assert(nd_hass_page_request("light","Desk's lamp",0,payload,sizeof(payload)));
  assert(strstr(payload,"/api/")==NULL);
  assert(nd_hass_page_request("All","\345",16,payload,sizeof(payload)));
  sprintf(page,"{\"total\":3040,\"items\":[%s]}",state);
  assert(nd_hass_page(page,items,2,&total)==1 && total==3040);
  assert(!nd_hass_page_request("light' or True","",0,payload,sizeof(payload)));
  assert(!nd_hass_page_request("light","",-1,payload,sizeof(payload)));
 }
 nd_default_config(&cfg);assert(nd_parse_line(&cfg,"PLUGIN=HASS|Plugins/HASS|HASS|METER|ON|YES"));
 for(i=0;i<(unsigned int)cfg.count;++i)if(!strcmp(cfg.items[i].id,"HASS")){index=(int)i;break;}assert(index>=0);
 assert(!strcmp(nd_option_get(&cfg,"HASS","TYPE"),"light"));
 assert(nd_option_set(&cfg,"HASS","DEVICE","0123456789abcdef0123456789abcdef"));
 assert(nd_option_set(&cfg,"HASS","DEVICE_NAME","Desk lamp"));
 assert(!strcmp(nd_option_get(&cfg,"HASS","READONLY"),"NO"));assert(!strcmp(nd_option_get(&cfg,"HASS","REFRESH"),"Default"));
 assert(nd_option_set(&cfg,"HASS","AREA","Study"));assert(nd_option_set(&cfg,"HASS","REFRESH","60"));assert(nd_option_set(&cfg,"HASS","READONLY","YES"));assert(nd_option_set(&cfg,"HASS","DISPLAY","Name and state"));assert(nd_option_set(&cfg,"HASS","TIMESTAMP","NO"));
 assert(nd_option_set(&cfg,"HASS","ENTITY","light.desk"));assert(nd_option_set(&cfg,"HASS","LABEL","Desk"));assert(nd_option_set(&cfg,"HASS","ICON","Bulb"));assert(nd_add_plugin_instance(&cfg,index));
 assert(!strcmp(nd_option_schema_id(&cfg,"HASS_2"),"HASS"));assert(!strcmp(nd_option_get(&cfg,"HASS_2","AREA"),"Study"));assert(!strcmp(nd_option_get(&cfg,"HASS_2","REFRESH"),"60"));assert(nd_option_set(&cfg,"HASS_2","AREA","Kitchen"));assert(!strcmp(nd_option_get(&cfg,"HASS","AREA"),"Study"));assert(nd_option_set(&cfg,"HASS_2","ENTITY","light.other"));assert(!strcmp(nd_option_get(&cfg,"HASS","ENTITY"),"light.desk"));
 assert(!strcmp(nd_option_get(&cfg,"HASS_2","DEVICE"),"0123456789abcdef0123456789abcdef"));
 assert(nd_option_set(&cfg,"HASS_2","DEVICE","abcdef0123456789abcdef0123456789"));
 assert(!strcmp(nd_option_get(&cfg,"HASS","DEVICE_NAME"),"Desk lamp"));
 puts("HASS metadata, action policy, authentication and independent-instance tests passed.");return 0;
}
