/* nd_layout_fit_enabled() must give exactly what fitting a compacted copy
 * gives, for any mix of enabled/disabled items, rows, sides and sizes. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_layout.h"

static struct NDConfig cfg, copy;

static unsigned long seed=12345UL;
static int rnd(int n) { seed=seed*1103515245UL+12345UL; return (int)((seed>>16)%(unsigned long)n); }

static void random_config(void)
{
    int i;
    memset(&cfg,0,sizeof(cfg));
    cfg.count=1+rnd(ND_MAX_ITEMS);
    cfg.side=rnd(4);
    cfg.item_size=rnd(ND_ITEM_SIZE_COUNT);
    cfg.gap=rnd(10);
    cfg.tile_width=55+rnd(40);
    cfg.tile_height=46+rnd(40);
    cfg.rows=ND_ROWS_MIN+rnd(ND_ROWS_MAX);
    cfg.row_layout=rnd(2);
    for (i=0;i<ND_ROWS_MAX;++i) cfg.row_counts[i]=rnd(30)-3;
    for (i=0;i<cfg.count;++i) {
        struct NDItem *it=&cfg.items[i];
        int k=rnd(10);
        it->kind=k<5 ? ND_ITEM_LAUNCH : (k<7 ? ND_ITEM_PLUGIN :
                 (k<9 ? ND_ITEM_SPACER : ND_ITEM_DRAWER));
        it->enabled=rnd(4)!=0;
        if (it->kind==ND_ITEM_PLUGIN) strcpy(it->id,rnd(3) ? "CLOCK" : "MUSIC");
        if (it->kind==ND_ITEM_SPACER) it->spacer_width=rnd(3) ? 0 : rnd(80);
    }
}

int main(void)
{
    static const int screens[][3]={{320,200,11},{640,256,11},{640,512,11},
                                   {800,600,15},{1280,1024,20},{160,120,11}};
    int n,s,checked=0,fitted=0;
    for (n=0;n<20000;++n) {
        random_config();
        for (s=0;s<6;++s) {
            struct NDLayout a,b;
            int ra,rb;
            memset(&a,0x5a,sizeof(a));memset(&b,0x5a,sizeof(b));
            copy=cfg;
            nd_compact_enabled(&copy);
            ra=nd_layout_fit(&copy,screens[s][0],screens[s][1],screens[s][2],&a);
            rb=nd_layout_fit_enabled(&cfg,screens[s][0],screens[s][1],screens[s][2],&b);
            if (ra!=rb || (ra && memcmp(&a,&b,sizeof(a)))) {
                fprintf(stderr,"mismatch at case %d screen %d: %d vs %d\n",n,s,ra,rb);
                return 1;
            }
            ++checked; if (ra) ++fitted;
        }
    }
    printf("Visible-dock fit without a working copy matches compacted fit (%d cases, %d fit).\n",
           checked,fitted);
    return 0;
}
