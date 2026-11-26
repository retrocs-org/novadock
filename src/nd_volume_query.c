/* Small SupportLog worker mode; no dock/GUI code or cached parent state. */
#ifndef ND_VOLUME_WORKER_TEST
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#endif
#include <string.h>
#include "nd_volume_job.h"
#include "nd_task.h"
static unsigned long sampled_seconds(void)
{struct DateStamp d;DateStamp(&d);return ((unsigned long)d.ds_Days*1440UL+(unsigned long)d.ds_Minute)*60UL+(unsigned long)d.ds_Tick/50UL;}
int query_volumes(unsigned int action,void *data,unsigned int bytes,const char *parent)
{
 struct NDVolumeBatch *batch=data;struct InfoData *info;unsigned int i;
 if(action!=1 || bytes!=sizeof(*batch) || batch->count>ND_MAX_ITEMS)return -1;
 /* DOS passes InfoData to the filesystem as a longword-aligned BPTR. */
 info=AllocVec(sizeof(*info),MEMF_PUBLIC);if(!info)return -1;
 for(i=0;i<batch->count;++i){
  struct NDVolumeReading *row=&batch->rows[i];BPTR lock;int live;
  Forbid();live=FindPort((STRPTR)parent)!=NULL;Permit();if(!live)break;
  row->valid=0;row->error=0;
  if(!memchr(row->path,0,sizeof(row->path)) || !row->path[0])continue;
  lock=Lock((STRPTR)row->path,ACCESS_READ);if(!lock){row->error=(unsigned long)(IoErr() ? IoErr():205);row->sampled=sampled_seconds();(void)nd_task_emit(parent,row,sizeof(*row));continue;}
  memset(info,0,sizeof(*info));
  if(Info(lock,info) && info->id_NumBlocks>0 && info->id_NumBlocksUsed>=0 &&
     info->id_NumBlocksUsed<=info->id_NumBlocks && info->id_BytesPerBlock>0){
   row->space.total=(unsigned long)info->id_NumBlocks;row->space.used=(unsigned long)info->id_NumBlocksUsed;row->space.block_bytes=(unsigned long)info->id_BytesPerBlock;row->valid=1;
  }
  if(!row->valid)row->error=(unsigned long)(IoErr() ? IoErr():205);
  UnLock(lock);row->sampled=sampled_seconds();(void)nd_task_emit(parent,row,sizeof(*row));
 }
 FreeVec(info);return i==batch->count ? 1:-1;
}
int nd_volume_worker(const char *parent){return nd_task_worker(parent,query_volumes);}
