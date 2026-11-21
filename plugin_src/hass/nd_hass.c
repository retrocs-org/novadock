/* Home Assistant REST data policy. No networking, credentials or platform UI. */
#include "nd_hass.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* JSON scanning is bounded by caller-owned, NUL-terminated response buffers. */
static const char *ws(const char *p){while(*p && strchr(" \r\n\t",*p))++p;return p;}
static const char *strend(const char *p)
{unsigned int i;if(*p++!='"')return NULL;while(*p && *p!='"'){if((unsigned char)*p<32)return NULL;if(*p++=='\\'){if(!*p)return NULL;if(*p=='u'){++p;for(i=0;i<4;++i){if(!isxdigit((unsigned char)*p))return NULL;++p;}}else{if(!strchr("\"\\/bfnrt",*p))return NULL;++p;}}}return *p=='"' ? p+1:NULL;}
static const char *skip(const char *p,unsigned int depth)
{
 char close;const char *q;if(depth>16)return NULL;p=ws(p);
 if(*p=='"')return strend(p);
 if(*p=='{' || *p=='['){close=*p=='{' ? '}':']';p=ws(p+1);if(*p==close)return p+1;
  for(;;){if(close=='}'){q=strend(p);if(!q)return NULL;p=ws(q);if(*p++!=':')return NULL;}
   p=skip(p,depth+1);if(!p)return NULL;p=ws(p);if(*p==close)return p+1;if(*p++!=',')return NULL;p=ws(p);}}
 if(!strncmp(p,"true",4))return p+4;if(!strncmp(p,"false",5))return p+5;if(!strncmp(p,"null",4))return p+4;
 q=p;if(*p=='-')++p;if(*p=='0')++p;else{if(*p<'1' || *p>'9')return NULL;while(isdigit((unsigned char)*p))++p;}
 if(*p=='.'){++p;if(!isdigit((unsigned char)*p))return NULL;while(isdigit((unsigned char)*p))++p;}
 if(*p=='e' || *p=='E'){++p;if(*p=='+' || *p=='-')++p;if(!isdigit((unsigned char)*p))return NULL;while(isdigit((unsigned char)*p))++p;}return p>q ? p:NULL;
}
/* Latin-1 UI labels; unsupported Unicode glyphs become '?', never markup. */
static int text_mode(const char *p,char *out,unsigned int cap,int identity)
{
 unsigned int n=0,c,v,i;int overflow=0;if(!p || !cap || !strend(p))return 0;++p;
 while(*p!='"'){
  c=(unsigned char)*p++;
  if(c=='\\'){c=(unsigned char)*p++;if(c=='u'){v=0;for(i=0;i<4;++i){v=v*16+(isdigit((unsigned char)*p) ? *p-'0':tolower((unsigned char)*p)-'a'+10);++p;}if(identity && v>255)return 0;c=v>255 ? '?':v;}else if(c=='n' || c=='r' || c=='t' || c=='b' || c=='f'){if(identity)return 0;c=' ';}}
  else if(c>=0xc2 && c<=0xdf && ((unsigned char)*p&0xc0)==0x80){c=((c&31)<<6)|((unsigned char)*p++&63);if(c>255){if(identity)return 0;c='?';}}
  else if(c>=128){if(identity)return 0;while(((unsigned char)*p&0xc0)==0x80)++p;c='?';}
  if(c<32 || c==127){if(identity)return 0;c=' ';}if(n+1<cap)out[n++]=(char)c;else overflow=1;
 }out[n]=0;return !overflow;
}
static int text(const char *p,char *out,unsigned int cap)
{return text_mode(p,out,cap,0);}
static const char *member(const char *p,const char *name)
{
 const char *end,*value=NULL;char key[64];p=ws(p);if(*p++!='{')return NULL;p=ws(p);
 while(*p && *p!='}') {end=strend(p);if(!end)return NULL;if(text(p,key,sizeof(key)) && !strcmp(key,name)){if(value)return NULL;value=ws(end);if(*value++!=':')return NULL;value=ws(value);}
 p=ws(end);if(*p++!=':')return NULL;p=skip(p,0);if(!p)return NULL;p=ws(p);if(*p=='}')break;if(*p++!=',')return NULL;p=ws(p);}return value;
}
int amihass_api_response(const char *json)
{
 const char *end;char message[64];
 if(!json || strlen(json)>1024)return 0;
 end=skip(json,0);
 return end && !*ws(end) && text(member(json,"message"),message,sizeof(message)) && !strcmp(message,"API running.");
}
int amihass_url(const char *url,int allow_http)
{
 const char *p,*host;unsigned long port=0;int n=0;
 if(!url || strlen(url)>=160)return 0;
 if(!strncmp(url,"https://",8))p=url+8;else if(allow_http && !strncmp(url,"http://",7))p=url+7;else return 0;
 host=p;while(*p && *p!=':'){if(!(isalnum((unsigned char)*p) || *p=='.' || *p=='-'))return 0;++p;++n;}
 if(!n || host[0]=='.' || host[0]=='-' || p[-1]=='.' || p[-1]=='-')return 0;
 if(*p==':'){++p;if(!*p)return 0;while(*p){if(!isdigit((unsigned char)*p) || port>6553)return 0;port=port*10+*p++-'0';}if(!port || port>65535)return 0;}
 return !*p;
}
int amihass_entity_id(const char *id)
{
 const char *p;int dot=0,n=0;if(!id || strlen(id)>=AMIHASS_ID)return 0;
 for(p=id;*p;++p){if(*p=='.'){if(dot || !n || !p[1])return 0;dot=1;n=0;}else{if(!((*p>='a' && *p<='z') || isdigit((unsigned char)*p) || *p=='_'))return 0;++n;}}return dot && n;
}
int amihass_token(const char *text,char token[1025])
{
 unsigned int n=0;if(!text || !token)return 0;
 while(text[n] && text[n]!='\n' && text[n]!='\r'){if(n>=1024 || !(isalnum((unsigned char)text[n]) || strchr("-_.",text[n])))return 0;++n;}
 if(n<20)return 0;if(text[n]=='\r' && text[n+1]=='\n'){if(text[n+2])return 0;}else if(text[n] && text[n+1])return 0;
 memcpy(token,text,n);token[n]=0;return 1;
}
static int state(const char *json,struct AmiHASSEntity *out)
{
 struct AmiHASSEntity e;const char *a;memset(&e,0,sizeof(e));
 if(!text(member(json,"entity_id"),e.id,sizeof(e.id)) || !amihass_entity_id(e.id) || !text(member(json,"state"),e.state,sizeof(e.state)))return 0;
 a=member(json,"attributes");if(a && *a=='{'){(void)text(member(a,"friendly_name"),e.name,sizeof(e.name));(void)text(member(a,"unit_of_measurement"),e.unit,sizeof(e.unit));(void)text(member(a,"device_class"),e.device_class,sizeof(e.device_class));}
 (void)text(member(json,"last_updated"),e.updated,sizeof(e.updated));if(!e.name[0])strncpy(e.name,e.id,sizeof(e.name)-1);*out=e;return 1;
}
int amihass_state(const char *json,struct AmiHASSEntity *e)
{const char *end;if(!json || !e || strlen(json)>AMIHASS_RESPONSE_LIMIT)return 0;end=skip(json,0);return end && !*ws(end) && state(json,e);}
int amihass_catalog(const char *json,struct AmiHASSEntity *items,unsigned int cap)
{
 const char *p,*end;unsigned int count=0;struct AmiHASSEntity e;
 if(!json || !items || !cap || strlen(json)>AMIHASS_RESPONSE_LIMIT)return -1;end=skip(json,0);if(!end || *ws(end))return -1;p=ws(json);if(*p++!='[')return -1;p=ws(p);
 while(*p!=']'){end=skip(p,0);if(!end || !state(p,&e) || count>=cap)return -1;items[count++]=e;p=ws(end);if(*p==']')break;if(*p++!=',')return -1;p=ws(p);}return (int)count;
}
static int contains(const char *a,const char *b)
{const char *p;unsigned int i;if(!b || !*b)return 1;for(p=a;*p;++p){for(i=0;b[i] && p[i] && tolower((unsigned char)p[i])==tolower((unsigned char)b[i]);++i){}if(!b[i])return 1;}return 0;}
int amihass_matches(const struct AmiHASSEntity *e,const char *domain,const char *filter)
{unsigned int n;if(!e)return 0;if(domain && *domain && strcmp(domain,"All")){n=(unsigned int)strlen(domain);if(strncmp(e->id,domain,n) || e->id[n]!='.')return 0;}return contains(e->name,filter) || contains(e->id,filter);}
const char *amihass_display_state(const struct AmiHASSEntity *e)
{if(!e)return "Unknown";if(!strncmp(e->id,"binary_sensor.",14)&&(!strcmp(e->device_class,"door")||!strcmp(e->device_class,"garage_door")||!strcmp(e->device_class,"opening"))){if(!strcmp(e->state,"on"))return "Open";if(!strcmp(e->state,"off"))return "Closed";}if(!strncmp(e->id,"lock.",5)){if(!strcmp(e->state,"locked"))return "Locked";if(!strcmp(e->state,"unlocked"))return "Unlocked";if(!strcmp(e->state,"open"))return "Latch open";}return e->state;}
int amihass_available(const struct AmiHASSEntity *e)
{return e && e->id[0] && strcmp(e->state,"unavailable") && strcmp(e->state,"unknown");}
int amihass_number(const char *s,long *value)
{long n=0,f=0;int sign=1,digits=0;if(!s||!*s)return 0;if(*s=='-'){sign=-1;++s;}if(!isdigit((unsigned char)*s))return 0;while(isdigit((unsigned char)*s)){n=n*10+*s++-'0';if(n>10000)return 0;}if(*s=='.'){++s;while(isdigit((unsigned char)*s)){if(digits>=2)return 0;f=f*10+*s++-'0';++digits;}if(!digits)return 0;}if(*s)return 0;if(digits==1)f*=10;*value=sign*(n*100+f);return 1;}
void amihass_number_text(long v,char out[40])
{unsigned long n=v<0 ? (unsigned long)-v:(unsigned long)v;if(n%100)sprintf(out,"%s%lu.%02lu",v<0 ? "-":"",n/100,n%100);else sprintf(out,"%s%lu",v<0 ? "-":"",n/100);}
static int number_member(const char *json,const char *key,long *value)
{const char *p=member(json,key),*end;char text[40];unsigned int n;if(!p)return 0;end=skip(p,0);if(!end)return 0;n=(unsigned int)(end-p);if(n>=sizeof(text))return 0;memcpy(text,p,n);text[n]=0;return amihass_number(text,value);}
static int color_value(const char *s,unsigned long *rgb)
{unsigned int i;unsigned long value=0;if(!s||strlen(s)!=7||*s!='#')return 0;for(i=1;i<7;++i){if(!isxdigit((unsigned char)s[i]))return 0;value=value*16+(isdigit((unsigned char)s[i])?s[i]-'0':tolower((unsigned char)s[i])-'a'+10);}*rgb=value;return 1;}
static int modes_contain(const char *a,const char *wanted)
{const char *p=member(a,"supported_color_modes"),*end;char mode[24];if(!p||*p++!='[')return 0;p=ws(p);while(*p&&*p!=']'){end=strend(p);if(!end)return 0;if(text(p,mode,sizeof(mode))&&!strcmp(mode,wanted))return 1;p=ws(end);if(*p==']')break;if(*p++!=',')return 0;p=ws(p);}return 0;}
static unsigned long media_features(const char *a)
{const char *p=member(a,"supported_features"),*end;unsigned long v=0;if(!p||(end=skip(p,0))==NULL||p==end)return 0;while(p<end){unsigned int n=(unsigned char)*p++-'0';if(n>9||v>(2147483647UL-n)/10UL)return 0;v=v*10UL+n;}return v;}
void amihass_controls(const char *json,struct AHControls *c)
{struct AmiHASSEntity e;const char *a,*p;long v,r,g,b;memset(c,0,sizeof(*c));if(!amihass_state(json,&e))return;a=member(json,"attributes");if(!a||*a!='{')return;
 if(!strncmp(e.id,"light.",6)){
  c->color=modes_contain(a,"rgb")||modes_contain(a,"rgbw")||modes_contain(a,"rgbww")||modes_contain(a,"hs")||modes_contain(a,"xy");
  c->brightness=c->color||modes_contain(a,"brightness")||modes_contain(a,"color_temp");
  if(number_member(a,"brightness",&v)&&v>=0&&v<=25500){c->brightness=1;c->level_known=1;c->level=(int)((v+127)/255);}
  if(!strcmp(e.state,"off")){c->level=0;c->level_known=1;}c->rgb=0xffffff;
  p=member(a,"rgb_color");if(p&&*p++=='['){char part[32];unsigned int i;long *values[3];values[0]=&r;values[1]=&g;values[2]=&b;for(i=0;i<3;++i){const char *end;p=ws(p);end=skip(p,0);if(!end||(unsigned int)(end-p)>=sizeof(part))break;memcpy(part,p,end-p);part[end-p]=0;if(!amihass_number(part,values[i])||*values[i]<0||*values[i]>25500||*values[i]%100)break;p=ws(end);if(i<2){if(*p++!=',')break;}}if(i==3&&*p==']'){c->color_known=1;c->rgb=((unsigned long)(r/100)<<16)|((unsigned long)(g/100)<<8)|(unsigned long)(b/100);}}
 }else if(!strncmp(e.id,"fan.",4)){
  if(number_member(a,"supported_features",&v) && v>=0 && v%100==0 && ((v/100)&1)){
   c->fan_speed=1;if(number_member(a,"percentage",&v) && v>=0 && v<=10000 && v%100==0){c->speed_known=1;c->speed=(int)(v/100);}
   if(!strcmp(e.state,"off")){c->speed=0;c->speed_known=1;}
  }
 }else if(!strncmp(e.id,"lock.",5)){
  p=member(a,"code_format");if(!p || !strncmp(p,"null",4)){c->lock=1;c->unlock=1;c->lock_open=(media_features(a)&1UL)!=0;}
 }else if(!strncmp(e.id,"cover.",6)){
  if(number_member(a,"supported_features",&v) && v>=0 && v%100==0){long features=v/100;c->cover_open=(features&1)!=0;c->cover_close=(features&2)!=0;c->position=(features&4)!=0;c->cover_stop=(features&8)!=0;}
  if(c->position && number_member(a,"current_position",&v) && v>=0 && v<=10000 && v%100==0){c->position_known=1;c->position_value=(int)(v/100);}
 }else if(!strncmp(e.id,"media_player.",13)){
  unsigned long features=media_features(a);c->media_play=(features&16384UL)!=0;c->media_pause=(features&1UL)!=0;c->media_previous=(features&16UL)!=0;c->media_next=(features&32UL)!=0;c->media_volume=(features&4UL)!=0;
  if(c->media_volume&&number_member(a,"volume_level",&v)&&v>=0&&v<=100){c->volume_known=1;c->volume=(int)v;}
  (void)text(member(a,"media_title"),c->media_title,sizeof(c->media_title));(void)text(member(a,"media_artist"),c->media_artist,sizeof(c->media_artist));
 }else if(!strncmp(e.id,"climate.",8)){
  if(number_member(a,"temperature",&c->target)&&number_member(a,"min_temp",&c->minimum)&&number_member(a,"max_temp",&c->maximum)&&number_member(a,"target_temp_step",&c->step)&&c->step>0&&c->minimum<c->maximum&&c->target>=c->minimum&&c->target<=c->maximum)c->temperature=1;
  (void)text(member(a,"temperature_unit"),c->unit,sizeof(c->unit));
 }
}
static int normalized_unit(const char *text,char unit[16])
{if(!text)return 0;if(!strcmp(text,"C")||!strcmp(text,"\260C")||!strcmp(text,"\302\260C")){strcpy(unit,"C");return 1;}if(!strcmp(text,"F")||!strcmp(text,"\260F")||!strcmp(text,"\302\260F")){strcpy(unit,"F");return 1;}return 0;}
int amihass_temperature_unit(const char *json,char unit[16])
{const char *end,*system;char value[16];unit[0]=0;if(!json||strlen(json)>AMIHASS_RESPONSE_LIMIT)return 0;end=skip(json,0);if(!end||*ws(end))return 0;system=member(json,"unit_system");return system&&text(member(system,"temperature"),value,sizeof(value))&&normalized_unit(value,unit);}
void amihass_control_unit(struct AHControls *c,const char *unit)
{char normalized[16];if(normalized_unit(c->unit,normalized))strcpy(c->unit,normalized);else if(normalized_unit(unit,normalized))strcpy(c->unit,normalized);else c->unit[0]=0;}
int amihass_control_matches(const struct AHControls *c,const char *state,const char *action)
{long v,diff;unsigned long rgb;unsigned int i,a,b;if(!c||!state||!action)return 0;
 if(!strcmp(action,"Lock"))return c->lock&&!strcmp(state,"locked");
 if(!strcmp(action,"Unlock"))return c->unlock&&!strcmp(state,"unlocked");
 if(!strcmp(action,"Open lock"))return c->lock_open&&!strcmp(state,"open");
 if(!strcmp(action,"Play"))return c->media_play&&!strcmp(state,"playing");
 if(!strcmp(action,"Pause"))return c->media_pause&&!strcmp(state,"paused");
 if(!strncmp(action,"Volume ",7)){if(!c->media_volume||!c->volume_known||!amihass_number(action+7,&v))return 0;diff=c->volume-v/100;return diff>=-1&&diff<=1;}
 if(!strncmp(action,"Fan speed ",10)){if(!c->fan_speed || !c->speed_known || !amihass_number(action+10,&v))return 0;diff=c->speed-v/100;return diff>=-1 && diff<=1;}
 if(!strncmp(action,"Position ",9)){if(!c->position || !c->position_known || !amihass_number(action+9,&v))return 0;diff=c->position_value-v/100;return diff>=-1 && diff<=1;}
 if(!strcmp(action,"Open cover"))return c->cover_open && !strcmp(state,"open");
 if(!strcmp(action,"Close cover"))return c->cover_close && !strcmp(state,"closed");
 if(!strcmp(action,"Stop cover"))return c->cover_stop && (!strcmp(state,"open") || !strcmp(state,"closed"));
 if(!strncmp(action,"Brightness ",11)){if(!c->brightness||!c->level_known||!amihass_number(action+11,&v))return 0;diff=c->level-v/100;return diff>=-1&&diff<=1&&!strcmp(state,v ? "on":"off");}
 if(!strncmp(action,"Color ",6)){if(!c->color||!c->color_known||strcmp(state,"on")||!color_value(action+6,&rgb))return 0;for(i=0;i<3;++i){a=(rgb>>(i*8))&255;b=(c->rgb>>(i*8))&255;if(a>b+3||b>a+3)return 0;}return 1;}
 if(!strncmp(action,"Temperature ",12)){if(!c->temperature||!c->unit[0]||!amihass_number(action+12,&v))return 0;diff=c->target-v;return diff>=-1&&diff<=1;}return 0;}
