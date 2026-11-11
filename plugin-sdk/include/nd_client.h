#ifndef ND_CLIENT_H
#define ND_CLIENT_H
#include <exec/io.h>
#include <dos/dos.h>
#include "nd_protocol.h"
/* Initializes message with host-facing ID, creates reply port.
 * The caller reuses NDClient for the entire process lifetime.
 */
struct NDClient {
    struct MsgPort *reply;
    struct MsgPort *timeout_port;
    struct IORequest *timeout_request;
    APTR wire_packet;
    union {
        struct NDGraphMessage graph;
        struct NDMediaMessage media;
        struct NDExtMessage ext;
    } packet;
    char quiet_caption[ND_TEXT_LEN];
    char quiet_value[ND_TEXT_LEN];
    ULONG quiet_elapsed_ticks;
    UWORD quiet_meter;
    UWORD quiet_valid;
    UWORD quiet_graph_a;
    UWORD quiet_graph_b;
    UWORD quiet_graph_valid;
    UWORD quiet_media_capabilities;
    UWORD quiet_media_state;
    UWORD quiet_media_volume;
    UWORD quiet_media_valid;
    UWORD timeout_open;
    UWORD timed_out;
    UWORD throttled;
    UWORD wake_pending;
    /* Interruptible idle wait (nd_client_wait). Plugin-private state: the
     * wire packet layout is unchanged. */
    struct MsgPort *wait_port;
    struct IORequest *wait_request;
    UWORD wait_open;
    /* Badge (nd_client_set_badge).  Sent only once the dock has shown it
     * understands badges (ND_EVENT_HOST_BADGES); older docks never see it. */
    char badge[ND_BADGE_LEN];
    UWORD badge_pending;
    UWORD host_badges;
    /* ABI 5: second event word collected from replies, and the public
     * wake port (see ND_WAKE_PORT_PREFIX). */
    ULONG event2;
    UWORD host_abi;
    UWORD wake_owned;
    struct MsgPort *wake_port;
    char wake_name[ND_ID_LEN+16];
};
int nd_client_init(struct NDClient *c, const char *id);
/* Shows TEXT (up to 7 characters, e.g. an unread count) as a badge on the
 * plugin's tile; "" removes it and "*" shows a plain dot.  Safe with any
 * NovaDock: a dock without badge support simply never shows it.  Returns
 * the dock's events like the update calls (ND_EVENT_NONE when nothing had
 * to be sent). */
unsigned int nd_client_set_badge(struct NDClient *c, const char *text);
/* Sends one status update. Returns QUIT if the host is missing or does not
 * reply within ND_PLUGIN_IPC_TIMEOUT_SECONDS. */
unsigned int nd_client_update(struct NDClient *c, const char *caption,
                              const char *value, unsigned int meter);
/* For stable status tiles, exchange only when the visible state changes or
 * HEARTBEAT_TICKS has elapsed. ELAPSED_TICKS is the caller's actual delay
 * since the preceding call. Skipped exchanges return ND_EVENT_NONE. Keep the
 * heartbeat below the host watchdog's stale interval (20 seconds). */
unsigned int nd_client_update_quiet(struct NDClient *c,const char *caption,
                                    const char *value,unsigned int meter,
                                    unsigned int elapsed_ticks,
                                    unsigned int heartbeat_ticks);
/* Sends independent normalized series. b=ND_GRAPH_NO_SAMPLE for one line.
 * Neither graph channel changes the existing text/progress meter.
 */
unsigned int nd_client_update_graph(struct NDClient *c, const char *caption,
                                   const char *value, unsigned int meter,
                                   unsigned int a, unsigned int b);
/* Graph equivalent of nd_client_update_quiet. Sampling can continue at its
 * normal cadence while unchanged graph packets are reduced to a watchdog-safe
 * heartbeat. A dock wake always forces the next exchange immediately. */
unsigned int nd_client_update_graph_quiet(struct NDClient *c,
                                   const char *caption,const char *value,
                                   unsigned int meter,unsigned int a,
                                   unsigned int b,unsigned int elapsed_ticks,
                                   unsigned int heartbeat_ticks);
/* Reports media controls and returns the requested absolute volume separately.
 * requested_volume is ND_MEDIA_VOLUME_UNKNOWN when no slider action occurred. */
unsigned int nd_client_update_media(struct NDClient *c, const char *caption,
                                   const char *value,
                                   unsigned int capabilities,
                                   unsigned int state,
                                   unsigned int volume,
                                   unsigned int *requested_volume);
/* Media equivalent of nd_client_update_quiet. Stable/offline players avoid
 * needless IPC, while a dock control/settings wake always forces the next
 * exchange immediately. */
unsigned int nd_client_update_media_quiet(struct NDClient *c,
                                   const char *caption,const char *value,
                                   unsigned int capabilities,
                                   unsigned int state,unsigned int volume,
                                   unsigned int *requested_volume,
                                   unsigned int elapsed_ticks,
                                   unsigned int heartbeat_ticks);
/* Returns ACTIVE_TICKS normally. While the dock is collapsed to its reveal
 * tab, bundled headless plugins back off to 2..10 seconds without exceeding
 * the host watchdog interval. */
unsigned int nd_client_delay_ticks(const struct NDClient *c,
                                   unsigned int active_ticks);
/* Sleep for up to TICKS (1/50 s) but return early when the dock has a
 * click, media action or settings change queued for this plugin; NovaDock
 * then sends ND_WAKE_SIGNAL. Returns the ticks that actually elapsed, which
 * is the value to pass as ELAPSED_TICKS to nd_client_update_quiet. Falls back
 * to Delay() if timer.device cannot be opened. */
unsigned int nd_client_wait(struct NDClient *c,unsigned int ticks);
/* ND_EVENT2_* bits received since the previous call (ABI 5 docks only;
 * always 0 with an older dock).  Reading clears them. */
unsigned long nd_client_events2(struct NDClient *c);
void nd_client_close(struct NDClient *c);
#endif
