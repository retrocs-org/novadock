#include "nd_alerts.h"
#include <string.h>
static void clean(char *out,unsigned int cap,const char *p)
{unsigned int n=0;if(!p)p="";while(*p && n+1<cap){unsigned char c=(unsigned char)*p++;if(c>=32 && c!=127)out[n++]=(char)c;}out[n]=0;}
void nd_alerts_add(struct NDAlerts *a,const char *source,const char *title,const char *text,unsigned int severity,unsigned long seconds)
{
 struct NDAlertEntry e;unsigned int i;if(!a || !title || !*title || severity>ND_ALERT_ERROR)return;
 memset(&e,0,sizeof(e));clean(e.source,sizeof(e.source),source);clean(e.title,sizeof(e.title),title);clean(e.text,sizeof(e.text),text);e.severity=(unsigned short)severity;e.seconds=seconds;
 if(a->count && !strcmp(a->entries[0].source,e.source) && !strcmp(a->entries[0].title,e.title) && !strcmp(a->entries[0].text,e.text) && a->entries[0].severity==e.severity && seconds>=a->entries[0].seconds && seconds-a->entries[0].seconds<30)return;
 if(a->count==ND_ALERT_MAX){if(!a->entries[a->count-1].read && a->unread)--a->unread;}else ++a->count;
 for(i=a->count-1;i>0;--i)a->entries[i]=a->entries[i-1];
 if(a->sequence<0xffffffffUL)++a->sequence;e.sequence=a->sequence;a->entries[0]=e;++a->unread;
}
void nd_alerts_ack(struct NDAlerts *a,unsigned long through)
{unsigned int i;if(!a)return;for(i=0;i<a->count;++i)if(!a->entries[i].read && a->entries[i].sequence<=through){a->entries[i].read=1;if(a->unread)--a->unread;}}
void nd_alerts_clear(struct NDAlerts *a)
{if(a){a->count=0;a->unread=0;}}