int amihass_control_allowed(const struct AHControls *c,const char *action)
{long value;unsigned long rgb;if(!c||!action)return 0;
 if(!strcmp(action,"Lock"))return c->lock;
 if(!strcmp(action,"Unlock"))return c->unlock;
 if(!strcmp(action,"Open lock"))return c->lock_open;
 if(!strcmp(action,"Play"))return c->media_play;if(!strcmp(action,"Pause"))return c->media_pause;if(!strcmp(action,"Previous"))return c->media_previous;if(!strcmp(action,"Next"))return c->media_next;
 if(!strncmp(action,"Volume ",7))return c->media_volume&&amihass_number(action+7,&value)&&value>=0&&value<=10000&&value%100==0;
 if(!strncmp(action,"Fan speed ",10))return c->fan_speed && amihass_number(action+10,&value) && value>=0 && value<=10000 && value%100==0;
 if(!strncmp(action,"Position ",9))return c->position && amihass_number(action+9,&value) && value>=0 && value<=10000 && value%100==0;
 if(!strcmp(action,"Open cover"))return c->cover_open;
 if(!strcmp(action,"Close cover"))return c->cover_close;
 if(!strcmp(action,"Stop cover"))return c->cover_stop;
 if(!strncmp(action,"Brightness ",11))return c->brightness&&amihass_number(action+11,&value)&&value>=0&&value<=10000&&value%100==0;if(!strncmp(action,"Color ",6))return c->color&&color_value(action+6,&rgb);if(!strncmp(action,"Temperature ",12))return c->temperature&&(!strcmp(c->unit,"C")||!strcmp(c->unit,"F"))&&amihass_number(action+12,&value)&&value>=c->minimum&&value<=c->maximum&&c->step>0&&(value-c->minimum)%c->step==0;return 1;}
