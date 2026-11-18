#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_warp_diag.h"
static void put32(unsigned char *p,unsigned long n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
int main(void)
{
 unsigned char buffer[ND_WARP_DIAG_SIZE],original[ND_WARP_DIAG_SIZE];struct NDSensors set,copy;int i;
 assert(nd_warp_diag_supported(30,74));
 assert(!nd_warp_diag_supported(30,73) && !nd_warp_diag_supported(30,75) && !nd_warp_diag_supported(31,74));
 /* Unrelated output deliberately contains nonzero bytes and private data. */
 memset(buffer,0xda,sizeof(buffer));strcpy((char *)buffer+212,"DO-NOT-PRINT-PRIVATE-SETTINGS");
 put32(buffer+44,0x4219999aUL);put32(buffer+48,0x420d3333UL);put32(buffer+52,0x42626666UL);
 put32(buffer+56,75);put32(buffer+60,0x409a3d71UL);put32(buffer+64,0x405147aeUL);buffer[37]=8;
 memcpy(original,buffer,sizeof(buffer));memset(&set,0,sizeof(set));assert(nd_sensor_put(&set,0,52000,99));
 assert(nd_warp_diag_decode(buffer,sizeof(buffer),&set,100)==7);
 assert(set.available==(0x3f80UL|1) && set.value[0]==52000 && set.stamp[0]==99);
 assert(set.value[7]==38400 && set.value[8]==35300 && set.value[9]==56600);
 assert(set.value[10]==75000 && set.value[11]==4820 && set.value[12]==3270 && set.value[13]==8000);
 for(i=7;i<ND_SENSOR_COUNT;++i)assert(set.stamp[i]==100);
 assert(!memcmp(original,buffer,sizeof(buffer)));
 copy=set;
 assert(!nd_warp_diag_decode(buffer,sizeof(buffer)-1,&set,101));assert(!memcmp(&copy,&set,sizeof(set)));
 assert(!nd_warp_diag_decode(NULL,sizeof(buffer),&set,101));assert(!nd_warp_diag_decode(buffer,sizeof(buffer),NULL,101));
 put32(buffer+44,0xc0133333UL);put32(buffer+48,0x80000000UL);put32(buffer+56,0xfffffffeUL);
 assert(nd_warp_diag_decode(buffer,sizeof(buffer),&set,101)==7);
 assert(set.value[7]==-2300 && set.value[8]==0 && set.value[10]==-2000);
 /* Invalid new readings must not reuse a prior successful sample. */
 put32(buffer+44,0x7f800000UL);put32(buffer+48,0x7fc00000UL);put32(buffer+52,0xff800000UL);
 put32(buffer+56,0x80000000UL);put32(buffer+60,0x7f7fffffUL);put32(buffer+64,0xbf800000UL);buffer[37]=255;
 assert(!nd_warp_diag_decode(buffer,sizeof(buffer),&set,102));assert(set.available==1);
 /* Finite zeros are legitimate, including negative zero and subnormals. */
 memset(buffer,0,sizeof(buffer));put32(buffer+48,1);put32(buffer+52,0x80000000UL);
 assert(nd_warp_diag_decode(buffer,sizeof(buffer),&set,103)==7);
 for(i=7;i<ND_SENSOR_COUNT;++i)assert(set.value[i]==0);
 put32(buffer+56,0xffffffc4UL);assert(nd_warp_diag_decode(buffer,sizeof(buffer),&set,104)==7 && set.value[10]==-60000);
 put32(buffer+56,0xffffffc3UL);assert(nd_warp_diag_decode(buffer,sizeof(buffer),&set,105)==6 && !(set.available&(1UL<<10)));
 puts("Warp diagnostic decoder: 30.74 gate, seven fields, units, invalid/zero values, bounds and unrelated-field isolation passed.");return 0;
}
