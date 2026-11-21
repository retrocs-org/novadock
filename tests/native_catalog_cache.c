#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
#include "nd_plugin_discovery.h"
long __stack=65536;
int main(void)
{struct NDPluginSummary *rows;unsigned long hits,misses,h,m;int n,i;rows=AllocVec(sizeof(*rows)*ND_PLUGIN_CATALOG_MAX,MEMF_PUBLIC|MEMF_CLEAR);if(!rows)return 10;nd_plugin_catalog_refresh();n=nd_plugin_catalog_inventory(rows,ND_PLUGIN_CATALOG_MAX);for(i=0;i<n;++i)if(rows[i].problem[0]){printf("FAIL: %s: %s\n",rows[i].id,rows[i].problem);FreeVec(rows);return 10;}nd_plugin_catalog_metrics(&hits,&misses);if(n<37){FreeVec(rows);return 10;}
 if(nd_plugin_catalog_inventory(rows,ND_PLUGIN_CATALOG_MAX)!=n){FreeVec(rows);return 10;}nd_plugin_catalog_metrics(&h,&m);printf("CACHE: %d entries, warm hits %lu, additional misses %lu\n",n,h-hits,m-misses);if(h-hits<(unsigned long)n||m!=misses){FreeVec(rows);return 10;}
 hits=h;misses=m;nd_plugin_catalog_refresh();if(nd_plugin_catalog_inventory(rows,ND_PLUGIN_CATALOG_MAX)!=n){FreeVec(rows);return 10;}nd_plugin_catalog_metrics(&h,&m);FreeVec(rows);if(h!=hits||m-misses<(unsigned long)n)return 10;puts("PASS: native cached inventory reuses healthy records and forced Refresh revalidates every package.");return 0;}