unsigned int amihass_control_items(const struct AHControls *c,struct AmiHASSEntity *items,unsigned int cap)
{unsigned int n=0,i;const char *keys[]={"control.temperature","control.minimum","control.maximum","control.step"};long values[4];struct AmiHASSEntity *e;if(c->brightness&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.brightness");if(c->level_known)sprintf(e->state,"%d",c->level);else strcpy(e->state,"unknown");strcpy(e->unit,"%");}if(c->color&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.color");if(c->color_known)sprintf(e->state,"#%06lx",c->rgb);else strcpy(e->state,"unknown");}if(c->temperature){values[0]=c->target;values[1]=c->minimum;values[2]=c->maximum;values[3]=c->step;for(i=0;i<4&&n<cap;++i){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,keys[i]);amihass_number_text(values[i],e->state);strcpy(e->unit,c->unit);}}if(c->fan_speed && n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.fan_speed");if(c->speed_known)sprintf(e->state,"%d",c->speed);else strcpy(e->state,"unknown");strcpy(e->unit,"%");}
 if(c->position && n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.position");if(c->position_known)sprintf(e->state,"%d",c->position_value);else strcpy(e->state,"unknown");strcpy(e->unit,"%");}
 {const char *ids[]={"control.cover_open","control.cover_close","control.cover_stop"};int supported[3];supported[0]=c->cover_open;supported[1]=c->cover_close;supported[2]=c->cover_stop;for(i=0;i<3 && n<cap;++i)if(supported[i]){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,ids[i]);strcpy(e->state,"yes");}}
 {unsigned int flags=(c->media_play ? 1:0)|(c->media_pause ? 2:0)|(c->media_previous ? 4:0)|(c->media_next ? 8:0)|(c->media_volume ? 16:0);if(flags&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.media");sprintf(e->state,"%u",flags);}if(c->media_volume&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.media_volume");if(c->volume_known)sprintf(e->state,"%d",c->volume);else strcpy(e->state,"unknown");}if(c->media_title[0]&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.media_title");strcpy(e->name,c->media_title);}if(c->media_artist[0]&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.media_artist");strcpy(e->name,c->media_artist);}}
 {const char *ids[]={"control.lock","control.unlock","control.lock_open"};int flags[3];flags[0]=c->lock;flags[1]=c->unlock;flags[2]=c->lock_open;for(i=0;i<3&&n<cap;++i)if(flags[i]){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,ids[i]);strcpy(e->state,"yes");}}
 return n;}
