/* Portable archive-name and command construction used by host tests and 68k. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "nd_archive.h"

static int same_text(const char *a,const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a)!=tolower((unsigned char)*b)) return 0;
        ++a;++b;
    }
    return !*a && !*b;
}
static const char *base_name(const char *path)
{
    const char *name=path,*p;
    for (p=path;*p;++p) if (*p==':' || *p=='/') name=p+1;
    return name;
}
static int ends_with(const char *name,const char *suffix)
{
    size_t n=strlen(name),s=strlen(suffix);
    return n>s && same_text(name+n-s,suffix);
}
/* Length of the archive suffix to strip from NAME, or 0. */
static size_t suffix_length(const char *name)
{
    static const char *suffixes[]={".tar.gz",".tgz",".tar",".zip",".lha",
        ".lzh",".rar",".arj",".lzx",".gz",NULL};
    int i;
    for (i=0;suffixes[i];++i)
        if (ends_with(name,suffixes[i])) return strlen(suffixes[i]);
    return 0;
}
int nd_archive_kind(const char *path)
{
    const char *name;
    if (!path || !*path) return ND_ARCHIVE_NONE;
    name=base_name(path);
    if (!*name || !suffix_length(name)) return ND_ARCHIVE_NONE;
    if (ends_with(name,".tar.gz") || ends_with(name,".tgz")) return ND_ARCHIVE_TGZ;
    if (ends_with(name,".tar")) return ND_ARCHIVE_TAR;
    if (ends_with(name,".zip")) return ND_ARCHIVE_ZIP;
    if (ends_with(name,".lha") || ends_with(name,".lzh")) return ND_ARCHIVE_LHA;
    if (ends_with(name,".rar")) return ND_ARCHIVE_RAR;
    if (ends_with(name,".arj")) return ND_ARCHIVE_ARJ;
    if (ends_with(name,".lzx")) return ND_ARCHIVE_LZX;
    if (ends_with(name,".gz")) return ND_ARCHIVE_GZ;
    return ND_ARCHIVE_NONE;
}
int nd_archive_safe_path(const char *path)
{
    const unsigned char *p=(const unsigned char *)path;
    if (!p || !*p) return 0;
    for (;*p;++p)
        if (*p<32U || *p=='"' || *p=='*' || *p=='|' || *p=='>' || *p=='<' ||
            *p=='`')
            return 0;
    return 1;
}
int nd_archive_destination(const char *archive,char *destination,
                           unsigned int size)
{
    char *name;
    size_t length,strip;
    if (!destination || size<2 || !nd_archive_safe_path(archive) ||
        nd_archive_kind(archive)==ND_ARCHIVE_NONE) return 0;
    length=strlen(archive);
    if (length>=size) return 0;
    strcpy(destination,archive);
    name=(char *)base_name(destination);
    strip=suffix_length(name);
    if (!strip || strip>=strlen(name)) return 0;
    name[strlen(name)-strip]='\0';
    return *name!='\0';
}
int nd_archive_gz_output(const char *archive,const char *destination,
                         char *output,unsigned int size)
{
    const char *name;
    size_t length,dest_length;
    if (!output || !size || !nd_archive_safe_path(archive) ||
        !nd_archive_safe_path(destination)) return 0;
    name=base_name(archive);
    length=strlen(name);
    if (ends_with(name,".gz")) length-=3; /* ".gz" */
    if (!length) return 0;
    dest_length=strlen(destination);
    if (dest_length+length+2>size) return 0;
    strcpy(output,destination);
    if (dest_length && output[dest_length-1]!=':' && output[dest_length-1]!='/')
        strcat(output,"/");
    strncat(output,name,length);
    return 1;
}
static int tool_is(const char *tool,const char *name)
{
    return same_text(base_name(tool),name);
}
int nd_archive_command(int kind,const char *tool,const char *archive,
                       const char *destination,char *command,
                       unsigned int size)
{
    char dest[260],output[300];
    size_t length;
    if (!command || !size || !nd_archive_safe_path(tool) ||
        !nd_archive_safe_path(archive) || !nd_archive_safe_path(destination) ||
        kind<ND_ARCHIVE_ZIP || kind>ND_ARCHIVE_LAST) return 0;
    length=strlen(destination);
    if (length+2>sizeof(dest)) return 0;
    strcpy(dest,destination);
    if (length && dest[length-1]!=':' && dest[length-1]!='/') strcat(dest,"/");
    if (2U*strlen(tool)+strlen(archive)+2U*strlen(dest)+40U>=size) return 0;
    command[0]='\0';
    switch (kind) {
    case ND_ARCHIVE_ZIP:
        sprintf(command,"\"%s\" -o \"%s\" -d \"%s\"",tool,archive,destination);
        break;
    case ND_ARCHIVE_LHA:
        /* Do not pass LhA's -q switch.  Its normal output names every entry
         * while it is extracted, which is the progress display shown in the
         * helper's CON: window. */
        sprintf(command,"\"%s\" x \"%s\" \"%s\"",tool,archive,dest);
        break;
    case ND_ARCHIVE_RAR:
        sprintf(command,"\"%s\" x -o+ \"%s\" \"%s\"",tool,archive,dest);
        break;
    case ND_ARCHIVE_ARJ:
        /* The common Amiga tool is UnARJ, which has no destination argument
         * and extracts into the current directory. */
        if (tool_is(tool,"UnARJ"))
            sprintf(command,"CD \"%s\"\n\"%s\" x \"%s\"",destination,tool,archive);
        else
            sprintf(command,"\"%s\" x -y \"%s\" \"%s\"",tool,archive,dest);
        break;
    case ND_ARCHIVE_LZX:
        if (tool_is(tool,"UnLZX"))
            sprintf(command,"CD \"%s\"\n\"%s\" -x \"%s\"",destination,tool,archive);
        else
            sprintf(command,"\"%s\" x \"%s\" \"%s\"",tool,archive,dest);
        break;
    case ND_ARCHIVE_TAR:
        sprintf(command,"CD \"%s\"\n\"%s\" -xvf \"%s\"",destination,tool,archive);
        break;
    case ND_ARCHIVE_TGZ:
        sprintf(command,"CD \"%s\"\n\"%s\" -xzvf \"%s\"",destination,tool,archive);
        break;
    case ND_ARCHIVE_GZ:
        /* AmigaDOS redirection must follow the command name directly. */
        if (!nd_archive_gz_output(archive,destination,output,sizeof(output)) ||
            strlen(tool)+strlen(output)+strlen(archive)+16>=size) return 0;
        sprintf(command,"\"%s\" >\"%s\" -dc \"%s\"",tool,output,archive);
        break;
    default:
        return 0;
    }
    return 1;
}
int nd_archive_status_name(unsigned long owner,char *name,unsigned int size)
{
    if (!name || size<36) return 0;
    sprintf(name,"T:NovaDockArchive-%08lx.status",owner);
    return 1;
}
const char *nd_archive_tool_names(int kind)
{
    switch (kind) {
    case ND_ARCHIVE_ZIP: return "UnZip";
    case ND_ARCHIVE_LHA: return "LhA";
    case ND_ARCHIVE_RAR: return "UnRAR or RAR";
    case ND_ARCHIVE_ARJ: return "UnARJ or ARJ";
    case ND_ARCHIVE_LZX: return "LZX or UnLZX";
    case ND_ARCHIVE_TAR: return "tar";
    case ND_ARCHIVE_TGZ: return "tar (with gzip)";
    case ND_ARCHIVE_GZ: return "gzip";
    }
    return "?";
}

