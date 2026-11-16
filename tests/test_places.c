#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_places.h"
#include "nd_config.h"
#include "nd_options.h"
static struct NDPlaces p;static struct NDConfig cfg,back;
int main(void){struct NDOptionDef defs[16];int i;char path[80];
 assert(!nd_places_path("")&&!nd_places_path("relative/path")&&!nd_places_path("Work:bad\npath"));assert(nd_places_path("Work:Music"));
 assert(nd_places_add(&p,"Music","Work:Music"));assert(!nd_places_add(&p,"Other","work:music/"));assert(!strcmp(p.items[0].label,"Music"));
 for(i=1;i<32;++i){sprintf(path,"Drive%d:",i);assert(nd_places_add(&p,NULL,path));}assert(!nd_places_add(&p,"Overflow","More:"));
 nd_default_config(&cfg);assert(nd_option_collect(&cfg,"PLACES",defs,16)==12);
 assert(nd_option_set(&cfg,"PLACES","NAME1","My modules")&&nd_option_set(&cfg,"PLACES","BOOKMARK1","Work:Modules"));
 assert(nd_write_config(&cfg,"places.cfg")&&nd_load_config(&back,"places.cfg")==0);
 assert(!strcmp(nd_option_get(&back,"PLACES","BOOKMARK1"),"Work:Modules")&&!strcmp(nd_option_get(&back,"PLACES","NAME1"),"My modules"));
 remove("places.cfg");puts("PLACES: safe absolute paths, duplicate/capacity bounds and permanent bookmarks passed.");return 0;}