void amihass_controls_items(const struct AmiHASSEntity *items,unsigned int count,struct AHControls *c)
{unsigned int i,mask=0;long v;memset(c,0,sizeof(*c));for(i=0;i<count&&i<AMIHASS_MAX_ENTITIES;++i){const struct AmiHASSEntity *e=&items[i];if(!strcmp(e->id,"control.media")&&amihass_number(e->state,&v)&&v>=0&&v<=3100&&v%100==0){unsigned int f=(unsigned int)(v/100);c->media_play=(f&1)!=0;c->media_pause=(f&2)!=0;c->media_previous=(f&4)!=0;c->media_next=(f&8)!=0;c->media_volume=(f&16)!=0;}
 else if(!strcmp(e->id,"control.lock")&&!strcmp(e->state,"yes"))c->lock=1;
 else if(!strcmp(e->id,"control.unlock")&&!strcmp(e->state,"yes"))c->unlock=1;
 else if(!strcmp(e->id,"control.lock_open")&&!strcmp(e->state,"yes"))c->lock_open=1;
 else if(!strcmp(e->id,"control.media_volume")&&amihass_number(e->state,&v)&&v>=0&&v<=10000&&v%100==0){c->volume_known=1;c->volume=(int)(v/100);}
 else if(!strcmp(e->id,"control.media_title")){strncpy(c->media_title,e->name,63);c->media_title[63]=0;}
 else if(!strcmp(e->id,"control.media_artist")){strncpy(c->media_artist,e->name,63);c->media_artist[63]=0;}
 else if(!strcmp(e->id,"control.fan_speed")){c->fan_speed=!strcmp(e->state,"unknown");if(amihass_number(e->state,&v) && v>=0 && v<=10000 && v%100==0){c->fan_speed=1;c->speed_known=1;c->speed=(int)(v/100);}}
 else if(!strcmp(e->id,"control.position")){c->position=!strcmp(e->state,"unknown");if(amihass_number(e->state,&v) && v>=0 && v<=10000 && v%100==0){c->position=1;c->position_known=1;c->position_value=(int)(v/100);}}
 else if(!strcmp(e->id,"control.cover_open") && !strcmp(e->state,"yes"))c->cover_open=1;
 else if(!strcmp(e->id,"control.cover_close") && !strcmp(e->state,"yes"))c->cover_close=1;
 else if(!strcmp(e->id,"control.cover_stop") && !strcmp(e->state,"yes"))c->cover_stop=1;
 else if(!strcmp(e->id,"control.brightness")&&amihass_number(e->state,&v)&&v>=0&&v<=10000&&v%100==0){c->brightness=1;c->level_known=1;c->level=(int)(v/100);}else if(!strcmp(e->id,"control.fan_speed")){c->fan_speed=!strcmp(e->state,"unknown");if(amihass_number(e->state,&v) && v>=0 && v<=10000 && v%100==0){c->fan_speed=1;c->speed_known=1;c->speed=(int)(v/100);}}
 else if(!strcmp(e->id,"control.position")){c->position=!strcmp(e->state,"unknown");if(amihass_number(e->state,&v) && v>=0 && v<=10000 && v%100==0){c->position=1;c->position_known=1;c->position_value=(int)(v/100);}}
 else if(!strcmp(e->id,"control.cover_open") && !strcmp(e->state,"yes"))c->cover_open=1;
 else if(!strcmp(e->id,"control.cover_close") && !strcmp(e->state,"yes"))c->cover_close=1;
 else if(!strcmp(e->id,"control.cover_stop") && !strcmp(e->state,"yes"))c->cover_stop=1;
 else if(!strcmp(e->id,"control.brightness")&&!strcmp(e->state,"unknown")){c->brightness=1;}else if(!strcmp(e->id,"control.color")){c->color=1;c->color_known=color_value(e->state,&c->rgb);if(!c->color_known)c->rgb=0xffffff;}else if(amihass_number(e->state,&v)){if(!strcmp(e->id,"control.temperature")){c->target=v;mask|=1;strncpy(c->unit,e->unit,15);}if(!strcmp(e->id,"control.minimum")){c->minimum=v;mask|=2;}if(!strcmp(e->id,"control.maximum")){c->maximum=v;mask|=4;}if(!strcmp(e->id,"control.step")){c->step=v;mask|=8;}}}amihass_control_unit(c,NULL);c->temperature=mask==15&&c->unit[0]&&c->step>0&&c->minimum<c->maximum&&c->target>=c->minimum&&c->target<=c->maximum;}
