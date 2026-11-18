#ifndef ND_NOTES_TILE_ART_H
#define ND_NOTES_TILE_ART_H
/* Plugin-owned tile authoring shapes; compiled into its artwork bank. */
static void nd_notes_tile_art(void *ctx,void (*box)(void *,int,int,int,int,int),int state)
{
int paper=3,i;
box(ctx,1,7,4,22,27);
    box(ctx,2,4,2,23,27);
    box(ctx,paper,6,4,19,23);
    box(ctx,4,6,4,3,23);
    for(i=10;i<24;i+=4)box(ctx,2,11,i,11,1);
    for(i=9;i<25;i+=5){
        box(ctx,1,i,1,2,5);
        box(ctx,4,i,1,1,4);
    }
}
#endif
