#include "nd_music_formats.h"
#include <string.h>
#define FORMAT(key,label,bit,value) {#key,label,value,1UL<<bit},
const struct NDMusicFormat nd_music_formats[ND_MUSIC_FORMAT_COUNT]={ND_MUSIC_FORMATS(FORMAT)};
#undef FORMAT
unsigned long nd_music_default_mask(void)
{unsigned long mask=0;int i;for(i=0;i<ND_MUSIC_FORMAT_COUNT;++i)if(!strcmp(nd_music_formats[i].default_value,"YES"))mask|=nd_music_formats[i].bit;return mask;}
int nd_music_format_index(const char *key)
{int i;if(key)for(i=0;i<ND_MUSIC_FORMAT_COUNT;++i)if(!strcmp(key,nd_music_formats[i].key))return i;return -1;}
static int equal_part(const char *text,const char *part,unsigned int count)
{unsigned int i;int a,b;for(i=0;i<count;++i){a=(unsigned char)text[i];b=(unsigned char)part[i];if(!a || !b)return 0;if(a>='A' && a<='Z')a+='a'-'A';if(b>='A' && b<='Z')b+='a'-'A';if(a!=b)return 0;}return 1;}
static int suffix(const char *name,const char *extension)
{unsigned int n=(unsigned int)strlen(name),e=(unsigned int)strlen(extension);return n>e+1 && name[n-e-1]=='.' && equal_part(name+n-e,extension,e);}
static int module(const char *name,const char *extension)
{unsigned int e=(unsigned int)strlen(extension);return suffix(name,extension) || (strlen(name)>e+1 && equal_part(name,extension,e) && name[e]=='.');}
int nd_music_file_matches(const char *name,unsigned long mask)
{
    if(!name || !*name || suffix(name,"info"))return 0;
    if((mask&(1UL<<0)) && suffix(name,"mp3"))return 1;
    if((mask&(1UL<<1)) && module(name,"mod"))return 1;
    if((mask&(1UL<<2)) && module(name,"xm"))return 1;
    if((mask&(1UL<<3)) && module(name,"s3m"))return 1;
    if((mask&(1UL<<4)) && module(name,"it"))return 1;
    if((mask&(1UL<<5)) && (module(name,"med") || suffix(name,"mmd0") || suffix(name,"mmd1") || suffix(name,"mmd2") || suffix(name,"mmd3")))return 1;
    if((mask&(1UL<<6)) && (module(name,"sid") || suffix(name,"psid") || suffix(name,"rsid")))return 1;
    if((mask&(1UL<<7)) && suffix(name,"wav"))return 1;
    if((mask&(1UL<<8)) && (suffix(name,"aif") || suffix(name,"aiff")))return 1;
    if((mask&(1UL<<9)) && (suffix(name,"ogg") || suffix(name,"oga")))return 1;
    return (mask&(1UL<<10)) && suffix(name,"flac");
}
