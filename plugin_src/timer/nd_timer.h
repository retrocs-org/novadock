#ifndef ND_TIMER_H
#define ND_TIMER_H
struct NDCountdown {unsigned long duration,remaining,fraction;int running,started,expired;};
void nd_countdown_reset(struct NDCountdown *timer,unsigned long seconds);
void nd_countdown_start(struct NDCountdown *timer);
/* EClock ticks elapsed, with a constant nonzero ticks/second frequency. */
int nd_countdown_advance(struct NDCountdown *timer,unsigned long ticks,unsigned long rate);
int nd_countdown_seconds(const char *text,unsigned long *seconds);
#endif
