#include "nd_monitor.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
static int equal_name(const char *a,const char *b)
{while(*a && *b){if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))return 0;}return !*a && !*b;}
const struct NDMonitorRow *nd_monitor_row(const struct NDMonitorHistory *h,unsigned int index)
{return &h->rows[(h->first+index)%ND_MONITOR_HISTORY];}
void nd_monitor_add(struct NDMonitorHistory *h,const char *text,const char *path,unsigned int severity,int unread)
{
 struct NDMonitorRow *r;
 if(h->count==ND_MONITOR_HISTORY){r=&h->rows[h->first];h->first=(h->first+1)%ND_MONITOR_HISTORY;}
 else r=&h->rows[(h->first+h->count++)%ND_MONITOR_HISTORY];memset(r,0,sizeof(*r));strncpy(r->text,text,ND_MONITOR_TEXT-1);if(path)strncpy(r->path,path,ND_MONITOR_PATH-1);r->severity=severity;r->unread=unread!=0;
 if(unread && h->unseen<65535U)++h->unseen;++h->sequence;
}
void nd_monitor_read(struct NDMonitorHistory *h)
{unsigned int i;h->unseen=0;++h->sequence;for(i=0;i<h->count;++i)h->rows[i].unread=0;}
/* Keep copies below 32 KiB for classic compiler/runtime combinations. Only
 * initialized directory entries belong to the snapshot, not the spare slots. */
void nd_watch_snapshot_copy(struct NDWatchSnapshot *out,const struct NDWatchSnapshot *in)
{unsigned int i;out->valid=in->valid && in->count<=ND_WATCH_MAX;out->count=out->valid ? in->count:0;for(i=0;i<out->count;++i)out->entries[i]=in->entries[i];}
static int wildcard(const char *name,const char *pattern)
{const char *star=NULL,*retry=NULL;while(*name){if(*pattern=='*' || (pattern[0]=='#' && pattern[1]=='?')){pattern+=*pattern=='*' ? 1:2;star=pattern;retry=name;}else if(*pattern=='?' || tolower((unsigned char)*pattern)==tolower((unsigned char)*name)){++pattern;++name;}else if(star){pattern=star;name=++retry;}else return 0;}while(*pattern=='*' || (pattern[0]=='#' && pattern[1]=='?'))pattern+=*pattern=='*' ? 1:2;return !*pattern;}
static int patterns(const char *name,const char *list)
{char p[128];unsigned int n;while(list && *list){n=0;while(*list && *list!=';'){if(n+1>=sizeof(p))return 0;p[n++]=*list++;}p[n]=0;if(n && wildcard(name,p))return 1;if(*list)++list;}return 0;}
int nd_watch_name(const char *name,const char *include,const char *exclude)
{return (!include || !*include || patterns(name,include)) && (!exclude || !*exclude || !patterns(name,exclude));}
unsigned int nd_watch_changes(struct NDMonitorHistory *h,const struct NDWatchSnapshot *old,const struct NDWatchSnapshot *next,const char *drawer,const char *time)
{return nd_watch_filtered(h,old,next,drawer,time,"#?","");}
unsigned int nd_watch_filtered(struct NDMonitorHistory *h,const struct NDWatchSnapshot *old,const struct NDWatchSnapshot *next,const char *drawer,const char *time,const char *include,const char *exclude)
{
 unsigned int i,j,changes=0;char text[ND_MONITOR_TEXT],path[ND_MONITOR_PATH];const struct NDWatchEntry *a,*b;
 if(!drawer || !*drawer || !old->valid || !next->valid || old->count>ND_WATCH_MAX || next->count>ND_WATCH_MAX)return 0;
 for(i=0;i<next->count;++i){b=&next->entries[i];if(!nd_watch_name(b->name,include,exclude))continue;a=NULL;for(j=0;j<old->count;++j)if(equal_name(b->name,old->entries[j].name)){a=&old->entries[j];break;}
  if(a && a->size==b->size && a->days==b->days && a->minutes==b->minutes && a->ticks==b->ticks && a->key==b->key && a->type==b->type)continue;
  if(strlen(drawer)+strlen(b->name)+2>sizeof(path))continue;
  sprintf(path,"%s%s%s",drawer,drawer[strlen(drawer)-1]==':' || drawer[strlen(drawer)-1]=='/' ? "":"/",b->name);
  sprintf(text,"%.12s [%s] %.107s",time,a ? "changed":"new",b->name);nd_monitor_add(h,text,path,0,1);++changes;
 }
 return changes;
}
static int contains(const char *s,const char *word)
{const char *a,*b;for(;*s;++s){a=s;b=word;while(*a && *b && tolower((unsigned char)*a)==*b){++a;++b;}if(!*b)return 1;}return 0;}
static unsigned int line(struct NDMonitorHistory *h,struct NDLogParser *p)
{
 unsigned int severity=0;char text[ND_MONITOR_TEXT];
 if(!p->used)return 0;p->partial[p->used]=0;
 if(contains(p->partial,"error") || contains(p->partial,"fatal") || contains(p->partial,"panic"))severity=2;
 else if(contains(p->partial,"warn"))severity=1;
 sprintf(text,"%s%.151s",severity==2 ? "[ERROR] ":severity ? "[WARN] ":"",p->partial);
 nd_monitor_add(h,text,NULL,severity,severity!=0);return severity!=0;
}
void nd_log_reset(struct NDLogParser *p,int skip_partial)
{memset(p,0,sizeof(*p));p->skip=skip_partial!=0;}
unsigned int nd_log_feed(struct NDMonitorHistory *h,struct NDLogParser *p,const unsigned char *data,unsigned int length)
{
 unsigned int i,matches=0;unsigned char c;
 for(i=0;i<length;++i){c=data[i];
  if(c=='\n'){if(!p->skip)matches+=line(h,p);p->used=p->skip=p->escape=0;continue;}
  if(p->skip)continue;
  if(p->escape){if(p->escape==1 && c=='['){p->escape=2;continue;}if(p->escape==2 && c>=0x30 && c<=0x3f)continue;if(p->escape==2 && c>=0x20 && c<=0x2f)continue;p->escape=0;continue;}
  if(c==27){p->escape=1;continue;}if(c==155){p->escape=2;continue;}
  if(c=='\t')c=' ';if(c<32 || c==127)continue;
  if(p->used+1<sizeof(p->partial))p->partial[p->used++]=(char)c;
 }
 return matches;
}
int nd_log_replaced(const struct NDLogCursor *c,long key,long size,const unsigned char *prefix,unsigned int length)
{return c->valid && (key!=c->key || size<c->offset || length<c->prefix_length || memcmp(c->prefix,prefix,c->prefix_length));}
