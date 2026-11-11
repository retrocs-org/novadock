/* Portable, deliberately small input validator for untrusted drag paths
 * and the dock's ARexx command line. No AmigaOS headers required. */
#include "nd_interaction.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int number(const char **cursor, int *value)
{
    const char *p = *cursor;
    long result = 0;
    while (*p == ' ' || *p == '\t') ++p;
    if (!isdigit((unsigned char)*p)) return 0;
    while (isdigit((unsigned char)*p)) {
        result = 10 * result + (*p++ - '0');
        if (result > 999) return 0;
    }
    if (result < 1) return 0;
    *value = (int)result;
    *cursor = p;
    return 1;
}
/* Read one argument: either a double-quoted string (no embedded quotes) or
 * a run of non-blank characters. */
static int word(const char **cursor,char *out,size_t size)
{
    const char *p=*cursor;
    size_t n=0;
    while (*p==' ' || *p=='\t') ++p;
    if (!*p) return 0;
    if (*p=='"') {
        ++p;
        while (*p && *p!='"') {
            if ((unsigned char)*p<32 || n+1>=size) return 0;
            out[n++]=*p++;
        }
        if (*p!='"') return 0;
        ++p;
    } else {
        while (*p && *p!=' ' && *p!='\t') {
            if ((unsigned char)*p<32 || *p=='"' || n+1>=size) return 0;
            out[n++]=*p++;
        }
    }
    out[n]='\0';
    *cursor=p;
    return 1;
}
/* The remainder of the line as one argument, optionally quoted. */
static int rest(const char *p,char *out,size_t size)
{
    size_t n;
    while (*p==' ' || *p=='\t') ++p;
    if (*p=='"') {
        if (!word(&p,out,size)) return 0;
        while (*p==' ' || *p=='\t') ++p;
        return *p=='\0' && *out;
    }
    n=strlen(p);
    while (n && (p[n-1]==' ' || p[n-1]=='\t')) --n;
    if (!n || n>=size) return 0;
    memcpy(out,p,n);out[n]='\0';
    for (p=out;*p;++p) if ((unsigned char)*p<32) return 0;
    return 1;
}
static int at_end(const char *p)
{
    while (*p==' ' || *p=='\t') ++p;
    return *p=='\0';
}
int nd_parse_rexx(const char *text, struct NDRexxCommand *cmd)
{
    char verb[24];
    int k=0, i;
    const char *p;
    if (!text || !cmd) return 0;
    memset(cmd,0,sizeof(*cmd));
    cmd->action=ND_RX_INVALID; cmd->meter=-1;
    p=text;
    while (*p == ' ' || *p == '\t') ++p;
    while (isalpha((unsigned char)*p)) {
        if (k >= 23) return 0;
        verb[k++]=(char)toupper((unsigned char)*p++);
    }
    verb[k]='\0';
    if (*p && *p!=' ' && *p!='\t') return 0;
    while (*p == ' ' || *p == '\t') ++p;
    if (!strcmp(verb,"OPEN")) {
        const char *q=p;
        if (number(&q,&cmd->first) && at_end(q)) {
            cmd->action=ND_RX_OPEN; return 1;
        }
        /* Labels never start like a number, so "OPEN -4" stays invalid. */
        if (isdigit((unsigned char)*p) || *p=='-' || *p=='+') return 0;
        cmd->first=0;
        if (!rest(p,cmd->text,sizeof(cmd->text))) return 0;
        cmd->action=ND_RX_OPEN;
        return 1;
    }
    if (!strcmp(verb,"MOVE") || !strcmp(verb,"CLICK") ||
        !strcmp(verb,"REMOVE")) {
        i=!strcmp(verb,"MOVE") ? ND_RX_MOVE :
          !strcmp(verb,"CLICK") ? ND_RX_CLICK : ND_RX_REMOVE;
        if (!number(&p,&cmd->first)) return 0;
        if (i==ND_RX_MOVE && !number(&p,&cmd->second)) return 0;
        if (!at_end(p)) return 0;
        cmd->action=i;
        return 1;
    }
    if (!strcmp(verb,"ADD")) {
        if (!rest(p,cmd->text,sizeof(cmd->text)) || !strchr(cmd->text,':'))
            return 0;
        cmd->action=ND_RX_ADD;
        return 1;
    }
    if (!strcmp(verb,"SETVALUE")) {
        char meter[8];
        const char *q;
        if (!word(&p,cmd->id,sizeof(cmd->id))) return 0;
        for (q=cmd->id;*q;++q)
            if (!isalnum((unsigned char)*q) && *q!='_' && *q!='-') return 0;
        if (!word(&p,cmd->text,32) || !word(&p,cmd->text2,32)) return 0;
        if (!at_end(p)) {
            if (!word(&p,meter,sizeof(meter)) || !at_end(p)) return 0;
            q=meter;
            if (!isdigit((unsigned char)*q)) return 0;
            cmd->meter=0;
            while (isdigit((unsigned char)*q)) {
                cmd->meter=cmd->meter*10+(*q++-'0');
                if (cmd->meter>100) return 0;
            }
            if (*q) return 0;
        }
        cmd->action=ND_RX_SETVALUE;
        return 1;
    }
    if (!strcmp(verb,"SETBADGE")) {
        const char *q;
        if (!word(&p,cmd->id,sizeof(cmd->id))) return 0;
        for (q=cmd->id;*q;++q)
            if (!isalnum((unsigned char)*q) && *q!='_' && *q!='-') return 0;
        /* No text clears the badge; "*" shows a dot. */
        if (!at_end(p) &&
            (!word(&p,cmd->text,ND_RX_BADGE_LEN) || !at_end(p))) return 0;
        cmd->action=ND_RX_SETBADGE;
        return 1;
    }
    if (!strcmp(verb,"LIST")) {
        char key[8];
        size_t n;
        cmd->action=ND_RX_LIST;
        if (at_end(p)) return 1;
        /* LIST STEM name: fill an ARexx stem instead of one result string. */
        if (!word(&p,key,sizeof(key))) return 0;
        for (n=0;key[n];++n) key[n]=(char)toupper((unsigned char)key[n]);
        if (strcmp(key,"STEM") || !word(&p,cmd->text,ND_RX_STEM_LEN) ||
            !at_end(p) || !isalpha((unsigned char)cmd->text[0])) return 0;
        for (n=0;cmd->text[n];++n) {
            unsigned char c=(unsigned char)cmd->text[n];
            if (!isalnum(c) && c!='_' && c!='.' && c!='!' && c!='?') return 0;
            cmd->text[n]=(char)toupper(c);
        }
        if (cmd->text[n-1]!='.') {
            if (n+1>=ND_RX_STEM_LEN) return 0;
            cmd->text[n]='.';cmd->text[n+1]='\0';
        }
        return 1;
    }
    if (*p) return 0;
    if (!strcmp(verb,"PING")) cmd->action=ND_RX_PING;
    else if (!strcmp(verb,"VERSION")) cmd->action=ND_RX_VERSION;
    else if (!strcmp(verb,"STATUS")) cmd->action=ND_RX_STATUS;
    else if (!strcmp(verb,"RELOAD")) cmd->action=ND_RX_RELOAD;
    else if (!strcmp(verb,"PREFS")) cmd->action=ND_RX_PREFS;
    else if (!strcmp(verb,"QUIT")) cmd->action=ND_RX_QUIT;
    else if (!strcmp(verb,"HELP")) cmd->action=ND_RX_HELP;
    else if (!strcmp(verb,"SHOW")) cmd->action=ND_RX_SHOW;
    else if (!strcmp(verb,"HIDE")) cmd->action=ND_RX_HIDE;
    else if (!strcmp(verb,"TOGGLE")) cmd->action=ND_RX_TOGGLE;
    else if (!strcmp(verb,"LOCK")) cmd->action=ND_RX_LOCK;
    else if (!strcmp(verb,"UNLOCK")) cmd->action=ND_RX_UNLOCK;
    else if (!strcmp(verb,"SUSPEND")) cmd->action=ND_RX_SUSPEND;
    else if (!strcmp(verb,"RESUME")) cmd->action=ND_RX_RESUME;
    return cmd->action != ND_RX_INVALID;
}
int nd_quote_path(char *dest,size_t limit,const char *path)
{
    size_t n=0;
    const unsigned char *p=(const unsigned char *)path;
    if (!dest || limit<3 || !path) return 0;
    dest[n++]='"';
    for (;*p;++p) {
        if (*p<32 || *p==127) return 0;
        if (*p=='*' || *p=='"') {
            if (n+2>=limit) return 0;
            dest[n++]='*';
        }
        if (n+1>=limit) return 0;
        dest[n++]=(char)*p;
    }
    if (n+2>limit) return 0;
    dest[n++]='"';dest[n]='\0';
    return 1;
}
int nd_join_path(char *dest,size_t limit,const char *dir,const char *name)
{
    size_t a,b;
    int sep;
    if (!dest || !dir || !name || !*dir || !*name) return 0;
    a=strlen(dir);b=strlen(name);
    sep=dir[a-1]!=':' && dir[a-1]!='/';
    if (a+b+(size_t)sep+1>limit) return 0;
    strcpy(dest,dir);
    if (sep) strcat(dest,"/");
    strcat(dest,name);
    return 1;
}
int nd_build_copy_command(char *dest,size_t limit,const char *source,
                          int source_is_drawer,const char *drawer)
{
    char target[300],qsource[520],qtarget[620];
    const char *name,*p;
    size_t length;
    if (!dest || !source || !drawer || !*source || !*drawer) return 0;
    length=strlen(source);
    if (source[length-1]==':') return 0; /* never copy a whole volume */
    name=source;
    for (p=source;*p;++p) if (*p==':' || *p=='/') name=p+1;
    if (!*name) return 0;
    if (source_is_drawer) {
        if (!nd_join_path(target,sizeof(target),drawer,name)) return 0;
    } else {
        if (strlen(drawer)>=sizeof(target)) return 0;
        strcpy(target,drawer);
    }
    if (!strcmp(target,source)) return 0;
    if (!nd_quote_path(qsource,sizeof(qsource),source) ||
        !nd_quote_path(qtarget,sizeof(qtarget),target)) return 0;
    if (strlen(qsource)+strlen(qtarget)+32>limit) return 0;
    sprintf(dest,"C:Copy %s TO %s%s CLONE QUIET",qsource,qtarget,
            source_is_drawer ? " ALL" : "");
    return 1;
}
int nd_command_name_matches(const char *command_name,const char *wanted)
{
    const char *base,*p;
    if (!command_name || !wanted || !*command_name || !*wanted) return 0;
    base=command_name;
    for (p=command_name;*p;++p) if (*p==':' || *p=='/') base=p+1;
    while (*base && *wanted) {
        if (tolower((unsigned char)*base)!=tolower((unsigned char)*wanted))
            return 0;
        ++base;++wanted;
    }
    return !*base && !*wanted;
}
static int safe_path(const char *s)
{
    int k=0;
    if (!s || !*s || *s=='@') return 0;
    while (*s) {
        unsigned char ch=(unsigned char)*s++;
        if (++k > 220 || ch < 32 || ch > 126 ||
            strchr("\"*`$|;<>\\(){}[]",ch) != NULL) return 0;
    }
    return 1;
}
int nd_build_openwith(char *dest, size_t limit, const char *tool,
                      const char *file)
{
    return nd_build_openwith_action(dest,limit,tool,"",file);
}
int nd_build_openwith_action(char *dest,size_t limit,const char *tool,
                             const char *arguments,const char *file)
{
    size_t a,b,c=0;
    const unsigned char *p;
    if (!dest || !limit) return 0;
    dest[0]='\0';
    if (!safe_path(tool) || !safe_path(file)) return 0;
    a=strlen(tool); b=strlen(file);
    if (arguments && *arguments) {
        p=(const unsigned char *)arguments;
        while (*p) {
            if (*p<' ' || *p>126) return 0;
            ++p;++c;
        }
        if (a+b+c+7>limit) return 0;
        sprintf(dest,"\"%s\" %s \"%s\"",tool,arguments,file);
    } else {
        if (a+b+6>limit) return 0;
        sprintf(dest,"\"%s\" \"%s\"",tool,file);
    }
    return 1;
}
int nd_drop_opens_item(int position,int extent)
{
    int edge;
    if (extent<5 || position<0 || position>=extent) return 0;
    edge=extent/5;
    if (edge<1) edge=1;
    return position>=edge && position<extent-edge;
}
int nd_drop_after_item(int position,int extent)
{
    if (extent<1 || position<0) return 0;
    if (position>=extent) position=extent-1;
    return position>=extent/2;
}
int nd_drop_destination(int from,int target,int after)
{
    if (from<0 || target<0) return -1;
    if (from==target) return from;
    if (after) return from>target ? target+1 : target;
    return from<target ? target-1 : target;
}
int nd_right_button_down(unsigned int event_class,unsigned int event_code)
{
    /* devices/inputevent.h: IECLASS_RAWMOUSE=2, IECODE_RBUTTON=0x69.
     * The release event has IECODE_UP_PREFIX (0x80) set and therefore fails
     * this exact comparison, as do IECODE_NOBUTTON pointer-motion reports. */
    return event_class==2U && event_code==0x69U;
}
static int same_ascii_nocase(const char *a,const char *b,size_t length)
{
    size_t i;
    for (i=0;i<length;++i)
        if (tolower((unsigned char)a[i])!=tolower((unsigned char)b[i]))
            return 0;
    return 1;
}
int nd_text_matches_pattern(const char *pattern,const char *text)
{
    const char *star=NULL,*retry=NULL;
    if (!pattern || !*pattern || !text) return 0;
    while (*text) {
        if (*pattern=='?' || (*pattern &&
            tolower((unsigned char)*pattern)==tolower((unsigned char)*text))) {
            ++pattern;++text;
        } else if (*pattern=='*') {
            star=pattern++;retry=text;
        } else if (star) {
            pattern=star+1;text=++retry;
        } else return 0;
    }
    while (*pattern=='*') ++pattern;
    return *pattern=='\0';
}
static int shell_title(const char *title,const char *name)
{
    size_t length;
    char next;
    if (!title || !name) return 0;
    length=strlen(name);
    if (strlen(title)<length || !same_ascii_nocase(title,name,length)) return 0;
    next=title[length];
    return next=='\0' || next==' ' || next=='\t' || next==':' || next=='-';
}
int nd_launcher_matches_shell_window(const char *command,const char *title)
{
    const char *p,*end,*base;
    size_t length;
    if (!command || !title) return 0;
    p=command;
    while (*p==' ' || *p=='\t' || *p=='\"') ++p;
    end=p;
    while (*end && *end!=' ' && *end!='\t' && *end!='\"') ++end;
    base=p;
    while (p<end) {
        if (*p=='/' || *p==':') base=p+1;
        ++p;
    }
    length=(size_t)(end-base);
    if (!((length==5 && same_ascii_nocase(base,"Shell",5)) ||
          (length==8 && same_ascii_nocase(base,"NewShell",8)))) return 0;
    return shell_title(title,"AmigaShell") || shell_title(title,"Shell");
}
int nd_hook_command(char *dest,size_t limit,const char *command,
                    const char *event,const char *detail)
{
    size_t n;
    if (!dest || !command || !*command || !event || !*event) return 0;
    if (!detail) detail="";
    n=strlen(command)+strlen(event)+2;
    if (n>=limit) return 0;
    strcpy(dest,command);strcat(dest," ");strcat(dest,event);strcat(dest," ");
    /* The detail is a volume or plugin name: always one quoted argument. */
    if (strchr(detail,'"') || strchr(detail,'\n') ||
        n+strlen(detail)+3>limit) return 0;
    strcat(dest,"\"");strcat(dest,detail);strcat(dest,"\"");
    return 1;
}
