#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_icon_mask.h"

int main(void)
{
    unsigned char pens[49],opaque[49];
    unsigned long queue[49];
    int x,y,w,h;
    nd_icon_art_box(84,68,0,&w,&h);assert(w==48 && h==40);
    nd_icon_art_box(168,68,0,&w,&h);assert(w==48 && h==40);
    nd_icon_art_box(40,28,1,&w,&h);assert(w==38 && h==26);
    nd_icon_art_box(30,42,0,&w,&h);assert(w==16 && h==18);
    assert(nd_icon_art_top(10,68,40,0)==15);
    assert(nd_icon_art_top(10,68,22,0)==24);
    assert(nd_icon_art_top(10,68,40,0)+20==nd_icon_art_top(10,68,22,0)+11);
    assert(nd_icon_art_top(10,28,22,1)==13);
    /* A two-pen checkerboard surrounds a solid outline. The centre reuses
     * background pen 0 but must survive because the outline encloses it. */
    for (y=0;y<7;++y) for (x=0;x<7;++x) {
        if (x==0 || y==0 || x==6 || y==6) pens[y*7+x]=(x+y)&1;
        else if (x==1 || y==1 || x==5 || y==5) pens[y*7+x]=2;
        else pens[y*7+x]=0;
        opaque[y*7+x]=1;
    }
    assert(nd_icon_remove_border_background(pens,opaque,7,7,queue,49)==24);
    for (x=0;x<7;++x) assert(!opaque[x] && !opaque[42+x]);
    for (y=0;y<7;++y) assert(!opaque[y*7] && !opaque[y*7+6]);
    assert(opaque[3*7+3]);
    assert(opaque[1*7+1]);
    puts("Classic icon border-mask tests passed.");
    return 0;
}
