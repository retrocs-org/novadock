/* Test-only client: drive every packaged frame through the real dock ABI. */
#include <exec/types.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_client.h"
long __stack=16384;
int main(int argc,char **argv)
{struct NDClient c;FILE *f;char name[24],next[24];unsigned int state=0,serial=0,next_state,next_serial,last=0,event,elapsed=0;
 if(argc<2||!nd_client_init(&c,argv[1]))return 20;name[0]=0;
 for(;;){f=fopen("QATests:icon-state","r");if(f){if(fscanf(f,"%23s %u %u",next,&next_state,&next_serial)==3){strcpy(name,next);state=next_state;serial=next_serial;}fclose(f);}
  if(name[0]){event=nd_client_update_art_quiet(&c,"Sensor temperature","33.8 C",name,state,elapsed,100U);if(serial!=last){f=fopen("QATests:icon-ack","w");if(f){fprintf(f,"%u meta=%u mode=%u key=%lu state=%u",serial,c.host_artwork_meta,c.art_mode,c.art_key,c.art_state);if(!fclose(f))last=serial;}}}
  else event=nd_client_update_quiet(&c,"WAIT","...",ND_NO_METER,elapsed,100U);
  if(event&ND_EVENT_QUIT)break;elapsed=nd_client_wait(&c,2);
 }
 nd_client_close(&c);return 0;}
