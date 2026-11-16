#ifndef ND_PLACES_H
#define ND_PLACES_H
#include <string.h>
#include <ctype.h>
#define ND_PLACES_MAX 32
#define ND_PLACES_BOOKMARKS 5
struct NDPlace {char label[48],path[240];};
struct NDPlaces {struct NDPlace items[ND_PLACES_MAX];int count;};
static int nd_places_path(const char *s)
{const char *p;if(!s||!*s||strlen(s)>=240||!strchr(s,':'))return 0;for(p=s;*p;++p)if((unsigned char)*p<32||*p==127)return 0;return 1;}
static int nd_places_same(const char *a,const char *b)
{size_t n=strlen(a),m=strlen(b),i;while(n&&a[n-1]=='/')--n;while(m&&b[m-1]=='/')--m;if(n!=m)return 0;for(i=0;i<n;++i)if(tolower((unsigned char)a[i])!=tolower((unsigned char)b[i]))return 0;return 1;}
static int nd_places_add(struct NDPlaces *p,const char *label,const char *path)
{
 int i;const char *name;
 if(!p||!nd_places_path(path)||p->count>=ND_PLACES_MAX)return 0;
 for(i=0;i<p->count;++i)if(nd_places_same(p->items[i].path,path))return 0;
 if(!label||!*label){name=strrchr(path,'/');label=name&&name[1] ? name+1:path;}
 strncpy(p->items[p->count].label,label,47);p->items[p->count].label[47]=0;
 strcpy(p->items[p->count].path,path);++p->count;return 1;
}
#endif
