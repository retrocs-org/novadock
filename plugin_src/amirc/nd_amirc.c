/* Portable AmIRC settings and display policy; no IRC/network connection. */
#include "nd_amirc.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
static int safe_text(const char *s,unsigned int size)
{
    if (!s || strlen(s)>=size) return 0;
    while (*s) {if ((unsigned char)*s<32 || *s=='|' || *s==127) return 0;++s;}
    return 1;
}
void nd_amirc_defaults(struct NDAmIRCConfig *cfg)
{
    memset(cfg,0,sizeof(*cfg));cfg->poll_ticks=100;cfg->display=ND_AMIRC_NICK;
    strcpy(cfg->port,"AUTO");cfg->alerts=3;cfg->clear_on_focus=1;
}
int nd_amirc_apply_option(void *context,const char *key,const char *value)
{
    struct NDAmIRCConfig *cfg=(struct NDAmIRCConfig *)context;
    char *end;unsigned long ticks;
    if (!cfg || !key || !value) return 0;
    if (!strcmp(key,"PROGRAM") && safe_text(value,sizeof(cfg->program)))
        strcpy(cfg->program,value);
    else if (!strcmp(key,"PORT") && *value && safe_text(value,sizeof(cfg->port)))
        strcpy(cfg->port,value);
    else if (!strcmp(key,"DISPLAY")) {
        if (!strcmp(value,"STATUS")) cfg->display=ND_AMIRC_STATUS;
        else if (!strcmp(value,"NICK")) cfg->display=ND_AMIRC_NICK;
        else if (!strcmp(value,"CHANNEL")) cfg->display=ND_AMIRC_CHANNEL;
        else if (!strcmp(value,"SERVER")) cfg->display=ND_AMIRC_SERVER;
        else return 0;
    } else if (!strcmp(key,"ALERTS")) {
        if (!strcmp(value,"BOTH")) cfg->alerts=3;
        else if (!strcmp(value,"PRIVATE")) cfg->alerts=1;
        else if (!strcmp(value,"HIGHLIGHT")) cfg->alerts=2;
        else if (!strcmp(value,"OFF")) cfg->alerts=0;else return 0;
    } else if (!strcmp(key,"CLEAR_ON_FOCUS")) {
        if (!strcmp(value,"YES"))cfg->clear_on_focus=1;
        else if (!strcmp(value,"NO"))cfg->clear_on_focus=0;else return 0;
    } else if (!strcmp(key,"POLL_TICKS")) {
        ticks=strtoul(value,&end,10);
        if (!*value || *end || ticks<50 || ticks>3000) return 0;
        cfg->poll_ticks=(unsigned int)ticks;
    } else return 0;
    return 1;
}
/* MUI uses a numbered application port. Match the complete base or a numeric
 * suffix; never select AmIRC's DCC/helper ports or a similarly named app. */
int nd_amirc_port_name(const char *name)
{
    static const char base[]="AMIRC";
    unsigned int i;
    if (!name) return 0;
    for (i=0;i<5;++i)
        if (!name[i] || toupper((unsigned char)name[i])!=base[i]) return 0;
    if (!name[5]) return 1;
    if (name[5]!='.' || !isdigit((unsigned char)name[6])) return 0;
    for (i=6;name[i];++i) if (!isdigit((unsigned char)name[i])) return 0;
    return 1;
}
void nd_amirc_clean(char *out,unsigned int size,const char *in)
{
    unsigned int n=0;unsigned char c;
    if (!out || !size) return;
    if (!in) in="";
    while ((c=(unsigned char)*in++)!=0 && n+1<size) {
        if (c==27) { /* Drop ANSI controls from arbitrary ARexx result text. */
            if (*in=='[') {++in;while (*in && !(*in>='@' && *in<='~')) ++in;
                if (*in) ++in;}
            continue;
        }
        if (c>=32 && c!=127) out[n++]=(char)c;
    }
    while (n && out[n-1]==' ') --n;
    out[n]=0;
}
const char *nd_amirc_display_query(int display)
{
    if (display==ND_AMIRC_NICK) return "GETMYNICK";
    if (display==ND_AMIRC_CHANNEL) return "GETCHANNEL";
    if (display==ND_AMIRC_SERVER) return "GETSERVERNAME";
    return NULL;
}
void nd_amirc_status(char *caption,unsigned int cs,char *value,unsigned int vs,
                     int present,int connected,int away,int display,
                     const char *detail,int unresponsive)
{
    const char *status;
    nd_amirc_clean(caption,cs,away && connected>0 ? "AMIRC AWAY" : "AMIRC");
    if (!present) status="OFFLINE";
    else if (unresponsive) status="NO RESPONSE";
    else if (connected<0) status="CHECKING";
    else if (!connected) status="DISCONNECTED";
    else if (display!=ND_AMIRC_STATUS && detail && *detail) status=detail;
    else status=away ? "AWAY" : "CONNECTED";
    nd_amirc_clean(value,vs,status);
}
int nd_amirc_connection_indicator(int present,int connected,int unresponsive)
{
    return present && connected>0 && !unresponsive;
}
int nd_amirc_quote(char *out,unsigned int size,const char *path)
{
    unsigned int n=0;unsigned char c;
    if (!out || size<3 || !path || !*path) return 0;
    out[n++]='"';
    while ((c=(unsigned char)*path++)!=0) {
        if (c<32 || c==127 || n+4>=size) return 0;
        if (c=='*' || c=='"') out[n++]='*';
        out[n++]=(char)c;
    }
    out[n++]='"';out[n]=0;return 1;
}

/* The hook forwards only event kind and originating application port, never
 * message text. Ignore other clients and unrecognised commands. */
static int same_port(const char *a,const char *b)
{
    while(*a && *b){if(toupper((unsigned char)*a++)!=toupper((unsigned char)*b++))return 0;}
    return !*a && !*b;
}
int nd_amirc_alert_command(const char *command,const char *host,int enabled)
{
    char verb[16],kind[16],source[64],extra[2];int mask;
    if(!command || !host || !*host ||
       sscanf(command,"%15s %15s %63s %1s",verb,kind,source,extra)!=3 ||
       strcmp(verb,"NOTIFY") || !same_port(host,source))return 0;
    mask=!strcmp(kind,"PRIVATE") ? ND_AMIRC_ALERT_PRIVATE :
         !strcmp(kind,"HIGHLIGHT") ? ND_AMIRC_ALERT_HIGHLIGHT : 0;
    return (enabled&mask)!=0;
}
void nd_amirc_alert_add(struct NDAmIRCAlerts *alerts)
{if(alerts->count<10000U)++alerts->count;}
void nd_amirc_badge(char *out,unsigned int size,unsigned int count,int connected)
{
    char text[16];
    if(count>999U)strcpy(text,"999+");
    else if(count)sprintf(text,"%u",count);
    else strcpy(text,connected ? "*" : "");
    nd_amirc_clean(out,size,text);
}
