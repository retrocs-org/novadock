#ifndef ND_TASK_H
#define ND_TASK_H
/* A bounded, independently loaded DOS worker with explicit packet ownership. */
/* Large enough for a full plugin catalogue (ND_PLUGIN_CATALOG_MAX summaries).
 * Packets are AllocVec()ed per job, never placed on a stack. */
#define ND_TASK_DATA 32768U
struct NDTaskPacket {struct Message message;unsigned long magic;int kind,result;unsigned int action,bytes;char guard[64];unsigned char data[ND_TASK_DATA];};
struct NDTaskProgress {struct Message message;unsigned long magic;int kind,result;unsigned int stage;};
typedef void (*NDTaskProgressAction)(const void *data,unsigned int bytes);
typedef void (*NDTaskDispose)(void *data,unsigned int bytes);
struct NDTask {struct MsgPort *port;struct NDTaskPacket *request;char name[96],guard[64];unsigned long started;int cancelled,busy;unsigned int stage;NDTaskProgressAction progress;NDTaskDispose dispose;};
typedef int (*NDTaskAction)(unsigned int action,void *data,unsigned int bytes,const char *parent);
int nd_task_start(struct NDTask *job,const char *program,const char *guard,unsigned int action,const void *data,unsigned int bytes);
struct NDTaskPacket *nd_task_poll(struct NDTask *job);
int nd_task_emit(const char *parent,const void *data,unsigned int bytes);
int nd_task_worker_owned(const char *parent,NDTaskAction action,NDTaskDispose dispose);
void nd_task_stage(const char *parent,unsigned int stage);
void nd_task_cancel(struct NDTask *job);
void nd_task_close(struct NDTask *job);
unsigned long nd_task_signals(const struct NDTask *job);
int nd_task_worker(const char *parent,NDTaskAction action);
#endif
