#ifndef ND_PROTOCOL_H
#define ND_PROTOCOL_H
/* NovaDock IPC ABI v1. All plugins are separate AmigaDOS processes.
 * ABI uses 32-bit Amiga ULONG, 16-bit UWORD and exec.library messages.
 */
#include <exec/types.h>
#include <exec/ports.h>

#define ND_PORT_NAME "NovaDock.1"
#define ND_MAGIC 0x4e444f4bUL    /* 'NDOK' */
#define ND_ABI_VERSION 1      /* supported unchanged for older plugins */
#define ND_GRAPH_ABI_VERSION 2 /* v1 header plus optional two-channel samples */
#define ND_MEDIA_ABI_VERSION 3 /* v1 header plus dock media controls */
/* Window/icon rebuilding can take more than five seconds on a real 68k.
 * Allow a bounded reload-sized pause before abandoning an in-flight reply. */
#define ND_PLUGIN_IPC_TIMEOUT_SECONDS 15U
#define ND_GRAPH_NO_SAMPLE 65535U
#define ND_ID_LEN 20
#define ND_TEXT_LEN 32
#define ND_NO_METER 65535U

#define ND_MSG_UPDATE 1U
#define ND_MSG_GOODBYE 2U
/* Badges (dock ABI 4): a small label on the tile's corner, for example an
 * unread count.  Sent as a v1 header whose value field holds the badge
 * ("" removes it, "*" shows a dot); caption and meter are ignored.  Older
 * docks answer unknown commands with QUIT, so a plugin sends ND_MSG_BADGE
 * only after a reply carried ND_EVENT_HOST_BADGES; nd_client_set_badge()
 * does this automatically. */
#define ND_MSG_BADGE 3U
#define ND_BADGE_LEN 8          /* including the terminating NUL */
#define ND_BADGE_ABI_VERSION 4  /* for manifests: NOVADOCK_ABI=4 */
#define ND_EVENT_NONE 0U
#define ND_EVENT_CLICK 1U
#define ND_EVENT_QUIT 2U
#define ND_EVENT_SETTINGS 4U /* optional v1-compatible settings change notice */
#define ND_EVENT_MEDIA_PREVIOUS 8U
#define ND_EVENT_MEDIA_PLAY_PAUSE 16U
#define ND_EVENT_MEDIA_NEXT 32U
#define ND_EVENT_MEDIA_VOLUME 64U
#define ND_EVENT_THROTTLE 128U /* host reveal-tab state; reduce background polling */
#define ND_EVENT_BITSKIM_REFRESH 4096U
#define ND_EVENT_BITSKIM_RECEIVING 8192U
#define ND_EVENT_BITSKIM_PREFS 16384U
#define ND_EVENT_BITSKIM_CANCEL 32768U
#define ND_EVENT_BITSKIM_ACTIONS (ND_EVENT_BITSKIM_REFRESH|ND_EVENT_BITSKIM_RECEIVING|ND_EVENT_BITSKIM_PREFS|ND_EVENT_BITSKIM_CANCEL)
#define ND_EVENT_NETWORK_ONLINE 1024U
#define ND_EVENT_NETWORK_OFFLINE 2048U
#define ND_EVENT_MANAGE_HISTORY 512U /* Clipboard context-menu manager */
#define ND_EVENT_HOST_BADGES 256U /* host understands ND_MSG_BADGE (ABI 4) */
/* Optional host-to-plugin wake-up (SIGBREAKF_CTRL_F).  NovaDock signals the
 * task that owns a plugin's reply port when a click, media action or settings
 * change is queued; plugins sleeping in nd_client_wait() return at once.
 * Plugins that ignore it are unaffected. */
#define ND_WAKE_SIGNAL (1UL << 15) /* the same bit as SIGBREAKF_CTRL_F */
/* A plugin built with the current nd_client publishes a signal-less port
 * named ND_WAKE_PORT_PREFIX<ID> whose mp_SigTask is the plugin's process.
 * The dock wakes it only through that port, so a stale task pointer is never
 * signalled.  Plugins without one keep the older task-pointer wake-up. */
#define ND_WAKE_PORT_PREFIX "NovaDock.Wake."

#define ND_MEDIA_CAP_PREVIOUS 1U
#define ND_MEDIA_CAP_PLAY_PAUSE 2U
#define ND_MEDIA_CAP_NEXT 4U
#define ND_MEDIA_CAP_VOLUME 8U
#define ND_MEDIA_STATE_PLAYING 1U
#define ND_MEDIA_VOLUME_UNKNOWN 65535U

struct NDMessage {
    struct Message exec_msg;
    ULONG magic;
    UWORD version;
    UWORD command;
    char id[ND_ID_LEN];       /* configuration ID; NUL terminated */
    char caption[ND_TEXT_LEN]; /* e.g. 'CLOCK', 'MEMORY' */
    char value[ND_TEXT_LEN];   /* e.g. '13:42', '6.2 MB' */
    UWORD meter;              /* 0..100, or ND_NO_METER */
    UWORD event;              /* reply: bitmask of ND_EVENT_* */
};
/* Version 2 extension: read appended fields ONLY after checking mn_Length.
 * graph_a/graph_b are separate scaled 0..100 samples. 65535 marks a gap.
 * The v1 NDMessage prefix and existing API stay compatible.
 */
struct NDGraphMessage {
    struct NDMessage base;
    UWORD graph_a;
    UWORD graph_b;
};
/* Version 3 media extension. The plugin supplies capabilities/state/volume.
 * On reply, requested_volume is meaningful only with ND_EVENT_MEDIA_VOLUME.
 * Keeping actions in the reply makes inline controls use the existing single
 * request/reply exchange without another public port or polling task. */
struct NDMediaMessage {
    struct NDMessage base;
    UWORD capabilities;
    UWORD state;
    UWORD volume;
    UWORD requested_volume;
};
/* Version 5: one superset packet with a 32-bit second event word, because
 * every bit of NDMessage.event is in use.  A dock that understands it sets
 * its public port's ln_Pri to ND_HOST_ABI; older docks leave 0, so clients
 * send v5 packets only to a dock that accepts them.  FLAGS says which of the
 * graph and media fields are meaningful; the reply carries EVENT2. */
#define ND_EXT_ABI_VERSION 5
#define ND_HOST_ABI ND_EXT_ABI_VERSION
#define ND_EXT_HAS_GRAPH 1U
#define ND_EXT_HAS_MEDIA 2U
#define ND_EVENT2_REFRESH 1UL  /* user asked for a fresh value now */
struct NDExtMessage {
    struct NDMediaMessage media; /* v1 header followed by the media fields */
    UWORD graph_a;
    UWORD graph_b;
    UWORD flags;                 /* ND_EXT_HAS_* */
    UWORD reserved;
    ULONG event2;                /* reply: bitmask of ND_EVENT2_* */
    ULONG reserved2[2];
};
#endif
