/* Portable Script tile helpers (see nd_script.h). */
#include "nd_script.h"
#include <stdlib.h>
#include <string.h>

static void copy_field(char *out,unsigned int size,const char *from,
                       const char *to)
{
    unsigned int n=0;
    while (from<to && n+1<size) {
        unsigned char c=(unsigned char)*from++;
        out[n++]=(char)(c<32 || c==127 ? ' ' : c);
    }
    out[n]='\0';
}
int nd_script_parse(const char *result,struct NDScriptResult *out)
{
    const char *start[5],*end[5],*p;
    int fields=1;
    if (!result || !out) return 0;
    memset(out,0,sizeof(*out));
    out->meter=ND_SCRIPT_NO_METER;
    start[0]=result;
    for (p=result;*p;++p)
        if (*p=='|') {
            if (fields==4) return 0;
            end[fields-1]=p;start[fields++]=p+1;
        }
    end[fields-1]=p;
    if (fields==1) {copy_field(out->value,sizeof(out->value),start[0],end[0]);return 1;}
    copy_field(out->caption,sizeof(out->caption),start[0],end[0]);
    out->has_caption=out->caption[0]!='\0';
    copy_field(out->value,sizeof(out->value),start[1],end[1]);
    if (fields>=3) {
        const char *from=start[2],*to=end[2];
        unsigned int value=0,digits=0;
        while (from<to && *from==' ') ++from;
        while (to>from && to[-1]==' ') --to;
        if (from<to && !(to-from==1 && *from=='-')) {
            while (from<to && *from>='0' && *from<='9') {
                value=value*10U+(unsigned int)(*from++-'0');
                ++digits;
                if (value>100U) return 0;
            }
            if (!digits) return 0;
            while (from<to && *from==' ') ++from;
            if (from<to && *from=='%') ++from;
            while (from<to && *from==' ') ++from;
            if (from!=to) return 0;
            out->meter=value;out->has_meter=1;
        }
    }
    if (fields==4) {
        copy_field(out->badge,sizeof(out->badge),start[3],end[3]);
        out->has_badge=1;
    }
    return 1;
}
int nd_script_command(char *out,unsigned int size,const char *script,
                      const char *verb,const char *id)
{
    int quote;
    size_t need;
    if (!out || !script || !*script || !verb || !id || strchr(script,'"'))
        return 0;
    quote=strchr(script,' ')!=NULL;
    need=strlen(script)+strlen(verb)+strlen(id)+3+(quote ? 2 : 0);
    if (need>=size) return 0;
    out[0]='\0';
    if (quote) strcat(out,"\"");
    strcat(out,script);
    if (quote) strcat(out,"\"");
    strcat(out," ");strcat(out,verb);strcat(out," ");strcat(out,id);
    return 1;
}

unsigned int nd_script_wait_ticks(int running,unsigned int since,
                                   unsigned int interval,int immediate)
{
    unsigned int wait;
    if(immediate)return 1U;
    if(running)return 25U;
    wait=since<interval ? interval-since : 1U;
    return wait>250U ? 250U : wait;
}
