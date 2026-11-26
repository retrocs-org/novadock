/* Optional asynchronous forwarding; the dock keeps its own session history. */
#ifndef ND_AMINOTIS_TEST
#include <proto/exec.h>
#include "aminotis_sdk/aminotis.h"
#endif
#include <string.h>
#include "nd_aminotis.h"
#define ND_NOTIS_QUEUE 8
struct NDNotisEntry {char source[32],title[81],body[513];unsigned int severity;};
static struct NDNotisEntry queue[ND_NOTIS_QUEUE];
static struct ANClient client;
static struct ANRequest request;
static struct ANResponse response;
static unsigned int head,count,age;
static int checking,ready;
static struct NDAmiNotisStats stats;
static void add_counter(unsigned long *value,unsigned long amount)
{if(amount>0xffffffffUL-*value)*value=0xffffffffUL;else *value+=amount;}
static void discard(unsigned int reason,unsigned int result)
{
 add_counter(&stats.dropped,count);
 if(reason==ND_AMINOTIS_FAILURE_TIMEOUT)add_counter(&stats.timeouts,1);
 if(reason==ND_AMINOTIS_FAILURE_UNAVAILABLE)add_counter(&stats.unavailable,1);
 if(reason==ND_AMINOTIS_FAILURE_REJECTED || reason==ND_AMINOTIS_FAILURE_CAPABILITY)add_counter(&stats.rejected,1);
 stats.last_failure=reason;stats.last_result=result;
 an_close(&client);head=count=age=0;checking=ready=0;
}
void nd_aminotis_stats(struct NDAmiNotisStats *out)
{if(out){*out=stats;out->queued=count;out->pending=client.pending!=NULL;}}
void nd_aminotis_reset_stats(void){memset(&stats,0,sizeof(stats));}
static void text_copy(char *out,unsigned int size,const char *text)
{unsigned int i=0;if(!text)text="";while(*text&&i+1<size){unsigned char c=(unsigned char)*text++;if(c>=32&&c!=127)out[i++]=(char)c;}out[i]=0;}
int nd_aminotis_available(void)
{int present;Forbid();present=FindPort(AN_PORT)!=NULL;Permit();return present;}
unsigned long nd_aminotis_signals(void)
{return client.reply&&client.pending ? 1UL<<client.reply->mp_SigBit:0;}
void nd_aminotis_close(void)
{add_counter(&stats.dropped,count);add_counter(&stats.cancelled,count);an_close(&client);head=count=age=0;checking=ready=0;}
void nd_aminotis_tick(int enabled,unsigned int milliseconds)
{
 int result;
 if(!enabled){if(client.reply||count)nd_aminotis_close();return;}
 if(client.pending){
  if(an_receive(&client,&response)){
   age=0;
   if(checking){ready=response.result==AN_OK&&(response.status.capabilities&AN_CAP_HISTORY)!=0;checking=0;if(!ready){discard(ND_AMINOTIS_FAILURE_CAPABILITY,response.result);return;}}
   else{if(response.result!=AN_OK&&response.result!=AN_REPLACED){discard(ND_AMINOTIS_FAILURE_REJECTED,response.result);return;}if(count){head=(head+1)%ND_NOTIS_QUEUE;--count;add_counter(&stats.delivered,1);}}
  }else{if(milliseconds>=3000-age){discard(ND_AMINOTIS_FAILURE_TIMEOUT,AN_TIMEOUT);return;}age+=milliseconds;return;}
 }
 if(!count)return;
 if(!nd_aminotis_available()){discard(ND_AMINOTIS_FAILURE_UNAVAILABLE,AN_UNAVAILABLE);return;}
 if(!ready){an_request(&request,AN_STATUS);checking=1;}
 else{
  an_request(&request,AN_POST);
  strcpy(request.event.app,"NovaDock");strcpy(request.event.name,"NovaDock");
  text_copy(request.event.source,sizeof(request.event.source),queue[head].source);
  text_copy(request.event.title,sizeof(request.event.title),queue[head].title);
  text_copy(request.event.body,sizeof(request.event.body),queue[head].body);
  request.event.severity=queue[head].severity;
 }
 result=an_submit(&client,&request);age=0;
 if(result!=AN_OK)discard(result==AN_UNAVAILABLE ? ND_AMINOTIS_FAILURE_UNAVAILABLE:ND_AMINOTIS_FAILURE_REJECTED,(unsigned int)result);
}
void nd_aminotis_post(int enabled,const char *source,const char *title,const char *text,unsigned int severity)
{
 struct NDNotisEntry *e;
 if(!enabled||!title||!*title||severity>AN_ERROR)return;
 add_counter(&stats.received,1);
 if(!nd_aminotis_available()){add_counter(&stats.dropped,1);add_counter(&stats.unavailable,1);stats.last_failure=ND_AMINOTIS_FAILURE_UNAVAILABLE;stats.last_result=AN_UNAVAILABLE;return;}
 if(count>=ND_NOTIS_QUEUE){add_counter(&stats.dropped,1);add_counter(&stats.queue_full,1);stats.last_failure=ND_AMINOTIS_FAILURE_QUEUE_FULL;stats.last_result=AN_BUSY;return;}
 e=&queue[(head+count)%ND_NOTIS_QUEUE];
 text_copy(e->source,sizeof(e->source),source);text_copy(e->title,sizeof(e->title),title);text_copy(e->body,sizeof(e->body),text);e->severity=severity;++count;
 nd_aminotis_tick(enabled,0);
}
