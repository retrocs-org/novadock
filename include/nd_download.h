#ifndef ND_DOWNLOAD_H
#define ND_DOWNLOAD_H
#include <string.h>
#define ND_DOWNLOAD_MAX 8
#define ND_DOWNLOAD_PATH 160
static int nd_download_url(const char *url)
{
 const char *p;if(!url || strlen(url)>=ND_DOWNLOAD_PATH)return 0;
 p=!strncmp(url,"https://",8) ? url+8:!strncmp(url,"http://",7) ? url+7:NULL;
 if(!p || !*p || *p=='/' || *p=='?' || *p=='#')return 0;
 for(;*p;++p)if((unsigned char)*p<=32 || *p=='"' || *p=='*' || *p=='@' || *p=='\\')return 0;
 return 1;
}
/* Only uniquely named staging drawers owned by this downloader are cleaned. */
static int nd_download_staging_dir(const char *path)
{
 const char *p,*base;if(!path || strlen(path)>150)return 0;base=path;
 for(p=path;*p;++p)if(*p==':' || *p=='/')base=p+1;
 if(strncmp(base,"NovaDockDownload-",17))return 0;p=base+17;if(!*p)return 0;
 for(;*p;++p)if(!((*p>='0'&&*p<='9')||(*p>='a'&&*p<='f')||*p=='-'))return 0;
 return 1;
}
#endif
