#!/usr/bin/env python3
"""Option lookup retains validation/override behavior while stopping at a match."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
REFERENCE=r'''
/* Values are stored through the option pool (option_store), as in config.c. */
static int reference_option_set(struct NDConfig *cfg, const char *id, const char *key,
                  const char *value)
{
    const struct NDOptionDef *built, *spec=NULL;
    const char *schema;
    int i,n;
    if (!cfg || !id || !key || !value ||
        !opt_token(id,20) || !opt_token(key,ND_OPT_KEY_LEN)) return 0;
    built=nd_builtin_options(&n);
    schema=nd_option_schema_id(cfg,id);
    for (i=0; i<n; ++i)
        if (!strcmp(built[i].id,schema) && !strcmp(built[i].key,key)) spec=&built[i];
    for (i=0; i<cfg->def_count; ++i)
        if (!strcmp(cfg->defs[i].id,id) && !strcmp(cfg->defs[i].key,key))
            spec=&cfg->defs[i];
    if(!spec && !strcmp(schema,"SENSORS") && nd_i2c_key(key)){
        static const struct NDOptionDef sensor_id={"SENSORS","","I2C sensor","BOOL","","YES"};spec=&sensor_id;
    }
    if (!spec || !nd_option_validate(spec,value)) return 0;
    if((!strcmp(schema,"WHDLOAD") || !strcmp(schema,"MUSICLIB") || !strcmp(schema,"ACTIONS") || !strcmp(schema,"SHELF")) && !strcmp(key,"ICON") &&
       nd_collection_icon_index(value)<0 && strcmp(value,"Custom") && strcmp(value,"Text"))return 0;
    if((!strcmp(schema,"WHDLOAD") || !strcmp(schema,"MUSICLIB") || !strcmp(schema,"SENSORS") || !strcmp(schema,"ACTIONS") || !strcmp(schema,"HASS") || !strcmp(schema,"WATCH") || !strcmp(schema,"LOGS") || !strcmp(schema,"SHELF")) && !strcmp(key,"LABEL")){
        if(!*value || strlen(value)>=ND_LABEL_LEN)return 0;
    }
    for (i=0; i<cfg->option_count; ++i) {
        if (!strcmp(cfg->options[i].id,id) && !strcmp(cfg->options[i].key,key)) {
            if (!option_store(cfg,i,value)) return 0;
            if(!strcmp(id,"NET") && !strcmp(key,"VIEW"))apply_network_view(cfg,value);
            if((!strcmp(schema,"WHDLOAD") || !strcmp(schema,"MUSICLIB") || !strcmp(schema,"SENSORS") || !strcmp(schema,"ACTIONS") || !strcmp(schema,"HASS") || !strcmp(schema,"WATCH") || !strcmp(schema,"LOGS") || !strcmp(schema,"SHELF")) && !strcmp(key,"LABEL")){
                int j;for(j=0;j<cfg->count;++j)if(cfg->items[j].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[j].id,id))
                    if(strcmp(cfg->items[j].label,value))copy_string(cfg->items[j].label,value,sizeof(cfg->items[j].label));
            }
            return 1;
        }
    }
    if (cfg->option_count >= ND_MAX_OPTIONS) return 0;
    i=cfg->option_count++;
    copy_string(cfg->options[i].id,id,sizeof(cfg->options[i].id));
    copy_string(cfg->options[i].key,key,sizeof(cfg->options[i].key));
    cfg->options[i].value=0;
    if (!option_store(cfg,i,value)) {
        memset(&cfg->options[i],0,sizeof(cfg->options[i]));--cfg->option_count;return 0;
    }
    if(!strcmp(id,"NET") && !strcmp(key,"VIEW"))apply_network_view(cfg,value);
    if((!strcmp(schema,"WHDLOAD") || !strcmp(schema,"MUSICLIB") || !strcmp(schema,"SENSORS") || !strcmp(schema,"ACTIONS") || !strcmp(schema,"HASS") || !strcmp(schema,"WATCH") || !strcmp(schema,"LOGS") || !strcmp(schema,"SHELF")) && !strcmp(key,"LABEL")){
                int j;for(j=0;j<cfg->count;++j)if(cfg->items[j].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[j].id,id))
                    if(strcmp(cfg->items[j].label,value))copy_string(cfg->items[j].label,value,sizeof(cfg->items[j].label));
            }
            return 1;
}'''
code='#include <assert.h>\n#include <stdio.h>\n#include "'+str(ROOT/'src/config.c')+'"\n'+REFERENCE+r'''
int main(void){static struct NDConfig a,b;const struct NDOptionDef *defs;int count,i,j,new_result,old_result;
 defs=nd_builtin_options(&count);assert(count>100);
 for(i=0;i<count;++i){
  for(j=0;j<i;++j)assert(strcmp(defs[i].id,defs[j].id)||strcmp(defs[i].key,defs[j].key));
  nd_default_config(&a);b=a;new_result=nd_option_set(&a,defs[i].id,defs[i].key,defs[i].default_value);old_result=reference_option_set(&b,defs[i].id,defs[i].key,defs[i].default_value);
  assert(new_result==old_result&&!memcmp(&a,&b,sizeof(a)));
  new_result=nd_option_set(&a,defs[i].id,defs[i].key,"bad|value");old_result=reference_option_set(&b,defs[i].id,defs[i].key,"bad|value");assert(new_result==old_result&&!memcmp(&a,&b,sizeof(a)));
 }
 nd_default_config(&a);a.def_count=1;memset(&a.defs[0],0,sizeof(a.defs[0]));strcpy(a.defs[0].id,"CPU");strcpy(a.defs[0].key,"POLL_TICKS");strcpy(a.defs[0].type,"UINT");strcpy(a.defs[0].choices,"1,100");strcpy(a.defs[0].default_value,"17");b=a;
 assert(nd_option_set(&a,"CPU","POLL_TICKS","17")==reference_option_set(&b,"CPU","POLL_TICKS","17"));assert(!memcmp(&a,&b,sizeof(a)));
 assert(nd_option_set(&a,"CPU","POLL_TICKS","200")==reference_option_set(&b,"CPU","POLL_TICKS","200"));assert(!memcmp(&a,&b,sizeof(a)));
 printf("All %d built-in option defaults and invalid inputs match the previous lookup; schemas are unique and explicit overrides remain authoritative.\n",count);return 0;}
'''
with tempfile.TemporaryDirectory(prefix='novadock-options-') as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'test.c'),str(ROOT/'src/nd_options.c'),str(ROOT/'src/nd_manifest.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],check=True)
