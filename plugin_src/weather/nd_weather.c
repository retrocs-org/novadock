/* Bounded Open-Meteo CSV forecast and geocoding readers; no floating point. */
#include "nd_weather.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static int integer(const char *s,int *value,int decimal)
{
    long n=0;int sign=1,digit=0,tenth=0;
    if(*s=='-'){sign=-1;++s;}else if(*s=='+')++s;
    while(isdigit((unsigned char)*s)){if(n>10000)return 0;n=n*10+(*s++-'0');digit=1;}
    if(!digit)return 0;
    if(*s=='.'){if(!decimal)return 0;++s;if(!isdigit((unsigned char)*s))return 0;tenth=*s++-'0';
        while(isdigit((unsigned char)*s))++s;}
    if(*s)return 0;
    n=sign*(decimal ? n*10+tenth:n);if(n < -10000 || n>10000)return 0;*value=(int)n;return 1;
}
static int date(const char *s,int timestamp)
{
    int i,y,m,d,h,minute,days;unsigned int n=timestamp ? 16:10;
    if(strlen(s)!=n || s[4]!='-' || s[7]!='-')return 0;
    for(i=0;i<10;++i)if(i!=4 && i!=7 && !isdigit((unsigned char)s[i]))return 0;
    y=atoi(s);m=atoi(s+5);d=atoi(s+8);if(y<1978 || m<1 || m>12 || d<1 || d>31)return 0;
    days=m==2 ? 28+((y%4==0 && y%100!=0) || y%400==0):
         (m==4 || m==6 || m==9 || m==11 ? 30:31);
    if(d>days)return 0;
    if(timestamp){if(s[10]!='T' || s[13]!=':' || !isdigit((unsigned char)s[11]) || !isdigit((unsigned char)s[12]) || !isdigit((unsigned char)s[14]) || !isdigit((unsigned char)s[15]))return 0;
        h=atoi(s+11);minute=atoi(s+14);if(h>23 || minute>59)return 0;}
    return 1;
}
const char *nd_weather_weekday(const char *ymd)
{
    static const char *names[]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
    static const int months[]={0,3,2,5,0,3,5,1,4,6,2,4};
    int y,m,d;
    if(!ymd || !date(ymd,0))return "--";
    y=atoi(ymd);m=atoi(ymd+5);d=atoi(ymd+8);if(m<3)--y;
    return names[(y+y/4-y/100+y/400+months[m-1]+d)%7];
}
int nd_weather_parse(const char *csv,struct NDWeather *weather)
{
    struct NDWeather parsed;char line[512],*fields[6],*p,*comma;const char *end,*next;unsigned int n,i;int phase=0,current=0;
    if(!csv || !weather || strlen(csv)>ND_WEATHER_RESPONSE)return 0;memset(&parsed,0,sizeof(parsed));
    while(*csv){end=strchr(csv,'\n');n=(unsigned int)(end ? end-csv:strlen(csv));if(n>=sizeof(line))return 0;
        next=end ? end+1:csv+n;
        memcpy(line,csv,n);line[n]=0;if(n && line[n-1]=='\r')line[--n]=0;csv=next;
        if(!n)continue;
        if(!strncmp(line,"time,temperature_2m (",21)){phase=1;continue;}
        if(!strncmp(line,"time,weather_code (",19)){phase=2;continue;}
        if(!phase)continue;
        p=line;i=0;do{if(i>=6)return 0;fields[i++]=p;comma=strchr(p,',');if(comma){*comma=0;p=comma+1;}}while(comma);
        if(phase==1){if(i!=3 || !date(fields[0],1) || !integer(fields[1],&parsed.temperature,1) || !integer(fields[2],&parsed.code,0) || parsed.code<0 || parsed.code>99)return 0;
            strcpy(parsed.updated,fields[0]);current=1;phase=0;}
        else{struct NDWeatherDay *day;if(i!=5 || parsed.count>=ND_WEATHER_DAYS)return 0;day=&parsed.days[parsed.count];
            if(!date(fields[0],0) || !integer(fields[1],&day->code,0) || !integer(fields[2],&day->high,1) || !integer(fields[3],&day->low,1) || !integer(fields[4],&day->rain,0) || day->code<0 || day->code>99 || day->rain<0 || day->rain>100 || day->low>day->high)return 0;
            if(parsed.count && strcmp(fields[0],parsed.days[parsed.count-1].date)<=0)return 0;
            strcpy(day->date,fields[0]);++parsed.count;}
    }
    if(!current || !parsed.count)return 0;*weather=parsed;return 1;
}
/* Skip a JSON value, enforcing bounded nesting and valid string escapes. */
static const char *space(const char *p){while(*p && isspace((unsigned char)*p))++p;return p;}
static const char *string_end(const char *p)
{
    int n;if(*p++!='"')return NULL;
    while(*p && *p!='"'){if((unsigned char)*p<32)return NULL;if(*p++=='\\'){
        if(!*p)return NULL;if(*p=='u'){++p;for(n=0;n<4;++n)if(!isxdigit((unsigned char)*p++))return NULL;}
        else{if(!strchr("\"\\/bfnrt",*p))return NULL;++p;}}}
    return *p=='"' ? p+1:NULL;
}
static const char *skip(const char *p,int depth)
{
    const char *end;char close;if(depth>16)return NULL;p=space(p);
    if(*p=='"')return string_end(p);
    if(*p=='{' || *p=='['){close=*p=='{' ? '}':']';++p;p=space(p);if(*p==close)return p+1;
        for(;;){if(close=='}'){end=string_end(p);if(!end)return NULL;p=space(end);if(*p++!=':')return NULL;}
            p=skip(p,depth+1);if(!p)return NULL;p=space(p);if(*p==close)return p+1;if(*p++!=',')return NULL;p=space(p);}}
    end=p;while(*p && !strchr(",]} \t\r\n",*p))++p;return p>end ? p:NULL;
}
static const char *member(const char *object,const char *key)
{
    const char *p=space(object),*end;size_t size=strlen(key);
    if(*p++!='{')return NULL;p=space(p);
    while(*p && *p!='}'){end=string_end(p);if(!end)return NULL;
        if((size_t)(end-p)==size+2 && !memcmp(p+1,key,size)){p=space(end);return *p==':' ? space(p+1):NULL;}
        p=space(end);if(*p++!=':')return NULL;p=skip(p,0);if(!p)return NULL;p=space(p);if(*p=='}')break;if(*p++!=',')return NULL;p=space(p);}
    return NULL;
}
static int coordinate(const char *p,char *out,unsigned int size,int limit)
{
    const char *end=p;unsigned int n;int value;
    while(*end && strchr("-+.0123456789",*end))++end;n=(unsigned int)(end-p);
    if(!n || n>=size || n>15)return 0;memcpy(out,p,n);out[n]=0;
    if(!integer(out,&value,1) || value < -limit*10 || value>limit*10)return 0;
    if(value==limit*10 || value==-limit*10){
        const char *dot=strchr(out,'.');if(dot)for(++dot;*dot;++dot)if(*dot!='0')return 0;
    }
    return !*end || strchr(",} \t\r\n",*end)!=NULL;
}
int nd_weather_geocode(const char *json,char *latitude,char *longitude,unsigned int size)
{
    const char *results,*object,*lat,*lon,*end;char a[20],b[20];
    if(!json || strlen(json)>ND_WEATHER_RESPONSE || !latitude || !longitude || size<2)return 0;
    end=skip(json,0);if(!end || *space(end))return 0;
    results=member(json,"results");if(!results || *results!='[')return 0;object=space(results+1);if(*object!='{')return 0;
    lat=member(object,"latitude");lon=member(object,"longitude");
    if(!lat || !lon || !coordinate(lat,a,sizeof(a),90) || !coordinate(lon,b,sizeof(b),180) || strlen(a)>=size || strlen(b)>=size)return 0;
    strcpy(latitude,a);strcpy(longitude,b);return 1;
}
int nd_weather_search_url(const char *location,char *url,unsigned int size)
{
    static const char hex[]="0123456789ABCDEF";char encoded[600],built[800];unsigned int i=0;unsigned char c;
    if(!location || !*location || strlen(location)>95)return 0;
    while((c=(unsigned char)*location++)!=0){unsigned char bytes[2];int n,j;if(c<32 || c==127)return 0;
        if(c<128 && (isalnum(c) || c=='-' || c=='_' || c=='.')){encoded[i++]=(char)c;continue;}
        if(c<128){bytes[0]=c;n=1;}else{bytes[0]=(unsigned char)(0xc0|(c>>6));bytes[1]=(unsigned char)(0x80|(c&63));n=2;}
        for(j=0;j<n;++j){encoded[i++]='%';encoded[i++]=hex[bytes[j]>>4];encoded[i++]=hex[bytes[j]&15];}}
    encoded[i]=0;sprintf(built,"https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1&language=en&format=json",encoded);
    if(strlen(built)>=size)return 0;strcpy(url,built);return 1;
}
int nd_weather_forecast_url(const char *latitude,const char *longitude,int fahrenheit,char *url,unsigned int size)
{
    char a[20],b[20],built[1000];
    if(!latitude || !longitude || !coordinate(latitude,a,sizeof(a),90) || !coordinate(longitude,b,sizeof(b),180))return 0;
    sprintf(built,"https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,weather_code&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max&timezone=auto&forecast_days=5&format=csv&temperature_unit=%s",a,b,fahrenheit ? "fahrenheit":"celsius");
    if(strlen(built)>=size)return 0;strcpy(url,built);return 1;
}
void nd_weather_temperature(int tenths,int fahrenheit,char *out)
{unsigned int v=(unsigned int)(tenths<0 ? -tenths:tenths);sprintf(out,"%s%u.%u %c",tenths<0 ? "-":"",v/10,v%10,fahrenheit ? 'F':'C');}
unsigned long nd_weather_key(const char *location,int fahrenheit)
{unsigned long h=2166136261UL;while(*location){h^=(unsigned char)*location++;h=(h*16777619UL)&0xffffffffUL;}return (h^(fahrenheit ? 1UL:0UL))&0xffffffffUL;}

const char *nd_weather_download_problem(const char *log)
{
    if(!log)return NULL;
    if(strstr(log,"Unsupported scheme") || strstr(log,"not supported or disabled in libcurl") || strstr(log,"Protocol \"https\" not supported"))
        return "Downloader has no HTTPS support; use TLS curl/Wget";
    if(strstr(log,"amissl") || strstr(log,"AmiSSL"))
        return "HTTPS library unavailable; check AmiSSL requirements";
    if(strstr(log,"certificate") || strstr(log,"Certificate") || strstr(log,"SSL peer"))
        return "HTTPS certificate check failed; check certs and date";
    if(strstr(log,"resolve") || strstr(log,"Name or service not known"))
        return "DNS lookup failed; check the network's name servers";
    if(strstr(log,"timed out") || strstr(log,"Timeout"))
        return "Weather download timed out";
    if(strstr(log,"SSL") || strstr(log,"TLS"))
        return "HTTPS connection failed; check downloader TLS support";
    return NULL;
}
