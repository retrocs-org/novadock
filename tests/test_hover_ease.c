/* Animated magnification: each step halves the distance and always lands
 * exactly on the target, growing or shrinking. */
#include <assert.h>
#include <stdio.h>
#include "nd_hover_ease.h"
int main(void)
{
    int from,to,v,steps;
    for(from=0;from<=96;++from)for(to=0;to<=96;++to){
        for(v=from,steps=0;v!=to;++steps){
            int next=nd_hover_ease(v,to);
            assert(steps<10);
            assert(to>v ? next>v&&next<=to : next<v&&next>=to);
            v=next;
        }
    }
    assert(nd_hover_ease(48,49)==49&&nd_hover_ease(48,47)==47&&nd_hover_ease(40,80)==60);
    puts("Hover easing: every transition up to 96 pixels settles exactly within ten frames, monotonically.");
    return 0;
}
