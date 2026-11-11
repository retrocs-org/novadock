#include <assert.h>
#include <stdio.h>
#include "nd_watchdog.h"

int main(void)
{
    assert(nd_watchdog_backoff_ticks(0)==20UL*50UL);
    assert(nd_watchdog_backoff_ticks(1)==40UL*50UL);
    assert(nd_watchdog_backoff_ticks(3)==160UL*50UL);
    assert(nd_watchdog_backoff_ticks(4)==300UL*50UL);
    assert(nd_watchdog_backoff_ticks(99)==300UL*50UL);
    assert(!nd_watchdog_elapsed(1099,1000,100));
    assert(nd_watchdog_elapsed(1100,1000,100));
    assert(nd_watchdog_elapsed(25,0xfffffff0UL,40));
    assert(!nd_watchdog_deadline_reached(99,100));
    assert(nd_watchdog_deadline_reached(100,100));
    assert(nd_watchdog_deadline_reached(25,0xfffffff0UL));
    puts("Plugin watchdog policy tests passed.");
    return 0;
}
