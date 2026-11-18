#include "nd_i2c_cache.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static void text(char *out,unsigned int cap,const char *s)
{unsigned int i=0;if(!s)s="";while(*s && i+1<cap){unsigned char c=(unsigned char)*s++;if(c>=32 && c!=127 && c!='|' && c!='\t')out[i++]=(char)c;}out[i]=0;}
long nd_i2c_milli(long fixed)
{unsigned long magnitude=fixed<0 ? (unsigned long)(-(fixed+1))+1UL:(unsigned long)fixed;long value=(long)((magnitude/65536UL)*1000UL+((magnitude%65536UL)*1000UL+32768UL)/65536UL);return fixed<0 ? -value:value;}
void nd_i2c_format(const struct NDI2CReading *r,char *out)
{unsigned long tenth,n=r->milli<0 ? (unsigned long)(-(r->milli+1))+1UL:(unsigned long)r->milli;if(!r->available){sprintf(out,"Unavailable");return;}tenth=(n+50UL)/100UL;sprintf(out,"%s%lu.%lu %s",r->milli<0 ? "-":"",tenth/10UL,tenth%10UL,r->unit);}
void nd_i2c_option(unsigned long id,char *key){sprintf(key,"I2C_%08lX",id&0xffffffffUL);if(!nd_i2c_key(key))key[0]=0;}
int nd_i2c_add(struct NDI2CCache *c,unsigned long id,long fixed,const char *name,const char *unit,unsigned long now)
{unsigned int i;struct NDI2CReading *r;if(!c || !id || id>0xffffffffUL || (!name && !unit))return 0;for(i=0;i<c->count;++i)if(c->readings[i].id==id)break;if(i==c->count){if(c->count>=ND_I2C_MAX)return 0;++c->count;}r=&c->readings[i];memset(r,0,sizeof(*r));r->id=id;r->milli=nd_i2c_milli(fixed);r->stamp=now;r->available=1;text(r->name,sizeof(r->name),name);text(r->unit,sizeof(r->unit),unit);if(!r->name[0])sprintf(r->name,"Sensor %08lX",id);return 1;}
int nd_i2c_write(FILE *f,const struct NDI2CCache *c)
{unsigned int i;if(!f || !c || c->count>ND_I2C_MAX || fprintf(f,"NDI2C1 %lu %u %u\n",c->stamp,c->provider,c->count)<0)return 0;for(i=0;i<c->count;++i){const struct NDI2CReading *r=&c->readings[i];if(!r->id || fprintf(f,"%lu %lu %ld %u|%s|%s\n",r->id,r->stamp,r->milli,r->available,r->name,r->unit)<0)return 0;}return !ferror(f);}
/* Explicit tokens avoid libc-specific scanf extensions on classic systems. */
static int token(char **cursor,unsigned long *out,char separator)
{char *p=*cursor;unsigned long v=0;unsigned int d;if(*p<'0' || *p>'9')return 0;
 while(*p>='0' && *p<='9'){d=(unsigned int)(*p++-'0');if(v>429496729UL || (v==429496729UL && d>5))return 0;v=v*10UL+d;}
 if(*p!=separator)return 0;*out=v;*cursor=separator ? p+1:p;return 1;}
int nd_i2c_read(FILE *f,struct NDI2CCache *c)
{
 struct NDI2CCache next;char line[200],*name,*unit,*end,*p;unsigned int i,j,n;unsigned long provider,count,available,magnitude;int negative;
 memset(&next,0,sizeof(next));if(!f || !c || !fgets(line,sizeof(line),f) || strncmp(line,"NDI2C1 ",7))return 0;
 p=line+7;if(!token(&p,&next.stamp,' ') || !token(&p,&provider,' ') || !token(&p,&count,'\n') || *p || provider>1 || count>ND_I2C_MAX)return 0;
 next.provider=(unsigned int)provider;n=(unsigned int)count;
 for(i=0;i<n;++i){struct NDI2CReading *r=&next.readings[i];
  if(!fgets(line,sizeof(line),f) || !(end=strchr(line,'\n')))return 0;*end=0;name=strchr(line,'|');if(!name)return 0;*name++=0;unit=strchr(name,'|');if(!unit)return 0;*unit++=0;
  if(strchr(unit,'|') || strlen(name)>=sizeof(r->name) || strlen(unit)>=sizeof(r->unit))return 0;
  p=line;if(!token(&p,&r->id,' ') || !r->id || !token(&p,&r->stamp,' '))return 0;negative=*p=='-';if(negative)++p;
  if(!token(&p,&magnitude,' ') || magnitude>32768000UL || !token(&p,&available,0) || available>1)return 0;
  r->available=(unsigned int)available;r->milli=negative ? -(long)magnitude:(long)magnitude;
  for(j=0;j<i;++j)if(next.readings[j].id==r->id)return 0;text(r->name,sizeof(r->name),name);text(r->unit,sizeof(r->unit),unit);
 }
 if(fgetc(f)!=EOF || ferror(f))return 0;next.count=n;*c=next;return 1;
}
int nd_i2c_load(struct NDI2CCache *c){FILE *f=fopen(ND_I2C_CACHE,"r");int ok;if(!f)return 0;ok=nd_i2c_read(f,c);fclose(f);return ok;}
int nd_i2c_fresh(const struct NDI2CCache *c,unsigned long now,unsigned long age){return c && c->provider && now>=c->stamp && now-c->stamp<=age;}
