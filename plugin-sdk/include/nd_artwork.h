#ifndef ND_ARTWORK_H
#define ND_ARTWORK_H
/* Plugin-owned 32x32 palette artwork. File integers are big-endian bytes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define ND_ARTWORK_VALUE 1U
#define ND_ARTWORK_LARGE 2U
#define ND_ARTWORK_DURATION 4U
#define ND_ARTWORK_MAX_BYTES 262144UL
#define ND_ARTWORK_MAX_FRAMES 256U
#define ND_ARTWORK_RECORD 32U
#define ND_ARTWORK_NAME 24U
#define ND_ARTWORK_PATH 520U
struct NDArtwork {unsigned char owned_pens[16];unsigned char *data;unsigned long size,mapped[16],*keys;char path[ND_ARTWORK_PATH];};
static unsigned long nd_artwork_u32(const unsigned char *p)
{return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
static unsigned int nd_artwork_u16(const unsigned char *p)
{return ((unsigned int)p[0]<<8)|p[1];}
static int nd_artwork_name(const char *s)
{unsigned int i;if(!s || !*s)return 0;for(i=0;s[i];++i)if(i>=ND_ARTWORK_NAME-1 || !((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')||s[i]=='_'||s[i]=='-'))return 0;return 1;}
static int nd_artwork_path(const char *base,const char *program,char *out,unsigned int cap)
{
 unsigned long n;if(!base || !program || !*program || !out || !cap)return 0;
 n=(unsigned long)strlen(program)+sizeof(".assets/Artwork.ndart");
 if(!strchr(program,':') && program[0]!='/')n+=strlen(base)+1;
 if(n>cap)return 0;out[0]=0;
 if(!strchr(program,':') && program[0]!='/'){strcpy(out,base);n=strlen(out);if(n && out[n-1]!=':' && out[n-1]!='/')strcat(out,"/");}
 strcat(out,program);strcat(out,".assets/Artwork.ndart");return 1;
}
static unsigned int nd_artwork_header(const unsigned char *d)
{return d[5]==2 ? 72U:56U;}
static unsigned long nd_artwork_key(const char *name)
{unsigned long key=2166136261UL;while(*name){key^=(unsigned char)*name++;key=(key*16777619UL)&0xffffffffUL;}return key ? key:1UL;}
static int nd_artwork_feature(const char *features)
{const char *p=features,*q;if(!p)return 0;while(*p){while(*p==',' || *p==' ')++p;q=p;while(*p && *p!=',')++p;while(q<p && p[-1]==' ')--p;if(p-q==7 && !strncmp(q,"artwork",7))return 1;while(*p && *p!=',')++p;}return 0;}
static int nd_artwork_validate(const unsigned char *d,unsigned long size)
{
 unsigned int frames,i,j,count,header;unsigned long offset,table,keys[ND_ARTWORK_MAX_FRAMES];const unsigned char *r,*p;
 if(!d || size<56 || size>ND_ARTWORK_MAX_BYTES || (memcmp(d,"NDAR\000",5) || (d[5]!=1 && d[5]!=2)))return 0;
 header=nd_artwork_header(d);if(size<header)return 0;
 if(d[5]==2){for(i=0;i<16;++i)if(d[56+i]>5 || (!i && d[56+i]))return 0;}
 frames=nd_artwork_u16(d+6);if(!frames || frames>ND_ARTWORK_MAX_FRAMES)return 0;table=(unsigned long)header+(unsigned long)frames*ND_ARTWORK_RECORD;if(table>size)return 0;
 for(i=0;i<frames;++i){r=d+header+(unsigned long)i*ND_ARTWORK_RECORD;
  if(!memchr(r,0,ND_ARTWORK_NAME) || !nd_artwork_name((const char *)r) || (d[5]==1 ? r[25]!=0:(r[25]&~7U)!=0))return 0;
  keys[i]=nd_artwork_key((const char *)r);
  for(j=0;j<i;++j){p=d+header+(unsigned long)j*ND_ARTWORK_RECORD;if(keys[i]==keys[j] && strcmp((const char *)r,(const char *)p))return 0;if(r[24]==p[24]&&!strcmp((const char *)r,(const char *)p))return 0;}
  count=nd_artwork_u16(r+26);offset=nd_artwork_u32(r+28);if(!count || count>1024 || offset<table || offset>size || (unsigned long)count*5UL>size-offset)return 0;
  for(j=0;j<count;++j){p=d+offset+(unsigned long)j*5;if(!p[2] || !p[3] || p[0]+p[2]>32 || p[1]+p[3]>32 || !p[4] || p[4]>15)return 0;}
 }
 return 1;
}
static void nd_artwork_close(struct NDArtwork *a)
{if(a){free(a->data);free(a->keys);a->data=NULL;a->keys=NULL;a->size=0;}}
static int nd_artwork_load(struct NDArtwork *a,const char *path)
{
 FILE *f;long size;unsigned char *d;int ok;unsigned int i,n;if(!a || !path || strlen(path)>=sizeof(a->path))return 0;
 nd_artwork_close(a);strcpy(a->path,path);f=fopen(path,"rb");if(!f)return 0;
 if(fseek(f,0,SEEK_END) || (size=ftell(f))<56 || (unsigned long)size>ND_ARTWORK_MAX_BYTES || fseek(f,0,SEEK_SET)){fclose(f);return 0;}
 d=(unsigned char *)malloc((unsigned long)size);if(!d){fclose(f);return 0;}
 ok=fread(d,1,(unsigned long)size,f)==(unsigned long)size;if(fclose(f))ok=0;
 if(!ok || !nd_artwork_validate(d,(unsigned long)size)){free(d);return 0;}
 n=nd_artwork_u16(d+6);a->keys=(unsigned long *)malloc(n*sizeof(*a->keys));if(!a->keys){free(d);return 0;}
 for(i=0;i<n;++i)a->keys[i]=nd_artwork_key((const char *)(d+nd_artwork_header(d)+(unsigned long)i*ND_ARTWORK_RECORD));
 a->data=d;a->size=(unsigned long)size;return 1;
}
static const unsigned char *nd_artwork_frame(const struct NDArtwork *a,const char *name,unsigned int state,unsigned int *count)
{
 unsigned int i,n;const unsigned char *r;if(count)*count=0;if(!a || !a->data || !name || state>255)return NULL;
 n=nd_artwork_u16(a->data+6);for(i=0;i<n;++i){r=a->data+nd_artwork_header(a->data)+(unsigned long)i*ND_ARTWORK_RECORD;if(r[24]==state&&!strcmp((const char *)r,name)){if(count)*count=nd_artwork_u16(r+26);return a->data+nd_artwork_u32(r+28);}}
 return NULL;
}
static const unsigned char *nd_artwork_key_frame(const struct NDArtwork *a,unsigned long key,unsigned int state,unsigned int *count,unsigned int *flags)
{unsigned int i,n;const unsigned char *r;if(count)*count=0;if(flags)*flags=0;if(!a || !a->data || !key || state>255)return NULL;n=nd_artwork_u16(a->data+6);for(i=0;i<n;++i){r=a->data+nd_artwork_header(a->data)+(unsigned long)i*ND_ARTWORK_RECORD;if(r[24]==state && (a->keys ? a->keys[i]:nd_artwork_key((const char *)r))==key){if(count)*count=nd_artwork_u16(r+26);if(flags)*flags=r[25];return a->data+nd_artwork_u32(r+28);}}return NULL;}
/* Package health checks are used by discovery and import, never by repaint. */
static int nd_artwork_package_problem(const char *program,const char *features,char *out,unsigned int cap)
{struct NDArtwork a;char path[ND_ARTWORK_PATH];FILE *f;unsigned char header[6];const char *why=NULL;int required=nd_artwork_feature(features);memset(&a,0,sizeof(a));if(out && cap)*out=0;
 if(!nd_artwork_path("",program,path,sizeof(path)))why="artwork path is too long; reinstall plugin";
 else{errno=0;f=fopen(path,"rb");if(!f){if(required || errno!=ENOENT)why=errno==ENOENT ? "artwork bank is missing; reinstall plugin":"artwork bank cannot be read; check installation";}
 else{size_t n=fread(header,1,6,f);fclose(f);if(n==6 && !memcmp(header,"NDAR\0",5) && header[5]>2)why="artwork format is newer; update NovaDock";else if(!nd_artwork_load(&a,path))why="artwork bank is invalid; reinstall plugin";nd_artwork_close(&a);}}
 if(why && out && cap){strncpy(out,why,cap-1);out[cap-1]=0;}return why!=NULL;}
/* A reserved caption is used only after the host advertises artwork support. */
static int nd_artwork_caption(char out[32],const char *name,unsigned int state)
{if(!out || !nd_artwork_name(name) || state>255)return 0;sprintf(out,"ART:%s:%u",name,state);return 1;}
static int nd_artwork_parse(const char *caption,char name[24],unsigned int *state)
{
 const char *p;unsigned int n=0,v=0;if(!caption || strncmp(caption,"ART:",4))return 0;p=caption+4;
 while(*p && *p!=':'){if(n>=23)return 0;name[n++]=*p++;}name[n]=0;if(*p++!=':' || !nd_artwork_name(name) || !*p)return 0;
 while(*p){if(*p<'0'||*p>'9')return 0;v=v*10+(unsigned int)(*p++-'0');if(v>255)return 0;}*state=v;return 1;
}
#endif
