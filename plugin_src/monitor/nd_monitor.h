#ifndef ND_MONITOR_H
#define ND_MONITOR_H
#define ND_MONITOR_HISTORY 64
#define ND_MONITOR_TEXT 160
#define ND_MONITOR_PATH 240
#define ND_WATCH_MAX 256
#define ND_LOG_CHUNK 8192
struct NDMonitorRow {char text[ND_MONITOR_TEXT],path[ND_MONITOR_PATH];unsigned int severity,unread;};
struct NDMonitorHistory {struct NDMonitorRow rows[ND_MONITOR_HISTORY];unsigned int count,unseen,first;unsigned long sequence;};
struct NDWatchEntry {char name[108];long size,days,minutes,ticks,key,type;};
struct NDWatchSnapshot {unsigned int count,valid;struct NDWatchEntry entries[ND_WATCH_MAX];};
struct NDLogParser {char partial[ND_MONITOR_TEXT];unsigned int used,skip,escape;};
/* Cursor belongs to one file identity, including a prefix to catch replacement
 * on filesystems which recycle disk keys. Signed offsets match AmigaDOS Seek. */
struct NDLogCursor {long offset,key;unsigned int valid,prefix_length;unsigned char prefix[32];};
const struct NDMonitorRow *nd_monitor_row(const struct NDMonitorHistory *h,unsigned int index);
void nd_monitor_add(struct NDMonitorHistory *h,const char *text,const char *path,unsigned int severity,int unread);
void nd_monitor_read(struct NDMonitorHistory *h);
void nd_watch_snapshot_copy(struct NDWatchSnapshot *out,const struct NDWatchSnapshot *in);
int nd_watch_name(const char *name,const char *include,const char *exclude);
unsigned int nd_watch_filtered(struct NDMonitorHistory *h,const struct NDWatchSnapshot *old,const struct NDWatchSnapshot *next,const char *drawer,const char *time,const char *include,const char *exclude);
unsigned int nd_watch_changes(struct NDMonitorHistory *h,const struct NDWatchSnapshot *old,const struct NDWatchSnapshot *next,const char *drawer,const char *time);
void nd_log_reset(struct NDLogParser *p,int skip_partial);
unsigned int nd_log_feed(struct NDMonitorHistory *h,struct NDLogParser *p,const unsigned char *data,unsigned int length);
int nd_log_replaced(const struct NDLogCursor *c,long key,long size,const unsigned char *prefix,unsigned int length);
#endif
