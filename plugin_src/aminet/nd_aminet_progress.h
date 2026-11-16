/* Bounded download progress records shared by the worker and browser. */
#ifndef ND_AMINET_PROGRESS_H
#define ND_AMINET_PROGRESS_H
#include <stdio.h>
#include <string.h>
#define ND_AMINET_DOWNLOAD_LIMIT 67108864UL
#include "nd_aminet.h"
static int nd_aminet_progress_read(FILE *file,unsigned long *done,unsigned long *total)
{
 char line[96],extra;unsigned long received,size;size_t n;
 if(!fgets(line,sizeof(line),file))return 0;n=strlen(line);
 if(!n||line[n-1]!='\n'||sscanf(line,"%lu %lu %c",&received,&size,&extra)!=2)return 0;
 if(size>ND_AMINET_DOWNLOAD_LIMIT||received>size)return 0;
 *done=received;*total=size;return 1;
}
#endif
