/* Bounded GitHub metadata and version policy; independent of AmigaOS. */
#include "nd_update.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
static const char *white(const char *p){while(*p && strchr(" \t\r\n",*p))++p;return p;}
static const char *string_end(const char *p)
{
 unsigned int i;if(*p++!='"')return NULL;
 while(*p && *p!='"'){
  if((unsigned char)*p<32)return NULL;
  if(*p++=='\\'){if(!*p)return NULL;
   if(*p=='u'){++p;for(i=0;i<4;++i){if(!isxdigit((unsigned char)*p))return NULL;++p;}}
   else{if(!strchr("\"\\/bfnrt",*p))return NULL;++p;}
  }
 }
 return *p=='"' ? p+1:NULL;
}
static const char *skip(const char *p,unsigned int depth)
{
 const char *q;char close;if(depth>16)return NULL;p=white(p);
 if(*p=='"')return string_end(p);
 if(*p=='{' || *p=='['){close=*p=='{' ? '}':']';p=white(p+1);if(*p==close)return p+1;
  for(;;){if(close=='}'){q=string_end(p);if(!q)return NULL;p=white(q);if(*p++!=':')return NULL;}
   p=skip(p,depth+1);if(!p)return NULL;p=white(p);if(*p==close)return p+1;if(*p++!=',')return NULL;p=white(p);
  }
 }
 if(!strncmp(p,"true",4))return p+4;if(!strncmp(p,"false",5))return p+5;if(!strncmp(p,"null",4))return p+4;
 q=p;if(*p=='-')++p;
 if(*p=='0')++p;else{if(*p<'1' || *p>'9')return NULL;while(isdigit((unsigned char)*p))++p;}
 if(*p=='.'){++p;if(!isdigit((unsigned char)*p))return NULL;while(isdigit((unsigned char)*p))++p;}
 if(*p=='e' || *p=='E'){++p;if(*p=='+' || *p=='-')++p;if(!isdigit((unsigned char)*p))return NULL;while(isdigit((unsigned char)*p))++p;}
 return p>q ? p:NULL;
}
/* Reject duplicate relevant keys rather than choosing an ambiguous value. */
static const char *member(const char *p,const char *name)
{
 const char *end,*value=NULL;unsigned int n=(unsigned int)strlen(name);
 p=white(p);if(*p++!='{')return NULL;p=white(p);
 while(*p && *p!='}'){
  end=string_end(p);if(!end)return NULL;
  if((unsigned int)(end-p)==n+2 && !memcmp(p+1,name,n)){
   if(value)return NULL;value=white(end);if(*value++!=':')return NULL;value=white(value);
  }
  p=white(end);if(*p++!=':')return NULL;p=skip(p,0);if(!p)return NULL;p=white(p);
  if(*p=='}')break;if(*p++!=',')return NULL;p=white(p);
 }
 return value;
}
static int text(const char *p,char *out,unsigned int cap)
{
 unsigned int n=0,i,v;unsigned char c;
 if(!p || !cap || !string_end(p))return 0;++p;
 while(*p!='"'){
  c=(unsigned char)*p++;
  if(c=='\\'){
   c=(unsigned char)*p++;
   if(c=='u'){v=0;for(i=0;i<4;++i){v=v*16+(unsigned int)(isdigit((unsigned char)*p) ? *p-'0':tolower((unsigned char)*p)-'a'+10);++p;}if(v<32 || v>126)return 0;c=(unsigned char)v;}
   else if(!strchr("\"\\/",c))return 0;
  }
  if(c<32 || c>126 || n+1>=cap)return 0;out[n++]=(char)c;
 }
 out[n]=0;return 1;
}
static int version(const char *p,unsigned int parts[4],char *bare)
{
 unsigned int count=0,n=0,value,digits;const char *start;
 if(!p)return 0;if(*p=='v' || *p=='V')++p;start=p;
 memset(parts,0,4*sizeof(*parts));
 for(;;){
  if(count==4 || !isdigit((unsigned char)*p))return 0;
  value=0;digits=0;do{if(++digits>5)return 0;value=value*10+*p++-'0';if(value>65535)return 0;}while(isdigit((unsigned char)*p));
  parts[count++]=value;if(!*p)break;if(*p++!='.')return 0;
 }
 n=(unsigned int)(p-start);if(count<2 || n>=32)return 0;if(bare){memcpy(bare,start,n);bare[n]=0;}return 1;
}
int nd_update_compare(const char *a,const char *b)
{
 unsigned int x[4],y[4],i;if(!version(a,x,NULL) || !version(b,y,NULL))return 2;
 for(i=0;i<4;++i){if(x[i]>y[i])return 1;if(x[i]<y[i])return -1;}return 0;
}
static int number(const char *p,unsigned long *n)
{
 unsigned long v=0;if(!p || !isdigit((unsigned char)*p))return 0;
 do{if(v>ND_UPDATE_FILE_LIMIT/10UL)return 0;v=v*10UL+*p++-'0';if(v>ND_UPDATE_FILE_LIMIT)return 0;}while(isdigit((unsigned char)*p));
 if(!strchr(",} \t\r\n",*p))return 0;*n=v;return 1;
}
static int is_false(const char *p){return p && !strncmp(p,"false",5) && strchr(",} \t\r\n",p[5])!=NULL;}
int nd_update_release(const char *json,struct NDUpdateRelease *r)
{
 struct NDUpdateRelease parsed;const char *p,*end,*asset,*v;char name[96],state[20],url[260],expected[260],sumname[96],digest[80];
 unsigned int parts[4],count=0,i;int have_file=0,have_sum=0;
 if(!json || !r || strlen(json)>ND_UPDATE_JSON_LIMIT)return 0;
 end=skip(json,0);if(!end || *white(end) || *white(json)!='{')return 0;
 memset(&parsed,0,sizeof(parsed));
 if(!is_false(member(json,"draft")) || !is_false(member(json,"prerelease")) ||
    !text(member(json,"tag_name"),parsed.tag,sizeof(parsed.tag)) || !version(parsed.tag,parts,parsed.version))return 0;
 sprintf(parsed.file,"NovaDock-%s.lha",parsed.version);sprintf(sumname,"%s.sha256",parsed.file);
 p=member(json,"assets");if(!p || *p++!='[')return 0;p=white(p);
 while(*p!=']'){
  if(++count>128 || *p!='{')return 0;asset=p;end=skip(p,0);if(!end)return 0;
  if(text(member(asset,"name"),name,sizeof(name)) && (!strcmp(name,parsed.file) || !strcmp(name,sumname))){
   if(!text(member(asset,"state"),state,sizeof(state)) || strcmp(state,"uploaded") ||
      !text(member(asset,"browser_download_url"),url,sizeof(url)))return 0;
   sprintf(expected,"https://github.com/retrocs-org/novadock/releases/download/%s/%s",parsed.tag,name);
   if(strcmp(url,expected))return 0;
   if(!strcmp(name,parsed.file)){
    if(have_file++ || !number(member(asset,"size"),&parsed.size) || !parsed.size)return 0;
    strcpy(parsed.url,url);v=member(asset,"digest");
    if(v && strncmp(v,"null",4)){
     if(!text(v,digest,sizeof(digest)) || strlen(digest)!=71 || strncmp(digest,"sha256:",7))return 0;
     for(i=0;i<64;++i){if(!isxdigit((unsigned char)digest[i+7]))return 0;parsed.digest[i]=(char)tolower((unsigned char)digest[i+7]);}
    }
   }else{if(have_sum++)return 0;strcpy(parsed.checksum_url,url);}
  }
  p=white(end);if(*p==']')break;if(*p++!=',')return 0;p=white(p);
 }
 if(!have_file || !have_sum)return 0;*r=parsed;return 1;
}
int nd_update_checksum(const char *p,const char *filename,char out[65])
{
 unsigned int i;if(!p || !filename || !out || strlen(p)<66 || strlen(p)>160)return 0;
 for(i=0;i<64;++i){if(!isxdigit((unsigned char)p[i]))return 0;out[i]=(char)tolower((unsigned char)p[i]);}out[64]=0;
 if(p[64]!=' ' || p[65]!=' ')return 0;p+=66;
 i=(unsigned int)strlen(filename);if(strncmp(p,filename,i))return 0;p+=i;
 if(*p=='\r')++p;if(*p=='\n')++p;return *p==0;
}
int nd_update_notice_due(const struct NDUpdateState *s,const char *v,unsigned long now)
{return s && v && (strcmp(s->noticed,v) || now<s->last_notice || now-s->last_notice>=ND_UPDATE_DAY);}
unsigned long nd_update_interval(unsigned int hours)
{return (hours==2 || hours==6 ? hours:4)*3600UL;}
int nd_update_schedule(struct NDUpdateSchedule *s,int startup,int running,unsigned int hours,int network,int busy,unsigned long now,unsigned long elapsed)
{
 int mode=(startup ? 1:0)|(running ? 2:0),reason;
 if(!s)return 0;
 if(!mode){memset(s,0,sizeof(*s));return 0;}
 if(hours!=2 && hours!=4 && hours!=6)hours=4;
 if(s->enabled!=mode || s->hours!=hours){
  int first=!s->enabled,was_startup=s->enabled&1;
  s->ticks=0;s->startup=startup && (first || !was_startup || s->startup);
  s->enabled=mode;s->hours=hours;s->next_due=now+nd_update_interval(hours);
 }else{s->ticks+=elapsed;if(s->ticks<3000UL)return 0;s->ticks=0;}
 if(busy)return 0;
 if(!network || (!s->startup && (!running || now<s->next_due)))return 0;
 reason=s->startup ? 1:2;s->startup=0;s->waiting=1;s->next_due=now+ND_UPDATE_RETRY;return reason;
}

int nd_update_staging_dir(const char *path)
{
 const char *p,*base;if(!path || strlen(path)>150 || !strchr(path,':'))return 0;base=path;
 for(p=path;*p;++p)if(*p==':' || *p=='/')base=p+1;
 if(strncmp(base,"NovaDockUpdate-",15))return 0;p=base+15;if(!*p)return 0;
 for(;*p;++p)if(!((*p>='0' && *p<='9') || (*p>='a' && *p<='f') || *p=='-'))return 0;
 return 1;
}
