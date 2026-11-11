/* Memory: used/total RAM and an occupancy meter. */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_metrics.h"
#include <stdlib.h>
#include <string.h>

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("Memory");
/* Honoured by the vbcc and libnix startup code; harmless elsewhere. */
long __stack=16384;

int main(int argc, char **argv)
{
    struct NDClient client;
    ULONG free_bytes, total, flags;
    ULONG chip_free,chip_total,fast_free,fast_total;
    unsigned int event, meter,chip_meter,fast_meter;
    unsigned int elapsed_ticks=50,effective_poll;
    char value[32];
    const char *caption;
    const char *option_id;
    char option[24];
    unsigned int poll=50;
    unsigned int last_meter=101;
    unsigned int stable_samples=0;
    int reread=1;
    if (argc < 2 || !nd_client_init(&client, argv[1])) return 20;
    option_id=argv[1];
    if (!strcmp(option_id,"CHIPRAM")) {
        flags=MEMF_CHIP;
        caption="CHIP USED";
    } else if (!strcmp(option_id,"FASTRAM")) {
        flags=MEMF_FAST;
        caption="FAST USED";
    } else {
        /* Keep the original RAM instance compatible with 0.1 configs. */
        flags=MEMF_ANY;
        caption="RAM USED";
        option_id="RAM";
    }
    for (;;) {
        if (reread) {
            nd_plugin_option_string(option_id,"POLL_TICKS","50",option,sizeof(option));
            poll=(unsigned int)atoi(option);
            if (poll<10 || poll>1000) poll=50;
            reread=0;
        }
        if (!strcmp(option_id,"RAM")) {
            chip_free=AvailMem(MEMF_CHIP);
            chip_total=AvailMem(MEMF_CHIP|MEMF_TOTAL);
            fast_free=AvailMem(MEMF_FAST);
            fast_total=AvailMem(MEMF_FAST|MEMF_TOTAL);
            if (chip_free>chip_total) chip_free=chip_total;
            if (fast_free>fast_total) fast_free=fast_total;
            nd_format_memory_pair(value,chip_free,chip_total,
                                  fast_free,fast_total);
            chip_meter=nd_memory_used_percent(chip_free,chip_total);
            fast_meter=nd_memory_used_percent(fast_free,fast_total);
            meter=nd_memory_used_percent(chip_free+fast_free,
                                         chip_total+fast_total);
            /* Always publish both channels. The host retains their history
             * even in bar mode, so changing to Graph is immediate. */
            event=nd_client_update_graph_quiet(&client,"RAM",value,meter,
                                         chip_meter,fast_meter,
                                         elapsed_ticks,500U);
        } else {
            free_bytes = AvailMem(flags);
            total = AvailMem(flags | MEMF_TOTAL);
            /* Measurements are separate snapshots in a multitasking OS. */
            if (free_bytes > total) free_bytes = total;
            nd_format_memory_usage(value, free_bytes, total);
            meter = nd_memory_used_percent(free_bytes, total);
            event=nd_client_update_graph_quiet(&client,caption,value,meter,meter,
                                           ND_GRAPH_NO_SAMPLE,elapsed_ticks,500U);
        }
        if (event & ND_EVENT_QUIT) break;
        if (last_meter<=100 &&
            ((meter>last_meter ? meter-last_meter : last_meter-meter)<=1))
            stable_samples++;
        else stable_samples=0;
        last_meter=meter;
        if (event & ND_EVENT_SETTINGS) {
            reread=1;
            stable_samples=0;
            last_meter=101;
        }
        effective_poll=poll;
        if (stable_samples>=50 && effective_poll<=333)
            effective_poll*=3;
        else if (stable_samples>=20 && effective_poll<=500)
            effective_poll*=2;
        effective_poll=nd_client_delay_ticks(&client,effective_poll);
        elapsed_ticks=nd_client_wait(&client,effective_poll);
    }
    nd_client_close(&client);
    return 0;
}
