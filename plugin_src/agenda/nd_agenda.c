#include "nd_agenda.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#if defined(__VBCC__) || defined(__amigaos__)
#include <dos/dos.h>
#include <proto/dos.h>
#endif
static int leap(int y){return y%4==0&&(y%100!=0||y%400==0);}
static int days(int y,int m){static const int n[]={31,28,31,30,31,30,31,31,30,31,30,31};return n[m-1]+(m==2&&leap(y));}
static long ordinal(int y,int m,int d){long n=0;int i;for(i=1978;i<y;++i)n+=365+leap(i);for(i=1;i<m;++i)n+=days(y,i);return n+d-1;}
static int date_parts(long day,int *y,int *m,int *d){int n;*y=1978;*m=1;if(day<0||day>ordinal(2099,12,31))return 0;while(day>=(n=365+leap(*y))){day-=n;++*y;}while(day>=(n=days(*y,*m))){day-=n;++*m;}*d=(int)day+1;return 1;}
int nd_agenda_date(const char *s,long *day)
{int y,m,d;unsigned int i;char tail;if(!s||!day)return 0;if(!*s){*day=-1;return 1;}for(i=0;s[i];++i)if(i!=4&&i!=7&&!isdigit((unsigned char)s[i]))return 0;if(strlen(s)!=10||s[4]!='-'||s[7]!='-'||sscanf(s,"%d-%d-%d%c",&y,&m,&d,&tail)!=3||y<1978||y>2099||m<1||m>12||d<1||d>days(y,m))return 0;*day=ordinal(y,m,d);return 1;}
void nd_agenda_date_text(long day,char out[16]){int y,m,d;out[0]=0;if(date_parts(day,&y,&m,&d))sprintf(out,"%04d-%02d-%02d",y,m,d);}
int nd_agenda_time(const char *s,unsigned int *minute){int h,m;unsigned int i;char tail;if(!s||!minute)return 0;if(!*s){*minute=0;return 1;}for(i=0;s[i];++i)if(i!=2&&!isdigit((unsigned char)s[i]))return 0;if(strlen(s)!=5||s[2]!=':'||sscanf(s,"%d:%d%c",&h,&m,&tail)!=2||h<0||h>23||m<0||m>59)return 0;*minute=(unsigned int)(h*60+m);return 1;}
const char *nd_agenda_repeat_text(unsigned int r){static const char *n[]={"NONE","DAILY","WEEKLY","MONTHLY"};return r<4 ? n[r]:"NONE";}
int nd_agenda_repeat(const char *s){unsigned int i,j;if(!s)return -1;for(i=0;i<4;++i){const char *n=nd_agenda_repeat_text(i);for(j=0;s[j]&&n[j]&&toupper((unsigned char)s[j])==n[j];++j){}if(!s[j]&&!n[j])return (int)i;}return -1;}
static int title_valid(const char *s){unsigned int i;if(!s||!*s||strlen(s)>95)return 0;for(i=0;s[i];++i)if((unsigned char)s[i]<32||s[i]=='|'||s[i]==127)return 0;return 1;}
int nd_agenda_put(struct NDAgenda *a,int index,const char *title,const char *date,const char *time,const char *repeat)
{struct NDAgendaItem i;int r,y,m,d;if(!a||a->count>ND_AGENDA_MAX||index<-1||(index>=0&&(unsigned int)index>=a->count)||!title_valid(title))return 0;memset(&i,0,sizeof(i));i.notified=-1;r=nd_agenda_repeat(repeat);if(r<0||!nd_agenda_date(date,&i.day)||!nd_agenda_time(time,&i.minute)||(i.day<0&&r))return 0;i.repeat=(unsigned int)r;if(i.day>=0){date_parts(i.day,&y,&m,&d);i.anchor=(unsigned int)d;}strcpy(i.title,title);
 if(index<0){if(a->count==ND_AGENDA_MAX||a->serial>=1000000000UL)return 0;i.id=++a->serial;a->items[a->count++]=i;}else{i.id=a->items[index].id;a->items[index]=i;}return 1;}
int nd_agenda_complete(struct NDAgenda *a,unsigned int index,long today)
{struct NDAgendaItem i;int y,m,d,limit;if(!a||index>=a->count||today<0||today>ordinal(2099,12,31))return 0;i=a->items[index];if(!i.repeat){i.done=!i.done;a->items[index]=i;return 1;}
 if(i.day<0||!i.anchor||i.anchor>31)return 0;do{if(i.repeat<3)i.day+=i.repeat==1 ? 1:7;else{if(!date_parts(i.day,&y,&m,&d))return 0;if(++m>12){m=1;++y;}if(y>2099)return 0;limit=days(y,m);i.day=ordinal(y,m,i.anchor>(unsigned int)limit ? limit:(int)i.anchor);}}while(i.day<=today);
 if(i.day>ordinal(2099,12,31))return 0;i.notified=-1;i.done=0;a->items[index]=i;return 1;}
