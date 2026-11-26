/* Support settings are separate from dock layouts and Preferences previews. */
#include "nd_support.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
#include <proto/dos.h>
#endif
static char *trim_support(char *s)
{char *end;while(isspace((unsigned char)*s))++s;end=s+strlen(s);while(end>s&&isspace((unsigned char)end[-1]))--end;*end=0;return s;}
int nd_support_read(const char *path,struct NDSupportSettings *settings)
{
    FILE *f;char line[128],*key,*value,*eq;struct NDSupportSettings next;int fields=0,ok=1;
    f=fopen(path,"r");if(!f)return 0;next.level=0;next.persist=0;
    while(fgets(line,sizeof(line),f)){
        if(!strchr(line,'\n')&&!feof(f)){ok=0;break;}
        key=trim_support(line);if(!*key||*key=='#'||*key==';')continue;
        eq=strchr(key,'=');if(!eq){ok=0;break;}*eq=0;value=trim_support(eq+1);key=trim_support(key);
        if(!strcmp(key,"MODE")){
            if(!strcmp(value,"OFF"))next.level=0;
            else if(!strcmp(value,"LOG"))next.level=1;
            else if(!strcmp(value,"TRACE"))next.level=2;
            else {ok=0;break;}fields|=1;
        }else if(!strcmp(key,"SAVE_TO_DISK")){
            if(!strcmp(value,"YES"))next.persist=1;
            else if(!strcmp(value,"NO"))next.persist=0;
            else {ok=0;break;}fields|=2;
        }else {ok=0;break;}
    }
    if(ferror(f))ok=0;if(fclose(f))ok=0;
    if(!ok||fields!=3)return 0;*settings=next;return 1;
}
static char support_error_text[512];
const char *nd_support_error(void)
{return support_error_text[0]?support_error_text:"Unknown support-settings error";}
static void support_io_error(const char *operation,const char *path)
{
    int code=errno?errno:EIO;
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
    LONG dos=IoErr();char reason[128];reason[0]=0;
    if(dos)Fault(dos,NULL,reason,sizeof(reason));
    sprintf(support_error_text,"%.200s\nStep: %.40s\nDOS error %ld: %.120s\nRuntime error %d",path,operation,dos,reason,code);
#else
    sprintf(support_error_text,"%.200s\nStep: %.40s\nError %d: %.120s",path,operation,code,strerror(code));
#endif
}
/* Some ENV handlers cannot replace or rename files, even though ordinary
 * writes work. Keep a complete previous generation before trying a copy. */
static int support_copy(const char *source,const char *destination,const char *operation)
{
    FILE *in,*out;char bytes[256];size_t count;int ok=1;
    in=fopen(source,"rb");if(!in){support_io_error(operation,source);return 0;}
    out=fopen(destination,"wb");if(!out){support_io_error(operation,destination);fclose(in);return 0;}
    while((count=fread(bytes,1,sizeof(bytes),in))!=0)
        if(fwrite(bytes,1,count,out)!=count){support_io_error(operation,destination);ok=0;break;}
    if(ok&&ferror(in)){support_io_error("read complete settings",source);ok=0;}
    if(fclose(out)&&ok){support_io_error("close settings file",destination);ok=0;}
    if(fclose(in)&&ok){support_io_error("close settings source",source);ok=0;}
    return ok;
}
int nd_support_write(const char *path,const struct NDSupportSettings *s)
{
    FILE *f;char temp[256],backup[256],failure[512];int ok,existed;
    struct NDSupportSettings previous,check;
    static const char *const mode[]={"OFF","LOG","TRACE"};
    support_error_text[0]=0;
    if(!path||!s||strlen(path)>240||s->level<0||s->level>2||(s->persist!=0&&s->persist!=1)){
        strcpy(support_error_text,"Invalid support settings or destination path");return 0;
    }
    sprintf(temp,"%s.new",path);sprintf(backup,"%s.bak",path);
    f=fopen(temp,"wb");if(!f){support_io_error("create staged settings",temp);return 0;}
    ok=fprintf(f,"# NovaDock support settings\nMODE=%s\nSAVE_TO_DISK=%s\n",mode[s->level],s->persist?"YES":"NO")>0;
    if(!ok)support_io_error("write staged settings",temp);
    if(fclose(f)&&ok){support_io_error("close staged settings",temp);ok=0;}
    if(!ok){remove(temp);return 0;}
    if(!nd_support_read(temp,&check)||check.level!=s->level||check.persist!=s->persist){
        support_io_error("verify staged settings",temp);remove(temp);return 0;
    }
    existed=nd_support_read(path,&previous);
    if(existed&&!support_copy(path,backup,"preserve previous settings")){remove(temp);return 0;}
    if(rename(temp,path)!=0){
        if(!support_copy(temp,path,"publish settings")){
            /* Retain the original failure even if rollback also fails. A
             * valid .bak is also loaded on the next settings read. */
            strcpy(failure,nd_support_error());
            if(existed){if(!support_copy(backup,path,"restore previous settings"))remove(path);}
            else remove(path);
            strcpy(support_error_text,failure);remove(temp);return 0;
        }
        remove(temp);
    }
    if(!nd_support_read(path,&check)||check.level!=s->level||check.persist!=s->persist){
        support_io_error("verify saved settings",path);strcpy(failure,nd_support_error());
        if(existed){if(!support_copy(backup,path,"restore previous settings"))remove(path);}
        else remove(path);
        strcpy(support_error_text,failure);return 0;
    }
    remove(ND_SUPPORT_CAPTURE_OWNER);
    support_error_text[0]=0;return 1;
}
void nd_support_load(struct NDSupportSettings *s,int trace_default)
{
    s->level=trace_default?ND_SUPPORT_TRACE:ND_SUPPORT_OFF;s->persist=trace_default?1:0;
    if(!nd_support_read(ND_SUPPORT_ENV,s)&&!nd_support_read(ND_SUPPORT_ENV".bak",s)&&
       !nd_support_read(ND_SUPPORT_ENVARC,s))nd_support_read(ND_SUPPORT_ENVARC".bak",s);
}

/* A short lease restores capture even if its countdown helper exits badly.
 * Explicit Use/Save invalidates the lease, so the user's later choice wins. */
void nd_support_expire_capture(void)
{
    FILE *f;unsigned long owner,until;struct NDSupportSettings previous;long now=(long)time(NULL);
    f=fopen(ND_SUPPORT_CAPTURE_OWNER,"r");if(!f)return;
    if(fscanf(f,"%lu %lu %d %d",&owner,&until,&previous.level,&previous.persist)!=4){fclose(f);return;}
    fclose(f);
    if(now>0 && ((unsigned long)now>=until || until-(unsigned long)now>65UL) && previous.level>=0 && previous.level<=2 && (previous.persist==0 || previous.persist==1))
        (void)nd_support_write(ND_SUPPORT_ENV,&previous);
}
int nd_support_recorded(void)
{
    FILE *f;unsigned long ended;unsigned int seconds;long now=(long)time(NULL);int completed;
    f=fopen(ND_SUPPORT_CAPTURE_FILE,"r");if(!f)return 0;
    completed=fscanf(f,"NovaDock capture 1 completed=1 seconds=%u end=%lu",&seconds,&ended)==2;fclose(f);
    if(!completed || !seconds || seconds>60 || now<=0 || (unsigned long)now<ended || (unsigned long)now-ended>86400UL)return 0;
    f=fopen(ND_SUPPORT_DISK "/dock.log","r");if(!f)return 0;fclose(f);return 1;
}
