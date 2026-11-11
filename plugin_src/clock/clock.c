/* Clock: click tile to toggle 24/12h. Uses local AmigaDOS DateStamp. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdlib.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include <string.h>

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("Clock");
/* Honoured by the vbcc and libnix startup code; harmless elsewhere. */
long __stack=16384;

int main(int argc, char **argv)
{
    struct NDClient client;
    struct DateStamp ds;
    char value[32];
    unsigned int event;
    int twelve = 0, hours, mins, seconds=0;
    unsigned int poll_ticks=50,elapsed_ticks=0,delay_ticks;
    char option[32],hour[8],sec[8];
    struct NDOptionString settings[3];
    int reread=1;
    if (argc < 2 || !nd_client_init(&client, argv[1])) return 20;
    for (;;) {
        if (reread) {
        settings[0].key="HOUR_MODE";settings[0].fallback="24";settings[0].out=hour;settings[0].limit=sizeof(hour);
        settings[1].key="SECONDS";settings[1].fallback="NO";settings[1].out=sec;settings[1].limit=sizeof(sec);
        settings[2].key="POLL_TICKS";settings[2].fallback="50";settings[2].out=option;settings[2].limit=sizeof(option);
        nd_plugin_option_strings("CLOCK",settings,3);
        twelve=!strcmp(hour,"12");seconds=!strcmp(sec,"YES");
        poll_ticks=(unsigned int)atoi(option);
        if (poll_ticks<10 || poll_ticks>500) poll_ticks=50;
        reread=0;
        }
        DateStamp(&ds);
        hours = (int)((ds.ds_Minute / 60L) % 24L);
        mins = (int)(ds.ds_Minute % 60L);
        if (twelve) {
            int hr = hours % 12;
            if (!hr) hr = 12;
            if (seconds) sprintf(value,"%02d:%02d:%02ld %s",hr,mins,
                              ds.ds_Tick/50L,hours>=12 ? "PM" : "AM");
            else sprintf(value,"%02d:%02d %s",hr,mins,hours>=12 ? "PM" : "AM");
        } else if (seconds) sprintf(value,"%02d:%02d:%02ld",hours,mins,ds.ds_Tick/50L);
        else sprintf(value,"%02d:%02d",hours,mins);
        event=nd_client_update_quiet(&client,"CLOCK",value,ND_NO_METER,
                                     elapsed_ticks,500U);
        if (event & ND_EVENT_QUIT) break;
        if (event & ND_EVENT_SETTINGS) reread=1;
        /* View preferences are permanent; clicks do not silently rewrite them.
         * Without seconds the text changes once a minute: sleep until just
         * after the minute changes, waking at least every ten seconds so
         * the dock's watchdog keeps hearing from the clock. */
        if (seconds) delay_ticks=poll_ticks;
        else {
            delay_ticks=(unsigned int)(3000L-ds.ds_Tick%3000L)+1U;
            if (delay_ticks>500U) delay_ticks=500U;
        }
        delay_ticks=nd_client_delay_ticks(&client,delay_ticks);
        elapsed_ticks=nd_client_wait(&client,delay_ticks); /* 50 ticks per second */
    }
    nd_client_close(&client);
    return 0;
}
