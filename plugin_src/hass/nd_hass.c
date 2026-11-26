/* Home Assistant REST data policy. No networking, credentials or platform UI. */
#include "nd_hass.h"
#include "service/utf8_display.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* The only attributes AmiHASS reads. Batched polls and push keep just these. */
static const char *const attribute_keys[]={"friendly_name","unit_of_measurement","device_class","supported_color_modes","brightness","rgb_color","supported_features","percentage","code_format","current_position","volume_level","media_title","media_artist","temperature","min_temp","max_temp","target_temp_step","temperature_unit","hvac_modes"};
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
static unsigned long hex4(const char *p)
{unsigned int i;unsigned long v=0;for(i=0;i<4;++i)v=v*16+(isdigit((unsigned char)p[i])?p[i]-'0':tolower((unsigned char)p[i])-'a'+10);return v;}
/* Decodes one JSON string. Overflow keeps whole glyphs only; with truncate
 * set, a well-formed but over-long value yields its longest whole-glyph prefix. */
static int text_decode(const char *p,char *out,unsigned int cap,int identity,int truncate)
{unsigned int n=0,i,len;unsigned long c,low;const unsigned char *u;char glyph[5];const char *piece;int overflow=0;if(!p||!cap||!strend(p))return 0;++p;while(*p!='"'){if(*p=='\\'){++p;c=(unsigned char)*p++;if(c=='u'){c=hex4(p);p+=4;if(c>=0xd800&&c<=0xdbff){if(p[0]=='\\'&&p[1]=='u'&&(low=hex4(p+2))>=0xdc00&&low<=0xdfff){p+=6;c=0x10000+((c-0xd800)<<10)+(low-0xdc00);}else c=0xfffd;}else if(c>=0xdc00&&c<=0xdfff)c=0xfffd;}else if(c=='n'||c=='r'||c=='t'||c=='b'||c=='f'){if(identity)return 0;c=' ';}}else{u=(const unsigned char *)p;c=ah_utf8_next(&u);p=(const char *)u;}if(identity){if(c<32||c==127||c==0xfffd)return 0;ah_utf8_encode(c,glyph);piece=glyph;}else piece=ah_display_glyph(c,glyph);len=(unsigned int)strlen(piece);if(!overflow&&n+len<cap){for(i=0;i<len;++i)out[n++]=piece[i];}else overflow=1;}out[n]=0;return !overflow||truncate;}
static int text_mode(const char *p,char *out,unsigned int cap,int identity)
{return text_decode(p,out,cap,identity,0);}
static int text(const char *p,char *out,unsigned int cap)
{return text_mode(p,out,cap,0);}
static const char *member(const char *p,const char *name)
{
 const char *end,*value=NULL;char key[64];p=ws(p);if(*p++!='{')return NULL;p=ws(p);
 while(*p && *p!='}') {end=strend(p);if(!end)return NULL;if(text(p,key,sizeof(key)) && !strcmp(key,name)){if(value)return NULL;value=ws(end);if(*value++!=':')return NULL;value=ws(value);}
 p=ws(end);if(*p++!=':')return NULL;p=skip(p,0);if(!p)return NULL;p=ws(p);if(*p=='}')break;if(*p++!=',')return NULL;p=ws(p);}return value;
}
/* One scan of an object for up to 32 keys. Duplicate keys are rejected
 * (NULL) exactly as member() does, so both helpers agree on every input. */
static int members(const char *p,const char *const *names,const char **values,unsigned int count)
{
 const char *end,*value;char key[64];unsigned int i;unsigned long seen=0,dup=0;
 for(i=0;i<count;++i)values[i]=NULL;p=ws(p);if(*p++!='{')return 0;p=ws(p);
 while(*p && *p!='}'){end=strend(p);if(!end)return 0;value=ws(end);if(*value++!=':')return 0;value=ws(value);
  if(text(p,key,sizeof(key)))for(i=0;i<count;++i)if(!strcmp(key,names[i])){if(seen&(1UL<<i))dup|=1UL<<i;else{seen|=1UL<<i;values[i]=value;}break;}
  p=skip(value,0);if(!p)return 0;p=ws(p);if(*p=='}')break;if(*p++!=',')return 0;p=ws(p);}
 if(*p!='}')return 0;for(i=0;i<count;++i)if(dup&(1UL<<i))values[i]=NULL;return 1;
}
int amihass_api_response(const char *json)
{
 const char *end;char message[64];
 if(!json || strlen(json)>1024)return 0;
 end=skip(json,0);
 return end && !*ws(end) && text(member(json,"message"),message,sizeof(message)) && !strcmp(message,"API running.");
}
/* scheme://host[:port][/prefix]. A path prefix is for reverse proxies; it
 * never ends in '/' or "/api", and contains no empty, "." or ".." segments. */
int amihass_url(const char *url,int allow_http)
{
 const char *p,*host,*segment;unsigned long port=0;int n=0;
 if(!url || strlen(url)>=160)return 0;
 if(!strncmp(url,"https://",8))p=url+8;else if(allow_http && !strncmp(url,"http://",7))p=url+7;else return 0;
 host=p;while(*p && *p!=':' && *p!='/'){if(!(isalnum((unsigned char)*p) || *p=='.' || *p=='-'))return 0;++p;++n;}
 if(!n || host[0]=='.' || host[0]=='-' || p[-1]=='.' || p[-1]=='-')return 0;
 if(*p==':'){++p;if(!isdigit((unsigned char)*p))return 0;while(isdigit((unsigned char)*p)){if(port>6553)return 0;port=port*10+*p++-'0';}if(!port || port>65535)return 0;}
 while(*p=='/'){segment=++p;while(*p && *p!='/'){if(!(isalnum((unsigned char)*p) || strchr("-._~",*p)))return 0;++p;}
  if(p==segment || (p-segment==1 && segment[0]=='.') || (p-segment==2 && !strncmp(segment,"..",2)))return 0;
  if(!*p && p-segment==3 && !strncmp(segment,"api",3))return 0;}
 return !*p;
}
/* Settings input: drop surrounding blanks and trailing '/' characters. */
void amihass_url_clean(char *url)
{size_t n,start=0;if(!url)return;while(url[start]==' '||url[start]=='\t')++start;if(start)memmove(url,url+start,strlen(url+start)+1);n=strlen(url);while(n && (url[n-1]==' '||url[n-1]=='\t'||url[n-1]=='\r'||url[n-1]=='\n'))url[--n]=0;while(n>8 && url[n-1]=='/' && strstr(url,"://") && strstr(url,"://")+3<url+n-1)url[--n]=0;}
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
 static const char *const top[]={"entity_id","state","attributes","last_updated"};static const char *const attrs[]={"friendly_name","unit_of_measurement","device_class"};
 struct AmiHASSEntity e;const char *v[4],*a[3];memset(&e,0,sizeof(e));
 if(!members(json,top,v,4) || !text(v[0],e.id,sizeof(e.id)) || !amihass_entity_id(e.id) || !text_decode(v[1],e.state,sizeof(e.state),0,1))return 0;
 if(v[2] && *v[2]=='{' && members(v[2],attrs,a,3)){(void)text_decode(a[0],e.name,sizeof(e.name),0,1);(void)text_decode(a[1],e.unit,sizeof(e.unit),0,1);(void)text_decode(a[2],e.device_class,sizeof(e.device_class),0,1);}
 (void)text(v[3],e.updated,sizeof(e.updated));if(!e.name[0])strncpy(e.name,e.id,sizeof(e.name)-1);*out=e;return 1;
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
{if(!e)return "Unknown";if((!strncmp(e->id,"button.",7)||!strncmp(e->id,"input_button.",13))&&!strcmp(e->state,"unknown"))return "Ready";if(!strncmp(e->id,"binary_sensor.",14)&&(!strcmp(e->device_class,"door")||!strcmp(e->device_class,"garage_door")||!strcmp(e->device_class,"opening"))){if(!strcmp(e->state,"on"))return "Open";if(!strcmp(e->state,"off"))return "Closed";}if(!strncmp(e->id,"lock.",5)){if(!strcmp(e->state,"locked"))return "Locked";if(!strcmp(e->state,"unlocked"))return "Unlocked";if(!strcmp(e->state,"open"))return "Latch open";}return e->state;}
int amihass_available(const struct AmiHASSEntity *e)
{return e && e->id[0] && strcmp(e->state,"unavailable") && (strcmp(e->state,"unknown")||!strncmp(e->id,"button.",7)||!strncmp(e->id,"input_button.",13));}
/* Decimal text to hundredths. Extra fractional digits round half-up, so
 * float noise such as 0.4000000059604645 or 4.444444 is accepted. */
