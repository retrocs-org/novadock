/* NovaDock Eyes plugin: heartbeats and lifecycle. The dock draws the two
 * native graphics eyes and calculates pupils from the Workbench pointer;
 * this separate process never passes a RastPort across process boundaries.
 * All preferences live in unified ENV:/ENVARC:NovaDock/novadock.cfg.
 */
#include <exec/types.h>
#include <proto/dos.h>
#include <stdlib.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("Eyes");
/* Honoured by the vbcc and libnix startup code; harmless elsewhere. */
long __stack=16384;

int main(int argc,char **argv)
{
    struct NDClient client;
    char option[32];
    unsigned int ticks=50,events,elapsed_ticks=0,delay_ticks;
    int reload=1;
    if (argc<2 || !nd_client_init(&client,argv[1])) return 20;
    for (;;) {
        if (reload) {
            nd_plugin_option_string(argv[1],"POLL_TICKS","50",
                                    option,sizeof(option));
            ticks=(unsigned int)atoi(option);
            if (ticks<10 || ticks>250) ticks=50;
            reload=0;
        }
        events=nd_client_update_quiet(&client,argv[1],"FOLLOWING",ND_NO_METER,
                                      elapsed_ticks,500U);
        if (events & ND_EVENT_QUIT) break;
        if (events & ND_EVENT_SETTINGS) reload=1;
        delay_ticks=nd_client_delay_ticks(&client,ticks);
        elapsed_ticks=nd_client_wait(&client,delay_ticks);
    }
    nd_client_close(&client);
    return 0;
}
