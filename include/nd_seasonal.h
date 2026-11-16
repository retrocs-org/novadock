#ifndef ND_SEASONAL_H
#define ND_SEASONAL_H
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "nd_config.h"
/* A permanent count, separate from session settings and layout profiles.
 * Any existing record (including a damaged one) suppresses the surprise. */
static int nd_christmas_seen(const char *path)
{
 FILE *f;int error;errno=0;f=fopen(path,"rb");error=errno;
 if(f){fclose(f);return 1;}return error!=ENOENT;
}
static int nd_christmas_remember(const char *path)
{
 FILE *f;char temp[320];int ok;
 if(nd_christmas_seen(path))return 1;
 if(strlen(path)+5>=sizeof(temp))return 0;
 strcpy(temp,path);strcat(temp,".new");f=fopen(temp,"wb");if(!f)return 0;
 ok=fputs("NOVADOCK_CHRISTMAS_ADDED=1\n",f)>=0;
 if(fclose(f))ok=0;
 if(!ok){remove(temp);return 0;}
 if(rename(temp,path)==0)return 1;
 remove(temp);return nd_christmas_seen(path);
}
static int nd_christmas_index(const struct NDConfig *cfg)
{int i;for(i=0;i<cfg->count;++i)if(cfg->items[i].kind==ND_ITEM_PLUGIN&&!strcmp(cfg->items[i].id,"CHRISTMAS"))return i;return -1;}
static int nd_christmas_eligible(const struct NDConfig *cfg,int month,int day,int available,const char *history)
{
 int i=nd_christmas_index(cfg);
 return month==12 && day>=10 && day<=25 && available && !(i>=0&&cfg->items[i].enabled) && !nd_christmas_seen(history) && (i>=0 || cfg->count<ND_MAX_ITEMS);
}
static int nd_christmas_enable(struct NDConfig *cfg)
{
 int i=nd_christmas_index(cfg);
 if(i<0){if(!nd_parse_line(cfg,"PLUGIN=XMAS|Plugins/Christmas|CHRISTMAS|METER|OFF|NO"))return 0;i=nd_christmas_index(cfg);}
 return i>=0 && nd_set_item_enabled(cfg,i,1);
}
#endif
