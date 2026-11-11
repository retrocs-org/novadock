#include "nd_watchdog.h"

unsigned long nd_watchdog_backoff_ticks(unsigned int failures)
{
    static const unsigned int seconds[]={20U,40U,80U,160U,300U};
    unsigned int index=failures;
    if (index>=sizeof(seconds)/sizeof(seconds[0])) index=4U;
    return (unsigned long)seconds[index]*ND_WATCHDOG_TICKS_PER_SECOND;
}
int nd_watchdog_elapsed(unsigned long now,unsigned long then,
                        unsigned long interval)
{
    return (unsigned long)(now-then)>=interval;
}
int nd_watchdog_deadline_reached(unsigned long now,unsigned long deadline)
{
    /* Valid for deadlines less than half an unsigned 32-bit clock cycle away. */
    return now==deadline || ((now-deadline)&0xffffffffUL)<0x80000000UL;
}
