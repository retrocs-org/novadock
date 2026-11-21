/* Preferences workers load their own segment/libraries; no GUI pointers cross IPC. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_catalog_job.h"
static unsigned long catalog_serial;
struct CatalogOwner {const char *parent;int last;};
static int keep_scanning(void *data,int count)
{struct CatalogOwner *owner=data;int live;Forbid();live=FindPort((STRPTR)owner->parent)!=NULL;Permit();if(live&&count!=owner->last){owner->last=count;nd_task_stage(owner->parent,(unsigned int)count);}return live;}
static int perform(unsigned int action,void *data,unsigned int bytes,const char *parent)
{struct NDCatalogWork *work=data;struct CatalogOwner owner;int result;if(bytes!=sizeof(*work))return -1;owner.parent=parent;owner.last=-1;if(!keep_scanning(&owner,0))return -1;nd_plugin_catalog_watch(keep_scanning,&owner);if(work->force)nd_plugin_catalog_refresh();if(action==ND_CATALOG_INVENTORY){work->count=nd_plugin_catalog_inventory(work->value.rows,ND_PLUGIN_CATALOG_MAX);result=work->count>=0 ? 0:-1;}else if(action==ND_CATALOG_LOOKUP){work->id[19]=0;work->count=nd_plugin_catalog_lookup(work->id,&work->value.entry);result=0;}else result=-1;nd_plugin_catalog_watch(NULL,NULL);return result;}
int nd_catalog_worker(const char *parent){return nd_task_worker(parent,perform);}
int nd_catalog_job_start(struct NDTask *job,unsigned int action,const char *id,int force)
{struct NDCatalogWork *work;char program[300],guard[64],name[300];BPTR dir;int ok;struct DateStamp stamp;unsigned long now;if(sizeof(*work)>ND_TASK_DATA)return 0;dir=GetProgramDir();if(!dir||!NameFromLock(dir,program,sizeof(program))||!AddPart(program,GetProgramName(name,sizeof(name)) ? FilePart(name):"NovaDockPrefs",sizeof(program)))return 0;work=AllocVec(sizeof(*work),MEMF_PUBLIC|MEMF_CLEAR);if(!work)return 0;work->force=force;if(id){strncpy(work->id,id,19);work->id[19]=0;}DateStamp(&stamp);now=((unsigned long)stamp.ds_Days*1440UL+stamp.ds_Minute)*3000UL+stamp.ds_Tick;sprintf(guard,"NovaDock.Catalog.%08lx.%08lx.%08lx",(unsigned long)FindTask(NULL),now,++catalog_serial);ok=nd_task_start(job,program,guard,action,work,sizeof(*work));FreeVec(work);return ok;}
