#ifndef ND_BENCH_H
#define ND_BENCH_H
struct NDBenchResult {unsigned long cpu,memory,graphics,clock_hz,cpu_flags,chip,fast;char date[32],memory_kind[16];unsigned int screen_width,screen_height,screen_depth;};
void nd_bench_copy(void *to,const void *from,unsigned long bytes);
#define ND_BENCH_HISTORY 16
struct NDBenchJob {struct NDBenchResult result,history[ND_BENCH_HISTORY];char path[240],names[ND_BENCH_HISTORY][96],report[2048];unsigned int count,duration;int save_result;};
unsigned long nd_bench_kernel(unsigned long seed,unsigned int rounds);
unsigned long nd_bench_rate(unsigned long units,unsigned long milliseconds);
unsigned int nd_bench_report(const struct NDBenchResult *r,char *out,unsigned int size);
void nd_bench_change(unsigned long current,unsigned long saved,char out[24]);
int nd_bench_parse(const char *text,struct NDBenchResult *out);
#endif
