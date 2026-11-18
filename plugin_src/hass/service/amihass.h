#ifndef AMIHASS_NATIVE_H
#define AMIHASS_NATIVE_H
#include <exec/ports.h>
#include "amihass_core.h"
#define AMIHASS_PORT "AmiHASS.Service"
#define AMIHASS_ABI 1UL
#define AMIHASS_MAGIC 0x41484153UL
#define AMIHASS_PAGE 16
#define AMIHASS_QUERY 1
#define AMIHASS_LIST 2
#define AMIHASS_ACTION 3
#define AMIHASS_RELOAD 4
#define AMIHASS_STATUS 5
#define AMIHASS_STOP 6
#define AMIHASS_DIAGNOSTICS 7
#define AMIHASS_RECONNECT 8
#define AMIHASS_DEVICES 9
#define AMIHASS_SUBSCRIBE 10
#define AMIHASS_UNSUBSCRIBE 11
#define AMIHASS_AREAS 12
/* Optional STATUS metadata; additive ABI 1 feature negotiation. */
#define AMIHASS_CAP_AREAS "feature.areas"
#define AMIHASS_CAP_FILTERS "feature.catalogue_filters"
#define AMIHASS_CAP_SUBSCRIPTIONS "feature.subscriptions"
/* AREAS: paged room catalogue; items[].id is the stable area ID, name the label. */
/* SUBSCRIBE uses QUERY_INTERVAL; renew within 120 s. Close releases this port. */
/* QUERY refresh flag: cache/on-demand read without a continuous poll lease. */
#define AMIHASS_QUERY_ONCE 1
/* Start a fresh state read; subsequent ordinary QUERY calls await its result. */
#define AMIHASS_QUERY_FRESH 2
/* Optional QUERY cache-age preference in seconds (5..3600); zero uses service default. */
#define AMIHASS_QUERY_INTERVAL(seconds) ((int)(seconds)<<8)
#define AMIHASS_QUERY_MAX_AGE(refresh) (((unsigned int)(refresh)>>8)&65535U)
/* STATUS item.state; status.connected remains a backward-compatible 0/1. */
#define AMIHASS_SERVER_UNCONFIGURED "not_configured"
#define AMIHASS_SERVER_NETWORK_DOWN "network_down"
#define AMIHASS_SERVER_CONNECTING "connecting"
#define AMIHASS_SERVER_SAMPLE "sample"
#define AMIHASS_SERVER_CONNECTED "connected"
#define AMIHASS_SERVER_RETRY_WAIT "retry_wait"
#define AMIHASS_SERVER_DISCONNECTED "disconnected"
#define AMIHASS_OK 1
#define AMIHASS_WAIT 2
#define AMIHASS_UNCONFIGURED 3
#define AMIHASS_FAILED 4
#define AMIHASS_STALE 5
#define AMIHASS_BUSY 6
#define AMIHASS_UNSUPPORTED 7
#define AMIHASS_UNAVAILABLE 8
struct AmiHASSStatus {ULONG uptime,last_contact;LONG subscriptions,queued,connected;char server[160];};
struct AmiHASSMessage {struct Message msg;ULONG magic,abi;LONG kind,result,offset,count,total,refresh;ULONG age;char owner[20],entity[96],action[20],domain[24],filter[64];struct AmiHASSEntity item,items[AMIHASS_PAGE];struct AmiHASSStatus status;};
struct AmiHASSClient {struct MsgPort *port;struct AmiHASSMessage *pending;unsigned long started;};
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,const char *action,int offset,int refresh,const char *domain,const char *filter);
int amihass_subscribe(struct AmiHASSClient *c,const char *owner,const char *entity,unsigned int seconds);
int amihass_unsubscribe(struct AmiHASSClient *c,const char *entity);
/* LIST/DEVICES area name or ID is carried in request item.id; wire size is unchanged. */
int amihass_send_area(struct AmiHASSClient *c,int kind,const char *owner,const char *device,int offset,int refresh,const char *domain,const char *filter,const char *area);
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c);
void amihass_client_close(struct AmiHASSClient *c);
int amihass_start(const char *program);
int amihass_launch(const char *name,const char *argument);
const char *amihass_problem(int result);
void amihass_erase(void *p,unsigned long size);
#endif
