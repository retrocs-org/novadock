#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nd_markdown.h"
static const char *colors[]={"#000000","#B22222","#164DAA","#146B38","#782A90","#666666"};
static const char *faces[]={"font-family:Topaz;font-size:8px","font-family:Topaz;font-size:9px","font-family:Courier;font-size:13px"};
static int same(struct NDMDStyle a,struct NDMDStyle b)
{return a.style==b.style && a.heading==b.heading;}

void nd_md_parse(const char *raw,struct NDMarkdown *v,unsigned int face,unsigned int style)
{
    unsigned int p=0,o=0,n,i,h,stack=0;char token[180];
    struct NDMDStyle f,parents[16];
    memset(v,0,sizeof(*v));memset(&f,0,sizeof(f));f.face=face<3 ? face:0;f.style=style&3;
    while(raw[p] && o<ND_NOTE_TEXT-1){
        if(!p || raw[p-1]=='\n'){
            f.heading=0;f.face=face<3 ? face:0;h=0;while(h<5 && raw[p+h]=='#')++h;
            if(h && h<=4 && raw[p+h]==' '){f.heading=h;f.face=h==1 ? 2:h==2 ? 1:0;p+=h+1;}
        }
        if(!strncmp(raw+p,"<span style=\"",13)){
            const char *end=strchr(raw+p,'>');
            if(end && (unsigned int)(end-(raw+p))<sizeof(token)-1 && stack<16){
                n=(unsigned int)(end-(raw+p))+1;memcpy(token,raw+p,n);token[n]=0;
                if(token[n-2]=='"' && strstr(token,"font-family:")){
                    struct NDMDStyle next=f;int valid=0;
                    for(i=0;i<3;++i)if(strstr(token,faces[i])){next.face=i;valid=1;break;}
                    for(i=0;i<6;++i)if(strstr(token,colors[i])){next.color=i;break;}
                    if(valid){parents[stack++]=f;f=next;p+=n;continue;}
                }
            }
        }
        if(stack && !strncmp(raw+p,"</span>",7)){f=parents[--stack];p+=7;continue;}
        if(raw[p]=='*' || raw[p]=='_'){
            unsigned int run=1,bit;char marker[4];
            while(run<3 && raw[p+run]==raw[p])++run;
            if(run==3 && (f.style&3)==1)run=2;
            else if(run==3 && (f.style&3)==2)run=1;
            bit=run==3 ? 3:run==2 ? 1:2;memset(marker,raw[p],run);marker[run]=0;
            if((f.style&bit)==bit || strstr(raw+p+run,marker)){f.style^=bit;p+=run;continue;}
            for(i=0;i<run && o<ND_NOTE_TEXT-1;++i){v->source[o]=p;v->format[o]=f;v->text[o++]=raw[p++];}continue;
        }
        if(raw[p]=='`' && (f.face==2 || strchr(raw+p+1,'`'))){f.face=f.face==2 ? face:2;++p;continue;}
        v->source[o]=(unsigned short)p;v->format[o]=f;
        if(raw[p]=='\\' && raw[p+1]){v->text[o++]=raw[p+1];p+=2;}
        else if(!strncmp(raw+p,"&lt;",4)){v->text[o++]='<';p+=4;}
        else if(!strncmp(raw+p,"&gt;",4)){v->text[o++]='>';p+=4;}
        else if(!strncmp(raw+p,"&amp;",5)){v->text[o++]='&';p+=5;}
        else v->text[o++]=raw[p++];
    }
    v->text[o]=0;v->source[o]=(unsigned short)p;v->format[o]=f;
}
struct Writer {char *text;unsigned int size;int ok;};
static void emit(struct Writer *w,const char *s)
{unsigned int n=(unsigned int)strlen(s);if(w->size+n>=ND_NOTE_TEXT){w->ok=0;return;}memcpy(w->text+w->size,s,n);w->size+=n;w->text[w->size]=0;}
static void markers(struct Writer *w,unsigned int style)
{if((style&3)==3)emit(w,"***");else if(style&1)emit(w,"**");else if(style&2)emit(w,"*");}
static void open_format(struct Writer *w,struct NDMDStyle f){markers(w,f.style);}
static void close_format(struct Writer *w,struct NDMDStyle f){markers(w,f.style);}

