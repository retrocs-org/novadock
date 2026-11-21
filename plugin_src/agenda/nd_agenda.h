#ifndef ND_AGENDA_H
#define ND_AGENDA_H
#include <stdio.h>
#define ND_AGENDA_MAX 64
struct NDAgendaItem {unsigned long id;long day,notified;unsigned int minute,repeat,done,anchor;char title[96];};
struct NDAgenda {unsigned int count;unsigned long serial;struct NDAgendaItem items[ND_AGENDA_MAX];};
int nd_agenda_date(const char *s,long *day);
void nd_agenda_date_text(long day,char out[16]);
int nd_agenda_time(const char *s,unsigned int *minute);
int nd_agenda_repeat(const char *s);
const char *nd_agenda_repeat_text(unsigned int repeat);
int nd_agenda_put(struct NDAgenda *a,int index,const char *title,const char *date,const char *time,const char *repeat);
int nd_agenda_complete(struct NDAgenda *a,unsigned int index,long today);
int nd_agenda_remove(struct NDAgenda *a,unsigned int index);
int nd_agenda_due(const struct NDAgendaItem *i,long day);
int nd_agenda_remind(const struct NDAgendaItem *i,long day,unsigned int minute);
int nd_agenda_read(FILE *f,struct NDAgenda *a);
int nd_agenda_save(const char *path,const struct NDAgenda *a);
#endif
