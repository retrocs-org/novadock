#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_manifest.h"
#include "nd_plugin_labels.h"
int main(void)
{
 static struct NDConfig cfg,loaded;struct NDManifest m;int i;
 assert(!strcmp(nd_manifest_builtin_name("EYES"),"EYES"));
 assert(nd_manifest_builtin("EYES",&m)&&!strcmp(m.name,"EYES"));
 nd_default_config(&cfg);
 for(i=0;i<cfg.count;++i)if(cfg.items[i].kind==ND_ITEM_PLUGIN){char upper[ND_LABEL_LEN];strcpy(upper,cfg.items[i].label);nd_plugin_name_upper(upper);assert(!strcmp(upper,cfg.items[i].label));}
 nd_default_config(&cfg);cfg.count=0;
 assert(nd_parse_line(&cfg,"PLUGIN=My sensor|Plugins/ThirdPartyCase|THIRD|METER|OFF"));
 assert(!strcmp(cfg.items[0].label,"MY SENSOR")&&!strcmp(cfg.items[0].command,"Plugins/ThirdPartyCase")&&!strcmp(cfg.items[0].id,"THIRD"));
 assert(nd_parse_line(&cfg,"LAUNCH=My editor|Work:MixedCase/Editor||ON"));assert(!strcmp(cfg.items[1].label,"My editor"));
 assert(nd_write_config(&cfg,"plugin-names.cfg"));assert(nd_load_config(&loaded,"plugin-names.cfg")==0);
 assert(!strcmp(loaded.items[0].label,"MY SENSOR")&&!strcmp(loaded.items[0].command,"Plugins/ThirdPartyCase"));
 nd_manifest_init(&m,"myTool");assert(nd_manifest_line(&m,"NOVADOCK_ID=THIRD"));assert(nd_manifest_line(&m,"NOVADOCK_NAME=My third-party plugin"));assert(nd_manifest_line(&m,"NOVADOCK_PROGRAM=ThirdPartyCase"));assert(nd_manifest_finish(&m));
 assert(!strcmp(m.name,"MY THIRD-PARTY PLUGIN")&&!strcmp(m.program,"ThirdPartyCase"));
 assert(nd_plugin_name_equal("MusicPlayer","MUSICPLAYER"));assert(!nd_plugin_name_equal("Artist: Example","MUSICPLAYER"));
 remove("plugin-names.cfg");remove("plugin-names.cfg.bak");remove("plugin-names.cfg.good");
 puts("Plugin names: bundled/custom/third-party display names uppercase; executable paths, IDs, launcher labels and unrelated captions preserved.");return 0;
}
