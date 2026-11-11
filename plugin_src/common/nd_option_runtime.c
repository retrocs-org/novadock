#include "nd_option_runtime.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
#include <dos/dos.h>
#include <proto/dos.h>
/* PROGDIR: only exists for a process with a home directory (not for a
 * resident or hand-launched program); touching it otherwise raises a
 * "Please insert volume PROGDIR" requester. */
#define ND_HAVE_PROGDIR() (GetProgramDir()!=0)
#else
#define ND_HAVE_PROGDIR() 1
#endif
/* Runtime reader does not allocate a full NDConfig on small 68k stacks. */
static char *strip(char *s)
{
    char *end;
    while (*s==' ' || *s=='\t') ++s;
    end=s+strlen(s);
    while (end>s && (end[-1]=='\r' || end[-1]=='\n' || end[-1]==' ' || end[-1]=='\t')) --end;
    *end=0;
    return s;
}
/* The dock's configuration lines may be up to ND_RUNTIME_LINE_LEN-1 bytes
 * (launcher lines carry several full paths).  The buffer is static so the
 * small default stack of a plugin process is not consumed, and any line that
 * is still too long is skipped rather than aborting the whole read: one long
 * launcher line must never discard every plugin's settings. */
#define ND_RUNTIME_LINE_LEN 1280
int nd_plugin_options_read(const char *path, const char *id,
                           NDOptionApply apply, void *ctx)
{
    static char buffer[ND_RUNTIME_LINE_LEN];
    FILE *f; char prefix[44]; int len, ok=1, ch;
    if (!path || !id || !apply || strlen(id)>19) return -1;
    sprintf(prefix,"OPTION.%s.",id); len=(int)strlen(prefix);
    f=fopen(path,"r"); if (!f) return -1;
    while (fgets(buffer,sizeof(buffer),f)) {
        char *line, *eq;
        const char *value;
        if (!strchr(buffer,'\n') && !feof(f)) {
            /* Over-long line: discard its remainder and carry on. */
            while ((ch=fgetc(f))!='\n' && ch!=EOF) {}
            continue;
        }
        line=strip(buffer);
        if (!strncmp(line,prefix,(size_t)len)) {
            eq=strchr(line+len,'=');
            if (!eq) { ok=0; break; }
            *eq++=0;
            value=strip(eq);
            if (!strcmp(line+len,"SCRIPT") &&
                !strcmp(value,"SYS:Utilities/NovaDock/Rexx/ScriptTile.rexx"))
                value="NovaDock:Rexx/ScriptTile.rexx";
            if (!apply(ctx,line+len,value)) {ok=0;break;}
        }
    }
    if (ferror(f)) ok=0;
    fclose(f);
    return ok;
}
/* NovaDock publishes the file it actually loaded (which may be a recovered
 * .bak generation) in ND_CONFIG_SOURCE_FILE.  Plugins follow it so they never
 * read options from a file the dock itself rejected. */
static int published_source(char *path,unsigned int size)
{
    FILE *f=fopen(ND_CONFIG_SOURCE_FILE,"r");
    char *end;
    if (!f) return 0;
    if (!fgets(path,(int)size,f)) {fclose(f);return 0;}
    fclose(f);
    end=strchr(path,'\n'); if (end) *end='\0';
    end=strchr(path,'\r'); if (end) *end='\0';
    return path[0]!='\0' && strchr(path,':')!=NULL;
}
int nd_plugin_options_effective(const char *id, NDOptionApply apply, void *ctx)
{
    char source[128];
    int status;
    if (published_source(source,sizeof(source))) {
        status=nd_plugin_options_read(source,id,apply,ctx);
        if (status>=0) return status;
    }
    status=nd_plugin_options_read("ENV:NovaDock/novadock.cfg",id,apply,ctx);
    if (status>=0) return status;
    status=nd_plugin_options_read("ENVARC:NovaDock/novadock.cfg",id,apply,ctx);
    if (status>=0) return status;
    status=nd_plugin_options_read("SYS:NovaDock/novadock.cfg.default",id,apply,ctx);
    if (status>=0) return status;
    if (!ND_HAVE_PROGDIR()) return -1;
    return nd_plugin_options_read("PROGDIR:/novadock.cfg",id,apply,ctx);
}
struct StringOption {const char *key; char *out; unsigned int limit; int found;};
static int string_option(void *ptr,const char *key,const char *value)
{
    struct StringOption *opt=(struct StringOption *)ptr;
    if (!strcmp(opt->key,key)) {
        if (strlen(value)>=opt->limit) return 0;
        strcpy(opt->out,value);opt->found=1;
    }
    return 1;
}
int nd_plugin_option_string(const char *id,const char *key,
                            const char *fallback,char *out,unsigned int limit)
{
    struct StringOption opt; int result;
    if (!out || !limit || !fallback || strlen(fallback)>=limit) return 0;
    strcpy(out,fallback); opt.key=key; opt.out=out;opt.limit=limit;opt.found=0;
    result=nd_plugin_options_effective(id,string_option,&opt);
    return result>=0;
}

struct StringOptions {struct NDOptionString *items;unsigned int count;};
static int apply_strings(void *context,const char *key,const char *value)
{
    struct StringOptions *s=(struct StringOptions *)context;
    unsigned int i;
    for(i=0;i<s->count;++i) if(!strcmp(s->items[i].key,key)) {
        if(strlen(value)>=s->items[i].limit)return 0;
        strcpy(s->items[i].out,value);
    }
    return 1;
}
int nd_plugin_option_strings(const char *id,struct NDOptionString *options,unsigned int count)
{
    struct StringOptions context;
    unsigned int i;int status;
    if(!options || !count)return 0;
    for(i=0;i<count;++i) if(!options[i].key || !options[i].out ||
        !options[i].fallback || strlen(options[i].fallback)>=options[i].limit)return 0;
    for(i=0;i<count;++i)strcpy(options[i].out,options[i].fallback);
    context.items=options;context.count=count;
    status=nd_plugin_options_effective(id,apply_strings,&context);
    if(status!=1)for(i=0;i<count;++i)strcpy(options[i].out,options[i].fallback);
    return status==1;
}
