#ifndef ND_PLACES_SAVER_ART_H
#define ND_PLACES_SAVER_ART_H
typedef void (*NDUtilityRect)(void *,int,int,int,int,int);
static void nd_places_art(void *ctx,NDUtilityRect b)
{
 b(ctx,0,2,8,28,21);b(ctx,1,3,9,26,19);b(ctx,3,4,6,10,5);b(ctx,3,4,11,24,15);b(ctx,2,5,12,21,2);
 b(ctx,0,17,4,10,10);b(ctx,4,18,5,8,8);b(ctx,4,19,13,6,2);b(ctx,4,20,15,4,2);b(ctx,4,21,17,2,2);b(ctx,0,21,8,2,2);
}
static void nd_saver_art(void *ctx,NDUtilityRect b)
{
 b(ctx,0,2,4,28,23);b(ctx,1,3,5,26,20);b(ctx,2,4,6,24,17);b(ctx,0,6,8,20,13);b(ctx,1,13,26,6,3);b(ctx,2,9,29,14,2);
 b(ctx,3,19,10,4,7);b(ctx,3,16,12,7,5);b(ctx,0,18,9,4,6);b(ctx,2,9,10,1,3);b(ctx,2,8,11,3,1);b(ctx,4,12,17,2,2);
}
#endif
