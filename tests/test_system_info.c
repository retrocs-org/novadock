#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_system_info.h"
int main(void){char text[48];unsigned long rate,high,low,expected,i;
 assert(!strcmp(nd_system_cpu(0),"68000"));assert(!strcmp(nd_system_cpu(1),"68010"));assert(!strcmp(nd_system_cpu(3),"68020"));assert(!strcmp(nd_system_cpu(7),"68030"));assert(!strcmp(nd_system_cpu(15),"68040"));assert(!strcmp(nd_system_cpu(143),"68060"));
 for(rate=1;rate<1000000UL;rate=rate*3UL+1UL)for(i=0;i<1000;++i){
  /* 64-bit arithmetic is confined to this host oracle. */
  unsigned long long ticks=(unsigned long long)(i*43217UL)*rate+i%rate;
  high=(unsigned long)(ticks>>32);low=(unsigned long)(ticks&0xffffffffULL);expected=(unsigned long)(ticks/rate);
  assert(nd_system_seconds(high,low,rate)==expected);
 }
 assert(nd_system_seconds(0,123,0)==0);assert(nd_system_seconds(1,0,1)==0xffffffffUL);assert(nd_system_seconds(0xffffffffUL,0xffffffffUL,0xffffffffUL)==0xffffffffUL);
 nd_system_uptime(90061UL,text);assert(!strcmp(text,"1 d 01:01:01"));puts("SYSTEM: CPU flags, 64-bit boot-clock division and day/time formatting passed.");return 0;}
