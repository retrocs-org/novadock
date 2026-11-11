#ifndef ND_METRICS_H
#define ND_METRICS_H

unsigned int nd_cpu_used_percent(unsigned long idle_ticks,
                                 unsigned long total_ticks);
unsigned int nd_memory_used_percent(unsigned long free_bytes,
                                    unsigned long total_bytes);
void nd_format_memory_usage(char *out, unsigned long free_bytes,
                            unsigned long total_bytes);
void nd_format_memory_pair(char *out,
                           unsigned long chip_free, unsigned long chip_total,
                           unsigned long fast_free, unsigned long fast_total);

#endif
