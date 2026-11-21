#ifndef ND_TASK_H
#define ND_TASK_H
/* A bounded, independently loaded DOS worker with explicit packet ownership. */
#define ND_TASK_DATA 24576U
struct NDTaskPacket {struct Message message;unsigned long magic;int kind,result;unsigned int action,bytes;char guard[64];unsigned char data[ND_TASK_DATA];};
struct NDTaskProgress {struct Message message;unsigned long magic;int kind,result;unsigned int stage;};
struct NDTask {struct MsgPort *port;struct NDTaskPacket *request;char name[96],guard[64];unsigned long started;int cancelled,busy;unsigned int stage;};
typedef int (*NDTaskAction)(unsigned int action,void *data,unsigned int bytes,const char *parent);
int nd_task_start(struct NDTask *job,const char *program,const char *guard,unsigned int action,const void *data,unsigned int bytes);
struct NDTaskPacket *nd_task_poll(struct NDTask *job);
void nd_task_stage(const char *parent,unsigned int stage);
void nd_task_cancel(struct NDTask *job);
void nd_task_close(struct NDTask *job);
unsigned long nd_task_signals(const struct NDTask *job);
int nd_task_worker(const char *parent,NDTaskAction action);
#endif
