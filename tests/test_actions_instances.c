#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
#include "nd_manifest.h"
#include "nd_actions.h"
static int find(struct NDConfig *c,const char *id){int i;for(i=0;i<c->count;++i)if(!strcmp(c->items[i].id,id))return i;return -1;}
static int visible(struct NDConfig *c,int at){int i,n=0;for(i=0;i<at;++i)if(c->items[i].enabled)++n;return n;}
int main(void){static struct NDConfig cfg,back;struct NDOptionDef defs[8];int a,b;
 nd_default_config(&cfg);assert(nd_parse_line(&cfg,"PLUGIN=ACTIONS|Plugins/Actions|ACTIONS|METER|ON|YES"));
 a=find(&cfg,"ACTIONS");assert(a>=0&&nd_independent_plugin_path(cfg.items[a].command)==4);
 assert(nd_option_collect(&cfg,"ACTIONS",defs,8)==5);
 assert(!strcmp(nd_option_get(&cfg,"ACTIONS","CONFIRM"),"YES"));
 assert(!strcmp(nd_option_get(&cfg,"ACTIONS","ACTION"),"Restart Workbench"));
 assert(nd_option_set(&cfg,"ACTIONS","ACTION","Open Shell"));
 assert(nd_option_set(&cfg,"ACTIONS","LABEL","Shell"));assert(nd_add_plugin_instance(&cfg,a));b=find(&cfg,"ACTIONS_2");assert(b>=0);
 assert(!strcmp(nd_option_schema_id(&cfg,"ACTIONS_2"),"ACTIONS"));
 assert(nd_option_set(&cfg,"ACTIONS_2","ACTION","Restart Amiga"));assert(nd_option_set(&cfg,"ACTIONS_2","LABEL","Reboot"));
 assert(nd_option_set(&cfg,"ACTIONS_2","ICON","Workbench")&&nd_option_set(&cfg,"ACTIONS_2","CONFIRM","NO"));
 assert(!nd_option_set(&cfg,"ACTIONS_2","ACTION","Delete all"));
 assert(!nd_option_set(&cfg,"ACTIONS_2","ICON","invalid"));
 assert(nd_write_config(&cfg,"actions.cfg")&&nd_load_config(&back,"actions.cfg")==0);
 assert(!strcmp(nd_option_get(&back,"ACTIONS","ACTION"),"Open Shell")&&!strcmp(nd_option_get(&back,"ACTIONS","CONFIRM"),"YES"));
 assert(!strcmp(nd_option_get(&back,"ACTIONS_2","ACTION"),"Restart Amiga")&&!strcmp(nd_option_get(&back,"ACTIONS_2","CONFIRM"),"NO"));
 b=find(&back,"ACTIONS_2");assert(!strcmp(back.items[b].label,"REBOOT"));
 assert(nd_remove_visible_item(&back,visible(&back,b))&&find(&back,"ACTIONS_2")<0);
 a=find(&back,"ACTIONS");assert(nd_remove_visible_item(&back,visible(&back,a)));
 assert(!strcmp(nd_option_get(&back,"ACTIONS","ACTION"),"Restart Workbench")&&!strcmp(nd_option_get(&back,"ACTIONS","CONFIRM"),"YES"));
 assert(!strcmp(back.items[find(&back,"ACTIONS")].label,"ACTIONS"));
 for(a=0;a<ND_ACTION_COUNT;++a)assert(nd_action_index(nd_actions[a].name)==a);
 assert(nd_action_index("arbitrary command")==-1);
 remove("actions.cfg");puts("ACTIONS: unique instances, save/load, independent choices, validated actions and removal defaults passed.");return 0;}
