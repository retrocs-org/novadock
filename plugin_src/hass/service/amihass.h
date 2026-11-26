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
#define AMIHASS_LABELS 14
#define AMIHASS_CAMERA_STATS 15
/* Up to 16 entity IDs in items[i].id (count = n). Reply: items[i] holds each
 * reading and AMIHASS_ITEM_RESULT(m,i) its result; m->result is OK when the
 * request was processed. SUBSCRIBE_MANY also takes a lease on each entity. */
#define AMIHASS_QUERY_MANY 16
#define AMIHASS_SUBSCRIBE_MANY 17
#define AMIHASS_ITEM_RESULT(m,i) ((int)((m)->action[(i)]-'0'))
#define AMIHASS_CAP_QUERY_MANY "feature.query_many"
/* All attributes of one entity, paged: items[i].id key, .name value text,
 * .device_class type (string/number/boolean/list/object/null). */
#define AMIHASS_ATTRIBUTES 18
#define AMIHASS_CAP_ATTRIBUTES "feature.attributes"
/* Recent numeric history of one entity, up to 64 points oldest first, paged
 * 16 at a time: items[i].state value, .updated ISO time. refresh = hours (1-48). */
#define AMIHASS_HISTORY 19
#define AMIHASS_CAP_HISTORY "feature.history"
/* Home Assistant persistent notifications (needs live updates). entity
 * empty: up to 8, items[i].id ID, .name title, .state message start,
 * .updated created; entity = an ID: the message in 63-character pieces in
 * items[i].name (total = its length). m->age changes with every update.
 * UNSUPPORTED with item.state "live_updates_off" when live updates are off. */
#define AMIHASS_NOTIFICATIONS 20
#define AMIHASS_STATE_LIVE_UPDATES_OFF "live_updates_off"
/* ACTION on an alarm panel that needs a code: put the code (1-16 letters or
 * digits) in m->filter; it is used for that request only and erased.
 * Without one the reply is UNSUPPORTED with item.state "code_required". */
#define AMIHASS_STATE_CODE_REQUIRED "code_required"
#define AMIHASS_CAP_LABELS "feature.labels"
#define AMIHASS_SNAPSHOT 13
#define AMIHASS_SNAPSHOT_BYTES 2048
#define AMIHASS_CAP_SNAPSHOTS "feature.camera_snapshots"
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
#define AMIHASS_SERVER_AUTH_REJECTED "auth_rejected"
/* item.state markers with AMIHASS_UNSUPPORTED / AMIHASS_BUSY (ABI unchanged). */
#define AMIHASS_STATE_CONFIRMATION "confirmation_required"
#define AMIHASS_STATE_SELECTION_FULL "selection_full"
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
/* Signal mask of the reply port (0 before the first request), for Wait(). */
unsigned long amihass_client_signal(const struct AmiHASSClient *c);
/* QUERY_MANY / SUBSCRIBE_MANY for up to 16 entities; refresh as for QUERY. */
int amihass_query_many(struct AmiHASSClient *c,const char *owner,const char ids[][96],unsigned int count,int refresh,int subscribe);
/* Full attributes (fresh: skip the 10 s cache) and numeric history (hours 1-48,
 * 0 = 24) of one entity, paged 16 items at a time from offset. */
int amihass_attributes(struct AmiHASSClient *c,const char *owner,const char *entity,int offset,int fresh);
int amihass_history(struct AmiHASSClient *c,const char *owner,const char *entity,int hours,int offset);
int amihass_notifications(struct AmiHASSClient *c,const char *owner,const char *id,int offset);
int amihass_start(const char *program);
int amihass_launch(const char *name,const char *argument);
const char *amihass_problem(int result);
void amihass_erase(void *p,unsigned long size);
int amihass_send_scope(struct AmiHASSClient*,int,const char*,const char*,int,int,const char*,const char*,const char*,const char*);
int amihass_snapshot(struct AmiHASSClient*,const char*,const char*,long,const char*,int);
/* small: request a 96x60 thumbnail (Dashboard tiles); 0 is the 320x200 view. */
int amihass_snapshot_size(struct AmiHASSClient*,const char*,const char*,long,const char*,int,int);
const unsigned char *amihass_snapshot_data(const struct AmiHASSMessage*,unsigned long*);
#endif
