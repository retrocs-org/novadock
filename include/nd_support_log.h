#ifndef ND_SUPPORT_LOG_H
#define ND_SUPPORT_LOG_H
#include <exec/types.h>
#include <exec/ports.h>
#include "nd_support.h"
#define ND_SUPPORT_LOG_PORT "NovaDock.SupportLog.1"
#define ND_SUPPORT_OUTBOX "NovaDock.SupportOut."
#define ND_SUPPORT_LOG_MAGIC 0x4e444c33UL
#define ND_SUPPORT_PENDING 8
struct NDSupportLogPort {struct MsgPort port;UWORD pending,abandoned;};
struct NDSupportLogPacket {struct Message message;ULONG magic;UWORD component,bytes;LONG result;char data[ND_SUPPORT_RECORD_MAX];};
int nd_support_append(const char *directory,unsigned int component,const char *data,unsigned int bytes);
void nd_support_log_reply(struct NDSupportLogPacket *packet);
void nd_support_sink_write(unsigned int component,const char *data,unsigned int bytes);
void nd_support_sink_poll(void);
void nd_support_sink_close(void);
void nd_support_sink_status(ULONG *queued,ULONG *dropped,ULONG *errors);
#endif
