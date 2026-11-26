#ifndef ND_HOVER_EASE_H
#define ND_HOVER_EASE_H
/* One animation step of a magnified size toward TARGET: half the remaining
 * distance (at least one pixel), so a transition settles in a few frames
 * and always ends exactly on the target. */
static int nd_hover_ease(int current,int target)
{
    int diff=target-current;
    if(diff>=-1 && diff<=1)return target;
    return current+diff/2;
}
#endif
