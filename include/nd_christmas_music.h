#ifndef ND_CHRISTMAS_MUSIC_H
#define ND_CHRISTMAS_MUSIC_H
#include <stdio.h>
#include <string.h>
#define ND_XMAS_AUDIO_BLOCK 16384U
#define ND_XMAS_AUDIO_PERIOD 256U
struct NDXmasModule {unsigned long offset,length;};
static unsigned int nd_xmas_be16(const unsigned char *p){return ((unsigned int)p[0]<<8)|p[1];}
/* Validate the included sample-based MOD, not arbitrary third-party modules.
 * Its six sample chunks form one continuous PCM stream. No mixing or tracker
 * effects are required in the runtime, and no whole-song Chip RAM allocation. */
static int nd_xmas_module(FILE *f,struct NDXmasModule *out)
{
 unsigned char header[1084],pattern[1024];unsigned long total=0,end;int i,j;unsigned int size;
 if(!f||!out||fseek(f,0,SEEK_SET)||fread(header,1,sizeof(header),f)!=sizeof(header))return 0;
 if(memcmp(header,"Wintery Loop",12)||memcmp(header+1080,"M.K.",4)||header[950]!=6)return 0;
 for(i=0;i<31;++i){size=nd_xmas_be16(header+20+i*30+22)*2U;
  if(i<6){if(size<2||header[20+i*30+24]||header[20+i*30+25]!=64||nd_xmas_be16(header+20+i*30+26)||nd_xmas_be16(header+20+i*30+28)!=1)return 0;total+=size;}
  else if(size)return 0;
 }
 for(i=0;i<6;++i){if(header[952+i]!=i||fread(pattern,1,1024,f)!=1024)return 0;
  if(pattern[0]!=1||pattern[1]||pattern[2]!=(i+1)*16||pattern[3]||pattern[4]||pattern[5]||pattern[6]!=15||pattern[7]!=130)return 0;
  for(j=8;j<1024;++j)if(pattern[j] && !(i==5&&j==63*16+6&&pattern[j]==11))return 0;
 }
 out->offset=1084UL+6UL*1024UL;out->length=total;
 if(fseek(f,0,SEEK_END)||(end=(unsigned long)ftell(f))!=out->offset+total)return 0;
 return total>0;
}
#endif
