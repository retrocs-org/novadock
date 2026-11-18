#ifndef ND_TRASH_TILE_ART_H
#define ND_TRASH_TILE_ART_H
/* Plugin-owned tile authoring shapes; compiled into its artwork bank. */
static void nd_trash_tile_art(void *ctx,void (*box)(void *,int,int,int,int,int),int state)
{
int full=state!=0,light=3,rib;
box(ctx,1,8,12,19,19);
    box(ctx,1,6,10,21,17);
    box(ctx,2,8,12,17,16);
    box(ctx,light,8,12,2,14);
    box(ctx,2,10,27,13,2);
    for (rib=12;rib<=20;rib+=4) {
        box(ctx,1,rib,15,2,10);
        box(ctx,light,rib,15,1,9);
    }
    if (full) {
        /* Two sheets rising above the rim make the full state distinct
         * even on a two-colour screen, independently of the count badge. */
        box(ctx,1,7,2,10,11);
        box(ctx,light,8,3,8,9);
        box(ctx,4,10,5,4,1);
        box(ctx,1,16,5,9,8);
        box(ctx,light,17,6,7,6);
        box(ctx,4,18,8,4,1);
        box(ctx,1,5,11,23,3);
        box(ctx,light,6,11,21,1);
    } else {
        /* A fitted lid and handle indicate the empty state. */
        box(ctx,1,12,4,9,5);
        box(ctx,light,13,5,7,2);
        box(ctx,1,4,8,25,5);
        box(ctx,light,5,9,23,2);
        box(ctx,2,6,11,21,1);
    }
}
#endif
