#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "nd_notes.h"
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
#include <dos/dos.h>
#include <proto/dos.h>
#endif
void nd_note_init(struct NDNote *n)
{memset(n,0,sizeof(*n));strcpy(n->font,"topaz.font");n->size=8;}
static int missing_file(void)
{
#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
    LONG error=IoErr();return error==ERROR_OBJECT_NOT_FOUND || error==ERROR_DIR_NOT_FOUND;
#else
    return errno==ENOENT;
#endif
}
static int read_file(const char *path,char *text)
{
    FILE *f;size_t n;int extra,failed;
    errno=0;f=fopen(path,"rb");if(!f){text[0]=0;return missing_file() ? 2:0;}
    n=fread(text,1,ND_NOTE_RAW-1,f);extra=fgetc(f);failed=ferror(f);fclose(f);
    if(failed)return 0;if(extra!=EOF || memchr(text,0,n))return -1;text[n]=0;return 1;
}
static int header_field(const char **input,const char *key,char *out,unsigned int limit)
{
    const char *p=*input,*end;size_t n,k=strlen(key);
    if(strncmp(p,key,k))return 0;p+=k;end=strchr(p,'\n');if(!end)return 0;
    n=(size_t)(end-p);if(n && p[n-1]=='\r')--n;if(n>=limit)return 0;
    memcpy(out,p,n);out[n]=0;*input=end+1;return 1;
}
static int valid_body(const char *text)
{
    unsigned int lines=1;const unsigned char *p=(const unsigned char *)text;
    for(;*p;++p){if(*p=='\n'){if(++lines>1024)return 0;}
        else if(*p<32 || *p==127)return 0;}
    return 1;
}
static int hex_value(int c)
{if(c>='0' && c<='9')return c-'0';if(c>='A' && c<='F')return c-'A'+10;return -1;}
static int decode_title(const char *in,char *out,unsigned int limit)
{
    unsigned int n=0;int a,b,c;
    while(*in){c=(unsigned char)*in++;
        if(c=='%'){a=hex_value(in[0]);b=in[0] ? hex_value(in[1]):-1;if(a<0 || b<0)return 0;c=a*16+b;in+=2;}
        if(c<32 || c==127 || n+1>=limit)return 0;out[n++]=(char)c;}
    out[n]=0;return 1;
}
static void encode_title(const char *in,char *out)
{
    static const char hex[]="0123456789ABCDEF";unsigned int c;
    while(*in){c=(unsigned char)*in++;if(c=='%' || c=='<' || c=='>' || c=='&' || c=='"' || c=='\\'){
        *out++='%';*out++=hex[c>>4];*out++=hex[c&15];}else *out++=(char)c;}*out=0;
}
static unsigned long format_hash(const char *text)
{unsigned long hash=2166136261UL;while(*text){hash^=(unsigned char)*text++;hash=(hash*16777619UL)&0xffffffffUL;}return hash;}
int nd_note_load(const char *path,struct NDNote *n)
{
    const char *p;char number[16],title[240],*end,*out;unsigned long value,hash=0;int rc,markdown=0;
    nd_note_init(n);rc=read_file(path,n->original);if(rc!=1)return rc;
    p=n->original;
    if(!strncmp(p,"---\nnovadock: 3\n",16)){
        p+=16;
        if(!header_field(&p,"title: ",title,sizeof(title)) ||
           !header_field(&p,"font: ",n->font,sizeof(n->font)) || !header_field(&p,"size: ",number,sizeof(number)))return -1;
        {size_t len=strlen(title);if(len<2 || title[0]!='"' || title[len-1]!='"')return -1;
         title[len-1]=0;if(!decode_title(title+1,n->title,sizeof(n->title)))return -1;}
        value=strtoul(number,&end,10);if(*end || value<6 || value>32)return -1;n->size=(unsigned int)value;
        if(!header_field(&p,"style: ",number,sizeof(number)))return -1;value=strtoul(number,&end,10);if(*end || value>3)return -1;n->style=value;
        if(!header_field(&p,"format: ",n->format,sizeof(n->format)) || !header_field(&p,"format-hash: ",number,sizeof(number)))return -1;
        hash=strtoul(number,&end,16);if(*end || strncmp(p,"---\n",4))return -1;p+=4;
        if(strlen(p)>=sizeof(n->text))return -1;strcpy(n->text,p);
        if(format_hash(n->text)!=hash)n->format[0]=0;
        if((strcmp(n->font,"topaz.font") || (n->size!=8 && n->size!=9)) &&
           (strcmp(n->font,"courier.font") || n->size!=13))return -1;
        return valid_body(n->text) ? 1:-1;
    }
    if(!strncmp(p,"NOVADOCK-NOTE 1\n",16))p+=16;
    else if(!strncmp(p,"<!-- NOVADOCK-NOTE 2\n",21)){p+=21;markdown=1;}
    else return -1;
    if(!header_field(&p,"Title=",title,sizeof(title)) ||
       !header_field(&p,"Font=",n->font,sizeof(n->font)) ||
       !header_field(&p,"Size=",number,sizeof(number)))return -1;
    if(markdown){if(!decode_title(title,n->title,sizeof(n->title)))return -1;}
    else {if(strlen(title)>=sizeof(n->title))return -1;strcpy(n->title,title);}
    value=strtoul(number,&end,10);if(*end || value<6 || value>32)return -1;n->size=(unsigned int)value;
    if(!header_field(&p,"Style=",number,sizeof(number)))return -1;
    value=strtoul(number,&end,10);if(*end || value>3)return -1;n->style=(unsigned int)value;
    if((strcmp(n->font,"topaz.font") || (n->size!=8 && n->size!=9)) &&
       (strcmp(n->font,"courier.font") || n->size!=13))return -1;
    if(markdown){if(strncmp(p,"-->\n",4))return -1;p+=4;}
    else if(*p++!='\n')return -1;
    if(strlen(p)>=ND_NOTE_TEXT)return -1;
    out=n->text;while(*p){if(*p!='\r')*out++=*p;++p;}*out=0;
    return valid_body(n->text) ? 1:-1;
}
static int valid_title(const char *text)
{const unsigned char *p=(const unsigned char *)text;for(;*p;++p)if(*p<32 || *p==127)return 0;return 1;}
static int encode(const struct NDNote *n,char *out)
{
    char title[240];
    if(!memchr(n->title,0,sizeof(n->title)) || !memchr(n->font,0,sizeof(n->font)) ||
       !memchr(n->text,0,sizeof(n->text)) || strchr(n->title,'\n') || strchr(n->title,'\r') ||
       strchr(n->font,'\n') || strchr(n->font,'\r') || !valid_body(n->text) || !valid_title(n->title) ||
       n->size<6 || n->size>32 || n->style>3 ||
       ((strcmp(n->font,"topaz.font") || (n->size!=8 && n->size!=9)) &&
        (strcmp(n->font,"courier.font") || n->size!=13)))return 0;
    encode_title(n->title,title);
    if(!memchr(n->format,0,sizeof(n->format)) || strspn(n->format,"0123456789,;")!=strlen(n->format))return 0;
    sprintf(out,"---\nnovadock: 3\ntitle: \"%s\"\nfont: %s\nsize: %u\nstyle: %u\nformat: %s\nformat-hash: %08lx\n---\n%s",
            title,n->font,n->size,n->style,n->format,format_hash(n->text),n->text);return 1;
}
int nd_note_dirty(const struct NDNote *n)
{char *text=(char *)malloc(ND_NOTE_RAW);int dirty=1;if(text){if(encode(n,text))dirty=strcmp(text,n->original)!=0;free(text);}return dirty;}
int nd_note_save(const char *path,const char *stage,struct NDNote *n)
{
    char *current,*text,fresh[350],backup[350];FILE *f;size_t len;int rc,had_original;
    if(strlen(stage)+10>=sizeof(fresh))return 0;
    current=(char *)malloc(ND_NOTE_RAW);text=(char *)malloc(ND_NOTE_RAW);
    if(!current || !text){free(current);free(text);return 0;}
    rc=read_file(path,current);had_original=rc==1;
    if(rc!=1 && rc!=2){rc=0;goto done;}
    if(strcmp(current,n->original)){rc=-2;goto done;}
    if(!encode(n,text)){rc=-1;goto done;}
    sprintf(fresh,"%s/new",stage);sprintf(backup,"%s/old",stage);
    f=fopen(fresh,"wb");if(!f){rc=0;goto done;}
    len=strlen(text);rc=fwrite(text,1,len,f)==len;if(fclose(f))rc=0;
    if(!rc){remove(fresh);goto done;}
    if(had_original && rename(path,backup)){remove(fresh);rc=0;goto done;}
    if(rename(fresh,path)){if(had_original)(void)rename(backup,path);rc=0;goto done;}
    if(had_original)remove(backup);strcpy(n->original,text);rc=1;
done:free(current);free(text);return rc;
}
int nd_note_insert(char *text,struct NDNoteText *e,const char *insert)
{
    unsigned int a=e->cursor<e->anchor ? e->cursor:e->anchor;
    unsigned int b=e->cursor>e->anchor ? e->cursor:e->anchor;
    size_t length=strlen(text),n=strlen(insert);char *copy;
    if(a>length || b>length || n>=ND_NOTE_TEXT || length-(b-a)+n>=ND_NOTE_TEXT)return 0;
    copy=(char *)malloc(ND_NOTE_TEXT);if(!copy)return 0;
    memcpy(copy,text,a);memcpy(copy+a,insert,n);strcpy(copy+a+n,text+b);
    if(!valid_body(copy)){free(copy);return 0;}
    strcpy(text,copy);free(copy);e->cursor=e->anchor=a+(unsigned int)n;return 1;
}
void nd_note_backspace(char *text,struct NDNoteText *e,int forward)
{
    size_t n=strlen(text);
    if(e->cursor==e->anchor){
        if(forward){if(e->cursor>=n)return;e->anchor=e->cursor+1;}
        else {if(!e->cursor)return;e->anchor=e->cursor-1;}
    }
    (void)nd_note_insert(text,e,"");
}

int nd_note_archive(const char *path,const char *destination,const char *original)
{
    char *current=(char *)malloc(ND_NOTE_RAW);int rc;
    if(!current)return 0;rc=read_file(path,current);
    if(rc==2){free(current);return 1;}
    if(rc!=1 || strcmp(current,original)){free(current);return -2;}
    rc=read_file(destination,current);free(current);if(rc!=2)return 0;
    return rename(path,destination)==0 ? 1:0;
}
