#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
static struct NDConfig cfg,before,back;
static struct NDOptionDef defs[8],wdefs[8];static char values[8][ND_OPT_VALUE_LEN],wvalues[8][ND_OPT_VALUE_LEN];
/* An unschema'd saved option, as an older profile may carry.  The parser
 * refuses those, so write the record directly; all share one pooled "keep". */
static void add_saved(struct NDConfig *c,const char *id)
{
 static unsigned short keep;unsigned int used=c->option_used ? c->option_used:1U;int n=c->option_count;
 if(!keep || keep>=used || strcmp(c->option_pool+keep,"keep")){memcpy(c->option_pool+used,"keep",5);keep=(unsigned short)used;c->option_used=used+5U;}
 assert(n<ND_MAX_OPTIONS);strcpy(c->options[n].id,id);strcpy(c->options[n].key,"SAVED");c->options[n].value=keep;c->option_count=n+1;
}
int main(int argc,char **argv)
{
 int i,n,old;
 nd_default_config(&cfg);n=nd_option_collect(&cfg,"WEATHER",defs,8);assert(n==5);
 for(i=0;i<n;++i)strcpy(values[i],nd_option_get(&cfg,"WEATHER",defs[i].key));
 old=cfg.option_count;assert(nd_option_apply(&cfg,"WEATHER",defs,(const char (*)[ND_OPT_VALUE_LEN])values,n)==1);assert(cfg.option_count==old);
 /* Reproduce an existing profile near the old 128-value limit. */
 for(i=cfg.option_count;i<127;++i){char id[20];sprintf(id,"TEST%d",i);add_saved(&cfg,id);}assert(cfg.option_count==127);
 for(i=0;i<n;++i){if(!strcmp(defs[i].key,"LOCATION"))strcpy(values[i],"Stockholm, Sweden");if(!strcmp(defs[i].key,"PROGRAM"))strcpy(values[i],"C:curl");if(!strcmp(defs[i].key,"ENGINE"))strcpy(values[i],"CURL");}
 assert(nd_option_apply(&cfg,"WEATHER",defs,(const char (*)[ND_OPT_VALUE_LEN])values,n)==1);assert(cfg.option_count>127);
 before=cfg;strcpy(values[n-1],"not valid");assert(nd_option_apply(&cfg,"WEATHER",defs,(const char (*)[ND_OPT_VALUE_LEN])values,n)==0);assert(!memcmp(&before,&cfg,sizeof(cfg)));
 for(i=0;i<n;++i)strcpy(values[i],nd_option_get(&cfg,"WEATHER",defs[i].key));
 /* A full table permits an existing-key edit but never a partial new batch. */
 for(i=cfg.option_count;i<ND_MAX_OPTIONS;++i){char id[20];sprintf(id,"TEST%d",i);add_saved(&cfg,id);}assert(cfg.option_count==ND_MAX_OPTIONS);
 for(i=0;i<n;++i){if(!strcmp(defs[i].key,"LOCATION"))strcpy(values[i],"Gothenburg, Sweden");if(!strcmp(defs[i].key,"REFRESH_MINUTES"))strcpy(values[i],"45");}
 before=cfg;assert(nd_option_apply(&cfg,"WEATHER",defs,(const char (*)[ND_OPT_VALUE_LEN])values,n)==-1);assert(!memcmp(&before,&cfg,sizeof(cfg)));
 for(i=0;i<n;++i)strcpy(values[i],nd_option_get(&cfg,"WEATHER",defs[i].key));
 for(i=0;i<n;++i)if(!strcmp(defs[i].key,"LOCATION"))strcpy(values[i],"Gothenburg, Sweden");
 assert(nd_option_apply(&cfg,"WEATHER",defs,(const char (*)[ND_OPT_VALUE_LEN])values,n)==1);
 /* A value can pass its type check yet be refused by nd_option_set (here a
  * collection ICON that names no built-in icon).  Earlier values in the batch
  * must be rolled back, byte for byte. */
 {int w;nd_default_config(&cfg);w=nd_option_collect(&cfg,"WHDLOAD",wdefs,8);assert(w==5);
 for(i=0;i<w;++i)strcpy(wvalues[i],nd_option_get(&cfg,"WHDLOAD",wdefs[i].key));
 for(i=0;i<w;++i){if(!strcmp(wdefs[i].key,"LABEL"))strcpy(wvalues[i],"Games");if(!strcmp(wdefs[i].key,"DRAWER"))strcpy(wvalues[i],"Games:WHD");if(!strcmp(wdefs[i].key,"ICON"))strcpy(wvalues[i],"NoSuchIcon");}
 before=cfg;assert(nd_option_apply(&cfg,"WHDLOAD",wdefs,(const char (*)[ND_OPT_VALUE_LEN])wvalues,w)==0);
 assert(!memcmp(&before,&cfg,sizeof(cfg)));
 for(i=0;i<w;++i)if(!strcmp(wdefs[i].key,"ICON"))strcpy(wvalues[i],"Joystick");
 assert(nd_option_apply(&cfg,"WHDLOAD",wdefs,(const char (*)[ND_OPT_VALUE_LEN])wvalues,w)==1);
 assert(!strcmp(nd_option_get(&cfg,"WHDLOAD","LABEL"),"GAMES"));}
 if(argc>1){
  assert(nd_load_config(&cfg,argv[1])==0);before=cfg;old=cfg.option_count;
  for(i=0;i<n;++i)strcpy(values[i],nd_option_get(&cfg,"WEATHER",defs[i].key));
  for(i=0;i<n;++i){if(!strcmp(defs[i].key,"LOCATION"))strcpy(values[i],"Stockholm, Sweden");if(!strcmp(defs[i].key,"PROGRAM"))strcpy(values[i],"C:curl");if(!strcmp(defs[i].key,"ENGINE"))strcpy(values[i],"CURL");}
  assert(nd_option_apply(&cfg,"WEATHER",defs,(const char (*)[ND_OPT_VALUE_LEN])values,n)==1);
  for(i=0;i<old;++i)if(strcmp(before.options[i].id,"WEATHER"))assert(!strcmp(cfg.options[i].id,before.options[i].id) && !strcmp(cfg.options[i].key,before.options[i].key) && !strcmp(nd_option_value(&cfg,&cfg.options[i]),nd_option_value(&before,&before.options[i])));
  assert(nd_write_config(&cfg,"weather-save.cfg"));assert(nd_load_config(&back,"weather-save.cfg")==0);
  assert(!strcmp(nd_option_get(&back,"WEATHER","LOCATION"),"Stockholm, Sweden"));assert(!strcmp(nd_option_get(&back,"WEATHER","PROGRAM"),"C:curl"));
  remove("weather-save.cfg");
 }
 puts("Settings batches: sparse defaults, large profiles, full-table edits and atomic failure passed.");return 0;
}
