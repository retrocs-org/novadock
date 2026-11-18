#include "nd_shelf.h"
#include <string.h>
#include <ctype.h>
static int valid(const char *p)
{unsigned int i;if(!p || !*p || strlen(p)>=ND_SHELF_PATH || !strchr(p,':'))return 0;for(i=0;p[i];++i)if((unsigned char)p[i]<32 || p[i]==127)return 0;return 1;}
static int same(const char *a,const char *b)
{while(*a && *b){if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++))return 0;}return !*a && !*b;}
int nd_shelf_id(const char *id)
{unsigned int i;if(!id || strncmp(id,"SHELF",5))return 0;if(!id[5])return 1;if(id[5]!='_' || !id[6])return 0;for(i=6;id[i];++i)if(id[i]<'0'||id[i]>'9')return 0;return i<20;}
int nd_shelf_add(struct NDShelf *s,const char *path)
{unsigned int i;if(!s || !valid(path) )return 0;for(i=0;i<s->count;++i)if(same(s->paths[i],path))return 1;if(s->count>=ND_SHELF_MAX)return 0;strcpy(s->paths[s->count++],path);return 1;}
int nd_shelf_remove(struct NDShelf *s,unsigned int index)
{unsigned int i;if(!s || index>=s->count)return 0;for(i=index+1;i<s->count;++i)strcpy(s->paths[i-1],s->paths[i]);--s->count;return 1;}
int nd_shelf_move(struct NDShelf *s,unsigned int index,int dir)
{char t[ND_SHELF_PATH];unsigned int next;if(!s || index>=s->count || (dir!=-1 && dir!=1) || (dir<0 && !index))return 0;next=(unsigned int)((int)index+dir);if(next>=s->count)return 0;strcpy(t,s->paths[index]);strcpy(s->paths[index],s->paths[next]);strcpy(s->paths[next],t);return 1;}
int nd_shelf_read(FILE *f,struct NDShelf *s)
{struct NDShelf next;char line[ND_SHELF_PATH+4];char *p;memset(&next,0,sizeof(next));if(!f || !s || !fgets(line,sizeof(line),f) || strcmp(line,"NDSHELF1\n"))return 0;while(fgets(line,sizeof(line),f)){p=strchr(line,'\n');if(!p)return 0;*p=0;if(!nd_shelf_add(&next,line))return 0;}if(ferror(f))return 0;*s=next;return 1;}
int nd_shelf_write(FILE *f,const struct NDShelf *s)
{unsigned int i;if(!f || !s || s->count>ND_SHELF_MAX || fputs("NDSHELF1\n",f)<0)return 0;for(i=0;i<s->count;++i)if(!valid(s->paths[i]) || fprintf(f,"%s\n",s->paths[i])<0)return 0;return !ferror(f);}
int nd_shelf_save(const char *path,const struct NDShelf *s)
{
 char tmp[300],bak[300];FILE *f;int ok,backed=0;if(!path || strlen(path)>280)return 0;sprintf(tmp,"%s.tmp",path);sprintf(bak,"%s.bak",path);
 f=fopen(tmp,"w");if(!f)return 0;ok=nd_shelf_write(f,s);if(fclose(f))ok=0;if(!ok){remove(tmp);return 0;}
 f=fopen(path,"r");if(f){fclose(f);remove(bak);if(rename(path,bak)){remove(tmp);return 0;}backed=1;}
 if(rename(tmp,path)){if(backed)rename(bak,path);remove(tmp);return 0;}return 1;
}
