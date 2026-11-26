/* Observe the real service through its public SDK in an isolated guest. */
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "../src/aminotis_sdk/aminotis.h"
long __stack=32768;
static struct ANResponse response;
static struct {ULONG id,epoch,fraction;} seen[64];
int main(void)
{
 struct ANClient client={0};struct ANRequest request;struct MsgPort *port;BPTR guard;unsigned i,j,n=0;FILE *f;char line[800];int result;
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);
 port=CreateMsgPort();if(!port)return 20;port->mp_Node.ln_Name="NovaDock.QA.NotisObserver";AddPort(port);
 while(!(SetSignal(0,0)&SIGBREAKF_CTRL_C)){
  an_request(&request,AN_LIST);strcpy(request.event.app,"NovaDock");result=an_call(&client,&request,&response,20);an_close(&client);
  if(result==AN_OK)for(i=0;i<response.count&&i<AN_PAGE;++i){
   struct ANEvent *e=&response.page.events[i];if(strcmp(e->app,"NovaDock"))continue;
   for(j=0;j<n;++j)if(seen[j].id==e->id&&seen[j].epoch==response.status.epoch&&seen[j].fraction==response.status.epoch_fraction)break;
   if(j<n||n==64)continue;
   sprintf(line,"%s|%s|%s|%s|%lu\n",e->app,e->source,e->title,e->body,e->severity);
   f=fopen("QATests:notis-received.log","a");if(f){fputs(line,f);if(!fclose(f)){seen[n].id=e->id;seen[n].epoch=response.status.epoch;seen[n].fraction=response.status.epoch_fraction;++n;}}
  }
  Delay(5);
 }
 an_close(&client);RemPort(port);DeleteMsgPort(port);return 0;
}
