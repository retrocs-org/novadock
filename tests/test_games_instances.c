#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
#include "nd_manifest.h"
static int find(struct NDConfig *cfg,const char *id){int i;for(i=0;i<cfg->count;++i)if(!strcmp(cfg->items[i].id,id))return i;return -1;}
static int visible(struct NDConfig *cfg,int at){int i,n=0;for(i=0;i<at;++i)if(cfg->items[i].enabled)++n;return n;}
int main(void){struct NDConfig *cfg=calloc(1,sizeof(*cfg)),*loaded=calloc(1,sizeof(*loaded));struct NDOptionDef defs[20];struct NDManifest m;int a,b,before,old_options;FILE *f;char long_label[40];assert(cfg && loaded);nd_default_config(cfg);
 assert(nd_parse_line(cfg,"PLUGIN=WHDLoad|Plugins/Games|WHDLOAD|METER|ON|NO"));a=find(cfg,"WHDLOAD");assert(a>=0 && nd_plugin_allows_multiple(&cfg->items[a]));assert(!strcmp(nd_option_get(cfg,"WHDLOAD","DRAWER"),""));
 assert(nd_option_set(cfg,"WHDLOAD","LABEL","Games"));assert(nd_option_set(cfg,"WHDLOAD","DRAWER","Work:Games"));assert(nd_add_plugin_instance(cfg,a));b=find(cfg,"WHDLOAD_2");assert(b>=0 && !strcmp(cfg->items[b].label,"Games"));
 assert(!strcmp(nd_option_schema_id(cfg,"WHDLOAD_2"),"WHDLOAD"));assert(nd_option_collect(cfg,"WHDLOAD_2",defs,5)==5);assert(!strcmp(defs[1].type,"FILE"));
 assert(!strcmp(nd_option_get(cfg,"WHDLOAD","DRAWER"),"Work:Games"));
 assert(!strcmp(nd_option_get(cfg,"WHDLOAD","COMMAND"),"WHDLoad"));
 assert(nd_option_set(cfg,"WHDLOAD_2","COMMAND","Work:Tools/WHDLoad Demo"));
 assert(!strcmp(nd_option_get(cfg,"WHDLOAD","COMMAND"),"WHDLoad"));
 assert(!strcmp(nd_games_icon_path(cfg,"WHDLOAD"),"Icons/WHDLoad.iff"));
 assert(nd_option_set(cfg,"WHDLOAD","ICON","Tools"));assert(!strcmp(nd_games_icon_path(cfg,"WHDLOAD"),"Icons/Tools.iff"));
 assert(nd_option_set(cfg,"WHDLOAD_2","ICON","Custom"));assert(nd_option_set(cfg,"WHDLOAD_2","ICONFILE","Work:Demos/Demo.info"));
 assert(!strcmp(nd_games_icon_path(cfg,"WHDLOAD_2"),"Work:Demos/Demo.info"));assert(!strcmp(nd_games_icon_path(cfg,"WHDLOAD"),"Icons/Tools.iff"));
 assert(!nd_option_set(cfg,"WHDLOAD","ICON","unknown"));
 assert(nd_option_set(cfg,"WHDLOAD_2","ICON","Demos"));assert(!strcmp(nd_games_icon_path(cfg,"WHDLOAD_2"),"Work:Demos/Demo.info"));assert(nd_option_set(cfg,"WHDLOAD_2","ICON","Custom"));
 assert(!strcmp(nd_option_get(cfg,"WHDLOAD_2","DRAWER"),"Work:Games"));assert(nd_option_set(cfg,"WHDLOAD_2","LABEL","Demos"));assert(nd_option_set(cfg,"WHDLOAD_2","DRAWER","Work:Demos"));
 assert(!strcmp(cfg->items[a].label,"Games") && !strcmp(cfg->items[b].label,"Demos"));assert(!strcmp(nd_option_get(cfg,"WHDLOAD","DRAWER"),"Work:Games"));
 memset(long_label,'x',sizeof(long_label)-1);long_label[sizeof(long_label)-1]=0;assert(!nd_option_set(cfg,"WHDLOAD_2","LABEL",long_label));assert(!nd_option_set(cfg,"WHDLOAD_2","LABEL",""));
 assert(nd_write_config(cfg,"whdload.cfg"));assert(nd_load_config(loaded,"whdload.cfg")==0);a=find(loaded,"WHDLOAD");b=find(loaded,"WHDLOAD_2");assert(a>=0 && b>=0);assert(!strcmp(loaded->items[b].label,"Demos"));assert(!strcmp(nd_option_get(loaded,"WHDLOAD_2","DRAWER"),"Work:Demos"));
 assert(!strcmp(nd_games_icon_path(loaded,"WHDLOAD_2"),"Work:Demos/Demo.info"));
 assert(!strcmp(nd_option_get(loaded,"WHDLOAD_2","COMMAND"),"Work:Tools/WHDLoad Demo"));
 assert(!strcmp(nd_option_get(loaded,"WHDLOAD","COMMAND"),"WHDLoad"));
 assert(nd_add_plugin_instance(loaded,b));assert(find(loaded,"WHDLOAD_3")>=0);assert(!strcmp(nd_option_get(loaded,"WHDLOAD_3","DRAWER"),"Work:Demos"));
 before=loaded->count;old_options=loaded->option_count;loaded->option_count=ND_MAX_OPTIONS;assert(!nd_add_plugin_instance(loaded,b));assert(loaded->count==before && loaded->option_count==ND_MAX_OPTIONS);loaded->option_count=old_options;
 assert(nd_manifest_builtin("WHDLOAD",&m) && m.multiple_instances);
 assert(nd_remove_visible_item(loaded,visible(loaded,b)));
 assert(find(loaded,"WHDLOAD_2")<0);assert(!strcmp(nd_option_get(loaded,"WHDLOAD_3","DRAWER"),"Work:Demos"));
 {int i;for(i=0;i<loaded->option_count;++i)assert(strcmp(loaded->options[i].id,"WHDLOAD_2"));}
 a=find(loaded,"WHDLOAD");assert(nd_remove_visible_item(loaded,visible(loaded,a)));
 assert(find(loaded,"WHDLOAD")>=0 && !loaded->items[find(loaded,"WHDLOAD")].enabled);
 assert(!strcmp(nd_option_get(loaded,"WHDLOAD","DRAWER"),""));assert(!strcmp(nd_games_icon_path(loaded,"WHDLOAD"),"Icons/WHDLoad.iff"));
 assert(!strcmp(loaded->items[find(loaded,"WHDLOAD")].label,"WHDLoad"));
 assert(!strcmp(nd_option_get(loaded,"WHDLOAD","COMMAND"),"WHDLoad"));
 assert(!strcmp(nd_option_get(loaded,"WHDLOAD_3","COMMAND"),"Work:Tools/WHDLoad Demo"));
 assert(nd_option_set(loaded,"WHDLOAD_3","ICON","Text"));assert(!strcmp(nd_games_icon_path(loaded,"WHDLOAD_3"),"Work:Demos/Demo.info"));
 assert(nd_option_set(loaded,"WHDLOAD_3","ICON","Custom"));assert(nd_option_set(loaded,"WHDLOAD_3","ICONFILE",""));assert(!strcmp(nd_games_icon_path(loaded,"WHDLOAD_3"),"Icons/WHDLoad.iff"));
 {static const char *const suffix[]={"idx","idx.tmp","idx.bak"};int i;char path[128];
  for(i=0;i<3;++i){sprintf(path,"ENVARC:NovaDock/Cache/WHDLoad/WHDLOAD_2.%s",suffix[i]);f=fopen(path,"w");assert(f);fputs("test cache",f);fclose(f);}
  f=fopen("ENVARC:NovaDock/Cache/WHDLoad/WHDLOAD_3.idx","w");assert(f);fclose(f);
  assert(!nd_games_forget_cache("../WHDLOAD_3"));assert(!nd_games_forget_cache(""));assert(!nd_games_forget_cache("Other:File"));
  assert(nd_games_forget_cache("WHDLOAD_2"));
  for(i=0;i<3;++i){sprintf(path,"ENVARC:NovaDock/Cache/WHDLoad/WHDLOAD_2.%s",suffix[i]);assert(!fopen(path,"r"));}
  f=fopen("ENVARC:NovaDock/Cache/WHDLoad/WHDLOAD_3.idx","r");assert(f);fclose(f);
 }
 f=fopen("whdload-legacy.cfg","w");assert(f);fputs("PLUGIN=Old Games|Plugins/Games|WHDLOAD|METER|ON\nOPTIONDEF=WHDLOAD|DRAWER|Games drawer|TEXT||Games:\nOPTION.WHDLOAD.DRAWER=Old:Collection\n",f);fclose(f);
 assert(nd_load_config(loaded,"whdload-legacy.cfg")==0);assert(!strcmp(nd_option_get(loaded,"WHDLOAD","DRAWER"),"Old:Collection"));assert(!strcmp(nd_option_get(loaded,"WHDLOAD","LABEL"),"Old Games"));assert(nd_plugin_allows_multiple(&loaded->items[find(loaded,"WHDLOAD")]));

 {char path[520];
  f=fopen("custom-icon.info","wb");assert(f);fputs("icon",f);fclose(f);
  assert(nd_collection_art_path("","Demos","custom-icon.info",path,sizeof(path))==2 && !strcmp(path,"custom-icon.info"));
  assert(nd_collection_art_path("","Demos","custom-icon",path,sizeof(path))==2 && !strcmp(path,"custom-icon.info"));
  assert(nd_collection_art_path("","Demos","missing.info",path,sizeof(path))==1 && !strcmp(path,"Icons/Demos.iff"));
  assert(nd_collection_art_path("","Music","missing.info",path,sizeof(path))==1 && !strcmp(path,"Icons/Music.iff"));
  assert(!nd_collection_art_path("","Music","custom-icon.info",path,2));remove("custom-icon.info");
 }
 assert(nd_parse_line(cfg,"PLUGIN=Music Library|Plugins/MusicLibrary|MUSICLIB|METER|ON|YES"));a=find(cfg,"MUSICLIB");assert(a>=0);
 assert(nd_collection_plugin_path(cfg->items[a].command)==2 && nd_plugin_allows_multiple(&cfg->items[a]));
 assert(nd_option_collect(cfg,"MUSICLIB",defs,20)==16);
 assert(!strcmp(nd_option_get(cfg,"MUSICLIB","ICON"),"Music") && !strcmp(nd_option_get(cfg,"MUSICLIB","DRAWER"),""));
 assert(!strcmp(nd_option_get(cfg,"MUSICLIB","SID"),"YES") && !strcmp(nd_option_get(cfg,"MUSICLIB","MP3"),"YES"));
 assert(nd_option_set(cfg,"MUSICLIB","DRAWER","Music:Modules") && nd_option_set(cfg,"MUSICLIB","SID","YES"));
 assert(nd_add_plugin_instance(cfg,a));b=find(cfg,"MUSICLIB_2");assert(b>=0 && !strcmp(nd_option_schema_id(cfg,"MUSICLIB_2"),"MUSICLIB"));
 assert(nd_option_set(cfg,"MUSICLIB_2","LABEL","SID tunes") && nd_option_set(cfg,"MUSICLIB_2","DRAWER","Music:SID"));
 assert(nd_option_set(cfg,"MUSICLIB_2","MOD","NO") && !strcmp(nd_option_get(cfg,"MUSICLIB","MOD"),"YES"));
 assert(nd_manifest_builtin("MUSICLIB",&m) && m.multiple_instances && !strcmp(m.name,"MUSIC") && !strcmp(m.version,"0.1"));
 /* Migrate former default labels, including independent instances, while
  * preserving user labels and all collection settings. */
 assert(nd_option_set(cfg,"MUSICLIB","LABEL","Music Library"));
 assert(nd_add_plugin_instance(cfg,a));assert(find(cfg,"MUSICLIB_3")>=0);
 assert(nd_option_set(cfg,"MUSICLIB_3","LABEL","Music Library"));
 assert(nd_write_config(cfg,"music-library.cfg") && nd_load_config(loaded,"music-library.cfg")==0);
 assert(!strcmp(loaded->items[find(loaded,"MUSICLIB")].label,"MUSIC"));
 assert(!strcmp(nd_option_get(loaded,"MUSICLIB","LABEL"),"MUSIC"));
 assert(!strcmp(loaded->items[find(loaded,"MUSICLIB_3")].label,"MUSIC"));
 assert(!strcmp(nd_option_get(loaded,"MUSICLIB_3","LABEL"),"MUSIC"));
 b=find(loaded,"MUSICLIB_2");assert(b>=0 && !strcmp(loaded->items[b].label,"SID tunes"));
 assert(nd_remove_visible_item(loaded,visible(loaded,b)));assert(find(loaded,"MUSICLIB_2")<0 && !strcmp(nd_option_get(loaded,"MUSICLIB","DRAWER"),"Music:Modules"));
 a=find(loaded,"MUSICLIB");assert(nd_remove_visible_item(loaded,visible(loaded,a)));
 assert(!strcmp(nd_option_get(loaded,"MUSICLIB","DRAWER"),"") && !strcmp(nd_option_get(loaded,"MUSICLIB","SID"),"YES"));
 assert(!strcmp(loaded->items[find(loaded,"MUSICLIB")].label,"MUSIC"));
 assert(nd_parse_line(cfg,"PLUGIN=SENSOR|Plugins/Sensors|SENSORS|METER|ON|NO"));
 assert(nd_manifest_builtin("SENSORS",&m) && m.multiple_instances);
 assert(nd_option_collect(cfg,"SENSORS",defs,20)==18);
 assert(nd_option_set(cfg,"SENSORS","ZZ_TEMP","NO"));
 assert(nd_write_config(cfg,"sensors.cfg") && nd_load_config(loaded,"sensors.cfg")==0);
 assert(!strcmp(nd_option_get(loaded,"SENSORS","ZZ_TEMP"),"NO"));
 assert(!strcmp(nd_option_get(loaded,"SENSORS","RTC_TEMP"),"YES"));
 a=find(loaded,"SENSORS");assert(nd_add_plugin_instance(loaded,a));b=find(loaded,"SENSORS_2");assert(b>=0);
 assert(!strcmp(nd_option_schema_id(loaded,"SENSORS_2"),"SENSORS"));
 assert(nd_option_set(loaded,"SENSORS_2","LABEL","Voltages") && nd_option_set(loaded,"SENSORS_2","ZZ_TEMP","YES"));
 assert(!strcmp(nd_option_get(loaded,"SENSORS","ZZ_TEMP"),"NO") && !strcmp(loaded->items[b].label,"Voltages"));
 assert(nd_remove_visible_item(loaded,visible(loaded,b)) && find(loaded,"SENSORS_2")<0);
 assert(!strcmp(nd_option_get(loaded,"SENSORS","ZZ_TEMP"),"NO"));
 free(cfg);free(loaded);puts("WHDLoad independent labels, folders, saved reload and legacy tests passed.");return 0;}
