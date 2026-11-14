#include <string.h>
#include "nd_timer.h"
void nd_countdown_reset(struct NDCountdown *t,unsigned long seconds)
{memset(t,0,sizeof(*t));t->duration=t->remaining=seconds;}
void nd_countdown_start(struct NDCountdown *t)
{if(t->remaining){t->running=1;t->started=1;t->expired=0;}}
int nd_countdown_advance(struct NDCountdown *t,unsigned long ticks,unsigned long rate)
{
    unsigned long seconds,part;
    if(!t->running || !rate)return 0;
    seconds=ticks/rate;part=ticks%rate;
    if(part>=rate-t->fraction){++seconds;t->fraction=part-(rate-t->fraction);}
    else t->fraction+=part;
    if(seconds>=t->remaining){t->remaining=0;t->running=0;t->fraction=0;t->expired=1;return 1;}
    t->remaining-=seconds;return 0;
}
int nd_countdown_seconds(const char *text,unsigned long *seconds)
{
    unsigned long n=0;
    if(!text || !*text)return 0;
    for(;*text;++text){if(*text<'0'||*text>'9')return 0;n=n*10UL+(*text-'0');if(n>86400UL)return 0;}
    if(!n)return 0;*seconds=n;return 1;
}
