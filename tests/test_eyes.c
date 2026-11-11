/* Native eyes geometry and unified plugin settings, portable C89. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_eyes.h"
#include "nd_config.h"
#include "nd_options.h"
int main(void)
{
    struct NDConfig *cfg;
    struct NDOptionDef defs[16];
    int x,y,i,n,eyes=0;
    const char *file="tests/eyes-roundtrip.cfg";
    nd_eyes_pupil(100,100,100,100,12,14,100,&x,&y);
    assert(x==100 && y==100);
    nd_eyes_pupil(100,100,1000,100,12,14,100,&x,&y);
    assert(x>100 && x<=112 && y==100);
    nd_eyes_pupil(100,100,-1000,100,12,14,100,&x,&y);
    assert(x>=88 && x<100 && y==100);
    nd_eyes_pupil(100,100,100,1000,12,14,100,&x,&y);
    assert(x==100 && y>100 && y<=114);
    nd_eyes_pupil(100,100,900,900,12,14,100,&x,&y);
    assert(x>100 && x<112 && y>100 && y<114);
    nd_eyes_pupil(100,100,900,900,12,14,10,&x,&y);
    assert(x>=100 && x<103 && y>=100 && y<103);
    assert(nd_eyes_moved(2,4,2,5));
    assert(!nd_eyes_moved(2,4,2,4));
    cfg=(struct NDConfig *)calloc(1,sizeof(*cfg));
    assert(cfg!=NULL);
    nd_default_config(cfg);
    assert(cfg->count==11);
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id,"EYES")) ++eyes;
    assert(eyes==1);
    assert(cfg->items[7].multiple_instances);
    assert(nd_add_plugin_instance(cfg,7));
    eyes=0;
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id,"EYES")) ++eyes;
    assert(eyes==2);
    n=nd_option_collect(cfg,"EYES",defs,16);
    assert(n==7);
    assert(!strcmp(nd_option_get(cfg,"EYES","SIZE"),"NORMAL"));
    assert(!nd_option_set(cfg,"EYES","SIZE","HUGE"));
    assert(!nd_option_set(cfg,"EYES","POLL_TICKS","3"));
    assert(nd_option_set(cfg,"EYES","SIZE","LARGE"));
    assert(nd_option_set(cfg,"EYES","BLINK","NO"));
    assert(nd_option_set(cfg,"EYES","SENSITIVITY","25"));
    assert(nd_write_config(cfg,file));
    assert(nd_load_config(cfg,file)==0);
    eyes=0;
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id,"EYES")) {
            ++eyes;
            assert(cfg->items[i].multiple_instances);
        }
    assert(eyes==2);
    assert(!strcmp(nd_option_get(cfg,"EYES","SIZE"),"LARGE"));
    assert(!strcmp(nd_option_get(cfg,"EYES","BLINK"),"NO"));
    assert(!strcmp(nd_option_get(cfg,"EYES","SENSITIVITY"),"25"));
    remove(file);
    free(cfg);
    puts("Eyes geometry, preferences UI schema, config round-trip passed.");
    return 0;
}