int nd_md_write(struct NDMarkdown *v,char *raw)
{
    struct Writer w;struct NDMDStyle active,plain;char *text;unsigned int i,h;int opened=0;
    text=(char *)malloc(ND_NOTE_TEXT);if(!text)return 0;memset(&plain,0,sizeof(plain));active=plain;
    w.text=text;w.size=0;w.ok=1;text[0]=0;
    for(i=0;v->text[i];++i){
        if(!i || v->text[i-1]=='\n'){
            h=v->format[i].heading;if(h>4)h=0;while(h--)emit(&w,"#");if(v->format[i].heading && v->format[i].heading<=4)emit(&w," ");
        }
        if(v->text[i]=='\n'){
            if(opened){close_format(&w,active);opened=0;}v->source[i]=w.size;emit(&w,"\n");continue;
        }
        if(!opened || !same(active,v->format[i])){
            if(opened)close_format(&w,active);active=v->format[i];open_format(&w,active);opened=1;
        }
        v->source[i]=w.size;
        if(v->text[i]=='<')emit(&w,"&lt;");else if(v->text[i]=='>')emit(&w,"&gt;");else if(v->text[i]=='&')emit(&w,"&amp;");
        else{char c[3];c[0]='\\';c[1]=v->text[i];c[2]=0;
            if(strchr("\\*_`#",v->text[i]))emit(&w,c);else emit(&w,c+1);}
    }
    if(opened)close_format(&w,active);v->source[i]=w.size;
    if(w.ok)strcpy(raw,text);free(text);return w.ok;
}
int nd_md_insert(struct NDMarkdown *v,struct NDNoteText *e,const char *text,struct NDMDStyle format)
{
    unsigned int a=e->cursor<e->anchor ? e->cursor:e->anchor,b=e->cursor>e->anchor ? e->cursor:e->anchor;
    unsigned int length=(unsigned int)strlen(v->text),n=(unsigned int)strlen(text),i;
    if(a>length || b>length || !nd_note_insert(v->text,e,text))return 0;
    memmove(v->format+a+n,v->format+b,(length-b+1)*sizeof(v->format[0]));
    for(i=0;i<n;++i){v->format[a+i]=format;if(text[i]=='\n')format.heading=0;}
    if(a+n==strlen(v->text))v->format[a+n]=format;
    {unsigned int heading=v->format[0].heading;
     for(i=0;v->text[i];++i){if(!i || v->text[i-1]=='\n')heading=v->format[i].heading;v->format[i].heading=heading;}}
    return 1;
}
void nd_md_backspace(struct NDMarkdown *v,struct NDNoteText *e,int forward)
{
    struct NDMDStyle f=v->format[e->cursor];unsigned int n=(unsigned int)strlen(v->text);
    if(e->cursor==e->anchor){if(forward){if(e->cursor>=n)return;e->anchor=e->cursor+1;}else{if(!e->cursor)return;e->anchor=e->cursor-1;}}
    (void)nd_md_insert(v,e,"",f);
}
void nd_md_format(struct NDMarkdown *v,const struct NDNoteText *e,int action,unsigned int value)
{
    unsigned int a=e->cursor<e->anchor ? e->cursor:e->anchor,b=e->cursor>e->anchor ? e->cursor:e->anchor,i,all=1,bit;
    unsigned int n=(unsigned int)strlen(v->text);if(a>n || b>n)return;
    if(action==5){
        while(a && v->text[a-1]!='\n')--a;
        {if(b>a && v->text[b-1]=='\n')--b;while(b<n && v->text[b]!='\n')++b;for(i=a;i<=b;++i){v->format[i].heading=value<5 ? value:0;v->format[i].face=value==1 ? 2:value==2 ? 1:0;}return;}
    }
    if(action==6){unsigned int first=a,last=b;
        while(first && v->text[first-1]!='\n')--first;
        if(last>first && v->text[last-1]=='\n')--last;while(last<n && v->text[last]!='\n')++last;
        for(i=first;i<=last;++i)v->format[i].heading=0;
    }
    if(a==b){a=e->cursor;b=a+1;}
    if(action==1 || action==2){bit=action==1 ? 1:2;for(i=a;i<b;++i)if(!(v->format[i].style&bit))all=0;
        for(i=a;i<b;++i)if(all)v->format[i].style&=~bit;else v->format[i].style|=bit;}
    else for(i=a;i<b;++i){if(action==3)v->format[i].face=value<3 ? value:0;
        else if(action==4)v->format[i].color=value<6 ? value:0;
        else if(action==6)memset(&v->format[i],0,sizeof(v->format[i]));}
}
unsigned int nd_md_position(const struct NDMarkdown *v,unsigned int offset)
{unsigned int i=0;while(v->text[i] && v->source[i]<offset)++i;return i;}