int nd_agenda_remove(struct NDAgenda *a,unsigned int index){unsigned int i;if(!a||index>=a->count)return 0;for(i=index+1;i<a->count;++i)a->items[i-1]=a->items[i];--a->count;return 1;}
int nd_agenda_due(const struct NDAgendaItem *i,long day){return i&&!i->done&&i->day>=0&&i->day<=day;}
int nd_agenda_remind(const struct NDAgendaItem *i,long day,unsigned int minute){return nd_agenda_due(i,day)&&i->notified!=i->day&&(i->day<day||minute>=i->minute);}
static int write_book(FILE *f,const struct NDAgenda *a){unsigned int k;if(a->count>ND_AGENDA_MAX||fprintf(f,"NDAGENDA1|%lu\n",a->serial)<0)return 0;for(k=0;k<a->count;++k){const struct NDAgendaItem *i=&a->items[k];if(!title_valid(i->title)||fprintf(f,"%lu|%ld|%u|%u|%u|%u|%ld|%s\n",i->id,i->day,i->minute,i->repeat,i->done,i->anchor,i->notified,i->title)<0)return 0;}return !ferror(f);}
static int integer(const char *s,long *out)
{unsigned long n=0;int negative=0;if(*s=='-'){negative=1;++s;}if(!*s)return 0;for(;*s;++s){unsigned int d=(unsigned char)*s-'0';if(d>9||n>100000000UL||(n==100000000UL&&d>0))return 0;n=n*10+d;}*out=negative ? -(long)n:(long)n;return 1;}
int nd_agenda_read(FILE *f,struct NDAgenda *a)
{struct NDAgenda next;char line[256],*end,*fields[8];unsigned int k,n;long values[7],serial;memset(&next,0,sizeof(next));if(!f||!a||!fgets(line,sizeof(line),f))return 0;end=strchr(line,'\n');if(!end||end[1])return 0;*end=0;if(strncmp(line,"NDAGENDA1|",10)||!integer(line+10,&serial)||serial<0)return 0;next.serial=(unsigned long)serial;
 while(fgets(line,sizeof(line),f)){struct NDAgendaItem *i;if(next.count>=ND_AGENDA_MAX)return 0;i=&next.items[next.count];end=strchr(line,'\n');if(!end||end[1])return 0;*end=0;fields[0]=line;for(n=0;n<7;++n){end=strchr(fields[n],'|');if(!end)return 0;*end=0;fields[n+1]=end+1;if(!integer(fields[n],&values[n]))return 0;}
 if(!title_valid(fields[7])||values[0]<=0||(unsigned long)values[0]>next.serial||values[1]<-1||values[1]>ordinal(2099,12,31)||values[2]<0||values[2]>1439||values[3]<0||values[3]>3||values[4]<0||values[4]>1||values[5]<0||values[5]>31||(values[1]>=0&&!values[5])||(values[1]<0&&values[3])||(values[6]!=-1&&values[6]!=values[1]))return 0;
 i->id=(unsigned long)values[0];i->day=values[1];i->minute=(unsigned int)values[2];i->repeat=(unsigned int)values[3];i->done=(unsigned int)values[4];i->anchor=(unsigned int)values[5];i->notified=values[6];for(k=0;k<next.count;++k)if(next.items[k].id==i->id)return 0;strcpy(i->title,fields[7]);++next.count;}if(ferror(f))return 0;*a=next;return 1;}
static int absent(const char *path)
{
#if defined(__VBCC__) || defined(__amigaos__)
 BPTR l=Lock((STRPTR)path,ACCESS_READ);LONG e;if(l){UnLock(l);return 0;}e=IoErr();return e==ERROR_OBJECT_NOT_FOUND||e==ERROR_DIR_NOT_FOUND;
#else
 (void)path;return errno==ENOENT;
#endif
}
int nd_agenda_save(const char *path,const struct NDAgenda *a)
{char tmp[300],bak[300];struct NDAgenda check;FILE *f;int old=0,ok;if(!path||strlen(path)>280||!a)return 0;sprintf(tmp,"%s.new",path);sprintf(bak,"%s.bak",path);f=fopen(path,"rb");if(f){ok=nd_agenda_read(f,&check);fclose(f);if(!ok)return 0;old=1;}else if(!absent(path))return 0;f=fopen(tmp,"wb");if(!f)return 0;ok=write_book(f,a);if(fclose(f))ok=0;if(!ok){remove(tmp);return 0;}f=fopen(tmp,"rb");if(!f){remove(tmp);return 0;}ok=nd_agenda_read(f,&check);fclose(f);if(!ok){remove(tmp);return 0;}if(old){remove(bak);if(rename(path,bak)){remove(tmp);return 0;}}if(rename(tmp,path)){if(old)rename(bak,path);remove(tmp);return 0;}return 1;}
