/* Local dock status only. AmiRocket owns all accounts and server traffic. */
#include "nd_amirocket.h"
#include <string.h>
#include <stdio.h>
static int port_name(const char *name)
{
    unsigned int i;
    if(!name || strncmp(name,"AMIROCKET",9))return 0;
    if(!name[9])return 1;
    if(name[9]!='.' || !name[10])return 0;
    for(i=10;name[i];++i)if(name[i]<'0' || name[i]>'9')return 0;
    return i<64;
}
int nd_rocket_port(const char *name){return port_name(name);}
static int text_ok(const char *s,unsigned int cap)
{if(!s || strlen(s)>=cap)return 0;while(*s){if((unsigned char)*s<32 || *s==127 || *s=='|')return 0;++s;}return 1;}
void nd_rocket_defaults(struct NDRocketConfig *cfg)
{memset(cfg,0,sizeof(*cfg));strcpy(cfg->program,"SYS:Utilities/AmiRocket/AmiRocket");strcpy(cfg->port,"AUTO");cfg->badge=1;}
int nd_rocket_option(void *context,const char *key,const char *value)
{
    struct NDRocketConfig *cfg=(struct NDRocketConfig *)context;
    if(!cfg || !key || !value)return 0;
    if(!strcmp(key,"PROGRAM") && text_ok(value,sizeof(cfg->program)))strcpy(cfg->program,value);
    else if(!strcmp(key,"PORT") && (!strcmp(value,"AUTO") || port_name(value)))strcpy(cfg->port,value);
    else if(!strcmp(key,"BADGE") && (!strcmp(value,"YES") || !strcmp(value,"NO")))cfg->badge=!strcmp(value,"YES");
    else return 0;
    return 1;
}
static int field(const char *p,const char *key,char *out,unsigned int cap)
{
    unsigned int n=(unsigned int)strlen(key),len;const char *end;int found=0;
    while(*p && *p!='\n' && *p!='\r'){
        while(*p==' ' || *p=='\t')++p;
        end=p;while(*end && *end!=' ' && *end!='\t' && *end!='\n' && *end!='\r')++end;
        if((unsigned int)(end-p)>n && !strncmp(p,key,n) && p[n]=='='){
            len=(unsigned int)(end-p)-n-1;if(found || len>=cap)return 0;
            memcpy(out,p+n+1,len);out[len]=0;found=1;
        }
        p=end;
    }
    return found;
}
static int number(const char *p,const char *key,unsigned long *out)
{
    char value[16];unsigned long n=0,d;unsigned int i;
    if(!field(p,key,value,sizeof(value)) || !*value)return 0;
    for(i=0;value[i];++i){if(value[i]<'0' || value[i]>'9')return 0;d=(unsigned long)(value[i]-'0');if(n>(0xffffffffUL-d)/10UL)return 0;n=n*10UL+d;}
    *out=n;return 1;
}
int nd_rocket_parse(struct NDRocketStatus *out,const char *snapshot)
{
    struct NDRocketStatus s;unsigned long version,connected,iconified;
    memset(&s,0,sizeof(s));
    if(!out || !snapshot || !number(snapshot,"version",&version) || version!=2 ||
       !number(snapshot,"connected",&connected) || connected>1 ||
       !number(snapshot,"iconified",&iconified) || iconified>1 ||
       !number(snapshot,"unread",&s.unread) || !number(snapshot,"alerts",&s.alerts) ||
       !number(snapshot,"servers",&s.servers) || s.servers>8 ||
       !number(snapshot,"connected_servers",&s.connected_servers) || s.connected_servers>s.servers ||
       connected!=(s.connected_servers!=0) ||
       !field(snapshot,"epoch",s.epoch,sizeof(s.epoch)) || !s.epoch[0])return 0;
    s.connected=(int)connected;s.iconified=(int)iconified;*out=s;return 1;
}
void nd_rocket_badge(char *out,unsigned long unread,int enabled,int fresh)
{if(!enabled || !fresh || !unread)*out=0;else if(unread>999UL)strcpy(out,"999+");else sprintf(out,"%lu",unread);}