int amihass_action(const char *entity,const char *action,char path[96],char body[180])
{
 char domain[32],number[40];const char *dot,*service=NULL,*target="homeassistant";unsigned int n;long value;unsigned long rgb;
 if(!amihass_entity_id(entity) || !action || !path || !body)return 0;dot=strchr(entity,'.');n=(unsigned int)(dot-entity);if(n>=sizeof(domain))return 0;memcpy(domain,entity,n);domain[n]=0;
 if(!strcmp(domain,"media_player")){
  if(!strcmp(action,"Play"))service="media_play";else if(!strcmp(action,"Pause"))service="media_pause";else if(!strcmp(action,"Previous"))service="media_previous_track";else if(!strcmp(action,"Next"))service="media_next_track";
  else if(!strncmp(action,"Volume ",7)){if(!amihass_number(action+7,&value)||value<0||value>10000||value%100)return 0;amihass_number_text(value/100,number);strcpy(path,"/api/services/media_player/volume_set");sprintf(body,"{\"entity_id\":\"%s\",\"volume_level\":%s}",entity,number);return 1;}
  if(service){sprintf(path,"/api/services/media_player/%s",service);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;}
 }
 if(!strcmp(domain,"lock")){
  if(!strcmp(action,"Lock"))service="lock";else if(!strcmp(action,"Unlock"))service="unlock";else if(!strcmp(action,"Open lock"))service="open";if(!service)return 0;sprintf(path,"/api/services/lock/%s",service);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;
 }
 if(!strcmp(domain,"fan") && !strncmp(action,"Fan speed ",10)){if(!amihass_number(action+10,&value) || value<0 || value>10000 || value%100)return 0;strcpy(path,"/api/services/fan/set_percentage");sprintf(body,"{\"entity_id\":\"%s\",\"percentage\":%ld}",entity,value/100);return 1;}
 if(!strcmp(domain,"cover")){
  if(!strncmp(action,"Position ",9)){if(!amihass_number(action+9,&value) || value<0 || value>10000 || value%100)return 0;strcpy(path,"/api/services/cover/set_cover_position");sprintf(body,"{\"entity_id\":\"%s\",\"position\":%ld}",entity,value/100);return 1;}
  if(!strcmp(action,"Open cover"))service="open_cover";else if(!strcmp(action,"Close cover"))service="close_cover";else if(!strcmp(action,"Stop cover"))service="stop_cover";if(!service)return 0;sprintf(path,"/api/services/cover/%s",service);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;
 }
 if(!strcmp(domain,"light")&&!strncmp(action,"Brightness ",11)){if(!amihass_number(action+11,&value)||value<0||value>10000||value%100)return 0;sprintf(path,"/api/services/light/turn_on");sprintf(body,"{\"entity_id\":\"%s\",\"brightness_pct\":%ld}",entity,value/100);return 1;}
 if(!strcmp(domain,"light")&&!strncmp(action,"Color ",6)){if(!color_value(action+6,&rgb))return 0;strcpy(path,"/api/services/light/turn_on");sprintf(body,"{\"entity_id\":\"%s\",\"rgb_color\":[%lu,%lu,%lu]}",entity,(rgb>>16)&255,(rgb>>8)&255,rgb&255);return 1;}
 if(!strcmp(domain,"climate")&&!strncmp(action,"Temperature ",12)){if(!amihass_number(action+12,&value)||value < -10000||value>100000)return 0;amihass_number_text(value,number);strcpy(path,"/api/services/climate/set_temperature");sprintf(body,"{\"entity_id\":\"%s\",\"temperature\":%s}",entity,number);return 1;}
 if(!strcmp(domain,"scene") || !strcmp(domain,"script")){if(strcmp(action,"Activate") && strcmp(action,"Turn on") && strcmp(action,"Toggle"))return 0;target=domain;service="turn_on";}
 else if(!strcmp(domain,"light") || !strcmp(domain,"switch") || !strcmp(domain,"fan") || !strcmp(domain,"input_boolean") || !strcmp(domain,"group") || !strcmp(domain,"media_player")){
  if(!strcmp(action,"Toggle"))service="toggle";else if(!strcmp(action,"Turn on"))service="turn_on";else if(!strcmp(action,"Turn off"))service="turn_off";
 }
 /* Explicit security and cover commands are capability-gated above. */
 if(!service)return 0;sprintf(path,"/api/services/%s/%s",target,service);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;
}
const char *amihass_icon(const struct AmiHASSEntity *e)
{
 if(!e || !e->id[0])return "Home";
 if(!strncmp(e->id,"light.",6))return !strcmp(e->device_class,"strip") ? "LightStrip":"Bulb";
 if(!strncmp(e->id,"fan.",4))return "Fan";
 if(!strncmp(e->id,"media_player.",13))return "TV";
 if(!strncmp(e->id,"climate.",8))return "AC";
 if(!strcmp(e->device_class,"temperature"))return "Thermometer";
 if(!strcmp(e->device_class,"garage_door"))return "Garage";
 if(!strcmp(e->device_class,"door"))return "Door";
 if(!strncmp(e->id,"cover.",6))return "Garage";
 if(!strncmp(e->id,"lock.",5))return "FrontDoor";
 if(!strncmp(e->id,"scene.",6) || !strncmp(e->id,"script.",7))return "Scene";
 if(!strncmp(e->id,"sensor.",7) || !strncmp(e->id,"binary_sensor.",14))return "Sensor";
 return "Home";
}

