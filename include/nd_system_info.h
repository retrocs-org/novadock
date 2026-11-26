#ifndef ND_SYSTEM_INFO_H
#define ND_SYSTEM_INFO_H
#include <stdio.h>
#include <string.h>
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
/* Closed tiles only wake for the next visible change or watchdog heartbeat.
 * UI signals still interrupt this wait immediately. */
static unsigned int nd_system_wait(const char *display,int panel,
                                  unsigned long seconds,unsigned long rotate)
{
 unsigned long wait;
 if(panel)return 50U;
 if(!strcmp(display,"CPU") || !strcmp(display,"Workbench"))return 500U;
 wait=(60UL-seconds%60UL)*50UL;
 if(strcmp(display,"Uptime")){unsigned long turn=250UL-rotate%250UL;if(turn<wait)wait=turn;}
 if(wait>500UL)wait=500UL;
 return (unsigned int)wait;
}
#endif
