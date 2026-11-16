/* Read-only sensor values, strict numeric parsing and a portable snapshot. */
#include "nd_sensors.h"
#include <string.h>
#include <stdlib.h>
#define DEF(key,label,short_label,unit,minimum,maximum) {#key,label,short_label,unit,minimum,maximum},
const struct NDSensorDef nd_sensor_defs[ND_SENSOR_COUNT]={ND_SENSOR_CHANNELS(DEF)};
#undef DEF
int nd_sensor_index(const char *key)
{int i;for(i=0;key && i<ND_SENSOR_COUNT;++i)if(!strcmp(key,nd_sensor_defs[i].key))return i;return -1;}
static int space(int c){return c==' ' || c=='\t' || c=='\r' || c=='\n';}
int nd_sensor_number(const char *text,long *milli)
{
 unsigned long whole=0,fraction=0,scale=100;int neg=0,digits=0;const char *p=text;
 if(!p || !milli)return 0;while(space(*p))++p;
 if(*p=='-' || *p=='+'){neg=*p=='-';++p;}
 while(*p>='0' && *p<='9'){whole=whole*10+(unsigned int)(*p++-'0');if(whole>1000000)return 0;++digits;}
 if(!digits)return 0;
 if(*p=='.' || *p==','){++p;digits=0;while(*p>='0' && *p<='9'){if(scale){fraction+=(unsigned int)(*p-'0')*scale;scale/=10;}++p;++digits;}if(!digits)return 0;}
 while(space(*p))++p;
 if(*p){if(!strncmp(p,"deg",3)){p+=3;while(space(*p))++p;}
  if(*p=='C' || *p=='V' || *p=='%')++p;
  else if((unsigned char)*p==176 && p[1]=='C')p+=2;
  else return 0;
  while(space(*p))++p;if(*p)return 0;
 }
 *milli=(long)(whole*1000UL+fraction);if(neg)*milli=-*milli;return 1;
}
int nd_sensor_put(struct NDSensors *set,int index,long milli,unsigned long now)
{
 if(!set || index<0 || index>=ND_SENSOR_COUNT || milli<nd_sensor_defs[index].minimum || milli>nd_sensor_defs[index].maximum)return 0;
 set->value[index]=milli;set->stamp[index]=now;set->available|=1UL<<index;return 1;
}
void nd_sensor_expire(struct NDSensors *set,unsigned long now,unsigned long max_age)
{int i;for(i=0;i<ND_SENSOR_COUNT;++i)if(now<set->stamp[i] || now-set->stamp[i]>max_age)set->available&=~(1UL<<i);}
void nd_sensor_format(int index,long milli,char *out)
{
 unsigned long n=(unsigned long)(milli<0 ? -milli:milli);const char *sign=milli<0 ? "-":"";
 if(!strcmp(nd_sensor_defs[index].unit,"V"))sprintf(out,"%s%lu.%02lu V",sign,n/1000,(n%1000)/10);
 else if(!strcmp(nd_sensor_defs[index].unit,"%"))sprintf(out,"%s%lu%%",sign,n/1000);
 else sprintf(out,"%s%lu.%lu C",sign,n/1000,(n%1000)/100);
}
/* Offsets and engineering units from ZZTop's public board interface. */
int nd_sensor_zz_sample(struct NDSensors *set,unsigned int (*read16)(void *,unsigned int),void *context,unsigned long now)
{
 unsigned int version=read16(context,0xc0),temp;
 if(version<0x0107 || version==0xffff)return 0;
 temp=read16(context,0xe0);nd_sensor_put(set,0,(temp&0x8000U ? (long)temp-65536L:(long)temp)*100L,now);
 nd_sensor_put(set,1,(long)read16(context,0xe4)*10L,now);
 nd_sensor_put(set,2,(long)read16(context,0xe2)*10L,now);
 return (set->available&7UL)!=0;
}
int nd_sensor_warp_line(struct NDSensors *set,const char *line,unsigned long now)
{
 static const struct {const char *label,*key;} fields[]={
  {"MC680x0 int. temp.","WARP_CPU"},{"MC680x0 ext. temp.","WARP_EXT"},
  {"ARM STM32H7 temp.","WARP_ARM"},{"Artix FPGA temp.","WARP_FPGA"},
  {"5V supply","WARP_5V"},{"Backup batt. voltage","WARP_BATT"},{"Fan current output","WARP_FAN"}};
 unsigned int i,n;long value;while(space(*line))++line;
 for(i=0;i<sizeof(fields)/sizeof(fields[0]);++i){n=(unsigned int)strlen(fields[i].label);
  if(strncmp(line,fields[i].label,n))continue;
  line+=n;if(!space(*line) && *line!=':' && *line!='=')return 0;
  while(space(*line) || *line==':' || *line=='=')++line;
  return nd_sensor_number(line,&value) && nd_sensor_put(set,nd_sensor_index(fields[i].key),value,now);
 }
 return 0;
}
int nd_sensor_snapshot_write(FILE *f,const struct NDSensors *set,unsigned long now)
{int i;if(fprintf(f,"NDSENS2 %lu %lu\n",now,set->available&ND_SENSOR_ALL)<0)return 0;
 for(i=0;i<ND_SENSOR_COUNT;++i)if(set->available&(1UL<<i))if(fprintf(f,"%s %ld %lu\n",nd_sensor_defs[i].key,set->value[i],set->stamp[i])<0)return 0;
 return !ferror(f);}
int nd_sensor_snapshot_read(FILE *f,struct NDSensors *set,unsigned long now,unsigned long max_age)
{
 struct NDSensors candidate;unsigned long stamp,mask,measured;char line[96],key[32],extra;long value;int i;
 memset(&candidate,0,sizeof(candidate));
 if(!fgets(line,sizeof(line),f) || sscanf(line,"NDSENS2 %lu %lu %c",&stamp,&mask,&extra)!=2 || (mask&~ND_SENSOR_ALL) || stamp>now || now-stamp>max_age)return 0;
 while(fgets(line,sizeof(line),f)){
  if(!strchr(line,'\n') || sscanf(line,"%31s %ld %lu %c",key,&value,&measured,&extra)!=3)return 0;
  i=nd_sensor_index(key);if(i<0 || !(mask&(1UL<<i)) || (candidate.available&(1UL<<i)) || measured>stamp || !nd_sensor_put(&candidate,i,value,measured))return 0;
 }
 if(ferror(f) || candidate.available!=mask)return 0;candidate.sampled=stamp;*set=candidate;return 1;
}
int nd_sensor_status_path(const char *id,char *out,unsigned int size)
{
 const char *p;if(!id || !*id || strlen(id)>19 || size<strlen(id)+sizeof(ND_SENSOR_DIRECTORY)+17)return 0;
 for(p=id;*p;++p)if(!((*p>='A' && *p<='Z') || (*p>='a' && *p<='z') || (*p>='0' && *p<='9') || *p=='_' || *p=='-'))return 0;
 sprintf(out,ND_SENSOR_DIRECTORY "/instance-%s.status",id);return 1;
}
unsigned long nd_sensor_available_for(const char *id,unsigned long now)
{char path[100];FILE *f;struct NDSensors set;int ok;if(!nd_sensor_status_path(id,path,sizeof(path)))return 0;f=fopen(path,"r");if(!f)return 0;ok=nd_sensor_snapshot_read(f,&set,now,180);fclose(f);return ok ? set.available:0;}
