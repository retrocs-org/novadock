#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_bench.h"
void nd_bench_copy(void *to,const void *from,unsigned long bytes){memcpy(to,from,bytes);}
unsigned long nd_bench_kernel(unsigned long seed,unsigned int rounds)
{while(rounds--){seed=(seed*1664525UL+1013904223UL)&0xffffffffUL;seed^=(seed>>13);seed=((seed<<7)|(seed>>25))&0xffffffffUL;}return seed;}
unsigned long nd_bench_rate(unsigned long units,unsigned long ms)
{unsigned long q,r;if(!ms || ms>60000)return 0;q=units/ms;r=units%ms;if(q>4294967UL)return 0xffffffffUL;q*=1000UL;r=r*1000UL/ms;return r>0xffffffffUL-q ? 0xffffffffUL:q+r;}
unsigned int nd_bench_report(const struct NDBenchResult *r,char *out,unsigned int size)
{int n;if(!r || !out || size<900 || !r->cpu || !r->memory || !r->graphics || !r->clock_hz)return 0;n=sprintf(out,"NDBENCH1\nDATE=%s\nKERNEL=1\nCPU_OPS=%lu\nMEMORY_KIB=%lu\nPLANAR_FILLS=%lu\nE_CLOCK_HZ=%lu\nCPU_FLAGS=%lu\nCHIP_BYTES=%lu\nFAST_BYTES=%lu\nMEMORY_KIND=%s\nSCREEN=%u,%u,%u\nWORKLOAD=32-bit integer kernel; 64 KiB copy; 320x128x2 off-screen planar fills\nNOTE=Rates depend on competing tasks, RAM placement and graphics hardware.\n",r->date,r->cpu,r->memory,r->graphics,r->clock_hz,r->cpu_flags,r->chip,r->fast,r->memory_kind,r->screen_width,r->screen_height,r->screen_depth);return n>0 && (unsigned int)n<size ? (unsigned int)n:0;}
static int number(const char *s,unsigned long *out)
{unsigned long v;char *end;if(!*s || *s<'0'||*s>'9')return 0;v=strtoul(s,&end,10);if(*end || v>0xffffffffUL)return 0;*out=v;return 1;}
int nd_bench_parse(const char *text,struct NDBenchResult *out)
{
 struct NDBenchResult r;char line[256];const char *p,*end;unsigned long v;unsigned int seen=0,n;
 if(!text || strlen(text)>2047 || strncmp(text,"NDBENCH1\n",9))return 0;memset(&r,0,sizeof(r));p=text+9;
 while(*p){end=strchr(p,'\n');if(!end)return 0;n=(unsigned int)(end-p);if(n>=sizeof(line))return 0;memcpy(line,p,n);line[n]=0;p=end+1;
  if(!strncmp(line,"DATE=",5)){if(seen&1 || !line[5] || strlen(line+5)>=sizeof(r.date))return 0;strcpy(r.date,line+5);seen|=1;}
  else if(!strncmp(line,"KERNEL=",7)){if(seen&2 || strcmp(line+7,"1"))return 0;seen|=2;}
  else if(!strncmp(line,"CPU_OPS=",8)){if(seen&4 || !number(line+8,&r.cpu)||!r.cpu)return 0;seen|=4;}
  else if(!strncmp(line,"MEMORY_KIB=",11)){if(seen&8 || !number(line+11,&r.memory)||!r.memory)return 0;seen|=8;}
  else if(!strncmp(line,"PLANAR_FILLS=",13)){if(seen&16 || !number(line+13,&r.graphics)||!r.graphics)return 0;seen|=16;}
  else if(!strncmp(line,"E_CLOCK_HZ=",11)){if(!number(line+11,&r.clock_hz)||!r.clock_hz)return 0;}
  else if(!strncmp(line,"CPU_FLAGS=",10)){if(!number(line+10,&r.cpu_flags))return 0;}
  else if(!strncmp(line,"CHIP_BYTES=",11)){if(!number(line+11,&r.chip))return 0;}
  else if(!strncmp(line,"FAST_BYTES=",11)){if(!number(line+11,&r.fast))return 0;}
  else if(!strncmp(line,"MEMORY_KIND=",12)){if(strlen(line+12)>=sizeof(r.memory_kind))return 0;strcpy(r.memory_kind,line+12);}
  else if(!strncmp(line,"SCREEN=",7)){char tail;if(sscanf(line+7,"%u,%u,%u%c",&r.screen_width,&r.screen_height,&r.screen_depth,&tail)!=3 || r.screen_width>8192 || r.screen_height>8192 || r.screen_depth>32)return 0;}
 }
 if(seen!=31 || !r.clock_hz)return 0;*out=r;return 1;
}

/* Fractional multiply/divide without overflowing the 32-bit target. */
static unsigned long fraction1000(unsigned long n,unsigned long d)
{unsigned long rem=0,q=0,bit;for(bit=512;bit;bit>>=1){q*=2;if(rem>=d-rem){rem-=d-rem;++q;}else rem+=rem;if(1000UL&bit){if(rem>=d-n){rem-=d-n;++q;}else rem+=n;}}return q;}
void nd_bench_change(unsigned long current,unsigned long saved,char out[24])
{unsigned long delta,whole,tenths;int up;if(!saved){strcpy(out,"n/a");return;}up=current>=saved;delta=up ? current-saved:saved-current;whole=delta/saved;if(whole>=100){strcpy(out,up ? ">=+10000%":"<=-10000%");return;}tenths=whole*1000UL+fraction1000(delta%saved,saved);sprintf(out,"%s%lu.%lu%%",delta ? (up ? "+":"-"):"",tenths/10,tenths%10);}
