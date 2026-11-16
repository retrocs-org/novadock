/* Adapted from Beam68. Copyright (c) 2026 Beam68 contributors.
 * MIT licence: see Aminet-parser-LICENSE.txt in the package. */
/* Aminet HTML/readme parsing, independent of the transport and GUI. */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "nd_aminet.h"
static int equal(const char *a,const char *b)
{ while(*a && *b) if(tolower((unsigned char)*a++)!=tolower((unsigned char)*b++)) return 0; return !*a && !*b; }
static int prefix(const char *a,const char *b)
{ while(*b) if(!*a || tolower((unsigned char)*a++)!=tolower((unsigned char)*b++)) return 0; return 1; }
int nd_aminet_path_valid(const char *s)
{
    unsigned int n=0;int slashes=0,component=0;
    if(!s || !*s || strstr(s,"..")) return 0;
    while(*s) {
        unsigned char c=(unsigned char)*s++;
        if(++n>=ND_AMINET_PATH) return 0;
        if(c=='/') { if(!component || ++slashes>2) return 0;component=0; }
        else {
            if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='+'||c=='.')) return 0;
            if(!component && c=='.') return 0;
            component++;
        }
    }
    return slashes==2 && component>0;
}
int nd_aminet_encode_query(const char *s,char *out,unsigned int cap)
{
    static const char hex[]="0123456789ABCDEF";
    unsigned int n=0,len=0;
    if(!s || !out || !cap) return 0;
    out[0]=0;
    while(*s) {
        unsigned char c=(unsigned char)*s++;
        if(++len>=ND_AMINET_QUERY || c<32 || c==127) return 0;
        if(isalnum(c) && c<128) { if(n+1>=cap) return 0;out[n++]=(char)c; }
        else { if(n+3>=cap) return 0;out[n++]='%';out[n++]=hex[c>>4];out[n++]=hex[c&15]; }
    }
    out[n]=0;return len>=2;
}
void nd_aminet_plain(const char *s,char *out,unsigned int cap)
{
    unsigned int n=0;int tag=0,space=0;
    if(!cap) return;
    while(*s && n+1<cap) {
        unsigned char c=(unsigned char)*s++;
        if(c=='<') { tag=1;continue; } if(tag) { if(c=='>') tag=0;continue; }
        if(c=='&') {
            const char *end=strchr(s,';');
            if(end && end-s<12) {
                if(!strncmp(s,"amp;",4)) c='&'; else if(!strncmp(s,"lt;",3)) c='<';
                else if(!strncmp(s,"gt;",3)) c='>'; else if(!strncmp(s,"quot;",5)) c='"';
                else if(!strncmp(s,"apos;",5)||!strncmp(s,"#39;",4)) c='\'';
                else if(!strncmp(s,"nbsp;",5)) c=' ';
                else if(*s=='#') {
                    char *tail;unsigned long v=strtoul(s+1,&tail,10);
                    c=(tail==end && v>=32 && v<127)?(unsigned char)v:'?';
                } else c='?';
                s=end+1;
            }
        }
        if(c<=32 || c==127 || (c>=128 && c<160)) { space=n>0;continue; }
        if(space && n+2<cap) out[n++]=' ';
        space=0;out[n++]=(char)c;
    }
    out[n]=0;
}
int nd_aminet_readme_path(const char *package,char *out,unsigned int cap)
{
    const char *dot;unsigned int n,len;
    if(!nd_aminet_path_valid(package)) return 0;
    len=(unsigned int)strlen(package);dot=strrchr(package,'.');
    n=dot?(unsigned int)(dot-package):len;
    if(len>7 && equal(package+len-7,".tar.gz")) n=len-7;
    if(n+8>cap) return 0;
    out[0]='/';memcpy(out+1,package,n);strcpy(out+n+1,".readme");return 1;
}
static void parse_row(char *row,int skip,struct NDAminetResults *r)
{
    char *name,*href,*end,*desc;char path[ND_AMINET_PATH];unsigned int n;
    if(!strstr(row,"pkg_row") || !(name=strstr(row,"name_col"))) return;
    href=strstr(name,"href=\"/");
    if(!href) return;
    href+=7;end=strchr(href,'"');if(!end) return;
    n=(unsigned int)(end-href);if(n>=sizeof(path)) return;
    memcpy(path,href,n);path[n]=0;
    if(!nd_aminet_path_valid(path)) return;
    r->seen++;
    if(r->seen<=skip || r->count>=ND_AMINET_ROWS) return;
    strcpy(r->rows[r->count].path,path);
    desc=strstr(end,"href=\"/package/");
    if(desc && (desc=strchr(desc,'>')) && (end=strstr(++desc,"</a>"))) {
        *end=0;nd_aminet_plain(desc,r->rows[r->count].title,121);
    } else r->rows[r->count].title[0]=0;
    if(!r->rows[r->count].title[0]) nd_aminet_plain(path,r->rows[r->count].title,121);
    r->count++;
}
int nd_aminet_search(FILE *file,int skip,struct NDAminetResults *r)
{
    static char row[8192];unsigned int n=0;int c,capture=0,overflow=0,listing=0,zero=0;
    unsigned int zero_match=0;
    static const char empty_marker[]="Found 0 matching";
    char tail[5]={0,0,0,0,0};
    if(!file || !r || skip<0 || skip>=50000) return 0;
    memset(r,0,sizeof(*r));
    while((c=fgetc(file))!=EOF) {
        if(c==empty_marker[zero_match]) {
            if(!empty_marker[++zero_match]) { zero=1;zero_match=0; }
        } else zero_match=c==empty_marker[0]?1:0;
        memmove(tail,tail+1,3);tail[3]=(char)c;
        if(!capture && tail[1]=='<' && tail[2]=='t' && tail[3]=='r') {
            capture=1;overflow=0;n=3;memcpy(row,"<tr",3);continue;
        }
        if(capture) {
            if(n+1<sizeof(row)) row[n++]=(char)c;else overflow=1;
            if(!strcmp(tail,"</tr")) {
                if(!overflow) { row[n]=0;if(strstr(row,"sort_link_name")) listing=1;parse_row(row,skip,r); }
                capture=0;
            }
        }
    }
    return !ferror(file) && !capture && (listing || zero || r->seen>0);
}
int nd_aminet_details(FILE *file,struct NDAminetDetails *d)
{
    char line[512];int lines=0;
    memset(d,0,sizeof(*d));
    while(lines++<120 && fgets(line,sizeof(line),file)) {
        char *colon=strchr(line,':');char *to=NULL;unsigned int cap=0;
        if(!colon) { if(line[0]=='\n'||line[0]=='\r') break;continue; }
        *colon++=0;
        if(equal(line,"Short")) { to=d->summary;cap=sizeof(d->summary); }
        else if(equal(line,"Author")) { to=d->author;cap=sizeof(d->author); }
        else if(equal(line,"Version")) { to=d->version;cap=sizeof(d->version); }
        else if(equal(line,"Architecture")) { to=d->architecture;cap=sizeof(d->architecture); }
        else if(equal(line,"Requires")) { to=d->requires;cap=sizeof(d->requires); }
        if(to) nd_aminet_plain(colon,to,cap);
    }
    return !ferror(file) && d->summary[0];
}
int nd_aminet_http_header(const char *header,unsigned long limit,long *length)
{
    const char *p,*end;int found=0;
    if(!header || !length || (!prefix(header,"HTTP/1.0 200 ") && !prefix(header,"HTTP/1.1 200 "))) return 0;
    *length=-1;p=strstr(header,"\r\n");if(!p) return 0;p+=2;
    while(*p && strncmp(p,"\r\n",2)) {
        unsigned long n=0;const char *v;
        end=strstr(p,"\r\n");if(!end) return 0;
        if(prefix(p,"Content-Length:")) {
            if(found++) return 0;
            v=p+15;while(v<end && (*v==' '||*v=='\t')) v++;
            if(v==end) return 0;
            while(v<end && *v>='0' && *v<='9') {
                unsigned int digit=(unsigned int)(*v++-'0');
                if(n>limit/10 || (n==limit/10 && digit>limit%10)) return 0;
                n=n*10+digit;
            }
            while(v<end && (*v==' '||*v=='\t')) v++;
            if(v!=end) return 0;
            *length=(long)n;
        } else if(prefix(p,"Transfer-Encoding:") || prefix(p,"Content-Encoding:")) {
            v=strchr(p,':')+1;while(v<end && (*v==' '||*v=='\t')) v++;
            if(end-v!=8 || !prefix(v,"identity")) return 0;
        }
        p=end+2;
    }
    return !strncmp(p,"\r\n",2);
}

