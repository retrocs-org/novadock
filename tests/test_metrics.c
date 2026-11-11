#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_metrics.h"

static void verify(int condition)
{
    if (!condition) {
        fprintf(stderr, "metrics test failed\n");
        exit(1);
    }
}

int main(void)
{
    char value[32];
    verify(nd_cpu_used_percent(100UL, 100UL) == 0U);
    verify(nd_cpu_used_percent(75UL, 100UL) == 25U);
    verify(nd_cpu_used_percent(0UL, 100UL) == 100U);
    verify(nd_memory_used_percent(6UL * 1048576UL,
                                  10UL * 1048576UL) == 40U);
    nd_format_memory_usage(value, 6UL * 1048576UL, 10UL * 1048576UL);
    verify(!strcmp(value, "4.0/10.0 MB"));
    nd_format_memory_pair(value,1UL*1048576UL,2UL*1048576UL,
                          3UL*1048576UL,8UL*1048576UL);
    verify(!strcmp(value,"C:1.0/2.0 F:5.0/8.0"));
    puts("CPU and memory metric tests passed.");
    return 0;
}
