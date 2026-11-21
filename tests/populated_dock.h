#ifndef ND_TEST_POPULATED_DOCK_H
#define ND_TEST_POPULATED_DOCK_H
#include <string.h>
static void populated_dock(struct NDConfig *cfg)
{
    int i;
    struct NDItem music,trash;
    nd_default_config(cfg);
    /* Keep this historical paging fixture independent of the factory order. */
    trash=cfg->items[3];
    for (i=3;i<cfg->count-1;++i) cfg->items[i]=cfg->items[i+1];
    cfg->items[cfg->count-1]=trash;
    cfg->count=11; /* omit Trash and newly bundled optional tools */
    /* A test-only extra meter keeps the paging fixture independent of the
     * number of retired or newly bundled optional plugins. */
    cfg->items[cfg->count]=cfg->items[3];
    strcpy(cfg->items[cfg->count].id,"TESTMETER");
    ++cfg->count;
    /* Keep the populated paging fixture's wide music tile in cell seven. */
    music=cfg->items[2];
    for (i=2;i<7;++i) cfg->items[i]=cfg->items[i+1];
    cfg->items[7]=music;
    for (i=0;i<cfg->count;++i)
        cfg->items[i].enabled=(i!=9 && i!=11);
}
#endif
