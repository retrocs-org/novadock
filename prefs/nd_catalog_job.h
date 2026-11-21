#ifndef ND_CATALOG_JOB_H
#define ND_CATALOG_JOB_H
#include "nd_plugin_discovery.h"
#include "nd_task.h"
#define ND_CATALOG_INVENTORY 1U
#define ND_CATALOG_LOOKUP 2U
struct NDCatalogWork {
    int count,force;
    char id[20];
    union {struct NDPluginSummary rows[ND_PLUGIN_CATALOG_MAX];struct NDPluginCatalogEntry entry;} value;
};
int nd_catalog_worker(const char *parent);
int nd_catalog_job_start(struct NDTask *job,unsigned int action,const char *id,int force);
#endif
