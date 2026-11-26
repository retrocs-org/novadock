#ifndef ND_SUPPORT_H
#define ND_SUPPORT_H
#define ND_SUPPORT_OFF 0
#define ND_SUPPORT_LOG 1
#define ND_SUPPORT_TRACE 2
#define ND_SUPPORT_ENV "ENV:NovaDock/support.cfg"
#define ND_SUPPORT_ENVARC "ENVARC:NovaDock/support.cfg"
#define ND_SUPPORT_RAM "RAM:NovaDock-Trace"
#define ND_SUPPORT_DISK "NovaDock:SupportLogs"
#define ND_SUPPORT_CAPTURE_OWNER "ENV:NovaDock/capture.owner"
#define ND_SUPPORT_CAPTURE_FILE ND_SUPPORT_DISK "/capture.txt"
void nd_support_expire_capture(void);
int nd_support_recorded(void);
/* Returns 0 complete, 5 partial/skipped, 10 write failure, 20 unavailable. */
int nd_support_capture(unsigned int seconds,int guided);
#define ND_SUPPORT_LOG_LIMIT 524288UL
#define ND_SUPPORT_RECORD_MAX 16384U
struct NDSupportSettings {int level,persist;};
int nd_support_read(const char *path,struct NDSupportSettings *settings);
int nd_support_write(const char *path,const struct NDSupportSettings *settings);
/* Last failed write: destination, operation and OS/runtime error. */
const char *nd_support_error(void);
void nd_support_load(struct NDSupportSettings *settings,int trace_default);
#endif