int nd_aminet_archive_header(const char *package,const unsigned char *data,unsigned int size)
{
    const char *dot=strrchr(package,'.');
    if(!dot || !data) return 0;
    if(equal(dot,".zip")) return size>=4 && data[0]=='P' && data[1]=='K' &&
        ((data[2]==3 && data[3]==4)||(data[2]==5 && data[3]==6));
    if(equal(dot,".gz") || equal(dot,".tgz")) return size>=3 && data[0]==0x1f && data[1]==0x8b && data[2]==8;
    if(equal(dot,".lha") || equal(dot,".lzh")) return size>=7 && data[2]=='-' && data[3]=='l' &&
        (data[4]=='h'||data[4]=='z') && data[5]>='0' && data[5]<='9' && data[6]=='-';
    return 0;
}

int nd_aminet_percent(unsigned long bytes,unsigned long total)
{
    unsigned long whole,remainder;int low=0,high=100;
    if(!total)return -1;
    if(bytes>=total)return 100;
    whole=total/100UL;remainder=total%100UL;
    while(low<high) {
        int mid=(low+high+1)/2;
        unsigned long threshold=(unsigned long)mid*whole+((unsigned long)mid*remainder+99UL)/100UL;
        if(bytes>=threshold)low=mid;else high=mid-1;
    }
    return low;
}
int nd_aminet_category_valid(const char *path)
{
 int slash=0,n=0,part=0;const unsigned char *p=(const unsigned char *)path;
 if(!p||!*p||strlen(path)>40||strstr(path,".."))return 0;
 for(;*p;++p){if(*p=='/'){if(!part||slash++)return 0;part=0;}else if(isalnum(*p)||*p=='_'){++part;++n;}else return 0;}
 return part&&n>0;
}
int nd_aminet_categories(FILE *file,const char *parent,struct NDAminetResults *out)
{
 char line[4096],path[ND_AMINET_PATH],*p,*end;unsigned int n,len;int i;
 memset(out,0,sizeof(*out));len=(unsigned int)strlen(parent);
 if(!nd_aminet_category_valid(parent)||strchr(parent,'/'))return 0;
 while(fgets(line,sizeof(line),file))for(p=line;(p=strstr(p,"href=\""))!=NULL;){
  p+=6;if(*p=='/')++p;if(!strncmp(p,"tree/",5))p+=5;else if(!strncmp(p,"tree?path=",10))p+=10;end=strchr(p,'"');if(!end)break;n=(unsigned int)(end-p);
  if(n>=sizeof(path)){p=end;continue;}memcpy(path,p,n);path[n]=0;p=end;
  if(strncmp(path,parent,len)||path[len]!='/'||!nd_aminet_category_valid(path))continue;
  for(i=0;i<out->count;++i)if(!strcmp(out->rows[i].path,path))break;
  if(i<out->count||out->count>=ND_AMINET_ROWS)continue;
  strcpy(out->rows[out->count].path,path);strcpy(out->rows[out->count].title,path);++out->count;
 }
 out->seen=out->count;return !ferror(file)&&out->count>0;
}