/* Build a fixed, read-only template with escaped literal filters. The server
 * renders only one page of compact state metadata, regardless of home size. */
static int literal(const char *s,char *out,unsigned int cap)
{
 unsigned int n=0;unsigned char c;if(!s)s="";
 while((c=(unsigned char)*s++)!=0){if(c<32)return 0;if(c>=127){static const char hex[]="0123456789abcdef";if(n+6>=cap)return 0;out[n++]='\\';out[n++]='u';out[n++]='0';out[n++]='0';out[n++]=hex[c>>4];out[n++]=hex[c&15];continue;}if(c=='\\' || c=='\''){if(n+2>=cap)return 0;out[n++]='\\';}if(n+1>=cap)return 0;out[n++]=(char)c;}out[n]=0;return 1;
}
static int quoted(const char *s,char *out,unsigned int cap)
{
 unsigned int n=0;unsigned char c;
 while((c=(unsigned char)*s++)!=0){if(c=='\\' || c=='"'){if(n+2>=cap)return 0;out[n++]='\\';}else if(c<32)return 0;if(n+1>=cap)return 0;out[n++]=(char)c;}out[n]=0;return 1;
}
int amihass_area_valid(const char *area)
{const unsigned char *p=(const unsigned char *)area;if(!area||strlen(area)>95)return 0;for(;*p;++p)if(*p<32||*p==127)return 0;return 1;}
int amihass_device_area_page_request(const char *device,const char *area,const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{
 char needle[390],escaped[6400],template[3200],scope[160],area_literal[576],area_scope[700];const char *p;
 if(!area)area="";if(!amihass_area_valid(area)||!literal(area,area_literal,sizeof(area_literal)))return 0;
 if(*area)sprintf(area_scope,"{%% set in_area=area_entities('%s') %%}",area_literal);else strcpy(area_scope,"{% set in_area=none %}");
 if(!device)device="";if(*device && !amihass_device_id(device))return 0;
 sprintf(scope,"{%% set selected=%s",*device ? "device_entities('":"none");if(*device){strcat(scope,device);strcat(scope,"')");}strcat(scope," %}");
 if(!domain || !*domain)domain="All";if(strlen(domain)>23 || offset<0 || offset>100000 || !literal(filter,needle,sizeof(needle)))return 0;
 for(p=domain;*p;++p)if(!((*p>='a' && *p<='z') || isdigit((unsigned char)*p) || *p=='_' || !strcmp(domain,"All")))return 0;
 sprintf(template,"%s%s{%% set ns=namespace(items=[]) %%}{%% set needle='%s'|lower %%}{%% for e in states %%}{%% if e.entity_id|length<96 and (selected is none or e.entity_id in selected) and (in_area is none or e.entity_id in in_area) and ('%s'=='All' or e.domain=='%s') and (needle in e.entity_id|lower or needle in e.name|lower) %%}{%% set ns.items=ns.items+[e] %%}{%% endif %%}{%% endfor %%}{%% set entries=ns.items|sort(attribute='name') %%}{\"total\":{{ entries|count }},\"items\":[{%% for e in entries[%d:%d] %%}{{ dict(entity_id=e.entity_id,state=e.state[:39],attributes=dict(friendly_name=e.name),last_updated=e.last_updated.isoformat())|to_json }}{%% if not loop.last %%},{%% endif %%}{%% endfor %%}]}",scope,area_scope,needle,domain,domain,offset,offset+16);
 if(!quoted(template,escaped,sizeof(escaped)) || strlen(escaped)+17>=cap)return 0;sprintf(out,"{\"template\":\"%s\"}",escaped);return 1;
}
int amihass_device_page_request(const char *device,const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{return amihass_device_area_page_request(device,NULL,domain,filter,offset,out,cap);}
int amihass_page_request(const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{return amihass_device_page_request("",domain,filter,offset,out,cap);}
int amihass_device_id(const char *id)
{unsigned int i;if(!id || strlen(id)!=32)return 0;for(i=0;i<32;++i)if(!((id[i]>='0'&&id[i]<='9')||(id[i]>='a'&&id[i]<='f')))return 0;return 1;}
int amihass_devices_area_request(const char *area,const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{
 char needle[390],template[3200],escaped[6400],area_literal[576],area_scope[700];const char *p;
 if(!area)area="";if(!amihass_area_valid(area)||!literal(area,area_literal,sizeof(area_literal)))return 0;
 if(*area)sprintf(area_scope,"{%% set in_area=area_devices('%s') %%}",area_literal);else strcpy(area_scope,"{% set in_area=none %}");
 if(!domain || !*domain)domain="All";if(strlen(domain)>23)return 0;for(p=domain;*p;++p)if(!((*p>='a'&&*p<='z')||isdigit((unsigned char)*p)||*p=='_'||!strcmp(domain,"All")))return 0;
 if(offset<0 || offset>100000 || !literal(filter,needle,sizeof(needle)))return 0;
 sprintf(template,"%s{%% set ns=namespace(items=[],seen=[]) %%}{%% set needle='%s'|lower %%}{%% for e in states %%}{%% set d=device_id(e.entity_id) %%}{%% if d and d not in ns.seen and (in_area is none or d in in_area) and ('%s'=='All' or e.domain=='%s') %%}{%% set ns.seen=ns.seen+[d] %%}{%% set name=device_attr(d,'name_by_user') or device_attr(d,'name') or e.name or e.entity_id %%}{%% set match=namespace(ok=true) %%}{%% for word in needle.split() %%}{%% if word not in name|lower and word not in d %%}{%% set match.ok=false %%}{%% endif %%}{%% endfor %%}{%% if match.ok %%}{%% set ns.items=ns.items+[dict(id=d,name=name,state=device_attr(d,'manufacturer') or '',model=device_attr(d,'model') or '')] %%}{%% endif %%}{%% endif %%}{%% endfor %%}{%% set entries=ns.items|sort(attribute='name') %%}{\"total\":{{ entries|count }},\"items\":{{ entries[%d:%d]|to_json }}}",area_scope,needle,domain,domain,offset,offset+16);
 if(!quoted(template,escaped,sizeof(escaped)) || strlen(escaped)+17>=cap)return 0;sprintf(out,"{\"template\":\"%s\"}",escaped);return 1;
}
int amihass_devices_domain_request(const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{return amihass_devices_area_request(NULL,domain,filter,offset,out,cap);}
int amihass_devices_request(const char *filter,int offset,char *out,unsigned int cap)
{return amihass_devices_domain_request("",filter,offset,out,cap);}
static int registry_page(const char *json,struct AmiHASSEntity *items,unsigned int cap,int *total,int rooms)
{
 const char *p,*end;unsigned long n=0;unsigned int count=0;struct AmiHASSEntity e;
 if(!json || !items || !total || strlen(json)>AMIHASS_RESPONSE_LIMIT)return -1;end=skip(json,0);if(!end || *ws(end))return -1;
 p=member(json,"total");if(!p || !isdigit((unsigned char)*p))return -1;while(isdigit((unsigned char)*p)){n=n*10+*p++-'0';if(n>100000)return -1;}
 if(!strchr(",} \r\n\t",*p))return -1;p=member(json,"items");if(!p || *p++!='[')return -1;p=ws(p);
 while(*p!=']'){end=skip(p,0);memset(&e,0,sizeof(e));if(!end || count>=cap || !text_mode(member(p,"id"),e.id,sizeof(e.id),rooms) || (rooms ? (!e.id[0] || !amihass_area_valid(e.id)):!amihass_device_id(e.id)))return -1;
 (void)text(member(p,"name"),e.name,sizeof(e.name));(void)text(member(p,"state"),e.state,sizeof(e.state));(void)text(member(p,"model"),e.device_class,sizeof(e.device_class));if(!e.name[0]){strncpy(e.name,e.id,sizeof(e.name)-1);e.name[sizeof(e.name)-1]=0;}items[count++]=e;p=ws(end);if(*p==']')break;if(*p++!=',')return -1;p=ws(p);}
 if(count>n)return -1;*total=(int)n;return (int)count;
}
int amihass_devices_page(const char *json,struct AmiHASSEntity *items,unsigned int cap,int *total)
{return registry_page(json,items,cap,total,0);}
int amihass_areas_page(const char *json,struct AmiHASSEntity *items,unsigned int cap,int *total)
{return registry_page(json,items,cap,total,1);}
int amihass_areas_request(int offset,char *out,unsigned int cap)
{
 char template[900],escaped[1800];
 if(offset<0 || offset>100000)return 0;
 sprintf(template,"{%% set ns=namespace(items=[]) %%}{%% for a in areas() %%}{%% if a|length<96 %%}{%% set ns.items=ns.items+[dict(id=a,name=(area_name(a) or a)[:63])] %%}{%% endif %%}{%% endfor %%}{%% set entries=ns.items|sort(attribute='name') %%}{\"total\":{{ entries|count }},\"items\":{{ entries[%d:%d]|to_json }}}",offset,offset+16);
 if(!quoted(template,escaped,sizeof(escaped)) || strlen(escaped)+17>=cap)return 0;
 sprintf(out,"{\"template\":\"%s\"}",escaped);return 1;
}
int amihass_page(const char *json,struct AmiHASSEntity *items,unsigned int cap,int *total)
{
 const char *p,*end;unsigned long n=0;unsigned int count=0;struct AmiHASSEntity e;
 if(!json || !items || !total || strlen(json)>AMIHASS_RESPONSE_LIMIT)return -1;end=skip(json,0);if(!end || *ws(end))return -1;
 p=member(json,"total");if(!p || !isdigit((unsigned char)*p))return -1;while(isdigit((unsigned char)*p)){n=n*10+*p++-'0';if(n>100000)return -1;}if(!strchr(",} \r\n\t",*p))return -1;
 p=member(json,"items");if(!p || *p++!='[')return -1;p=ws(p);
 while(*p!=']'){end=skip(p,0);if(!end || !state(p,&e) || count>=cap)return -1;items[count++]=e;p=ws(end);if(*p==']')break;if(*p++!=',')return -1;p=ws(p);}
 if(count>n)return -1;*total=(int)n;return (int)count;
}
int amihass_command(int argc,char **argv)
{
 if(argc==2 && !strcmp(argv[1],"BACKEND"))return 1;
 if(argc==3 && !strcmp(argv[1],"FETCH"))return 2;
 if((argc==2 && !strcmp(argv[1],"CONNECTION")) || (argc==3 && !strcmp(argv[2],"CONNECTION")))return 3;
 return 0;
}

static int request_with_action(const struct AmiHASSConnection *c,const char *raw,const char *path,const char *entity,const char *action,char *out,unsigned int cap)
{
 char token[1025],payload[4096],escaped[8192];const char *p;unsigned int needed;volatile char *wipe;int catalogue;
 if(!c || !out || !raw || !path || !amihass_url(c->url,c->allow_http))return 0;catalogue=!strcmp(path,"/api/template");
 if(catalogue){const char *end;if(!entity || strlen(entity)>4095)return 0;end=skip(entity,0);if(!end || *ws(end) || !member(entity,"template") || !quoted(entity,escaped,sizeof(escaped)))return 0;}
 else{
  if(entity){char allowed[96],body[180];static const char *actions[]={"Toggle","Turn on","Turn off","Activate"};unsigned int i;int valid=0;
   if(action){if(!amihass_action(entity,action,allowed,body)||strcmp(allowed,path))return 0;strcpy(payload,body);valid=1;}else{for(i=0;i<4;++i)if(amihass_action(entity,actions[i],allowed,body) && !strcmp(allowed,path))valid=1;sprintf(payload,"{\"entity_id\":\"%s\"}",entity);}if(!valid)return 0;if(!quoted(payload,escaped,sizeof(escaped)))return 0;
  }else if(strcmp(path,"/api/") && strcmp(path,"/api/config") && strcmp(path,"/api/states") && (strncmp(path,"/api/states/",12) || !amihass_entity_id(path+12)))return 0;
 }
 if(strlen(path)>127 || strstr(path,"..") || strstr(path,"//"))return 0;
 for(p=path;*p;++p)if(!(isalnum((unsigned char)*p) || strchr("/_.",*p)))return 0;
 if(strlen(raw)>1026)return 0;
 needed=(unsigned int)(strlen(c->url)+strlen(path)+strlen(raw)+120+(entity ? strlen(escaped):0));
 if(needed>=cap || !amihass_token(raw,token))return 0;
 sprintf(out,"url = \"%s%s\"\nheader = \"Authorization: Bearer %s\"\nheader = \"Content-Type: application/json\"\n",c->url,path,token);
 if(entity)sprintf(out+strlen(out),"request = \"POST\"\ndata = \"%s\"\n",escaped);
 wipe=token;needed=sizeof(token);while(needed--)*wipe++=0;return 1;
}

int amihass_request(const struct AmiHASSConnection *c,const char *raw,const char *path,const char *entity,char *out,unsigned int cap)
{return request_with_action(c,raw,path,entity,NULL,out,cap);}
int amihass_control_request(const struct AmiHASSConnection *c,const char *raw,const char *entity,const char *action,char *out,unsigned int cap)
{char path[96],body[180];if(!amihass_action(entity,action,path,body))return 0;return request_with_action(c,raw,path,entity,action,out,cap);}
