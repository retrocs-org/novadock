#ifndef ND_ARCHIVE_TILE_ART_H
#define ND_ARCHIVE_TILE_ART_H
/* Plugin-owned tile authoring shapes; compiled into its artwork bank. */
static void nd_archive_tile_art(void *ctx,void (*box)(void *,int,int,int,int,int),int state)
{
int packing=state==1,unpacking=state==2,both=state==3,metal=3,i;
box(ctx,1,3,7,26,24);
    box(ctx,2,5,9,22,20);
    box(ctx,4,5,9,2,18);
    box(ctx,metal,4,4,24,6);
    box(ctx,2,5,5,22,3);
    if(packing || unpacking || both){
        int up=packing || both;
        box(ctx,1,10,10,13,17);
        box(ctx,5,15,12,3,13);
        for(i=0;i<5;++i)
            box(ctx,5,11+i,up ? 15-i:20+i,11-2*i,1);
        if(both)for(i=0;i<4;++i)
            box(ctx,4,12+i,21+i,9-2*i,1);
    } else {
        box(ctx,1,14,4,5,24);
        for(i=5;i<26;i+=4){
            box(ctx,metal,14,i,3,2);
            box(ctx,4,16,i+2,3,2);
        }
        box(ctx,metal,13,17,7,5);
        box(ctx,1,15,18,3,2);
    }
}
#endif
