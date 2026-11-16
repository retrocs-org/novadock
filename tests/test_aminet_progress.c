#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_aminet_progress.h"
static int record(const char *text,unsigned long expected,unsigned long size)
{FILE *f=tmpfile();unsigned long done=123,total=456;int ok;assert(f);fputs(text,f);rewind(f);ok=nd_aminet_progress_read(f,&done,&total);fclose(f);if(ok){assert(done==expected&&total==size);}else assert(done==123&&total==456);return ok;}
int main(void)
{
 unsigned long size,done;unsigned int percent;
 assert(nd_aminet_percent(0,0)==-1&&nd_aminet_percent(0,1)==0);
 assert(nd_aminet_percent(1,1)==100&&nd_aminet_percent(300,100)==100);
 for(size=1;size<=1000;++size)for(done=0;done<=size;++done)
  assert(nd_aminet_percent(done,size)==(int)(done*100UL/size));
 for(percent=0;percent<=100;++percent){done=(ND_AMINET_DOWNLOAD_LIMIT/100)*percent;assert(nd_aminet_percent(done,ND_AMINET_DOWNLOAD_LIMIT)==(int)(((unsigned long long)done*100)/ND_AMINET_DOWNLOAD_LIMIT));}
 assert(nd_aminet_percent(42949672UL,4294967295UL)==0);
 assert(nd_aminet_percent(2147483647UL,4294967295UL)==49);
 assert(record("0 0\n",0,0));assert(record("50 100\n",50,100));assert(record("67108864 67108864\n",67108864UL,67108864UL));
 assert(!record("50 100",0,0));assert(!record("",0,0));assert(!record("50 ",0,0));assert(!record("101 100\n",0,0));assert(!record("1 0\n",0,0));assert(!record("1 67108865\n",0,0));assert(!record("-1 100\n",0,0));assert(!record("1 100 junk\n",0,0));
 puts("AMINET progress: exact percentages, 32-bit bounds, partial records and invalid sizes passed.");return 0;
}
