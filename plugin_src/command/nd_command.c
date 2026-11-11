/* Portable core of the Command-output tile; see nd_command.h. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_command.h"

void nd_cmd_defaults(struct NDCommandConfig *cfg)
{
    memset(cfg,0,sizeof(*cfg));
    strcpy(cfg->command,"Version");
    cfg->interval=60;
    cfg->line=1;
    cfg->timeout=30;
    cfg->meter=ND_CMD_METER_NONE;
    cfg->click=ND_CMD_CLICK_RERUN;
}
static int unsigned_value(const char *value,unsigned long lo,unsigned long hi,
                          unsigned int *out)
{
    char *end;
    unsigned long n;
    if (!value || !isdigit((unsigned char)*value)) return 0;
    n=strtoul(value,&end,10);
    if (*end || n<lo || n>hi) return 0;
    *out=(unsigned int)n;
    return 1;
}
static int safe_command(const char *value)
{
    const unsigned char *p;
    for (p=(const unsigned char *)value;*p;++p) if (*p<32) return 0;
    return 1;
}
int nd_cmd_apply_option(void *ctx,const char *key,const char *value)
{
    struct NDCommandConfig *cfg=(struct NDCommandConfig *)ctx;
    if (!cfg || !key || !value) return 0;
    if (!strcmp(key,"COMMAND")) {
        if (!*value || strlen(value)>=sizeof(cfg->command) || !safe_command(value))
            return 0;
        strcpy(cfg->command,value);return 1;
    }
    if (!strcmp(key,"CLICK_COMMAND")) {
        if (strlen(value)>=sizeof(cfg->click_command) || !safe_command(value))
            return 0;
        strcpy(cfg->click_command,value);return 1;
    }
    if (!strcmp(key,"CAPTION")) {
        if (strlen(value)>=sizeof(cfg->caption) || !safe_command(value)) return 0;
        strcpy(cfg->caption,value);return 1;
    }
    if (!strcmp(key,"INTERVAL")) return unsigned_value(value,1,86400,&cfg->interval);
    if (!strcmp(key,"LINE")) return unsigned_value(value,1,20,&cfg->line);
    if (!strcmp(key,"TIMEOUT")) return unsigned_value(value,1,600,&cfg->timeout);
    if (!strcmp(key,"METER")) {
        if (!strcmp(value,"NONE")) cfg->meter=ND_CMD_METER_NONE;
        else if (!strcmp(value,"PERCENT")) cfg->meter=ND_CMD_METER_PERCENT;
        else return 0;
        return 1;
    }
    if (!strcmp(key,"CLICK")) {
        if (!strcmp(value,"RERUN")) cfg->click=ND_CMD_CLICK_RERUN;
        else if (!strcmp(value,"NONE")) cfg->click=ND_CMD_CLICK_NONE;
        else if (!strcmp(value,"COMMAND")) cfg->click=ND_CMD_CLICK_COMMAND;
        else return 0;
        return 1;
    }
    return 1; /* unknown keys: written by a newer version, ignore */
}
int nd_cmd_file_names(const char *id,char *out_file,char *done_file,
                      unsigned int size)
{
    const char *p;
    if (!id || !*id || strlen(id)+24>size) return 0;
    for (p=id;*p;++p)
        if (!isalnum((unsigned char)*p) && *p!='_' && *p!='-') return 0;
    sprintf(out_file,"T:NovaDockCmd-%s.out",id);
    sprintf(done_file,"T:NovaDockCmd-%s.done",id);
    return 1;
}
int nd_cmd_build_script(const char *command,const char *done_file,
                        char *script,unsigned int size)
{
    if (!command || !*command || !done_file || !script || !safe_command(command) ||
        strchr(done_file,'"') ||
        strlen(command)+strlen(done_file)+64>size) return 0;
    /* FailAt keeps the final Echo running even when the command fails, so
     * the plugin always learns that the command finished. */
    sprintf(script,"FailAt 2147483647\nStack 16384\n%s\nEcho >\"%s\" \"$RC\"",
            command,done_file);
    return 1;
}
/* Skip a console control sequence starting at P; returns the next byte. */
static const char *skip_escape(const char *p)
{
    unsigned char c=(unsigned char)*p;
    if (c==0x9b || (c==0x1b && p[1]=='[')) {
        p+=c==0x9b ? 1 : 2;
        while (*p && !(isalpha((unsigned char)*p) || *p=='~' || *p=='@')) ++p;
        return *p ? p+1 : p;
    }
    return p+1;
}
int nd_cmd_extract(const char *output,unsigned int line,int meter_mode,
                   char *value,unsigned int size,unsigned int *meter)
{
    const char *p,*start;
    unsigned int current=0,n=0;
    int space=0;
    if (meter) *meter=65535U;
    if (!value || size<2) return 0;
    value[0]='\0';
    if (!output || !line) return 0;
    p=output;
    while (*p) {
        /* Find the next line and test whether it has visible content. */
        const char *q;
        int visible=0;
        start=p;
        for (q=p;*q && *q!='\n';) {
            unsigned char c=(unsigned char)*q;
            if (c==0x9b || c==0x1b) { q=skip_escape(q); continue; }
            if (c>32 && c!=127) visible=1;
            ++q;
        }
        p=*q ? q+1 : q;
        if (!visible || ++current!=line) continue;
        for (q=start;*q && *q!='\n';) {
            unsigned char c=(unsigned char)*q;
            if (c==0x9b || c==0x1b) { q=skip_escape(q); continue; }
            ++q;
            if (c<32 || c==127 || c==' ') {
                if (n) space=1;
                continue;
            }
            if (space && n+1<size) value[n++]=' ';
            space=0;
            if (n+1<size) value[n++]=(char)c;
        }
        value[n]='\0';
        if (meter_mode==ND_CMD_METER_PERCENT && meter) {
            const char *d=value;
            while (*d && !isdigit((unsigned char)*d)) ++d;
            if (*d) {
                unsigned long v=strtoul(d,NULL,10);
                *meter=(unsigned int)(v>100 ? 100 : v);
            }
        }
        return 1;
    }
    return 0;
}
long nd_cmd_parse_rc(const char *text)
{
    char *end;
    long rc;
    if (!text) return -1;
    while (*text==' ' || *text=='"') ++text;
    if (!isdigit((unsigned char)*text) && *text!='-') return -1;
    rc=strtol(text,&end,10);
    return end==text ? -1 : rc;
}
