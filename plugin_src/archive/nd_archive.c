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
        !nd_archive_safe_path(destination) ||
        nd_archive_kind(archive)!=ND_ARCHIVE_GZ) return 0;
    name=base_name(archive);
    length=strlen(name)-3; /* ".gz" */
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
