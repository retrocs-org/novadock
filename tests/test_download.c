#include <assert.h>
#include <stdio.h>
#include "nd_download.h"
int main(void){const char *bad[]={"","ftp://site/file","https://","https:///file","https://?x","https://#x","https://user:pass@site/file","https://site/a b","https://site/\"x","https://site/*x","https://site/\\x","https://site/\nx",NULL};int i;
 assert(nd_download_url("https://example.org/a.zip?x=1&y=2"));assert(nd_download_url("http://example.org/a"));for(i=0;bad[i];++i)assert(!nd_download_url(bad[i]));
 assert(nd_download_staging_dir("Work:Downloads/NovaDockDownload-1234abcd-12345678-0"));assert(!nd_download_staging_dir("Work:Downloads"));assert(!nd_download_staging_dir("Work:NovaDockDownload-"));assert(!nd_download_staging_dir("Work:NovaDockDownload-xyz"));assert(!nd_download_staging_dir("Work:NovaDockDownload-1234/other"));assert(!nd_download_staging_dir("Work:NovaDockDownload-1234/.."));puts("DOWNLOAD: protocol restrictions, credential rejection and staging ownership passed.");return 0;}
