/* Filesystem requests run in a separately loaded DOS process, never the UI. */
#ifndef ND_VOLUME_WORKER_TEST
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#endif
#include <string.h>
#include "nd_volume_worker.h"
#include "nd_task.h"
#include "nd_volume_job.h"
static struct NDTask volume_job;
static struct NDVolumeBatch wanted;
static struct NDVolumeReading cache[ND_MAX_ITEMS];
static unsigned int cache_count;
static unsigned long good_stamp[ND_MAX_ITEMS];
static int volume_changed;
static unsigned long volume_seconds(void)
{struct DateStamp d;DateStamp(&d);return ((unsigned long)d.ds_Days*1440UL+(unsigned long)d.ds_Minute)*60UL+(unsigned long)d.ds_Tick/50UL;}
static int desired(const char *path)
{unsigned int i;for(i=0;i<wanted.count;++i)if(!strcmp(wanted.rows[i].path,path))return 1;return 0;}
static void volume_result(const void *data,unsigned int bytes)
{
 const struct NDVolumeReading *row=data;unsigned int j;int previous;
 if(bytes!=sizeof(*row) || !memchr(row->path,0,sizeof(row->path)) || !row->path[0] || !desired(row->path))return;
 for(j=0;j<cache_count;++j)if(!strcmp(cache[j].path,row->path))break;
 if(j==cache_count){if(cache_count<ND_MAX_ITEMS)++cache_count;else{for(j=0;j<cache_count;++j)if(!desired(cache[j].path))break;if(j==cache_count)return;}memset(&cache[j],0,sizeof(cache[j]));strcpy(cache[j].path,row->path);good_stamp[j]=0;}
 previous=cache[j].valid;
 if(row->valid && nd_volume_valid(&row->space)){
  if(!previous || memcmp(&cache[j].space,&row->space,sizeof(row->space)))volume_changed=1;
  cache[j].space=row->space;cache[j].valid=1;good_stamp[j]=row->sampled;
 }
 if(cache[j].error!=row->error)volume_changed=1;
 cache[j].error=row->error;cache[j].sampled=row->sampled;
}
int nd_volume_meters_status(const char *path,struct NDVolumeStatus *out)
{
 unsigned int i;unsigned long now=volume_seconds();if(!out || !path)return 0;memset(out,0,sizeof(*out));
 for(i=0;i<cache_count;++i)if(!strcmp(cache[i].path,path)){out->error=cache[i].error;out->age_seconds=now>=good_stamp[i] ? now-good_stamp[i]:0;out->state=!cache[i].valid ? ND_VOLUME_UNAVAILABLE : cache[i].error || out->age_seconds>15UL ? ND_VOLUME_STALE:ND_VOLUME_FRESH;return 1;}
 return 0;
}
void nd_volume_meters_begin(void){wanted.count=0;}
void nd_volume_meters_add(const char *path)
{
 unsigned int i;struct NDVolumeReading *row;
 if(!path || !*path || strlen(path)>=ND_COMMAND_LEN)return;
 for(i=0;i<wanted.count;++i)if(!strcmp(wanted.rows[i].path,path))return;
 if(wanted.count>=ND_MAX_ITEMS)return;
 row=&wanted.rows[wanted.count++];memset(row,0,sizeof(*row));strcpy(row->path,path);
}
int nd_volume_meters_submit(const char *program)
{
 if(volume_job.port || !wanted.count || sizeof(wanted)>ND_TASK_DATA || !program || !*program)return 0;
 if(!nd_task_start(&volume_job,program,"NovaDock.VolumeMeters",1,&wanted,sizeof(wanted)))return 0;volume_job.progress=volume_result;return 1;
}
int nd_volume_meters_cached(const char *path,struct NDVolumeSpace *space)
{unsigned int i;if(!path||!space)return 0;for(i=0;i<cache_count;++i)if(cache[i].valid && !strcmp(cache[i].path,path)){*space=cache[i].space;return 1;}return 0;}
int nd_volume_meters_poll(void)
{
 struct NDTaskPacket *packet=nd_task_poll(&volume_job);unsigned int i;int changed;
 if(packet){if(packet->result==1 && packet->bytes==sizeof(wanted)){struct NDVolumeBatch *batch=(struct NDVolumeBatch *)packet->data;if(batch->count<=ND_MAX_ITEMS)for(i=0;i<batch->count;++i)volume_result(&batch->rows[i],sizeof(batch->rows[i]));}FreeVec(packet);}
 changed=volume_changed;volume_changed=0;return changed;
}
unsigned long nd_volume_meters_signals(void){return nd_task_signals(&volume_job);}
void nd_volume_meters_close(void){nd_task_close(&volume_job);wanted.count=cache_count=0;volume_changed=0;memset(good_stamp,0,sizeof(good_stamp));memset(cache,0,sizeof(cache));}
