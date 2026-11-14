#include <assert.h>
#include <stdio.h>
#include "nd_timer.h"
int main(void)
{
    struct NDCountdown t;unsigned long n;unsigned int i;
    assert(nd_countdown_seconds("1",&n)&&n==1);
    assert(nd_countdown_seconds("86400",&n)&&n==86400);
    assert(!nd_countdown_seconds("0",&n));assert(!nd_countdown_seconds("86401",&n));
    assert(!nd_countdown_seconds("999999999999999999999999999",&n));
    assert(!nd_countdown_seconds("-5",&n));assert(!nd_countdown_seconds("2x",&n));
    nd_countdown_reset(&t,3);assert(!nd_countdown_advance(&t,500,1000)&&t.remaining==3);
    nd_countdown_start(&t);assert(!nd_countdown_advance(&t,500,1000)&&t.remaining==3);
    t.running=0;assert(!nd_countdown_advance(&t,9000,1000)&&t.remaining==3&&t.fraction==500);
    nd_countdown_start(&t);assert(!nd_countdown_advance(&t,600,1000)&&t.remaining==2&&t.fraction==100);
    assert(nd_countdown_advance(&t,2000,1000)&&!t.running&&t.expired&&!t.remaining);
    assert(!nd_countdown_advance(&t,2000,1000)); /* alarm fires once */
    nd_countdown_reset(&t,60);nd_countdown_start(&t);
    /* EClock frequency need not be a multiple of 50 or 1000. */
    for(i=0;i<599;++i)assert(!nd_countdown_advance(&t,70937,709379));
    assert(t.remaining==1);assert(nd_countdown_advance(&t,709379,709379));
    nd_countdown_reset(&t,86400);nd_countdown_start(&t);
    assert(!nd_countdown_advance(&t,0,1000));assert(!nd_countdown_advance(&t,1,0));
    assert(nd_countdown_advance(&t,86400000UL,1000));
    puts("Countdown: pause/resume, fractional ticks, limits and one-shot alarms pass.");return 0;
}
