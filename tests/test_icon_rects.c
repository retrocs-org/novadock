/* nd_icon_rects(): exact, non-overlapping coverage for every bundled
 * collection icon, plus synthetic edge cases. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_ilbm.h"
#include "nd_collection_icons.h"
#include "nd_icon_rects.h"
static struct NDILBM image;
static long px[1024];
static struct NDIconRect rects[1024];
static void check(void)
{
    static int cover[1024];
    int n=nd_icon_rects(px,rects,1024),i,x,y;
    assert(n>=0 && n<=1024 && nd_icon_rects(px,NULL,0)==n);
    memset(cover,0,sizeof(cover));
    for (i=0;i<n;++i) {
        assert(rects[i].x0<rects[i].x1 && rects[i].x1<=32);
        assert(rects[i].y0<rects[i].y1 && rects[i].y1<=32);
        for (y=rects[i].y0;y<rects[i].y1;++y)
            for (x=rects[i].x0;x<rects[i].x1;++x) {
                assert(px[y*32+x]==(long)rects[i].pen);
                ++cover[y*32+x];
            }
    }
    for (i=0;i<1024;++i) assert(cover[i]==(px[i]==ND_ICON_RECT_CLEAR ? 0 : 1));
}
int main(void)
{
    int i,x,y,k,loaded=0;
    long runs=0,total=0;
    char path[300];
    for (i=0;i<1024;++i) px[i]=ND_ICON_RECT_CLEAR;
    check();
    assert(nd_icon_rects(px,rects,1024)==0);
    for (i=0;i<1024;++i) px[i]=7;
    check();
    assert(nd_icon_rects(px,rects,1024)==1);
    for (i=0;i<1024;++i) px[i]=(i+i/32)&1;           /* checkerboard: worst case */
    check();
    assert(nd_icon_rects(px,rects,1024)==1024);
    assert(nd_icon_rects(px,rects,10)==1024);        /* counts beyond MAX */
    for (i=0;i<ND_COLLECTION_ICON_COUNT;++i) {
        const char *file=strrchr(nd_collection_icons[i].path,'/');
        sprintf(path,"Icons/%s",file ? file+1 : nd_collection_icons[i].path);
        if (!nd_ilbm_load(path,&image)) continue;
        ++loaded;
        for (y=0;y<32;++y) for (x=0;x<32;++x) {
            k=image.pixels[(y*image.height/32)*ND_ICON_MAX_DIM+x*image.width/32];
            px[y*32+x]=(k!=image.transparent && k<image.colors) ? (long)k : ND_ICON_RECT_CLEAR;
        }
        check();
        total+=nd_icon_rects(px,NULL,0);
        for (y=0;y<32;++y) for (x=0;x<32;) {
            long c=px[y*32+x];
            while (x<32 && px[y*32+x]==c) ++x;
            if (c!=ND_ICON_RECT_CLEAR) ++runs;
        }
    }
    assert(loaded==ND_COLLECTION_ICON_COUNT);
    assert(total*2<runs);
    printf("Icon rectangles: %d icons need %ld fills instead of %ld row runs.\n",loaded,total,runs);
    return 0;
}
