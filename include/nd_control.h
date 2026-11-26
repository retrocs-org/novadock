#ifndef ND_CONTROL_H
#define ND_CONTROL_H
/* Independent from the public plugin ABI. Control messages are synchronous:
 * caller must keep packet/reply port alive until dock replies. */
#include <exec/types.h>
#include "nd_alerts.h"
#include "nd_aminotis.h"
#include <exec/ports.h>
#define ND_CONTROL_PORT "NovaDock.Control.1"
#define ND_CONTROL_MAGIC 0x4e444354UL /* 'NDCT' */
#define ND_CONTROL_VERSION 1U
#include "nd_plugin_support.h"
#define ND_CONTROL_PLUGIN_RESOURCES 17U
#define ND_RESOURCE_PAGE 8
#define ND_CONTROL_RELOAD 1U
#define ND_CONTROL_QUIT 2U
#define ND_CONTROL_DIAGNOSTICS 3U
#define ND_CONTROL_DIAGNOSTICS_RESET 4U
#define ND_CONTROL_SHOW 5U   /* reveal and bring to front (second launch) */
#define ND_CONTROL_HIDE 6U   /* collapse to the reveal tab */
#define ND_CONTROL_TOGGLE 7U
/* Two-stage reload.  BEGIN is answered at once with a positive ticket and
 * the dock then reloads; STATUS (an NDControlReloadMessage carrying that
 * ticket) is answered with the outcome once the reload has finished.  A
 * dock that predates these actions answers ND_CONTROL_INVALID, and clients
 * then fall back to the single-stage ND_CONTROL_RELOAD. */
#define ND_CONTROL_RELOAD_BEGIN 8U
#define ND_CONTROL_RELOAD_STATUS 9U
#define ND_CONTROL_SAVE_FLUSH 10U
#define ND_CONTROL_SAVE_LOCK 11U
#define ND_CONTROL_SAVE_UNLOCK 12U
#define ND_CONTROL_SUPPORT_MARK 16U /* timestamp a reproduction step */
#define ND_CONTROL_SUPPORT_FLUSH 15U /* apply support settings, flush bounded RAM capture */
#define ND_CONTROL_ALERTS 14U /* bounded history: list, acknowledge or clear */
#define ND_CONTROL_PLUGIN_HEALTH 13U /* selected-instance, read-only status */
#define ND_DIAGNOSTIC_MAX_PLUGINS 12
#define ND_DIAGNOSTIC_TICKS_PER_SECOND 50UL
#define ND_DIAGNOSTIC_AGE_UNKNOWN 0xffffffffUL
#define ND_TIMER_IDLE_QUANTA 5U
#define ND_CONTROL_OK 0L
#define ND_CONTROL_INVALID (-1L)
#define ND_CONTROL_NOMEM (-2L)
#define ND_CONTROL_NOFIT (-3L)
#define ND_CONTROL_NOHOST (-4L)
#define ND_CONTROL_TIMEOUT (-5L)
/* The dock accepted the reload but had not finished when the client
 * stopped waiting: the change is being applied, not refused. */
#define ND_CONTROL_PENDING (-6L)
#define ND_CONTROL_SAVE_FAILED (-7L)
#define ND_CONTROL_TIMEOUT_SECONDS 5U
/* A resource-changing reload rebuilds the window and artwork before
 * replying; compatible item/layout edits retain the existing window.  DataTypes-backed artwork on a classic 68k system can make that
 * legitimately slower than an ordinary control command. */
#define ND_CONTROL_RELOAD_TIMEOUT_SECONDS 20U
struct NDControlMessage {
    struct Message msg;
    ULONG magic;
    UWORD version;
    UWORD action;
    LONG result;
};
struct NDControlReloadMessage {
    struct NDControlMessage control;
    ULONG ticket;        /* from RELOAD_BEGIN */
    LONG outcome;        /* RELOAD_STATUS: the reload's ND_CONTROL_* result */
    ULONG elapsed_ticks; /* RELOAD_STATUS: how long it took (50 Hz) */
};
/* A client gives up waiting for the outcome after this long overall. */
#define ND_CONTROL_RELOAD_WAIT_SECONDS 60U
/* The fixed command prefix stays compatible with older control clients.  A
 * diagnostics request appends a read-only snapshot of the running dock. */
