/* Opt-in, one-shot diagnostic. Not installed or invoked by the dock. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>
#include "nd_warp_diag.h"
#include "nd_cswarp_probe.h"
long __stack=32768;
struct Guarded {unsigned char before[16],data[ND_WARP_DIAG_SIZE],after[16];};
static void erase(void *p,unsigned int size)
{volatile unsigned char *b=p;while(size--)*b++=0;}
int main(int argc,char **argv)
{
 struct Library *library;struct Guarded *buffer;struct NDSensors set;
 unsigned int i;int result=0,ok,count;char value[40];
 if(argc!=2 || strcmp(argv[1],"--read-sensors")){
  puts("WarpSensorProbe --read-sensors");
  puts("Experimental: one diagnostic read using cswarp.library 30.74 only.");
  puts("Prints sensor values only. Compare with WarpTool Diag on real hardware.");
  puts("The vendor call can block if the board does not reply.");
  return argc==1 ? 0:10;
 }
 library=OpenLibrary("cswarp.library",30);
 if(!library){puts("cswarp.library 30+ is not available.");return 10;}
 printf("cswarp.library %u.%u\n",(unsigned int)library->lib_Version,(unsigned int)library->lib_Revision);
 if(!nd_warp_diag_supported(library->lib_Version,library->lib_Revision)){
  CloseLibrary(library);puts("Unsupported library version; no diagnostic call made.");return 10;
 }
 buffer=AllocVec(sizeof(*buffer),MEMF_PUBLIC|MEMF_CLEAR);
 if(!buffer){CloseLibrary(library);puts("Not enough memory.");return 20;}
 memset(buffer->before,0xa5,sizeof(buffer->before));memset(buffer->after,0xa5,sizeof(buffer->after));
 puts("Reading one diagnostic frame...");fflush(stdout);
 ok=nd_cswarp_probe_diag(library,buffer->data)!=0;
 CloseLibrary(library);
 for(i=0;i<sizeof(buffer->before);++i)
  if(buffer->before[i]!=0xa5 || buffer->after[i]!=0xa5)result=20;
 if(result)puts("Result buffer guard changed; stop testing this interface.");
 else if(!ok){puts("The diagnostic call failed; no sensor readings used.");result=10;}
 else{
  memset(&set,0,sizeof(set));count=nd_warp_diag_decode(buffer->data,sizeof(buffer->data),&set,0);
  /* The full result also contains unrelated settings. Never print/dump it. */
  erase(buffer,sizeof(*buffer));
  for(i=7;i<ND_SENSOR_COUNT;++i){
   if(set.available&(1UL<<i))nd_sensor_format((int)i,set.value[i],value);
   else strcpy(value,"unavailable / invalid");
   printf("%s: %s\n",nd_sensor_defs[i].label,value);
  }
  if(count!=7){puts("Some readings were invalid; compare against WarpTool Diag.");result=5;}
 }
 erase(buffer,sizeof(*buffer));FreeVec(buffer);return result;
}
