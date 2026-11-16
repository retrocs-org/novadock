#ifndef ND_SYSTEM_INFO_H
#define ND_SYSTEM_INFO_H
#include <stdio.h>
#include "nd_cpu_info.h"
/* Divide a 64-bit E-clock without requiring compiler long-long support. */
static unsigned long nd_system_seconds(unsigned long high,unsigned long low,unsigned long rate)
{
 unsigned long remainder=0,result=0,bit,carry;int i;
 if(!rate)return 0;
 for(i=63;i>=0;--i){
  bit=i>=32 ? (high>>(i-32))&1UL:(low>>i)&1UL;
  carry=remainder&0x80000000UL;remainder=((remainder<<1)|bit)&0xffffffffUL;
  if(carry || remainder>=rate){remainder=(remainder-rate)&0xffffffffUL;if(i>=32)return 0xffffffffUL;result|=1UL<<i;}
 }
 return result;
}
static void nd_system_uptime(unsigned long seconds,char *out)
{sprintf(out,"%lu d %02lu:%02lu:%02lu",seconds/86400UL,(seconds/3600UL)%24UL,(seconds/60UL)%60UL,seconds%60UL);}
#endif
