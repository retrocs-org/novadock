#ifndef AMINOTIS_H
#define AMINOTIS_H

#define AN_PORT "AMINOTIS.SERVICE"
#define AN_PRESENTER_PORT "AMINOTIS.PRESENTER"
#define AN_REXX_PORT "AMINOTIS"
#define AN_VERSION 2
#define AN_MAGIC 0x414E4F54UL
#define AN_MAX_HISTORY 64
#define AN_MAX_SOURCES 16
#define AN_PAGE 4
#define AN_MAX_BATCH 8

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
    AN_ACTION_REGISTER, AN_ACTION_TAKE, AN_ACTION_REMOVE, AN_STORAGE_REPORT, AN_STORAGE_WAKE,
    AN_SNOOZE, AN_MUTE_SOURCE, AN_SOURCE_DIAGNOSTICS, AN_RELOAD_RULES, AN_UNSNOOZE, AN_MUTE_LIST, AN_DIAGNOSTICS, AN_WORKER_WAKE, AN_POPUP_FAILED, AN_BATCH, AN_UNDO, AN_POLICY,
    AN_ERASE_HISTORY, AN_ACTION_LIST, AN_ADAPTER_STATUS, AN_TEST_HOOK, AN_PIN, AN_SIMULATE, AN_DIGEST_VIEW, AN_DIGEST_SNAPSHOT
};
enum ANResult {
    AN_OK, AN_REPLACED, AN_UNAVAILABLE, AN_BUSY, AN_INVALID,
    AN_VERSION_ERROR, AN_NOT_FOUND, AN_STALE, AN_TIMEOUT, AN_INBOX_FULL
};
enum ANSeverity { AN_INFO, AN_WARNING, AN_ERROR };
enum ANHealth { AN_UNKNOWN, AN_ONLINE, AN_OFFLINE, AN_SOURCE_STALE };
#define AN_ATTENTION 1UL
#define AN_UNREAD_ONLY 2UL
#define AN_SOURCE_FILTER 4UL
#define AN_SEVERITY_FILTER 8UL
#define AN_INCLUDE_SNOOZED 16UL
#define AN_VIEW_SNAPSHOT 32UL
#define AN_ONLY_SNOOZED 64UL
#define AN_SAVE_SNAPSHOT 128UL
#define AN_ONLY_PINNED 256UL
#define AN_QUIET_WINDOW 512UL
#define AN_COALESCE_GROUP 1024UL
#define AN_JOB 2048UL
#define AN_HIDE_COMPLETED 4096UL
#define AN_ONLY_COMPLETED 8192UL
#define AN_PRESENT_POPUP 1UL
#define AN_PRESENT_SOUND 2UL
#define AN_PRESENT_QUIET_OVERRIDE 0x10000UL
/* High bits of event.presentation record the popup decision. */
#define AN_POPUP_REASON 0xff00UL
#define AN_EVENT_PIN 0x20000UL
#define AN_EVENT_QUIET 0x40000UL
#define AN_GROUP_COUNT_SHIFT 19
#define AN_GROUP_COUNT_MASK 0xfff80000UL
/* Job state occupies unused low bits. Jobs use the group count field for progress. */
#define AN_JOB_RUNNING 4UL
#define AN_JOB_COMPLETE 8UL
#define AN_JOB_FAILED 12UL
#define AN_JOB_STATE 12UL
#define AN_JOB_UNKNOWN 101UL
#define AN_EVENT_COUNTER(value) (((value) & AN_GROUP_COUNT_MASK) >> AN_GROUP_COUNT_SHIFT)
#define AN_GROUP_COUNT(value) (((value) & AN_JOB_STATE) ? 0UL : AN_EVENT_COUNTER(value))
#define AN_EVENT_ATTRIBUTES (AN_EVENT_PIN | AN_EVENT_QUIET | AN_GROUP_COUNT_MASK | AN_JOB_STATE)
#define AN_POPUP_DND 0x0100UL
#define AN_POPUP_MUTED 0x0200UL
#define AN_POPUP_RULE 0x0300UL
#define AN_POPUP_QUEUE_FULL 0x0400UL
#define AN_POPUP_RATE_LIMIT 0x0500UL
#define AN_POPUP_UPDATE 0x0600UL
#define AN_POPUP_SNOOZED 0x0700UL
#define AN_POPUP_QUEUED 0x0800UL
#define AN_POPUP_DISPLAY_FAILED 0x0900UL
#define AN_POPUP_MANUAL_RESTORE 0x0a00UL
#define AN_POPUP_CLIENT_ONLY 0x0b00UL
#define AN_CAP_HISTORY 1UL
#define AN_CAP_WATCH 2UL
#define AN_CAP_DND 4UL
#define AN_CAP_SOURCES 8UL
#define AN_CAP_OPEN_HISTORY 16UL
#define AN_CAP_OPEN_SOURCE 32UL
#define AN_CAP_ACTIONS 64UL
#define AN_CAP_PERSISTENCE 128UL
#define AN_CAP_CHANGE_MASK 256UL
#define AN_CAP_INBOX_TOOLS 512UL
#define AN_CAP_PERSISTENT_INBOX 1024UL
#define AN_CAP_POPUP_REASONS 2048UL
#define AN_CAP_ADVANCED 4096UL
#define AN_CAP_RULE_PREVIEW 8192UL
#define AN_CAP_RESUMABLE_WATCH 16384UL
#define AN_CAP_GROUP_COUNT 32768UL
#define AN_CAP_DIGEST_SNAPSHOT 65536UL
#define AN_CAP_JOBS 131072UL
#define AN_CAP_INBOX_PRIORITY 262144UL
#define AN_CAP_COMPLETED_JOBS 524288UL

/* AN_WATCH takes an optional interest mask in value and returns changes in id. */
#define AN_WATCH_RESYNC 1UL
#define AN_CHANGE_HISTORY 1UL
#define AN_CHANGE_UNREAD 2UL
#define AN_CHANGE_DND 4UL
#define AN_CHANGE_SOURCES 8UL
#define AN_CHANGE_DIAGNOSTICS 16UL
#define AN_CHANGE_ACTIONS 32UL
#define AN_CHANGE_PRESENTATION 64UL
#define AN_CHANGE_STORAGE_SETTINGS 128UL
#define AN_CHANGE_MUTES 256UL
#define AN_CHANGE_ALL 511UL

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

void an_use_cursor(struct ANRequest *request, const struct ANStatus *status);
int an_watch_resync(const struct ANStatus *previous, const struct ANResponse *reply);
void an_request(struct ANRequest *request, enum ANOperation operation);
int an_submit(struct ANClient *client, const struct ANRequest *request);
int an_receive(struct ANClient *client, struct ANResponse *response);
void an_close(struct ANClient *client);
int an_call(struct ANClient *client, const struct ANRequest *request,
            struct ANResponse *response, unsigned ticks);
#endif
#endif