/* Common signatures also recognise archives whose filename has no suffix. */
int nd_archive_detect(const char *path,const unsigned char *h,unsigned int n)
{
    if (h && n>=4 && h[0]=='P' && h[1]=='K' &&
        ((h[2]==3 && h[3]==4) || (h[2]==5 && h[3]==6) ||
         (h[2]==7 && h[3]==8))) return ND_ARCHIVE_ZIP;
    if (h && n>=7 && h[2]=='-' && h[3]=='l' &&
        (h[4]=='h' || h[4]=='z') && h[6]=='-') return ND_ARCHIVE_LHA;
    if (h && n>=2 && h[0]==0x1f && h[1]==0x8b)
        return nd_archive_kind(path)==ND_ARCHIVE_TGZ ? ND_ARCHIVE_TGZ : ND_ARCHIVE_GZ;
    if (h && n>=7 && !memcmp(h,"Rar!\x1a\x07",6)) return ND_ARCHIVE_RAR;
    if (h && n>=2 && h[0]==0x60 && h[1]==0xea) return ND_ARCHIVE_ARJ;
    if (h && n>=3 && !memcmp(h,"LZX",3)) return ND_ARCHIVE_LZX;
    if (h && n>=262 && !memcmp(h+257,"ustar",5)) return ND_ARCHIVE_TAR;
    return nd_archive_kind(path);
}
static int compression_path(const char *path)
{
    return nd_archive_safe_path(path) && !strpbrk(path,"?#[]()~$");
}
int nd_archive_output_name(const char *name,int kind,char *output,unsigned int size)
{
    const char *ext;
    unsigned int n;
    if (!output || !name || !compression_path(name) ||
        strchr(name,':') || strchr(name,'/') || name[0]=='-' ||
        !strcmp(name,".") || !strcmp(name,"..")) return 0;
    if (kind==ND_ARCHIVE_ZIP) ext=".zip";
    else if (kind==ND_ARCHIVE_LHA) ext=".lha";
    else return 0;
    n=(unsigned int)strlen(name);
    if (ends_with(name,".zip") || ends_with(name,".lha")) n-=4;
    if (!n || n+5>size) return 0;
    memcpy(output,name,n);strcpy(output+n,ext);return 1;
}
int nd_archive_output_allowed(const char *source,int drawer,const char *output)
{
    const char *a=source,*b=output;
    if (!compression_path(source) || !compression_path(output)) return 0;
    while (*a && *b && tolower((unsigned char)*a)==tolower((unsigned char)*b)) {++a;++b;}
    if (!*a && (!*b || (drawer && (*b=='/' ||
        (a>source && a[-1]==':'))))) return 0;
    return 1;
}
int nd_archive_compress_command(int kind,const char *tool,const char *source,
                                const char *output,int include_info,char *command,unsigned int size)
{
    char parent[260];
    const char *name;
    unsigned int n;
    if (!command || !compression_path(tool) || !compression_path(source) ||
        !compression_path(output) || !strchr(output,':')) return 0;
    name=base_name(source);n=(unsigned int)(name-source);
    if (!n || !*name || name[0]=='-' || n>=sizeof(parent)) return 0;
    memcpy(parent,source,n);parent[n]='\0';
    if (n>1 && parent[n-1]=='/') parent[n-1]='\0';
    if (strlen(parent)+strlen(tool)+strlen(output)+strlen(name)+32+
        (include_info ? strlen(name)+8:0)>=size) return 0;
    if (kind==ND_ARCHIVE_ZIP)
        sprintf(command,"CD \"%s\"\n\"%s\" -r \"%s\" \"%s\"",parent,tool,output,name);
    else if (kind==ND_ARCHIVE_LHA)
        sprintf(command,"CD \"%s\"\n\"%s\" -r a \"%s\" \"%s\"",parent,tool,output,name);
    else return 0;
    if(include_info){
        strcat(command," \"");strcat(command,name);strcat(command,".info\"");
    }
    return 1;
}

