#ifndef AMINOTIS_H
#define AMINOTIS_H

#define AN_PORT "AMINOTIS.SERVICE"
#define AN_REXX_PORT "AMINOTIS"
#define AN_VERSION 2
#define AN_MAGIC 0x414E4F54UL
#define AN_MAX_HISTORY 64
#define AN_MAX_SOURCES 16
#define AN_PAGE 4

#ifdef AN_HOST
#include <stdint.h>
typedef uint32_t ANNumber;
#else
#include <exec/types.h>
typedef ULONG ANNumber;
#endif

enum ANOperation {
    AN_STATUS = 1, AN_POST, AN_UPDATE, AN_WITHDRAW, AN_READ,
    AN_DISMISS, AN_LIST, AN_WATCH, AN_READ_ALL, AN_CLEAR,
    AN_SET_DND, AN_STOP, AN_OPEN_HISTORY, AN_SOURCE_SET, AN_SOURCE_LIST, AN_OPEN_SOURCE, AN_PRESENT,
    AN_ACTION_REGISTER, AN_ACTION_TAKE, AN_ACTION_REMOVE, AN_STORAGE_REPORT
};
enum ANResult {
    AN_OK, AN_REPLACED, AN_UNAVAILABLE, AN_BUSY, AN_INVALID,
    AN_VERSION_ERROR, AN_NOT_FOUND, AN_STALE, AN_TIMEOUT
};
enum ANSeverity { AN_INFO, AN_WARNING, AN_ERROR };
enum ANHealth { AN_UNKNOWN, AN_ONLINE, AN_OFFLINE, AN_SOURCE_STALE };
#define AN_ATTENTION 1UL
#define AN_UNREAD_ONLY 2UL
#define AN_SOURCE_FILTER 4UL
#define AN_PRESENT_POPUP 1UL
#define AN_PRESENT_SOUND 2UL
#define AN_CAP_HISTORY 1UL
#define AN_CAP_WATCH 2UL
#define AN_CAP_DND 4UL
#define AN_CAP_SOURCES 8UL
#define AN_CAP_OPEN_HISTORY 16UL
#define AN_CAP_OPEN_SOURCE 32UL
#define AN_CAP_ACTIONS 64UL
#define AN_CAP_PERSISTENCE 128UL

/* Strings use ISO-8859-1. All arrays include space for the terminator. */
struct ANEvent {
    ANNumber id, created, updated, attention, expires;
    ANNumber severity, unread, presentation, badge;
    char app[32], name[48], source[32], event[48], group[32];
    char action[32], item[64];
    char title[81], body[513];
};

struct ANSource {
    char app[32], source[32], name[48];
    ANNumber health, count_known, unread, updated;
};

struct ANStatus {
    ANNumber epoch, epoch_fraction, revision, capabilities;
    ANNumber count, unread, badge, dropped, presentation_dropped, coalesced, dnd, dnd_until, sources, storage;
};

struct ANRequest {
    ANNumber operation, flags, id, offset, limit;
    ANNumber epoch, epoch_fraction, revision, value;
    struct ANEvent event;
    struct ANSource source;
};

struct ANResponse {
    ANNumber result, id, count, more;
    struct ANStatus status;
    union {
        struct ANEvent events[AN_PAGE];
        struct ANSource sources[AN_PAGE];
    } page;
};

#ifndef AN_HOST
#include <exec/ports.h>

struct ANMessage {
    struct Message message;
    ANNumber magic, version, phase;
    struct ANRequest request;
    struct ANResponse response;
};

struct ANClient {
    struct MsgPort *reply;
    struct ANMessage *pending;
};

void an_request(struct ANRequest *request, enum ANOperation operation);
int an_submit(struct ANClient *client, const struct ANRequest *request);
int an_receive(struct ANClient *client, struct ANResponse *response);
void an_close(struct ANClient *client);
int an_call(struct ANClient *client, const struct ANRequest *request,
            struct ANResponse *response, unsigned ticks);
#endif
#endif
