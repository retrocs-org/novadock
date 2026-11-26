/* Disposable dock plugin: send a numbered internal notice on request. */
#include <exec/types.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_client.h"
long __stack=16384;
int main(int argc,char **argv)
{
 struct NDClient c;FILE *f;unsigned serial=0,body=0,last=0,acked=0,event,elapsed=0;char text[513],title[81];
 if(argc<2||!nd_client_init(&c,argv[1]))return 20;
 for(;;){event=nd_client_update_quiet(&c,"Test notifier","Ready",ND_NO_METER,elapsed,100);
  f=fopen("QATests:notis-trigger","r");if(f){if(fscanf(f,"%u %u",&serial,&body)!=2)serial=last;fclose(f);}
  if(c.host_alerts&&serial!=last){if(body>=90){memset(title,'T',80);title[80]=0;memset(text,'X',511);text[511]=(char)('A'+body-90);text[512]=0;}else{strcpy(title,"Forward test");sprintf(text,"Case %u",body);}event|=nd_client_notify(&c,title,text,1);last=serial;}
  if(last&&acked!=last){f=fopen("QATests:notis-ack","w");if(f){fprintf(f,"%u",last);if(!fclose(f))acked=last;}}
  if(event&ND_EVENT_QUIT)break;elapsed=nd_client_wait(&c,10);
 }
 nd_client_close(&c);return 0;
}
