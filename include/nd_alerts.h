#ifndef ND_ALERTS_H
#define ND_ALERTS_H
#define ND_ALERT_MAX 64
#define ND_ALERT_PAGE 6
#define ND_ALERT_INFO 0
#define ND_ALERT_WARNING 1
#define ND_ALERT_ERROR 2
struct NDAlertEntry {unsigned long sequence,seconds;unsigned short severity,read;char source[20],title[32],text[96];};
struct NDAlerts {struct NDAlertEntry entries[ND_ALERT_MAX];unsigned long sequence;unsigned int count,unread;};
void nd_alerts_add(struct NDAlerts *a,const char *source,const char *title,const char *text,unsigned int severity,unsigned long seconds);
void nd_alerts_ack(struct NDAlerts *a,unsigned long through);
void nd_alerts_clear(struct NDAlerts *a);
#endif
