#include "nd_volume.h"
#include <stdio.h>
int nd_volume_valid(const struct NDVolumeSpace *v)
{return v && v->total && v->used<=v->total && v->block_bytes && v->total<=0xffffffffUL && v->block_bytes<=0xffffffffUL;}
void nd_volume_size(unsigned long a,unsigned long b,char *out)
{
    unsigned long a0=a&65535UL,a1=(a>>16)&65535UL,b0=b&65535UL,b1=(b>>16)&65535UL;
    unsigned long lo=a0*b0,hi=a1*b1,t,old;unsigned int unit=0,fraction=0;static const char *units[]={"B","KB","MB","GB","TB","PB","EB"};
    t=a1*b0;old=lo;lo=(lo+((t&65535UL)<<16))&0xffffffffUL;hi=(hi+(t>>16)+(lo<old))&0xffffffffUL;
    t=a0*b1;old=lo;lo=(lo+((t&65535UL)<<16))&0xffffffffUL;hi=(hi+(t>>16)+(lo<old))&0xffffffffUL;
    while((hi || lo>=1024UL) && unit<6){fraction=(unsigned int)(((lo&1023UL)*10UL+512UL)/1024UL);lo=(lo>>10)|((hi&1023UL)<<22);hi>>=10;++unit;}
    if(fraction==10){++lo;fraction=0;}
    if(unit && lo<100)sprintf(out,"%lu.%u %s",lo,fraction,units[unit]);else sprintf(out,"%lu %s",lo,units[unit]);
}
void nd_volume_summary(const struct NDVolumeSpace *v,char *out)
{char f[24],u[24],t[24];if(!nd_volume_valid(v)){sprintf(out,"Capacity unavailable");return;}nd_volume_size(v->total-v->used,v->block_bytes,f);nd_volume_size(v->used,v->block_bytes,u);nd_volume_size(v->total,v->block_bytes,t);sprintf(out,"Free %s / Used %s / Total %s",f,u,t);}
int nd_volume_low(const struct NDVolumeSpace *v,unsigned int percent)
{
    unsigned long free,threshold;
    if(!nd_volume_valid(v) || !percent || percent>99)return 0;
    free=v->total-v->used;threshold=(v->total/100UL)*percent+((v->total%100UL)*percent+99UL)/100UL;
    return free<threshold;
}
