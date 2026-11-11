#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_option_runtime.h"
int main(void){struct NDConfig *c=malloc(sizeof(*c));assert(c);
 nd_default_config(c);c->side=ND_SIDE_LEFT;
 assert(nd_write_config(c,"PROGDIR:novadock.cfg"));
 assert(nd_load_effective(c)==0 && c->side==ND_SIDE_LEFT);
 assert(!strcmp(nd_effective_config_source(),"PROGDIR:novadock.cfg"));
 assert(nd_write_config(c,"ENV:NovaDock/novadock.cfg"));c->side=ND_SIDE_RIGHT;
 assert(nd_write_config(c,"PROGDIR:novadock.cfg"));
 assert(nd_load_effective(c)==0 && c->side==ND_SIDE_LEFT);
 assert(nd_config_low_resource(c));
 assert(c->refresh_mode==ND_REFRESH_LOW_MEMORY && c->folder_view==1);
 assert(!c->running_apps && !c->always_on_top);
 assert(!strcmp(nd_option_get(c,"CPU","POLL_TICKS"),"100"));
 {
  struct NDOptionString fields[2];char hour[8],seconds[8];FILE *f;
  fields[0].key="HOUR_MODE";fields[0].fallback="24";fields[0].out=hour;fields[0].limit=sizeof(hour);
  fields[1].key="SECONDS";fields[1].fallback="NO";fields[1].out=seconds;fields[1].limit=sizeof(seconds);
  f=fopen("ENV:NovaDock/novadock.cfg","w");assert(f);
  fputs("OPTION.CLOCK.HOUR_MODE=12\nOPTION.CLOCK.SECONDS=YES\n",f);fclose(f);
  assert(nd_plugin_option_strings("CLOCK",fields,2));assert(!strcmp(hour,"12") && !strcmp(seconds,"YES"));
  f=fopen("ENV:NovaDock/novadock.cfg","w");assert(f);
  fputs("OPTION.CLOCK.HOUR_MODE=12\nOPTION.CLOCK.SECONDS=oversizedvalue\n",f);fclose(f);
  assert(!nd_plugin_option_strings("CLOCK",fields,2));assert(!strcmp(hour,"24") && !strcmp(seconds,"NO"));
 }
 nd_default_config(c);c->option_count=ND_MAX_OPTIONS;
 assert(!nd_config_low_resource(c));assert(c->folder_view==0 && c->running_apps==1);
 free(c);puts("Packaged fallback and ENV precedence passed.");return 0;}