int amihass_number(const char *s,long *value)
{
 char d[40];int nd=0,point=-1,exp=0,neg=0,esign=1,i,round;long n=0,f=0;
 if(!s||!*s||!value)return 0;if(*s=='-'){neg=1;++s;}if(!isdigit((unsigned char)*s))return 0;
 while(isdigit((unsigned char)*s)||*s=='.'){if(*s=='.'){if(point>=0)return 0;point=nd;++s;if(!isdigit((unsigned char)*s))return 0;continue;}if(nd<(int)sizeof(d))d[nd++]=*s;else if(point<0)return 0;++s;}
 if(point<0)point=nd;
 if(*s=='e'||*s=='E'){++s;if(*s=='+'||*s=='-')esign=*s++=='-' ? -1:1;if(!isdigit((unsigned char)*s))return 0;while(isdigit((unsigned char)*s)){exp=exp*10+*s++-'0';if(exp>99)return 0;}}
 if(*s)return 0;point+=esign*exp;
 for(i=0;i<point;++i){n=n*10+(i<nd ? d[i]-'0':0);if(n>10000)return 0;}
 for(i=point;i<point+2;++i)f=f*10+(i>=0&&i<nd ? d[i]-'0':0);
 i=point+2;round=i>=0&&i<nd&&d[i]>='5';n=n*100+f+round;
 *value=neg ? -n:n;return 1;
}
void amihass_number_text(long v,char out[40])
{unsigned long n=v<0 ? (unsigned long)-v:(unsigned long)v;if(n%100)sprintf(out,"%s%lu.%02lu",v<0 ? "-":"",n/100,n%100);else sprintf(out,"%s%lu",v<0 ? "-":"",n/100);}
static int color_value(const char *s,unsigned long *rgb)
{unsigned int i;unsigned long value=0;if(!s||strlen(s)!=7||*s!='#')return 0;for(i=1;i<7;++i){if(!isxdigit((unsigned char)s[i]))return 0;value=value*16+(isdigit((unsigned char)s[i])?s[i]-'0':tolower((unsigned char)s[i])-'a'+10);}*rgb=value;return 1;}
/* Color modes as bits: 1 rgb/rgbw/rgbww/hs/xy, 2 brightness/color_temp. */
static unsigned int color_modes(const char *p)
{const char *end;char mode[24];unsigned int bits=0;if(!p||*p++!='[')return 0;p=ws(p);while(*p&&*p!=']'){end=strend(p);if(!end)return bits;if(text(p,mode,sizeof(mode))){if(!strcmp(mode,"rgb")||!strcmp(mode,"rgbw")||!strcmp(mode,"rgbww")||!strcmp(mode,"hs")||!strcmp(mode,"xy"))bits|=1;else if(!strcmp(mode,"brightness")||!strcmp(mode,"color_temp"))bits|=2;}p=ws(end);if(*p==']')break;if(*p++!=',')return bits;p=ws(p);}return bits;}
/* climate hvac_modes as bits in hvac_mode_names order. */
static const char *const hvac_mode_names[]={"off","heat","cool","auto","heat_cool","dry","fan_only"};
static unsigned int hvac_bits(const char *p)
{const char *end;char mode[24];unsigned int bits=0,i;if(!p||*p++!='[')return 0;p=ws(p);while(*p&&*p!=']'){end=strend(p);if(!end)return bits;if(text(p,mode,sizeof(mode)))for(i=0;i<7;++i)if(!strcmp(mode,hvac_mode_names[i]))bits|=1U<<i;p=ws(end);if(*p==']')break;if(*p++!=',')return bits;p=ws(p);}return bits;}
static int hvac_index(const char *mode)
{unsigned int i;for(i=0;i<7;++i)if(!strcmp(mode,hvac_mode_names[i]))return (int)i;return -1;}
static int number_value(const char *p,long *value)
{const char *end;char buffer[40];unsigned int n;if(!p)return 0;end=skip(p,0);if(!end)return 0;n=(unsigned int)(end-p);if(n>=sizeof(buffer))return 0;memcpy(buffer,p,n);buffer[n]=0;return amihass_number(buffer,value);}
static int rgb_close(unsigned long a,unsigned long b)
{unsigned int i,x,y;for(i=0;i<3;++i){x=(unsigned int)((a>>(i*8))&255);y=(unsigned int)((b>>(i*8))&255);if(x>y+3||y>x+3)return 0;}return 1;}
/* Home Assistant reports rgb_color scaled to full brightness. */
static unsigned long rgb_normalized(unsigned long rgb)
{unsigned long r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255,m=r>g ? r:g;if(b>m)m=b;if(!m)return rgb;return (((r*255+m/2)/m)<<16)|(((g*255+m/2)/m)<<8)|((b*255+m/2)/m);}
static int normalized_unit_text(const char *text,char unit[16])
{if(!text)return 0;if(!strcmp(text,"C")||!strcmp(text,"\260C")||!strcmp(text,"\302\260C")){strcpy(unit,"C");return 1;}if(!strcmp(text,"F")||!strcmp(text,"\260F")||!strcmp(text,"\302\260F")){strcpy(unit,"F");return 1;}return 0;}
static unsigned long media_features(const char *p)
{const char *end;unsigned long v=0;if(!p||(end=skip(p,0))==NULL||p==end)return 0;while(p<end){unsigned int n=(unsigned char)*p++-'0';if(n>9||v>(2147483647UL-n)/10UL)return 0;v=v*10UL+n;}return v;}
void amihass_controls(const char *json,struct AHControls *c)
{
 static const char *const keys[]={"supported_color_modes","brightness","rgb_color","supported_features","percentage","code_format","current_position","volume_level","media_title","media_artist","temperature","min_temp","max_temp","target_temp_step","temperature_unit","hvac_modes"};
 enum {MODES,BRIGHTNESS,RGB,FEATURES,PERCENT,CODE,POSITION,VOLUME,TITLE,ARTIST,TARGET,MIN,MAX,STEP,UNIT,HVAC};
 static const char *const top[]={"attributes"};
 struct AmiHASSEntity e;const char *a,*v[16],*p;long value,r,g,b;
 memset(c,0,sizeof(*c));if(!amihass_state(json,&e) || !members(json,top,&a,1) || !a || *a!='{' || !members(a,keys,v,16))return;
 if(!strncmp(e.id,"button.",7)||!strncmp(e.id,"input_button.",13)){c->press=1;}else if(!strncmp(e.id,"light.",6)){
  unsigned int modes=color_modes(v[MODES]);c->color=(modes&1)!=0;c->brightness=modes!=0;
  if(number_value(v[BRIGHTNESS],&value)&&value>=0&&value<=25500){c->brightness=1;c->level_known=1;c->level=(int)((value+127)/255);}
  if(!strcmp(e.state,"off")){c->level=0;c->level_known=1;}c->rgb=0xffffff;
  p=v[RGB];if(p&&*p++=='['){unsigned int i;long *values[3];values[0]=&r;values[1]=&g;values[2]=&b;for(i=0;i<3;++i){const char *end;p=ws(p);end=skip(p,0);if(!end||!number_value(p,values[i])||*values[i]<0||*values[i]>25500)break;*values[i]=(*values[i]+50)/100;p=ws(end);if(i<2){if(*p++!=',')break;}}if(i==3&&*p==']'){c->color_known=1;c->rgb=((unsigned long)r<<16)|((unsigned long)g<<8)|(unsigned long)b;}}
 }else if(!strncmp(e.id,"fan.",4)){
  if(number_value(v[FEATURES],&value) && value>=0 && value%100==0 && ((value/100)&1)){
   c->fan_speed=1;if(number_value(v[PERCENT],&value) && value>=0 && value<=10000){c->speed_known=1;c->speed=(int)((value+50)/100);}
   if(!strcmp(e.state,"off")){c->speed=0;c->speed_known=1;}
  }
 }else if(!strncmp(e.id,"lock.",5)){
  p=v[CODE];if(!p || !strncmp(p,"null",4)){c->lock=1;c->unlock=1;c->lock_open=(media_features(v[FEATURES])&1UL)!=0;}
 }else if(!strncmp(e.id,"cover.",6)){
  if(number_value(v[FEATURES],&value) && value>=0 && value%100==0){long features=value/100;c->cover_open=(features&1)!=0;c->cover_close=(features&2)!=0;c->position=(features&4)!=0;c->cover_stop=(features&8)!=0;}
  if(c->position && number_value(v[POSITION],&value) && value>=0 && value<=10000){c->position_known=1;c->position_value=(int)((value+50)/100);}
 }else if(!strncmp(e.id,"media_player.",13)){
  unsigned long features=media_features(v[FEATURES]);c->media_play=(features&16384UL)!=0;c->media_pause=(features&1UL)!=0;c->media_previous=(features&16UL)!=0;c->media_next=(features&32UL)!=0;c->media_volume=(features&4UL)!=0;
  if(c->media_volume&&number_value(v[VOLUME],&value)&&value>=0&&value<=100){c->volume_known=1;c->volume=(int)value;}
  (void)text_decode(v[TITLE],c->media_title,sizeof(c->media_title),0,1);(void)text_decode(v[ARTIST],c->media_artist,sizeof(c->media_artist),0,1);
 }else if(!strncmp(e.id,"climate.",8)){
  (void)text(v[UNIT],c->unit,sizeof(c->unit));
  if(number_value(v[TARGET],&c->target)&&number_value(v[MIN],&c->minimum)&&number_value(v[MAX],&c->maximum)&&c->minimum<c->maximum&&c->target>=c->minimum&&c->target<=c->maximum){
   /* Like Home Assistant's frontend: a missing step is 0.5 C or 1 F. */
   if(!number_value(v[STEP],&c->step)||c->step<=0){char unit[16];c->step_default=1;c->step=normalized_unit_text(c->unit,unit)&&!strcmp(unit,"F") ? 100:50;}
   c->temperature=1;
  }
  c->hvac_modes=hvac_bits(v[HVAC]);
 }else if(!strncmp(e.id,"alarm_control_panel.",20)){
  /* 1: no code needed; 2: a code is asked for each time and never stored. */
  p=v[CODE];c->alarm=!p || !strncmp(p,"null",4) ? 1:*p=='"' ? 2:0;
 }
}
static int normalized_unit(const char *text,char unit[16])
{return normalized_unit_text(text,unit);}
int amihass_temperature_unit(const char *json,char unit[16])
{const char *end,*system;char value[16];unit[0]=0;if(!json||strlen(json)>AMIHASS_RESPONSE_LIMIT)return 0;end=skip(json,0);if(!end||*ws(end))return 0;system=member(json,"unit_system");return system&&text(member(system,"temperature"),value,sizeof(value))&&normalized_unit(value,unit);}
void amihass_control_unit(struct AHControls *c,const char *unit)
{char normalized[16];if(normalized_unit(c->unit,normalized))strcpy(c->unit,normalized);else if(normalized_unit(unit,normalized))strcpy(c->unit,normalized);else c->unit[0]=0;if(c->step_default&&c->unit[0])c->step=!strcmp(c->unit,"F") ? 100:50;}
int amihass_control_matches(const struct AHControls *c,const char *state,const char *action)
{long v,diff;unsigned long rgb;if(!c||!state||!action)return 0;
 if(!strncmp(action,"HVAC ",5))return !strcmp(state,action+5);
 if(!strcmp(action,"Open valve"))return !strcmp(state,"open");if(!strcmp(action,"Close valve"))return !strcmp(state,"closed");
 if(!strcmp(action,"Arm home"))return !strcmp(state,"armed_home");if(!strcmp(action,"Arm away"))return !strcmp(state,"armed_away");if(!strcmp(action,"Arm night"))return !strcmp(state,"armed_night");if(!strcmp(action,"Disarm"))return !strcmp(state,"disarmed");
 if(!strcmp(action,"Start"))return !strcmp(state,"cleaning");if(!strcmp(action,"Return home"))return !strcmp(state,"returning")||!strcmp(state,"docked");if(!strcmp(action,"Pause cleaning"))return !strcmp(state,"paused");
 if(!strcmp(action,"Lock"))return c->lock&&!strcmp(state,"locked");
 if(!strcmp(action,"Unlock"))return c->unlock&&!strcmp(state,"unlocked");
 if(!strcmp(action,"Open lock"))return c->lock_open&&!strcmp(state,"open");
 if(!strcmp(action,"Press"))return 0;
 if(!strcmp(action,"Play"))return c->media_play&&!strcmp(state,"playing");
 if(!strcmp(action,"Pause"))return c->media_pause&&!strcmp(state,"paused");
 if(!strncmp(action,"Volume ",7)){if(!c->media_volume||!c->volume_known||!amihass_number(action+7,&v))return 0;diff=c->volume-v/100;return diff>=-1&&diff<=1;}
 if(!strncmp(action,"Fan speed ",10)){if(!c->fan_speed || !c->speed_known || !amihass_number(action+10,&v))return 0;diff=c->speed-v/100;return diff>=-1 && diff<=1;}
 if(!strncmp(action,"Position ",9)){if(!c->position || !c->position_known || !amihass_number(action+9,&v))return 0;diff=c->position_value-v/100;return diff>=-1 && diff<=1;}
 if(!strcmp(action,"Open cover"))return c->cover_open && !strcmp(state,"open");
 if(!strcmp(action,"Close cover"))return c->cover_close && !strcmp(state,"closed");
 if(!strcmp(action,"Stop cover"))return c->cover_stop && (!strcmp(state,"open") || !strcmp(state,"closed"));
 if(!strncmp(action,"Brightness ",11)){if(!c->brightness||!c->level_known||!amihass_number(action+11,&v))return 0;diff=c->level-v/100;return diff>=-1&&diff<=1&&!strcmp(state,v ? "on":"off");}
 if(!strncmp(action,"Color ",6)){if(!c->color||!c->color_known||strcmp(state,"on")||!color_value(action+6,&rgb))return 0;return rgb_close(rgb,c->rgb)||rgb_close(rgb_normalized(rgb),c->rgb);}
 if(!strncmp(action,"Temperature ",12)){if(!c->temperature||!c->unit[0]||!amihass_number(action+12,&v))return 0;diff=c->target-v;return diff>=-1&&diff<=1;}return 0;}
