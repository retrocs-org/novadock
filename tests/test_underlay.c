#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "nd_underlay.h"
static int inside(const struct NDRect *r,int x,int y)
{return x>=r->left&&y>=r->top&&x<r->left+r->width&&y<r->top+r->height;}
int main(void)
{
    struct NDRect a,b,strips[4];int n,i,x,y,k,hits,expected;
    srand(17);
    for(k=0;k<2500;++k){
        a.left=rand()%21;a.top=rand()%21;a.width=rand()%61;a.height=rand()%61;
        b.left=rand()%21;b.top=rand()%21;b.width=rand()%61;b.height=rand()%61;
        n=nd_underlay_added(&a,&b,strips);assert(n>=0&&n<=4);
        for(i=0;i<n;++i)assert(strips[i].width>0&&strips[i].height>0);
        for(y=0;y<82;++y)for(x=0;x<82;++x){
            hits=0;for(i=0;i<n;++i)hits+=inside(&strips[i],x,y);
            expected=inside(&a,x,y)&&!inside(&b,x,y);assert(hits==expected);
        }
    }
    puts("Underlay strips: growth, shrink, translation, overlap, disjoint and empty rectangles cover only newly hidden pixels exactly once.");return 0;
}