struct NDPluginDiagnostics {
    char id[16];
    ULONG updates;
    ULONG age_ticks;
    ULONG restarts;
    UWORD online;
    UWORD stale;
};
struct NDDiagnosticsSnapshot {
    ULONG uptime_ticks;
    ULONG wake_count;
    ULONG paint_count;
    ULONG tile_count;
    ULONG update_count;
    ULONG watchdog_restarts;
    UWORD plugin_count;
    UWORD online_count;
    UWORD stale_count;
    UWORD hidden;
    UWORD timer_active;
    UWORD listed_plugins;
    struct NDPluginDiagnostics plugins[ND_DIAGNOSTIC_MAX_PLUGINS];
    /* Extension tail: the original v1 snapshot prefix above stays binary
     * compatible with older Preferences builds. */
    ULONG unchanged_updates;
    ULONG coalesced_updates;
    ULONG reload_requests;
    ULONG reload_applied;
    ULONG reload_skipped;
    ULONG reload_failures;
    ULONG reload_last_ticks;
    ULONG reload_total_ticks;
    UWORD timer_quanta; /* 1 = responsive 5 Hz, 5 = idle 1 Hz */
    UWORD reserved; /* telemetry v2: reclaimed abandoned replies, saturates at 65535 */
    /* Memory (bytes; zero from docks that predate these fields). */
    ULONG mem_free;               /* AvailMem(MEMF_ANY) now */
    ULONG mem_largest;            /* largest free block now */
    ULONG mem_chip_free;          /* AvailMem(MEMF_CHIP) now */
    ULONG reload_largest_before;  /* largest free block as the last reload began */
    ULONG reload_largest_after;   /* ... and once it had finished */
    ULONG reload_free_low;        /* free memory at the reload's busiest point */
    UWORD reload_icons_reused;    /* icons carried over by the last reload */
    UWORD reload_icons_loaded;    /* icons it had to read again */
    ULONG settings_telemetry_version;
    ULONG live_updates,window_rebuilds;
    UWORD update_mode; /* 0 none, 1 live, 2 rebuild, 3 unchanged, 4 failed */
    UWORD save_state; /* 0 clean, 1 queued, 2 writing, 3 retry */
    ULONG saves_started,saves_completed,saves_failed,saves_coalesced;
    ULONG save_last_ticks,save_total_ticks,save_env_ticks,save_envarc_ticks;
    UWORD save_env_ok,save_envarc_ok;
    UWORD support_version,support_level,support_persist,support_reserved;
    ULONG support_queued,support_dropped,support_errors;
    UWORD viewport_version,viewport_pending;
    WORD viewport_left,viewport_top,viewport_width,viewport_height;
    LONG viewport_scroll_x,viewport_scroll_y;
    ULONG counter_epoch; /* changes on restart or diagnostics Reset */
    UWORD grip_version,dock_position;
    WORD grip_left,grip_top,grip_width,grip_height;
    UWORD render_version,frame_interval_ms;
    ULONG frame_average_us,frame_peak_us,frame_samples;
    ULONG hover_cache_bytes,hover_cache_budget,hover_cache_evictions;
    UWORD aminotis_version,aminotis_enabled;
    struct NDAmiNotisStats aminotis;
    UWORD async_version,icon_jobs,icons_ready,launch_jobs;
};
struct NDPluginResourceRow {
 char id[20],label[32];struct NDPluginSupportStats stats;
 ULONG age_ticks,cpu_hits,cpu_samples;UWORD online,stale,cpu_valid,reserved;
};
struct NDControlPluginResourcesMessage {
 struct NDControlMessage control;UWORD operation,offset,count,total;
 ULONG sample_remaining,sample_age;
 struct NDPluginResourceRow rows[ND_RESOURCE_PAGE];
};
struct NDControlPluginHealthMessage {
    struct NDControlMessage control;
    char id[20],value[32];
    UWORD online,stale;
};
struct NDControlDiagnosticsMessage {
    struct NDControlMessage control;
    struct NDDiagnosticsSnapshot snapshot;
};
struct NDControlAlertsMessage {
    struct NDControlMessage control;
    ULONG through;
    UWORD operation,offset,count,total,unread,reserved;
    struct NDAlertEntry entries[ND_ALERT_PAGE];
};
#endif
