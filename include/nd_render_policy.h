#ifndef ND_RENDER_POLICY_H
#define ND_RENDER_POLICY_H
/* Duration samples are independent of optional support tracing. */
static unsigned long nd_frame_average(unsigned long average,unsigned long sample)
{
    if(sample>1000000UL)sample=1000000UL;
    return average ? average-average/8UL+sample/8UL:sample;
}
static unsigned int nd_frame_interval(unsigned long average)
{
    unsigned long ms=(average+999UL)/1000UL;
    /* Leave CPU time for other tasks. 25 fps on fast systems; down to 5 fps. */
    ms=ms+ms/2UL;if(ms<40UL)ms=40UL;if(ms>200UL)ms=200UL;
    return (unsigned int)((ms+19UL)/20UL)*20U;
}
struct NDFrameClock {unsigned long seconds,micros,carry;int valid;};
/* Carry real elapsed time, including painting and interrupted waits. A backward or large clock
 * adjustment uses the requested interval instead of a huge/negative jump. */
static unsigned int nd_timer_quanta(struct NDFrameClock *clock,unsigned long seconds,
                                    unsigned long micros,unsigned long fallback)
{
    unsigned long elapsed=fallback,total;unsigned int quanta;
    if(clock->valid && (seconds>clock->seconds || (seconds==clock->seconds && micros>=clock->micros)) && seconds-clock->seconds<60UL)
        elapsed=(seconds-clock->seconds)*1000000UL+micros-clock->micros;
    clock->seconds=seconds;clock->micros=micros;clock->valid=1;
    total=clock->carry+elapsed;quanta=(unsigned int)(total/200000UL);clock->carry=total%200000UL;
    return quanta;
}
#endif
