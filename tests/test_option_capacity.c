/* Legacy maximum-sized profiles, compaction-safe aliases and atomic batches. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
static struct NDConfig cfg,back,before;
static void long_value(char *out,int item,int key,int round)
{memset(out,'a'+(item+key+round)%26,ND_OPT_VALUE_LEN-1);sprintf(out,"%02d-%02d-%04d",item,key,round);out[10]='-';out[ND_OPT_VALUE_LEN-1]=0;}
static void fill_append_space(void)
{
 char value[ND_OPT_VALUE_LEN];unsigned int len,free_bytes;
 while((free_bytes=ND_OPTION_POOL_SIZE-cfg.option_used)>=6U){
  len=free_bytes-3U;if(len>ND_OPT_VALUE_LEN-1U)len=ND_OPT_VALUE_LEN-1U;
  memset(value,'x',len);value[len]=0;assert(nd_option_set(&cfg,"WEATHER","LOCATION",value));
 }
}
static void legacy_capacity(void)
{
 static const char *keys[]={"DEVICE","AREA","ENTITY","ICONFILE","DEVICE_NAME","ENTITY_NAME","DEVICE_CLASS","ICON"};
 struct NDOptionDef defs[2];char values[2][ND_OPT_VALUE_LEN],text[ND_OPT_VALUE_LEN];
 FILE *f;int i,j,round;
 assert(ND_OPTION_POOL_SIZE>=1UL+ND_MAX_OPTIONS*(unsigned long)ND_OPT_VALUE_LEN);
 f=fopen("legacy-options.cfg","w");assert(f);
 for(i=0;i<ND_MAX_ITEMS;++i){
  fprintf(f,"PLUGIN=Room%d|Plugins/HASS|HASS_%d|METER|OFF\n",i,i);
  for(j=0;j<8;++j){long_value(text,i,j,0);fprintf(f,"OPTION.HASS_%d.%s=%s\n",i,keys[j],text);}
 }
 assert(!fclose(f));assert(!nd_load_config(&cfg,"legacy-options.cfg"));
 assert(cfg.count==ND_MAX_ITEMS && cfg.option_count==ND_MAX_OPTIONS);
 assert(cfg.option_used==ND_OPTION_POOL_SIZE);
 for(i=0;i<ND_MAX_ITEMS;++i)for(j=0;j<8;++j){char id[20];sprintf(id,"HASS_%d",i);long_value(text,i,j,0);assert(!strcmp(nd_option_get(&cfg,id,keys[j]),text));}
 /* A full table still permits same-sized replacements and multi-key Save. */
 for(round=1;round<=100;++round){long_value(text,0,0,round);assert(nd_option_set(&cfg,"HASS_0","DEVICE",text));assert(!strcmp(nd_option_get(&cfg,"HASS_0","DEVICE"),text));}
 memset(defs,0,sizeof(defs));
 for(j=0;j<2;++j){strcpy(defs[j].id,"HASS_0");strcpy(defs[j].key,keys[j]);strcpy(defs[j].type,"TEXT");long_value(values[j],0,j,200);}
 assert(nd_option_apply(&cfg,"HASS_0",defs,(const char (*)[ND_OPT_VALUE_LEN])values,2)==1);
 for(j=0;j<2;++j)assert(!strcmp(nd_option_get(&cfg,"HASS_0",keys[j]),values[j]));
 assert(nd_write_config(&cfg,"legacy-options-roundtrip.cfg"));assert(!nd_load_config(&back,"legacy-options-roundtrip.cfg"));
 assert(nd_config_runtime_equal(&cfg,&back));
 /* Duplicated instances may share offsets; replacing one must keep its twin. */
 cfg.options[8].value=cfg.options[0].value;nd_option_pool_compact(&cfg);
 strcpy(text,nd_option_get(&cfg,"HASS_1","DEVICE"));
 long_value(values[0],0,0,300);assert(nd_option_set(&cfg,"HASS_0","DEVICE",values[0]));
 assert(!strcmp(nd_option_get(&cfg,"HASS_1","DEVICE"),text));
 remove("legacy-options.cfg");remove("legacy-options-roundtrip.cfg");remove("legacy-options-roundtrip.cfg.bak");
}
static void alias_and_rollback(void)
{
 struct NDOptionDef defs[2];char values[2][ND_OPT_VALUE_LEN];const char *value;char line[128];int i;
 nd_default_config(&cfg);
 strcpy(line,"PLUGIN=Games|Plugins/Games|WHDLOAD|METER|ON");assert(nd_parse_line(&cfg,line));
 strcpy(line,"PLUGIN=Music|Plugins/MusicLibrary|MUSICLIB|METER|OFF");assert(nd_parse_line(&cfg,line));
 assert(nd_option_set(&cfg,"WHDLOAD","LABEL","First"));assert(nd_option_set(&cfg,"WHDLOAD","LABEL","Wanted"));
 value=nd_option_get(&cfg,"WHDLOAD","LABEL");fill_append_space();
 assert(nd_option_set(&cfg,"MUSICLIB","LABEL",value));
 assert(!strcmp(nd_option_get(&cfg,"MUSICLIB","LABEL"),"WANTED"));
 for(i=0;i<cfg.count;++i)if(!strcmp(cfg.items[i].id,"MUSICLIB"))assert(!strcmp(cfg.items[i].label,"WANTED"));
 /* An invalid collection icon is rejected after an earlier valid edit has
  * compacted the pool. Roll back values, labels and untouched options. */
 assert(nd_option_set(&cfg,"WHDLOAD","ICON","WHDLoad"));fill_append_space();before=cfg;
 memset(defs,0,sizeof(defs));strcpy(defs[0].id,"WHDLOAD");strcpy(defs[0].key,"LABEL");strcpy(defs[0].type,"TEXT");
 defs[1]=defs[0];strcpy(defs[1].key,"ICON");strcpy(values[0],"After");strcpy(values[1],"NoSuchIcon");
 assert(nd_option_apply(&cfg,"WHDLOAD",defs,(const char (*)[ND_OPT_VALUE_LEN])values,2)==0);
 assert(nd_config_runtime_equal(&cfg,&before));
 for(i=0;i<cfg.count;++i)assert(!strcmp(cfg.items[i].label,before.items[i].label));
 strcpy(values[1],"Joystick");assert(nd_option_apply(&cfg,"WHDLOAD",defs,(const char (*)[ND_OPT_VALUE_LEN])values,2)==1);
 assert(!strcmp(nd_option_get(&cfg,"WHDLOAD","LABEL"),"AFTER"));
}
int main(void){legacy_capacity();alias_and_rollback();puts("Option compatibility: all 512 maximum-length values load/save/edit, aliases preserve labels, shared offsets and compacted-batch rollback passed.");return 0;}
