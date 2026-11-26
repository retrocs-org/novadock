"""Exercise production volume batching, caching and worker-only filesystem I/O."""
from pathlib import Path
import os,subprocess,tempfile
R=Path(__file__).resolve().parents[1]
code=r'''#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ND_VOLUME_WORKER_TEST
#define ACCESS_READ 0
#define MEMF_PUBLIC 1
typedef int BPTR;typedef char *STRPTR;
struct Message {int unused;};struct MsgPort {int unused;};
struct InfoData {long id_NumBlocks,id_NumBlocksUsed,id_BytesPerBlock;};
static void *info_buffer;
static void *AllocVec(size_t bytes,int flags)
{
 assert(!info_buffer && bytes==sizeof(struct InfoData) && flags==MEMF_PUBLIC);
 info_buffer=malloc(bytes);
 assert(info_buffer);
 return info_buffer;
}
static void FreeVec(void *p)
{
 if(p==info_buffer)info_buffer=NULL;
 free(p);
}
struct DateStamp {long ds_Days,ds_Minute,ds_Tick;};
static unsigned long seconds_now=100;
static void DateStamp(struct DateStamp *d){d->ds_Days=0;d->ds_Minute=seconds_now/60;d->ds_Tick=(seconds_now%60)*50;}
static long IoErr(void){return 205;}
static int queries,locks,starts,alive=1,fail_info,used=20;static void *work;static int complete;
static void Forbid(void){}static void Permit(void){}
static void *FindPort(const char *p){(void)p;return alive ? &alive:NULL;}
static BPTR Lock(const char *path,int mode){(void)mode;++locks;return strcmp(path,"missing:")!=0;}
static void UnLock(BPTR lock){assert(lock);}
static int Info(BPTR lock,struct InfoData *out){assert(lock);assert(out==info_buffer && (unsigned long)out % 4 == 0);++queries;out->id_NumBlocks=100;out->id_NumBlocksUsed=used;out->id_BytesPerBlock=512;return !fail_info;}
#include "nd_task.h"
static int nd_task_start(struct NDTask *j,const char *program,const char *guard,unsigned int action,const void *data,unsigned int bytes){struct NDTaskPacket *p;assert(!j->port&&!work&&bytes<=ND_TASK_DATA&&action==1);assert(!strcmp(program,"System:Utilities/SupportLog")&&!strcmp(guard,"NovaDock.VolumeMeters"));p=calloc(1,sizeof(*p));assert(p);p->bytes=bytes;memcpy(p->data,data,bytes);work=p;j->port=(struct MsgPort *)&alive;++starts;return 1;}
static struct NDTaskPacket *nd_task_poll(struct NDTask *j){struct NDTaskPacket *p;if(!complete)return NULL;p=work;work=NULL;complete=0;j->port=NULL;return p;}
static unsigned long nd_task_signals(const struct NDTask *j){return j->port ? 16:0;}
static void nd_task_close(struct NDTask *j){j->port=NULL;}
static int nd_task_worker(const char *parent,NDTaskAction action){(void)parent;(void)action;return 0;}
#include "src/nd_volume_worker.c"
#include "src/nd_volume_query.c"
int nd_task_emit(const char *parent,const void *data,unsigned int bytes){(void)parent;if(volume_job.progress)volume_job.progress(data,bytes);return 1;}
static void finish(void){struct NDTaskPacket *p=work;assert(p);p->result=query_volumes(1,p->data,p->bytes,"parent");assert(!info_buffer);complete=1;}
int main(void){struct NDVolumeSpace value;struct NDTaskPacket *late;int i;
 nd_volume_meters_begin();assert(!nd_volume_meters_submit("System:Utilities/SupportLog")&&!queries);
 for(i=0;i<64;++i)nd_volume_meters_add("SYS:");nd_volume_meters_add("missing:");assert(wanted.count==2);assert(nd_volume_meters_submit("System:Utilities/SupportLog")&&starts==1&&!queries&&nd_volume_meters_signals()==16);
 /* The UI can continue querying its cache while the filesystem is stuck. */
 for(i=0;i<1000;++i){assert(!nd_volume_meters_poll());assert(!nd_volume_meters_cached("SYS:",&value));nd_volume_meters_begin();nd_volume_meters_add("SYS:");nd_volume_meters_add("missing:");assert(!nd_volume_meters_submit("System:Utilities/SupportLog"));}assert(starts==1&&!queries);
 finish();assert(nd_volume_meters_poll()&&queries==1&&locks==2);assert(nd_volume_meters_cached("SYS:",&value)&&value.used==20);assert(!nd_volume_meters_cached("other:",&value));
 fail_info=1;nd_volume_meters_begin();nd_volume_meters_add("SYS:");assert(nd_volume_meters_submit("System:Utilities/SupportLog"));finish();assert(nd_volume_meters_poll());assert(nd_volume_meters_cached("SYS:",&value)&&value.used==20);{struct NDVolumeStatus status;assert(nd_volume_meters_status("SYS:",&status)&&status.state==ND_VOLUME_STALE&&status.error==205);}
 fail_info=0;used=50;nd_volume_meters_begin();nd_volume_meters_add("SYS:");assert(nd_volume_meters_submit("System:Utilities/SupportLog"));finish();assert(nd_volume_meters_poll());assert(nd_volume_meters_cached("SYS:",&value)&&value.used==50);
 nd_volume_meters_begin();nd_volume_meters_add("SYS:");assert(nd_volume_meters_submit("System:Utilities/SupportLog"));finish();assert(!nd_volume_meters_poll());
 nd_volume_meters_begin();nd_volume_meters_add("new:");assert(nd_volume_meters_submit("System:Utilities/SupportLog"));late=work;nd_volume_meters_close();assert(!nd_volume_meters_signals());alive=0;assert(query_volumes(1,late->data,late->bytes,"parent")==-1);assert(!info_buffer);free(late);work=NULL;
 assert(!nd_volume_meters_cached("SYS:",&value));
 alive=1;nd_volume_meters_begin();for(i=0;i<64;++i){char path[32];sprintf(path,"Volume%d:",i);nd_volume_meters_add(path);}assert(nd_volume_meters_submit("System:Utilities/SupportLog"));finish();assert(nd_volume_meters_poll());
 nd_volume_meters_begin();for(i=0;i<63;++i){char path[32];sprintf(path,"Volume%d:",i);nd_volume_meters_add(path);}nd_volume_meters_add("Replacement:");assert(nd_volume_meters_submit("System:Utilities/SupportLog"));finish();assert(nd_volume_meters_poll());
 for(i=0;i<63;++i){char path[32];sprintf(path,"Volume%d:",i);assert(nd_volume_meters_cached(path,&value));}assert(nd_volume_meters_cached("Replacement:",&value));
 seconds_now+=20;{struct NDVolumeStatus status;assert(nd_volume_meters_status("Replacement:",&status)&&status.state==ND_VOLUME_STALE&&status.age_seconds==20&&!status.error);}
 nd_volume_meters_close();
 puts("Volume worker: 1000 nonblocking pending polls, deduplication, worker-only I/O, last-good cache, changed paths, unchanged results and detached cleanup passed.");return 0;}
'''
# Native platform declarations are supplied by this adapter; compile production logic.
code=code.replace('static int nd_task_start','int nd_task_start').replace('static struct NDTaskPacket *nd_task_poll','struct NDTaskPacket *nd_task_poll').replace('static unsigned long nd_task_signals','unsigned long nd_task_signals').replace('static void nd_task_close','void nd_task_close').replace('static int nd_task_worker','int nd_task_worker')
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(R),'-I'+str(R/'include'),'-I'+str(R/'plugin_src/common'),str(p/'test.c'),str(R/'src/nd_volume.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
s=(R/'src/dock_draw.c').read_text();a=s.index('static void apply_volume_meters(');b=s.index('void flush_dirty_tiles(',a)
assert 'Lock(' not in s[a:b] and 'Info(' not in s[a:b]
assert 'process_volume_meters(dock)' in (R/'src/novadock.c').read_text()