int nd_md_formats_write(const struct NDMarkdown *v,char *out,unsigned int size)
{
    char temp[ND_NOTE_FORMAT],run[40];unsigned int a=0,b,n=(unsigned int)strlen(v->text),used=0;
    temp[0]=0;
    while(a<n){
        if(v->format[a].face==(v->format[a].heading==1 ? 2:v->format[a].heading==2 ? 1:0) && !v->format[a].color){++a;continue;}
        b=a+1;while(b<n && v->format[b].face==v->format[a].face && v->format[b].color==v->format[a].color)++b;
        sprintf(run,"%u,%u,%u,%u;",a,b,v->format[a].face,v->format[a].color);
        if(used+strlen(run)>=sizeof(temp) || used+strlen(run)>=size)return 0;
        strcpy(temp+used,run);used+=(unsigned int)strlen(run);a=b;
    }
    strcpy(out,temp);return 1;
}
int nd_md_formats_read(struct NDMarkdown *v,const char *metadata)
{
    const char *p;char *end;unsigned long fields[4];unsigned int pass,j,i,n=(unsigned int)strlen(v->text);
    for(pass=0;pass<2;++pass){p=metadata;
        while(*p){
            for(j=0;j<4;++j){if(*p<'0' || *p>'9')return 0;fields[j]=strtoul(p,&end,10);
                if(*end!=(j==3 ? ';':','))return 0;p=end+1;}
            if(fields[0]>fields[1] || fields[1]>n || fields[2]>2 || fields[3]>5)return 0;
            if(pass)for(i=(unsigned int)fields[0];i<(unsigned int)fields[1];++i){v->format[i].face=fields[2];v->format[i].color=fields[3];}
        }
    }
    return 1;
}

void nd_md_reconcile(const struct NDMarkdown *before,struct NDMarkdown *after)
{
    unsigned int old=(unsigned int)strlen(before->text),n=(unsigned int)strlen(after->text),prefix=0,suffix=0,i;
    struct NDMDStyle inherited;
    while(prefix<old && prefix<n && before->text[prefix]==after->text[prefix])++prefix;
    while(suffix<old-prefix && suffix<n-prefix && before->text[old-suffix-1]==after->text[n-suffix-1])++suffix;
    inherited=before->format[prefix];
    for(i=0;i<n;++i){struct NDMDStyle f=i<prefix ? before->format[i]:i>=n-suffix ? before->format[old-(n-i)]:inherited;
        if(f.face!=(f.heading==1 ? 2:f.heading==2 ? 1:0))after->format[i].face=f.face;
        after->format[i].color=f.color;}
}
