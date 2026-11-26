#ifndef ND_AMINOTIS_H
#define ND_AMINOTIS_H
#define ND_AMINOTIS_FAILURE_NONE 0U
#define ND_AMINOTIS_FAILURE_UNAVAILABLE 1U
#define ND_AMINOTIS_FAILURE_QUEUE_FULL 2U
#define ND_AMINOTIS_FAILURE_TIMEOUT 3U
#define ND_AMINOTIS_FAILURE_REJECTED 4U
#define ND_AMINOTIS_FAILURE_CAPABILITY 5U
/* Session counters saturate; reading diagnostics never probes the service. */
struct NDAmiNotisStats {
 unsigned long received,delivered,dropped,unavailable,queue_full,timeouts,rejected,cancelled;
 unsigned int queued,pending,last_failure,last_result;
};
#define ND_AMINOTIS_FAILURE_NAME(r) ((r)==ND_AMINOTIS_FAILURE_UNAVAILABLE ? "service unavailable" : (r)==ND_AMINOTIS_FAILURE_QUEUE_FULL ? "queue full" : (r)==ND_AMINOTIS_FAILURE_TIMEOUT ? "timeout" : (r)==ND_AMINOTIS_FAILURE_REJECTED ? "request rejected" : (r)==ND_AMINOTIS_FAILURE_CAPABILITY ? "incompatible service" : "none")
int nd_aminotis_available(void);
void nd_aminotis_tick(int enabled,unsigned int milliseconds);
void nd_aminotis_post(int enabled,const char *source,const char *title,const char *text,unsigned int severity);
void nd_aminotis_close(void);
unsigned long nd_aminotis_signals(void);
void nd_aminotis_stats(struct NDAmiNotisStats *stats);
void nd_aminotis_reset_stats(void);
#endif
