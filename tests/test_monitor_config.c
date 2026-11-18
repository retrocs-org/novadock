#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
#include "nd_manifest.h"
static struct NDConfig config,restored;
int main(void)
{
 struct NDManifest m;int base,clone,visible,i;
 nd_default_config(&config);
 assert(nd_manifest_bundled_id("WATCH") && nd_manifest_bundled_id("LOGS"));
 assert(nd_manifest_builtin("WATCH",&m) && m.multiple_instances && !strcmp(m.version,"0.1"));
 assert(nd_manifest_merge(&config,&m,"Plugins/Watch")==ND_MANIFEST_ADDED);base=config.count-1;
 assert(nd_option_set(&config,"WATCH","DRAWER","RAM:one"));assert(nd_option_set(&config,"WATCH","LABEL","Builds"));
 assert(nd_add_plugin_instance(&config,base));clone=config.count-1;
 assert(!strcmp(config.items[clone].id,"WATCH_2"));assert(!strcmp(nd_option_schema_id(&config,"WATCH_2"),"WATCH"));
 assert(nd_option_set(&config,"WATCH_2","DRAWER","RAM:two"));assert(!strcmp(nd_option_get(&config,"WATCH","DRAWER"),"RAM:one"));
 assert(!nd_option_set(&config,"WATCH_2","INTERVAL","1"));assert(!nd_option_set(&config,"WATCH_2","INTERVAL","301"));assert(nd_option_set(&config,"WATCH_2","INTERVAL","2"));
 assert(nd_manifest_builtin("LOGS",&m) && m.multiple_instances);assert(nd_manifest_merge(&config,&m,"Plugins/Logs")==ND_MANIFEST_ADDED);base=config.count-1;
 assert(nd_option_set(&config,"LOGS","FILE","RAM:one.log"));assert(nd_add_plugin_instance(&config,base));assert(!strcmp(nd_option_schema_id(&config,"LOGS_2"),"LOGS"));
 assert(nd_option_set(&config,"LOGS_2","FILE","RAM:two.log"));assert(nd_option_set(&config,"LOGS_2","LABEL","Bridge log"));assert(!strcmp(config.items[config.count-1].label,"Bridge log"));
 assert(nd_write_config(&config,"monitors.cfg"));assert(!nd_load_config(&restored,"monitors.cfg"));assert(!strcmp(nd_option_get(&restored,"LOGS_2","FILE"),"RAM:two.log"));assert(!strcmp(nd_option_get(&restored,"WATCH_2","DRAWER"),"RAM:two"));
 visible=0;for(i=0;i<config.count-1;++i)if(config.items[i].enabled)++visible;assert(nd_remove_visible_item(&config,visible));
 for(i=0;i<config.count;++i)assert(strcmp(config.items[i].id,"LOGS_2"));assert(!strcmp(nd_option_get(&config,"LOGS","FILE"),"RAM:one.log"));
 puts("WATCH/LOGS discovery, independent settings, save/reload and removal passed.");return 0;
}
