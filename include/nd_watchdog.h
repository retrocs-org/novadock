#ifndef ND_WATCHDOG_H
#define ND_WATCHDOG_H
/* Portable watchdog policy. Runtime time values use the 50 Hz DOS DateStamp
 * clock; the dock's existing 5 Hz housekeeping timer only triggers checks. */
#define ND_WATCHDOG_TICKS_PER_SECOND 50UL
#define ND_WATCHDOG_STALE_TICKS (20UL*ND_WATCHDOG_TICKS_PER_SECOND)
#define ND_WATCHDOG_GOODBYE_TICKS (5UL*ND_WATCHDOG_TICKS_PER_SECOND)
#define ND_WATCHDOG_CHECK_TIMER_TICKS 25

unsigned long nd_watchdog_backoff_ticks(unsigned int failures);
int nd_watchdog_elapsed(unsigned long now,unsigned long then,
                        unsigned long interval);
int nd_watchdog_deadline_reached(unsigned long now,unsigned long deadline);
#endif