int amihass_control_allowed(const struct AHControls *c,const char *action)
{long value;unsigned long rgb;if(!c||!action)return 0;if(!strcmp(action,"Press"))return c->press;
 if(!strncmp(action,"HVAC ",5)){int i=hvac_index(action+5);return i>=0&&(c->hvac_modes>>i)&1U;}
 if(!strncmp(action,"Arm ",4)||!strcmp(action,"Disarm"))return c->alarm!=0;
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
 if(!strncmp(action,"Brightness ",11))return c->brightness&&amihass_number(action+11,&value)&&value>=0&&value<=10000&&value%100==0;if(!strncmp(action,"Color ",6))return c->color&&color_value(action+6,&rgb);if(!strncmp(action,"Temperature ",12))return c->temperature&&(!strcmp(c->unit,"C")||!strcmp(c->unit,"F"))&&amihass_number(action+12,&value)&&value>=c->minimum&&value<=c->maximum&&c->step>0&&((value-c->minimum)%c->step==0||value%c->step==0||value==c->maximum);return 1;}
unsigned int amihass_control_items(const struct AHControls *c,struct AmiHASSEntity *items,unsigned int cap)
{unsigned int n=0,i;const char *keys[]={"control.temperature","control.minimum","control.maximum","control.step"};long values[4];struct AmiHASSEntity *e;if(c->brightness&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.brightness");if(c->level_known)sprintf(e->state,"%d",c->level);else strcpy(e->state,"unknown");strcpy(e->unit,"%");}if(c->color&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.color");if(c->color_known)sprintf(e->state,"#%06lx",c->rgb);else strcpy(e->state,"unknown");}if(c->temperature){values[0]=c->target;values[1]=c->minimum;values[2]=c->maximum;values[3]=c->step;for(i=0;i<4&&n<cap;++i){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,keys[i]);amihass_number_text(values[i],e->state);strcpy(e->unit,c->unit);}}if(c->fan_speed && n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.fan_speed");if(c->speed_known)sprintf(e->state,"%d",c->speed);else strcpy(e->state,"unknown");strcpy(e->unit,"%");}
 if(c->position && n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.position");if(c->position_known)sprintf(e->state,"%d",c->position_value);else strcpy(e->state,"unknown");strcpy(e->unit,"%");}
 {const char *ids[]={"control.cover_open","control.cover_close","control.cover_stop"};int supported[3];supported[0]=c->cover_open;supported[1]=c->cover_close;supported[2]=c->cover_stop;for(i=0;i<3 && n<cap;++i)if(supported[i]){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,ids[i]);strcpy(e->state,"yes");}}
 {unsigned int flags=(c->media_play ? 1:0)|(c->media_pause ? 2:0)|(c->media_previous ? 4:0)|(c->media_next ? 8:0)|(c->media_volume ? 16:0);if(flags&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.media");sprintf(e->state,"%u",flags);}if(c->media_volume&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.media_volume");if(c->volume_known)sprintf(e->state,"%d",c->volume);else strcpy(e->state,"unknown");}if(c->media_title[0]&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.media_title");strcpy(e->name,c->media_title);}if(c->media_artist[0]&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.media_artist");strcpy(e->name,c->media_artist);}}
 {const char *ids[]={"control.lock","control.unlock","control.lock_open"};int flags[3];flags[0]=c->lock;flags[1]=c->unlock;flags[2]=c->lock_open;for(i=0;i<3&&n<cap;++i)if(flags[i]){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,ids[i]);strcpy(e->state,"yes");}}
 if(c->press&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.press");strcpy(e->state,"yes");}if(c->press&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.delivery");sprintf(e->state,"%lu",c->delivery);}
 if(c->hvac_modes&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.hvac_modes");sprintf(e->state,"%u",c->hvac_modes);}
 if(c->alarm&&n<cap){e=&items[n++];memset(e,0,sizeof(*e));strcpy(e->id,"control.alarm");strcpy(e->state,c->alarm==2?"code":"yes");}
 return n;}
