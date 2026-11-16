#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_seasonal.h"
static struct NDConfig cfg,back;
int main(void){int i,month;FILE *f;const char *history="Christmas.history";
 remove(history);nd_default_config(&cfg);
 for(month=1;month<=11;++month)assert(!nd_christmas_eligible(&cfg,month,15,1,history));
 assert(!nd_christmas_eligible(&cfg,12,15,0,history));assert(nd_christmas_eligible(&cfg,12,15,1,history));
 assert(!nd_christmas_eligible(&cfg,12,9,1,history));assert(nd_christmas_eligible(&cfg,12,10,1,history));assert(nd_christmas_eligible(&cfg,12,25,1,history));assert(!nd_christmas_eligible(&cfg,12,26,1,history));
 assert(nd_christmas_enable(&cfg));i=nd_christmas_index(&cfg);assert(i>=0&&cfg.items[i].enabled);assert(!nd_christmas_eligible(&cfg,12,15,1,history));
 assert(nd_christmas_remember(history));assert(nd_christmas_seen(history));assert(nd_christmas_remember(history));
 assert(nd_set_item_enabled(&cfg,i,0));assert(!nd_christmas_eligible(&cfg,12,15,1,history));
 assert(nd_write_config(&cfg,"seasonal.cfg")&&nd_load_config(&back,"seasonal.cfg")==0);assert(!nd_christmas_eligible(&back,12,15,1,history));
 f=fopen(history,"wb");assert(f);assert(fputs("damaged",f)>=0);fclose(f);assert(!nd_christmas_eligible(&cfg,12,15,1,history));
 assert(!nd_christmas_remember("absent/directory/history"));
 remove(history);nd_default_config(&cfg);cfg.count=ND_MAX_ITEMS;assert(!nd_christmas_eligible(&cfg,12,15,1,history));
 /* A seasonal record suppresses another introduction in the same season. */
 nd_default_config(&cfg);assert(nd_christmas_enable(&cfg));assert(nd_christmas_remember(history));assert(nd_set_item_enabled(&cfg,nd_christmas_index(&cfg),0));assert(!nd_christmas_eligible(&cfg,12,15,1,history));
 remove(history);remove("seasonal.cfg");puts("Christmas introduction: December 10-25 boundaries, seasonal history, removal/restart, damaged history and full dock passed.");return 0;}
