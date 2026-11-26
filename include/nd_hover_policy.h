#ifndef ND_HOVER_POLICY_H
#define ND_HOVER_POLICY_H
static int nd_hover_axis(int position,int previous)
{
    if(position>=previous*4-1 && position<=previous*4+4)return previous;
    return position/4;
}
#endif
