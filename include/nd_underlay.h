#ifndef ND_UNDERLAY_H
#define ND_UNDERLAY_H
#include "nd_autohide.h"
#if defined(__GNUC__)
#define ND_UNDERLAY static __attribute__((unused))
#else
#define ND_UNDERLAY static
#endif
/* NEW minus OLD, in non-overlapping strips. Already covered pixels must
 * never be sampled from the screen: they contain the dock's own artwork. */
ND_UNDERLAY int nd_underlay_added(const struct NDRect *fresh,const struct NDRect *old,struct NDRect strips[4])
{
    int l=fresh->left,t=fresh->top,r=l+fresh->width,b=t+fresh->height;
    int il=l>old->left?l:old->left,it=t>old->top?t:old->top;
    int ir=r<old->left+old->width?r:old->left+old->width;
    int ib=b<old->top+old->height?b:old->top+old->height,n=0;
    if(fresh->width<=0||fresh->height<=0)return 0;
    if(ir<=il||ib<=it){strips[0]=*fresh;return 1;}
    if(it>t){strips[n].left=l;strips[n].top=t;strips[n].width=r-l;strips[n++].height=it-t;}
    if(b>ib){strips[n].left=l;strips[n].top=ib;strips[n].width=r-l;strips[n++].height=b-ib;}
    if(il>l){strips[n].left=l;strips[n].top=it;strips[n].width=il-l;strips[n++].height=ib-it;}
    if(r>ir){strips[n].left=ir;strips[n].top=it;strips[n].width=r-ir;strips[n++].height=ib-it;}
    return n;
}
#endif
