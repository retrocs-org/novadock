#include <stdio.h>
#include "nd_metrics.h"

unsigned int nd_cpu_used_percent(unsigned long idle_ticks,
                                 unsigned long total_ticks)
{
    unsigned long idle_percent;
    if (!total_ticks || idle_ticks >= total_ticks) return 0U;
    if (total_ticks >= 100UL)
        idle_percent = idle_ticks / (total_ticks / 100UL);
    else
        idle_percent = idle_ticks * 100UL / total_ticks;
    if (idle_percent > 100UL) idle_percent = 100UL;
    return (unsigned int)(100UL - idle_percent);
}

unsigned int nd_memory_used_percent(unsigned long free_bytes,
                                    unsigned long total_bytes)
{
    unsigned long free_kb, total_kb;
    if (!total_bytes) return 0U;
    if (free_bytes > total_bytes) free_bytes = total_bytes;
    free_kb = free_bytes / 1024UL;
    total_kb = total_bytes / 1024UL;
    if (!total_kb) return 0U;
    return (unsigned int)((total_kb - free_kb) * 100UL / total_kb);
}

void nd_format_memory_usage(char *out, unsigned long free_bytes,
                            unsigned long total_bytes)
{
    unsigned long used, used_tenths, total_tenths;
    if (free_bytes > total_bytes) free_bytes = total_bytes;
    used = total_bytes - free_bytes;
    used_tenths = used / (1048576UL / 10UL);
    total_tenths = total_bytes / (1048576UL / 10UL);
    sprintf(out, "%lu.%lu/%lu.%lu MB",
            used_tenths / 10UL, used_tenths % 10UL,
            total_tenths / 10UL, total_tenths % 10UL);
}

void nd_format_memory_pair(char *out,
                           unsigned long chip_free, unsigned long chip_total,
                           unsigned long fast_free, unsigned long fast_total)
{
    unsigned long chip_used, chip_used_tenths, chip_total_tenths;
    unsigned long fast_used, fast_used_tenths, fast_total_tenths;
    if (chip_free > chip_total) chip_free = chip_total;
    if (fast_free > fast_total) fast_free = fast_total;
    chip_used = chip_total - chip_free;
    fast_used = fast_total - fast_free;
    chip_used_tenths = chip_used / (1048576UL / 10UL);
    chip_total_tenths = chip_total / (1048576UL / 10UL);
    fast_used_tenths = fast_used / (1048576UL / 10UL);
    fast_total_tenths = fast_total / (1048576UL / 10UL);
    sprintf(out, "C:%lu.%lu/%lu.%lu F:%lu.%lu/%lu.%lu",
            chip_used_tenths / 10UL, chip_used_tenths % 10UL,
            chip_total_tenths / 10UL, chip_total_tenths % 10UL,
            fast_used_tenths / 10UL, fast_used_tenths % 10UL,
            fast_total_tenths / 10UL, fast_total_tenths % 10UL);
}