void amihass_controls_items(const struct AmiHASSEntity *items,unsigned int count,struct AHControls *c)
{unsigned int i,mask=0;long v;memset(c,0,sizeof(*c));for(i=0;i<count&&i<AMIHASS_MAX_ENTITIES;++i){const struct AmiHASSEntity *e=&items[i];
 if(!strcmp(e->id,"control.hvac_modes")&&amihass_number(e->state,&v)&&v>=0&&v<=12700&&v%100==0){c->hvac_modes=(unsigned int)(v/100);continue;}
 if(!strcmp(e->id,"control.alarm")&&(!strcmp(e->state,"yes")||!strcmp(e->state,"code"))){c->alarm=e->state[0]=='c'?2:1;continue;}if(!strcmp(e->id,"control.media")&&amihass_number(e->state,&v)&&v>=0&&v<=3100&&v%100==0){unsigned int f=(unsigned int)(v/100);c->media_play=(f&1)!=0;c->media_pause=(f&2)!=0;c->media_previous=(f&4)!=0;c->media_next=(f&8)!=0;c->media_volume=(f&16)!=0;}
 else if(!strcmp(e->id,"control.press")&&!strcmp(e->state,"yes"))c->press=1;
 else if(!strcmp(e->id,"control.delivery")){const char *p=e->state;unsigned long v=0;int valid=*p!=0;for(;*p;++p){if(*p<'0'||*p>'9'||v>(4294967295UL-(unsigned int)(*p-'0'))/10UL){valid=0;break;}v=v*10UL+*p-'0';}if(valid)c->delivery=v;}
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
 else if(!strcmp(e->id,"control.brightness")&&amihass_number(e->state,&v)&&v>=0&&v<=10000&&v%100==0){c->brightness=1;c->level_known=1;c->level=(int)(v/100);}
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
 /* Domains added in 0.3: each maps fixed labels (or a numeric/option
  * argument) to one Home Assistant service. */
 if(!strcmp(domain,"climate")&&!strncmp(action,"HVAC ",5)){static const char *modes[]={"off","heat","cool","auto","heat_cool","dry","fan_only",NULL};unsigned int i;for(i=0;modes[i];++i)if(!strcmp(action+5,modes[i]))break;if(!modes[i])return 0;strcpy(path,"/api/services/climate/set_hvac_mode");sprintf(body,"{\"entity_id\":\"%s\",\"hvac_mode\":\"%s\"}",entity,modes[i]);return 1;}
 if(!strcmp(domain,"climate")&&(!strcmp(action,"Turn on")||!strcmp(action,"Turn off"))){sprintf(path,"/api/services/climate/%s",action[5]=='n' ? "turn_on":"turn_off");sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;}
 if(!strcmp(domain,"automation")&&!strcmp(action,"Trigger")){strcpy(path,"/api/services/automation/trigger");sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;}
 if(!strcmp(domain,"input_select")||!strcmp(domain,"select")){
  if(!strcmp(action,"Next option"))service="select_next";else if(!strcmp(action,"Previous option"))service="select_previous";
  else if(!strncmp(action,"Option ",7)&&action[7]){const char *o;char option[24];unsigned int k=0;for(o=action+7;*o;++o){if((unsigned char)*o<32||*o=='"'||*o=='\\'||k+1>=sizeof(option))return 0;option[k++]=*o;}option[k]=0;sprintf(path,"/api/services/%s/select_option",domain);sprintf(body,"{\"entity_id\":\"%s\",\"option\":\"%s\"}",entity,option);return 1;}
  if(!service)return 0;sprintf(path,"/api/services/%s/%s",domain,service);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;
 }
 if((!strcmp(domain,"number")||!strcmp(domain,"input_number"))&&!strncmp(action,"Value ",6)){if(!amihass_number(action+6,&value)||value< -100000000L||value>100000000L)return 0;amihass_number_text(value,number);sprintf(path,"/api/services/%s/set_value",domain);sprintf(body,"{\"entity_id\":\"%s\",\"value\":%s}",entity,number);return 1;}
 if(!strcmp(domain,"vacuum")){if(!strcmp(action,"Start"))service="start";else if(!strcmp(action,"Pause cleaning"))service="pause";else if(!strcmp(action,"Stop"))service="stop";else if(!strcmp(action,"Return home"))service="return_to_base";else if(!strcmp(action,"Locate"))service="locate";if(!service)return 0;sprintf(path,"/api/services/vacuum/%s",service);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;}
 if(!strcmp(domain,"valve")){
  if(!strncmp(action,"Position ",9)){if(!amihass_number(action+9,&value)||value<0||value>10000||value%100)return 0;strcpy(path,"/api/services/valve/set_valve_position");sprintf(body,"{\"entity_id\":\"%s\",\"position\":%ld}",entity,value/100);return 1;}
  if(!strcmp(action,"Open valve"))service="open_valve";else if(!strcmp(action,"Close valve"))service="close_valve";else if(!strcmp(action,"Stop valve"))service="stop_valve";if(!service)return 0;sprintf(path,"/api/services/valve/%s",service);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;
 }
 if(!strcmp(domain,"humidifier")&&!strncmp(action,"Humidity ",9)){if(!amihass_number(action+9,&value)||value<0||value>10000||value%100)return 0;strcpy(path,"/api/services/humidifier/set_humidity");sprintf(body,"{\"entity_id\":\"%s\",\"humidity\":%ld}",entity,value/100);return 1;}
 if(!strcmp(domain,"alarm_control_panel")){if(!strcmp(action,"Arm home"))service="alarm_arm_home";else if(!strcmp(action,"Arm away"))service="alarm_arm_away";else if(!strcmp(action,"Arm night"))service="alarm_arm_night";else if(!strcmp(action,"Disarm"))service="alarm_disarm";if(!service)return 0;sprintf(path,"/api/services/alarm_control_panel/%s",service);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;}
 if(!strcmp(domain,"button")||!strcmp(domain,"input_button")){if(strcmp(action,"Press"))return 0;sprintf(path,"/api/services/%s/press",domain);sprintf(body,"{\"entity_id\":\"%s\"}",entity);return 1;}
 if(!strcmp(domain,"scene") || !strcmp(domain,"script")){if(strcmp(action,"Activate") && strcmp(action,"Run") && strcmp(action,"Confirm and run") && strcmp(action,"Turn on") && strcmp(action,"Toggle"))return 0;target=domain;service="turn_on";}
 else if(!strcmp(domain,"light") || !strcmp(domain,"switch") || !strcmp(domain,"fan") || !strcmp(domain,"input_boolean") || !strcmp(domain,"group") || !strcmp(domain,"media_player") || !strcmp(domain,"automation") || !strcmp(domain,"siren") || !strcmp(domain,"humidifier")){
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
 while((c=(unsigned char)*s++)!=0){if(c<32)return 0;if(c>=194){const unsigned char *u=(const unsigned char *)s-1,*end=u;unsigned long cp=ah_utf8_next(&end);if(cp!=0xfffdUL){unsigned int len=(unsigned int)(end-u);if(n+len>=cap)return 0;memcpy(out+n,u,len);n+=len;s=(const char *)end;continue;}}if(c>=127){static const char hex[]="0123456789abcdef";if(n+6>=cap)return 0;out[n++]='\\';out[n++]='u';out[n++]='0';out[n++]='0';out[n++]=hex[c>>4];out[n++]=hex[c&15];continue;}if(c=='\\' || c=='\''){if(n+2>=cap)return 0;out[n++]='\\';}if(n+1>=cap)return 0;out[n++]=(char)c;}out[n]=0;return 1;
}
static int quoted(const char *s,char *out,unsigned int cap)
{
 unsigned int n=0;unsigned char c;
 while((c=(unsigned char)*s++)!=0){if(c=='\\' || c=='"'){if(n+2>=cap)return 0;out[n++]='\\';}else if(c<32)return 0;if(n+1>=cap)return 0;out[n++]=(char)c;}out[n]=0;return 1;
}
int amihass_area_valid(const char *area)
{const unsigned char *p=(const unsigned char *)area;if(!area||strlen(area)>95)return 0;for(;*p;++p)if(*p<32||*p==127)return 0;return 1;}
int amihass_device_scope_page_request(const char *device,const char *area,const char *label,const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{
 char needle[390],escaped[12800],template[6400],scope[160],area_literal[576],area_scope[700],label_literal[390],label_scope[1000];const char *p;
 if(!label)label="";if(strlen(label)>63||!literal(label,label_literal,sizeof(label_literal)))return 0;if(*label)sprintf(label_scope,"{%% set in_label=label_entities('%s') %%}{%% set in_label_devices=label_devices('%s') %%}",label_literal,label_literal);else strcpy(label_scope,"{% set in_label=none %}");
 if(!area)area="";if(!amihass_area_valid(area)||!literal(area,area_literal,sizeof(area_literal)))return 0;
 if(*area)sprintf(area_scope,"{%% set in_area=area_entities('%s') %%}",area_literal);else strcpy(area_scope,"{% set in_area=none %}");
 if(!device)device="";if(*device && !amihass_device_id(device))return 0;
 sprintf(scope,"{%% set selected=%s",*device ? "device_entities('":"none");if(*device){strcat(scope,device);strcat(scope,"')");}strcat(scope," %}");
 if(!domain || !*domain)domain="All";if(strlen(domain)>23 || offset<0 || offset>100000 || !literal(filter,needle,sizeof(needle)))return 0;
 for(p=domain;*p;++p)if(!((*p>='a' && *p<='z') || isdigit((unsigned char)*p) || *p=='_' || !strcmp(domain,"All")))return 0;
 sprintf(template,"%s%s%s{%% set ns=namespace(items=[],count=0) %%}{%% set needle='%s'|lower %%}{%% set q=states if '%s'=='All' else states['%s'] %%}{%% if selected is not none %%}{%% set q=q|selectattr('entity_id','in',selected) %%}{%% endif %%}{%% if in_area is not none %%}{%% set q=q|selectattr('entity_id','in',in_area) %%}{%% endif %%}{%% if in_label is not none %%}{%% set q=q|selectattr('entity_id','in',in_label+(in_label_devices|map('device_entities')|sum(start=[]))) %%}{%% endif %%}{%% for e in q|sort(attribute='name') %%}{%% if e.entity_id|length<96 and (needle in e.entity_id|lower or needle in e.name|lower) %%}{%% set ns.count=ns.count+1 %%}{%% if ns.count>%d and ns.count<=%d %%}{%% set ns.items=ns.items+[e] %%}{%% endif %%}{%% endif %%}{%% endfor %%}{\"total\":{{ ns.count }},\"items\":[{%% for e in ns.items %%}{{ dict(entity_id=e.entity_id,state=e.state[:39],attributes=dict(friendly_name=e.name),last_updated=e.last_updated.isoformat())|to_json }}{%% if not loop.last %%},{%% endif %%}{%% endfor %%}]}",scope,area_scope,label_scope,needle,domain,domain,offset,offset+16);
 if(!quoted(template,escaped,sizeof(escaped)) || strlen(escaped)+17>=cap)return 0;sprintf(out,"{\"template\":\"%s\"}",escaped);return 1;
}
int amihass_device_page_request(const char *device,const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{return amihass_device_area_page_request(device,NULL,domain,filter,offset,out,cap);}
int amihass_page_request(const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{return amihass_device_page_request("",domain,filter,offset,out,cap);}
/* One request for several selected entities. Only the attributes AmiHASS
 * reads are rendered; a missing entity is reported in place, never dropped. */
/* Accepts only ISO-8601 timestamp characters, so `known` is safe in a template. */
static int iso_stamp(const char *t)
{unsigned int n=0;if(!t)return 0;for(;*t;++t,++n)if(!(isdigit((unsigned char)*t)||strchr("T:.+-Z",*t))||n>=39)return 0;return n>=10;}
/* One request for several selected entities. Only the attributes AmiHASS
 * reads are rendered; a missing entity is reported in place, never dropped.
 * With `known` (last_updated values already held), an unchanged entity comes
 * back as {"entity_id":...,"same":true} instead of its full state. */
int amihass_states_request_known(const char ids[][96],const char known[][40],unsigned int count,char *out,unsigned int cap)
{
 char template[4200],escaped[8400],list[2700];unsigned int i;
 if(!ids||!out||!count||count>AMIHASS_BATCH)return 0;list[0]=0;
 for(i=0;i<count;++i){if(!amihass_entity_id(ids[i]))return 0;if(i)strcat(list,",");strcat(list,"('");strcat(list,ids[i]);strcat(list,"','");if(known&&iso_stamp(known[i]))strcat(list,known[i]);strcat(list,"')");}
 strcpy(template,"{% set keys=[");for(i=0;i<sizeof(attribute_keys)/sizeof(attribute_keys[0]);++i){if(i)strcat(template,",");strcat(template,"'");strcat(template,attribute_keys[i]);strcat(template,"'");}
 sprintf(template+strlen(template),"] %%}[{%% for id,known in [%s] %%}{%% set s=states[id] %%}{%% if s is none %%}{\"entity_id\":{{ id|to_json }},\"missing\":true}{%% elif known and s.last_updated.isoformat()==known %%}{\"entity_id\":{{ id|to_json }},\"same\":true}{%% else %%}{\"entity_id\":{{ s.entity_id|to_json }},\"state\":{{ s.state|to_json }},\"last_updated\":{{ s.last_updated.isoformat()|to_json }},\"attributes\":{ {%%- for k in keys if k in s.attributes %%}{{ k|to_json }}:{{ s.attributes[k]|to_json }}{%% if not loop.last %%},{%% endif %%}{%% endfor -%%} }}{%% endif %%}{%% if not loop.last %%},{%% endif %%}{%% endfor %%}]",list);
 if(!quoted(template,escaped,sizeof(escaped)) || strlen(escaped)+17>=cap){if(known)return amihass_states_request_known(ids,NULL,count,out,cap);return 0;}sprintf(out,"{\"template\":\"%s\"}",escaped);return 1;
}
int amihass_states_request(const char ids[][96],unsigned int count,char *out,unsigned int cap)
{return amihass_states_request_known(ids,NULL,count,out,cap);}
/* Actions that open a door or cover: the Dashboard asks first, and the
 * service refuses them from other clients unless the user allows it. */
int amihass_action_sensitive(const char *entity,const char *action)
{return entity&&action&&((!strncmp(entity,"lock.",5)&&(!strcmp(action,"Unlock")||!strcmp(action,"Open lock")))||(!strncmp(entity,"cover.",6)&&(!strcmp(action,"Open cover")||!strncmp(action,"Position ",9)))||(!strncmp(entity,"alarm_control_panel.",20)&&!strcmp(action,"Disarm")));}
int amihass_device_id(const char *id)
{unsigned int i;if(!id || strlen(id)!=32)return 0;for(i=0;i<32;++i)if(!((id[i]>='0'&&id[i]<='9')||(id[i]>='a'&&id[i]<='f')))return 0;return 1;}
int amihass_devices_scope_request(const char *area,const char *label,const char *domain,const char *filter,int offset,char *out,unsigned int cap)
{
 char needle[390],template[6400],escaped[12800],area_literal[576],area_scope[700],label_literal[390],label_scope[1000];const char *p;
 if(!label)label="";if(strlen(label)>63||!literal(label,label_literal,sizeof(label_literal)))return 0;if(*label)sprintf(label_scope,"{%% set in_label=label_entities('%s') %%}{%% set in_label_devices=label_devices('%s') %%}",label_literal,label_literal);else strcpy(label_scope,"{% set in_label=none %}");
 if(!area)area="";if(!amihass_area_valid(area)||!literal(area,area_literal,sizeof(area_literal)))return 0;
 if(*area)sprintf(area_scope,"{%% set in_area=area_devices('%s') %%}",area_literal);else strcpy(area_scope,"{% set in_area=none %}");
 if(!domain || !*domain)domain="All";if(strlen(domain)>23)return 0;for(p=domain;*p;++p)if(!((*p>='a'&&*p<='z')||isdigit((unsigned char)*p)||*p=='_'||!strcmp(domain,"All")))return 0;
 if(offset<0 || offset>100000 || !literal(filter,needle,sizeof(needle)))return 0;
 sprintf(template,"%s%s{%% set needle='%s'|lower %%}{%% set q=states if '%s'=='All' else states['%s'] %%}{%% if in_label is not none %%}{%% set q=q|selectattr('entity_id','in',in_label+(in_label_devices|map('device_entities')|sum(start=[]))) %%}{%% endif %%}{%% set q=q|list %%}{%% set ns=namespace(items=[]) %%}{%% for d in q|map(attribute='entity_id')|map('device_id')|reject('none')|unique %%}{%% if in_area is none or d in in_area %%}{%% set name=device_attr(d,'name_by_user') or device_attr(d,'name') %%}{%% if not name %%}{%% set first=q|selectattr('entity_id','in',device_entities(d))|first %%}{%% set name=first.name or first.entity_id %%}{%% endif %%}{%% set match=namespace(ok=true) %%}{%% for word in needle.split() %%}{%% if word not in name|lower and word not in d %%}{%% set match.ok=false %%}{%% endif %%}{%% endfor %%}{%% if match.ok %%}{%% set ns.items=ns.items+[dict(id=d,name=name,state=device_attr(d,'manufacturer') or '',model=device_attr(d,'model') or '')] %%}{%% endif %%}{%% endif %%}{%% endfor %%}{%% set entries=ns.items|sort(attribute='name') %%}{\"total\":{{ entries|count }},\"items\":{{ entries[%d:%d]|to_json }}}",area_scope,label_scope,needle,domain,domain,offset,offset+16);
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

static int history_path_valid(const char *path);
static int history_stamp(const char *s);
/* An alarm code: 1-16 letters or digits. It goes only into the request
 * body for that one action. */
int amihass_alarm_code(const char *code)
{unsigned int n;if(!code)return 0;for(n=0;code[n];++n)if(!isalnum((unsigned char)code[n]))return 0;return n>=1&&n<=16;}
static const char *action_code;
static int request_with_action(const struct AmiHASSConnection *c,const char *raw,const char *path,const char *entity,const char *action,char *out,unsigned int cap)
{
 char token[1025],payload[4096],escaped[8192];const char *p;unsigned int needed;volatile char *wipe;int catalogue,camera=0,history=0;char camera_id[96];const char *query;
 if(!c || !out || !raw || !path || !amihass_url(c->url,c->allow_http))return 0;catalogue=!strcmp(path,"/api/template");if(!strncmp(path,"/api/camera_proxy/",18)){query=strchr(path+18,'?');if(!query||(strcmp(query,"?width=320&height=200")&&strcmp(query,"?width=96&height=60"))||query-(path+18)>=96)return 0;memcpy(camera_id,path+18,(unsigned int)(query-path-18));camera_id[query-path-18]=0;if(strncmp(camera_id,"camera.",7)||!amihass_entity_id(camera_id)||entity)return 0;camera=1;}
 if(!strncmp(path,"/api/history/period",19)){if(entity||!history_path_valid(path))return 0;history=1;}
 if(catalogue){const char *end;if(!entity || strlen(entity)>4095)return 0;end=skip(entity,0);if(!end || *ws(end) || !member(entity,"template") || !quoted(entity,escaped,sizeof(escaped)))return 0;}
 else{
  if(entity){char allowed[96],body[180];static const char *actions[]={"Toggle","Turn on","Turn off","Activate"};unsigned int i;int valid=0;
   if(action){if(!amihass_action(entity,action,allowed,body)||strcmp(allowed,path))return 0;strcpy(payload,body);valid=1;
    if(action_code){unsigned int n=(unsigned int)strlen(payload);if(strncmp(entity,"alarm_control_panel.",20)||!amihass_alarm_code(action_code)||n<2||payload[n-1]!='}')return 0;sprintf(payload+n-1,",\"code\":\"%s\"}",action_code);}}else{for(i=0;i<4;++i)if(amihass_action(entity,actions[i],allowed,body) && !strcmp(allowed,path))valid=1;sprintf(payload,"{\"entity_id\":\"%s\"}",entity);}if(!valid)return 0;if(!quoted(payload,escaped,sizeof(escaped)))return 0;
  }else if(!camera && !history && strcmp(path,"/api/") && strcmp(path,"/api/config") && strcmp(path,"/api/states") && (strncmp(path,"/api/states/",12) || !amihass_entity_id(path+12)))return 0;
 }
 if(strlen(path)>(camera?160:history?240:127) || strstr(path,"..") || strstr(path,"//"))return 0;
 for(p=path;*p;++p)if(!(isalnum((unsigned char)*p) || strchr(camera ? "/_.?=&":history ? "/_.?=&:-":"/_.",*p)))return 0;
 if(strlen(raw)>1026)return 0;
 needed=(unsigned int)(strlen(c->url)+strlen(path)+strlen(raw)+120+(entity ? strlen(escaped):0));
 if(needed>=cap || !amihass_token(raw,token))return 0;
 sprintf(out,"url = \"%s%s\"\nheader = \"Authorization: Bearer %s\"\nheader = \"Content-Type: application/json\"\n",c->url,path,token);
 if(entity)sprintf(out+strlen(out),"request = \"POST\"\ndata = \"%s\"\n",escaped);
 wipe=token;needed=sizeof(token);while(needed--)*wipe++=0;if(action_code){wipe=payload;needed=sizeof(payload);while(needed--)*wipe++=0;wipe=escaped;needed=sizeof(escaped);while(needed--)*wipe++=0;}return 1;
}

int amihass_request(const struct AmiHASSConnection *c,const char *raw,const char *path,const char *entity,char *out,unsigned int cap)
{return request_with_action(c,raw,path,entity,NULL,out,cap);}
int amihass_control_request_code(const struct AmiHASSConnection *c,const char *raw,const char *entity,const char *action,const char *code,char *out,unsigned int cap)
{int ok;char path[96],body[180];if(!amihass_action(entity,action,path,body))return 0;action_code=code&&*code?code:NULL;ok=request_with_action(c,raw,path,entity,action,out,cap);action_code=NULL;memset(body,0,sizeof(body));return ok;}
int amihass_control_request(const struct AmiHASSConnection *c,const char *raw,const char *entity,const char *action,char *out,unsigned int cap)
{char path[96],body[180];if(!amihass_action(entity,action,path,body))return 0;return request_with_action(c,raw,path,entity,action,out,cap);}

const char *ah_json_member(const char *json,const char *key){return json ? member(json,key):NULL;}
const char *ah_json_end(const char *value){return value ? skip(value,0):NULL;}
int ah_json_text(const char *value,char *out,unsigned int cap,int identity){return text_mode(value,out,cap,identity);}
int ah_json_text_prefix(const char *value,char *out,unsigned int cap,int identity){return text_decode(value,out,cap,identity,1);}
int amihass_attribute_wanted(const char *name)
{unsigned int i;for(i=0;i<sizeof(attribute_keys)/sizeof(attribute_keys[0]);++i)if(!strcmp(name,attribute_keys[i]))return 1;return 0;}

int amihass_device_area_page_request(const char *device,const char *area,const char *domain,const char *filter,int offset,char *out,unsigned int cap){return amihass_device_scope_page_request(device,area,NULL,domain,filter,offset,out,cap);}
int amihass_devices_area_request(const char *area,const char *domain,const char *filter,int offset,char *out,unsigned int cap){return amihass_devices_scope_request(area,NULL,domain,filter,offset,out,cap);}

int amihass_labels_request(int offset,char *out,unsigned int cap)
{char text[1000],escaped[2000];if(offset<0||offset>100000)return 0;sprintf(text,"{%% set ns=namespace(items=[]) %%}{%% for l in labels() %%}{%% if l|length<64 %%}{%% set ns.items=ns.items+[dict(id=l,name=(label_name(l) or l)[:63])] %%}{%% endif %%}{%% endfor %%}{%% set entries=ns.items|sort(attribute='name') %%}{\"total\":{{ entries|count }},\"items\":{{ entries[%d:%d]|to_json }}}",offset,offset+16);if(!quoted(text,escaped,sizeof(escaped))||strlen(escaped)+17>=cap)return 0;sprintf(out,"{\"template\":\"%s\"}",escaped);return 1;}
int amihass_labels_page(const char *json,struct AmiHASSEntity *items,unsigned int cap,int *total)
{int n=registry_page(json,items,cap,total,1),i;for(i=0;i<n;++i)if(strlen(items[i].id)>63)return -1;return n;}
/* --- Full attributes and history (SDK ATTRIBUTES / HISTORY) --- */
/* Raw JSON text of a non-string value, whitespace outside strings removed,
 * cut to whole bytes; strings are decoded for display. */
static void value_text(const char *p,const char *end,char *out,unsigned int cap)
{unsigned int n=0;int instr=0,esc=0;for(;p<end&&n+1<cap;++p){if(instr){if(esc)esc=0;else if(*p=='\\')esc=1;else if(*p=='"')instr=0;}else if(*p=='"')instr=1;else if(strchr(" \r\n\t",*p))continue;if((unsigned char)*p<32)continue;out[n++]=*p;}out[n]=0;}
static const char *value_type(const char *p)
{p=ws(p);return *p=='"'?"string":*p=='{'?"object":*p=='['?"list":*p=='t'||*p=='f'?"boolean":*p=='n'?"null":"number";}
/* Every attribute of a /api/states/<id> reply, in server order. Returns the
 * number stored (at most cap); *total is the full count; -1 if malformed. */
int amihass_attributes_page(const char *json,struct AmiHASSEntity *items,unsigned int cap,int *total)
{
 const char *p,*end,*value,*vend;unsigned int count=0;int n=0;struct AmiHASSEntity e;
 if(!json||!items||!total||strlen(json)>AMIHASS_RESPONSE_LIMIT)return -1;end=skip(json,0);if(!end||*ws(end))return -1;
 p=member(json,"attributes");if(!p)return -1;p=ws(p);if(*p++!='{')return -1;p=ws(p);
 while(*p!='}'){end=strend(p);if(!end)return -1;value=ws(end);if(*value++!=':')return -1;value=ws(value);vend=skip(value,0);if(!vend)return -1;
  memset(&e,0,sizeof(e));if(!text_decode(p,e.id,sizeof(e.id),0,1))return -1;strcpy(e.device_class,value_type(value));
  if(*value=='"'){if(!text_decode(value,e.name,sizeof(e.name),0,1))return -1;}else value_text(value,vend,e.name,sizeof(e.name));
  if(count<cap)items[count++]=e;++n;p=ws(vend);if(*p=='}')break;if(*p++!=',')return -1;p=ws(p);}
 *total=n;return (int)count;
}
/* Template body asking the server for "now minus hours" in UTC, so history
 * windows never depend on the Amiga clock. */
/* chunks+1 window boundaries from the server clock, oldest first, separated
 * by spaces: a busy sensor's day can then be read in smaller pieces. */
int amihass_history_bounds_request(int hours,int chunks,char *out,unsigned int cap)
{if(hours<1||hours>48||chunks<1||chunks>12||!out||cap<260)return 0;
 sprintf(out,"{\"template\":\"{%% set s = utcnow() - timedelta(hours=%d) %%}{%% for i in range(%d) %%}{{ (s + timedelta(seconds=%ld*i)).strftime('%%Y-%%m-%%dT%%H:%%M:%%SZ') }} {%% endfor %%}\"}",hours,chunks+1,(long)hours*3600L/chunks);return 1;}
int amihass_history_bounds(const char *reply,char bounds[][24],int count)
{int i;unsigned int n;if(!reply||count<2)return 0;for(i=0;i<count;++i){reply=ws(reply);n=0;while(reply[n]&&!strchr(" \r\n\t",reply[n])&&n<23){bounds[i][n]=reply[n];++n;}bounds[i][n]=0;reply+=n;if(!history_stamp(bounds[i])||(i&&strcmp(bounds[i],bounds[i-1])<=0))return 0;}return !*ws(reply);}
int amihass_history_start_request(int hours,char *out,unsigned int cap)
{if(hours<1||hours>48||!out||cap<120)return 0;sprintf(out,"{\"template\":\"{{ (utcnow()-timedelta(hours=%d)).strftime('%%Y-%%m-%%dT%%H:%%M:%%SZ') }}\"}",hours);return 1;}
static int history_stamp(const char *s)
{static const char shape[]="dddd-dd-ddTdd:dd:ddZ";unsigned int i;if(!s||strlen(s)!=20)return 0;for(i=0;i<20;++i)if(shape[i]=='d'?!isdigit((unsigned char)s[i]):s[i]!=shape[i])return 0;return 1;}
/* Reads the template reply (plain text, possibly padded). */
int amihass_history_start(const char *reply,char start[24])
{char t[24];unsigned int n;if(!reply)return 0;reply=ws(reply);n=0;while(reply[n]&&!strchr(" \r\n\t",reply[n])&&n<23){t[n]=reply[n];++n;}t[n]=0;if(*ws(reply+n)||!history_stamp(t))return 0;strcpy(start,t);return 1;}
/* start may be empty: the server's default window (the last 24 hours). */
int amihass_history_range_path(const char *entity,const char *start,const char *end,char *out,unsigned int cap)
{if(!out||!amihass_entity_id(entity)||(start&&*start&&!history_stamp(start))||(end&&*end&&(!start||!*start||!history_stamp(end)))||cap<strlen(entity)+120)return 0;
 sprintf(out,"/api/history/period%s%s?filter_entity_id=%s%s%s&minimal_response&no_attributes",start&&*start?"/":"",start&&*start?start:"",entity,end&&*end?"&end_time=":"",end&&*end?end:"");return 1;}
int amihass_history_path(const char *entity,const char *start,char *out,unsigned int cap){return amihass_history_range_path(entity,start,NULL,out,cap);}
static int history_path_valid(const char *path)
{char entity[96],start[24],end[24],again[240];const char *q=path+19,*e;unsigned int n;start[0]=end[0]=0;
 if(*q=='/'){++q;if(strlen(q)<20)return 0;memcpy(start,q,20);start[20]=0;q+=20;}
 if(strncmp(q,"?filter_entity_id=",18))return 0;q+=18;e=strchr(q,'&');if(!e||(n=(unsigned int)(e-q))>=sizeof(entity))return 0;memcpy(entity,q,n);entity[n]=0;
 if(!strncmp(e,"&end_time=",10)){if(strlen(e+10)<20)return 0;memcpy(end,e+10,20);end[20]=0;}
 return amihass_history_range_path(entity,start,end,again,sizeof(again))&&!strcmp(again,path);}
/* States of a minimal history reply (gaps as empty), thinned to at most cap points
 * spread evenly across the window (the newest point is always kept). */
static int history_numeric(const char *s)
{if(*s=='-')++s;if(!isdigit((unsigned char)*s))return 0;while(isdigit((unsigned char)*s))++s;if(*s=='.'){++s;if(!isdigit((unsigned char)*s))return 0;while(isdigit((unsigned char)*s))++s;}return !*s;}
int amihass_history_points(const char *json,unsigned long size,struct AmiHASSEntity *items,unsigned int cap)
{
 const char *p,*end,*outer;unsigned long n=0,k;unsigned int count=0,pass;char state[40],when[40];
 if(!json||!items||cap<2||strlen(json)!=size)return -1;end=skip(json,0);if(!end||*ws(end))return -1;
 p=ws(json);if(*p++!='[')return -1;p=ws(p);if(*p==']')return 0;if(*p!='[')return -1;outer=p;
 for(pass=0;pass<2;++pass){/* pass 0 counts numeric points, pass 1 stores the chosen ones */
  p=ws(outer+1);k=0;
  while(*p!=']'){end=skip(p,0);if(!end||*p!='{')return -1;
   /* Non-numeric states (unavailable, unknown) are kept as gaps: empty state. */
   if(text_decode(member(p,"state"),state,sizeof(state),0,1)){if(!history_numeric(state))state[0]=0;
    if(!pass)++n;
    else{if(count<cap&&(n<=cap||k==(unsigned long)count*(n-1)/(cap-1))){memset(&items[count],0,sizeof(items[count]));strcpy(items[count].state,state);if(text_decode(member(p,"last_changed"),when,sizeof(when),1,0)||text_decode(member(p,"last_updated"),when,sizeof(when),1,0))strcpy(items[count].updated,when);++count;}++k;}}
   p=ws(end);if(*p==']')break;if(*p++!=',')return -1;p=ws(p);}
  if(!n)break;}
 return (int)count;
}
/* A history state as hundredths (|value| < 1 million); no floating point. */
int amihass_history_value(const char *s,long *value)
{long n=0,f=0;int neg=0,digits=0;if(!s||!value)return 0;if(*s=='-'){neg=1;++s;}if(!isdigit((unsigned char)*s))return 0;while(isdigit((unsigned char)*s)){n=n*10+*s++-'0';if(n>=1000000L)return 0;}
 if(*s=='.'){++s;if(!isdigit((unsigned char)*s))return 0;while(isdigit((unsigned char)*s)){if(digits<2){f=f*10+*s-'0';++digits;}else if(digits==2){if(*s>='5')++f;++digits;}++s;}while(digits<2){f*=10;++digits;}}
 if(*s)return 0;n=n*100+f;*value=neg?-n:n;return 1;}
