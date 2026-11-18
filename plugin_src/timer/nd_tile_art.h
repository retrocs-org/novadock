#ifndef ND_TIMER_TILE_ART_H
#define ND_TIMER_TILE_ART_H
/* Plugin-owned tile authoring shapes; compiled into its artwork bank. */
static void nd_timer_tile_art(void *ctx,void (*box)(void *,int,int,int,int,int),int state)
{
    static const unsigned char widths[24]={10,16,18,20,22,22,24,24,24,24,24,24,
        24,24,24,24,24,24,22,22,20,18,16,10};
int light=3,i;
box(ctx,1,12,0,10,5);
    box(ctx,light,13,0,8,2);
    box(ctx,4,23,4,5,4);
    for(i=0;i<24;++i){
        box(ctx,1,(32-widths[i])/2+1,i+7,widths[i],1);
        box(ctx,2,(32-widths[i])/2,i+6,widths[i],1);
        if(i>2 && i<21)box(ctx,light,8,i+6,16,1);
    }
    if(state==1){
        box(ctx,4,12,13,3,10);
        box(ctx,4,18,13,3,10);
    }else if(state==2){
        box(ctx,5,15,11,3,10);
        box(ctx,5,15,24,3,2);
    }else{
        box(ctx,1,15,10,2,9);
        box(ctx,4,15,17,7,2);
        box(ctx,5,14,16,4,4);
    }
}
#endif
