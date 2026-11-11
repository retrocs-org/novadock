/* Minimal self-describing NovaDock plugin.  Build with nd_client.c and
 * nd_option_runtime.c; ship it with Hello.ndplugin (or the same NOVADOCK_
 * tool types in Hello.info) so Preferences can offer it automatically. */
#include <proto/dos.h>
#include <stdlib.h>
#include "nd_client.h"
#include "nd_option_runtime.h"

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]="$VER: Hello 1.0 (26.10.2026)";
/* Honoured by the vbcc and libnix startup code; harmless elsewhere. */
long __stack=16384;

static void read_settings(const char *id,char *greeting,unsigned int size,
                          unsigned int *poll)
{
    char ticks[8];
    struct NDOptionString fields[2];
    /* Defaults mirror Hello.ndplugin; the user's values come from the one
     * novadock.cfg as OPTION.<ID>.<key>, edited in Preferences. */
    fields[0].key="GREETING";fields[0].fallback="HELLO AMIGA";fields[0].out=greeting;fields[0].limit=size;
    fields[1].key="POLL_TICKS";fields[1].fallback="50";fields[1].out=ticks;fields[1].limit=sizeof(ticks);
    nd_plugin_option_strings(id,fields,2);
    *poll=(unsigned int)atoi(ticks);
    if (*poll<10 || *poll>500) *poll=50;
}
int main(int argc, char **argv)
{
    struct NDClient client;
    unsigned int event,poll;
    char greeting[32];
    int clicked = 0;
    /* NovaDock supplies the configured, unique plugin ID as argv[1]. */
    if (argc < 2 || !nd_client_init(&client, argv[1])) return 20;
    read_settings(argv[1],greeting,sizeof(greeting),&poll);
    for (;;) {
        event = nd_client_update(&client, "HELLO",
                                 clicked ? "CLICKED!" : greeting,
                                 ND_NO_METER);
        if (event & ND_EVENT_QUIT) break;
        if (event & ND_EVENT_CLICK) {
            clicked = !clicked;
            /* A badge on the tile's corner; docks without badges ignore it. */
            event |= nd_client_set_badge(&client, clicked ? "1" : "");
            if (event & ND_EVENT_QUIT) break;
        }
        if (event & ND_EVENT_SETTINGS)
            read_settings(argv[1],greeting,sizeof(greeting),&poll);
        /* Sleeps, but wakes at once when the tile is clicked. */
        (void)nd_client_wait(&client,nd_client_delay_ticks(&client,poll));
    }
    nd_client_close(&client);
    return 0;
}