int nd_archive_extract_destination(const char *parent,int create_drawer,const char *name,
                                   char *output,unsigned int size)
{
    unsigned int n;
    if(!output || !size || !compression_path(parent) || !strchr(parent,':'))return 0;
    n=(unsigned int)strlen(parent);
    if(n>=size)return 0;
    if(create_drawer){
        if(!name || !*name || !compression_path(name) || strchr(name,':') ||
           strchr(name,'/') || name[0]=='-' || !strcmp(name,".") || !strcmp(name,".."))return 0;
        if(n+strlen(name)+2>size)return 0;
    }
    strcpy(output,parent);
    if(create_drawer){
        if(n && output[n-1]!=':' && output[n-1]!='/')strcat(output,"/");
        strcat(output,name);
    }
    return 1;
}

int nd_archive_close_input(struct NDArchiveCloseInput *input,unsigned char byte)
{
    unsigned int field;
    if(byte==0x9b || byte==0x1b){
        input->state=byte==0x1b ? 1:2;input->field=0;
        input->part[0]=input->part[1]=input->part[2]=0;return 0;
    }
    if(input->state==1){input->state=byte=='[' ? 2:0;return 0;}
    if(input->state==2){
        field=input->field;
        if(byte>='0' && byte<='9'){
            if(field<3 && input->part[field]<6554U){
                input->part[field]=input->part[field]*10U+byte-'0';
                if(input->part[field]>65535U)input->part[field]=65535U;
            }
        }else if(byte==';'){if(input->field<9)++input->field;}
        else if(byte=='|'){
            input->state=0;
            /* IECLASS_RAWMOUSE / IECODE_LBUTTON, or IECLASS_CLOSEWINDOW. */
            return field>=2 && (input->part[0]==11U ||
                (input->part[0]==2U && input->part[2]==104U));
        }else if(byte>=0x40 || byte<0x20)input->state=0;
        return 0;
    }
    return byte=='\r' || byte=='\n';
}

static int path_is_info(const char *path,const char *owner)
{
    char info[266];
    if(strlen(owner)+6>sizeof(info))return 0;
    strcpy(info,owner);strcat(info,".info");return same_text(path,info);
}
unsigned int nd_archive_plan_sources(struct NDArchiveSource *sources,unsigned int count)
{
    unsigned int i,j;
    if(!sources || !count || count>ND_ARCHIVE_MAX_SOURCES)return 0;
    for(i=0;i<count;++i){
        if(!memchr(sources[i].path,0,sizeof(sources[i].path)) ||
           !compression_path(sources[i].path) || !strchr(sources[i].path,':') ||
           !*base_name(sources[i].path) || *base_name(sources[i].path)=='-')return 0;
    }
    i=0;
    while(i<count){
        for(j=0;j<count;++j){
            if(j==i)continue;
            if((same_text(sources[i].path,sources[j].path) && j<i) ||
               (sources[j].drawer && !same_text(sources[i].path,sources[j].path) &&
                !nd_archive_output_allowed(sources[j].path,1,sources[i].path)) ||
               (sources[j].include_info && path_is_info(sources[i].path,sources[j].path)))break;
        }
        if(j<count){memmove(sources+i,sources+i+1,(count-i-1)*sizeof(*sources));--count;}
        else ++i;
    }
    for(i=0;i<count;++i)for(j=i+1;j<count;++j){
        if(same_text(base_name(sources[i].path),base_name(sources[j].path)) ||
           (sources[i].include_info && path_is_info(base_name(sources[j].path),base_name(sources[i].path))) ||
           (sources[j].include_info && path_is_info(base_name(sources[i].path),base_name(sources[j].path))))return 0;
    }
    return count;
}
