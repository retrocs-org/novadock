#ifndef ND_DOCK_INTERNAL_H
#define ND_DOCK_INTERNAL_H
#include "nd_save.h"
#include "nd_volume_worker.h"
#include "nd_async_io.h"
#include "nd_hover_policy.h"
#include "nd_tile_state.h"
#include "nd_options.h"
#include "nd_input_policy.h"
#include "nd_hover_dirty.h"
/* NovaDock dock internals shared by src/novadock.c and src/dock_*.c.
 * Not a public interface: plugins use include/nd_protocol.h. */
#include <graphics/gfxmacros.h>
/* NovaDock: Workbench-native dock for AmigaOS 3.0+ (68000 compatible).
 * Palette-aware Intuition UI, one public Exec message port, no MUI/RTG needed.
 * This is an initial source release; see README for build/test caveats.
 */
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/timer.h>
#include <libraries/commodities.h>
#include <exec/io.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <graphics/view.h>
#include <graphics/modeid.h>
#include <graphics/gfxmacros.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/imageclass.h>
#include <workbench/icon.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <graphics/layers.h>
#include <proto/datatypes.h>
#include <proto/input.h>
#include <proto/commodities.h>
#include <proto/rexxsyslib.h>
#include <stddef.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "nd_version.h"
#include "nd_config.h"
#include "nd_update.h"
#include "nd_layout.h"
#include "nd_graph.h"
#include "nd_protocol.h"
#include "nd_control.h"
#include "nd_ilbm.h"
#include "nd_icon_mask.h"
#include "nd_interaction.h"
#include "nd_autohide.h"
#include "nd_eyes.h"
#include "nd_theme.h"
#include "nd_backdrop.h"
#include "nd_transparency.h"
#include "nd_watchdog.h"
#include "nd_archive.h"
#include "nd_option_runtime.h"
#include "nd_control_client.h"
#include "nd_render_policy.h"
#include "nd_screennotify.h"
#include "nd_locale.h"
#include "nd_media.h"
#include "nd_tasks.h"
#include "nd_folder.h"
#include "nd_about.h"

#define ND_REXX_PORT "NOVADOCK"
/* Stack given to plugins and CLI launches.  Without an explicit value a
 * shell started from a Workbench-launched process gets the 4 KB default. */
#define ND_DEFAULT_STACK 16384UL
#define ND_MIN_STACK 4096UL
#define ND_MAX_STACK 1048576UL
/* ENVARC: write after a CPU/RAM view toggle is deferred (~10 s). */
#define ND_ENVARC_DELAY_TICKS 50
#define ND_MENU_PROFILES 4   /* profiles listed on the dock menu */
#define ND_TIMER_MICROSECONDS 200000UL
#define ND_TIP_TIMER_TICKS 2 /* about 0.4 seconds at five checks/sec */
#define ND_TIP_INTUI_TICKS 4 /* fallback: about 0.4 seconds at 10 ticks/sec */
#define ND_DOCK_IDCMP_BASE (IDCMP_REFRESHWINDOW | IDCMP_CHANGEWINDOW | \
                           IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS | \
                           IDCMP_VANILLAKEY | IDCMP_RAWKEY | \
                           IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW)
#define ND_BORDER ND_LAYOUT_BORDER
#define ND_SCREEN_MARGIN ND_LAYOUT_MARGIN
#define ND_COLOR_COUNT 33
#define ND_METRIC_COLORS 13
#define ND_ICON_DARK 8
#define ND_ICON_CASE 9
#define ND_ICON_LIGHT 10
#define ND_ICON_ACCENT 11
#define ND_ICON_SECONDARY 12
#define dock_left(d) ((d)->window->LeftEdge-((d)->canvas_view && !(d)->hidden ? (d)->viewport.left:0))
#define dock_top(d) ((d)->window->TopEdge-((d)->canvas_view && !(d)->hidden ? (d)->viewport.top:0))
#define dock_width(d) ((d)->canvas_view && !(d)->hidden ? (d)->width:(d)->window->Width)
#define dock_height(d) ((d)->canvas_view && !(d)->hidden ? (d)->height:(d)->window->Height)
#define dock_view_x(d) ((d)->canvas_view && !(d)->hidden ? (d)->viewport.left:0)
#define dock_view_y(d) ((d)->canvas_view && !(d)->hidden ? (d)->viewport.top:0)
#define ND_BG 0
#define ND_TILE 1
#define ND_HOVER 2
#define ND_TEXT 3
#define ND_SUBTLE 4
#define ND_ACCENT 5
#define ND_BAR_BG 6
#define ND_ACCENT2 7
#define ND_CX_RBUTTON_ID 30000L
#define ND_CX_POINTER_ID 30003L
#define ND_CX_DISK_ID 30001L
#define ND_CX_FOCUS_ID 30002L   /* DOCK_HOTKEY: keyboard control */
/* Removable media: rescan the DOS volume list every 2 s (timer ticks of
 * 0.2 s), and shortly after a disk-change input event. */
#define ND_MEDIA_SCAN_TICKS 10
#define ND_MEDIA_EVENT_TICKS 3
/* SUSPEND safety limit: reopen after 30 s (timer ticks of 0.2 s). */
#define ND_SUSPEND_TICKS 150
#define RGBBYTE(n) ((ULONG)(n) * 0x01010101UL)


/* vbcc's startup code grows the stack to this size when the dock is started
 * with less (Preferences' Start/Restart, a Shell, or an icon with a small
 * stack).  The deepest path - drop, save, publish and re-parse - needs well
 * over 10 KB plus OS overhead, so the 4 KB Shell default overflows. */

/* A graph tile as last painted in full, so a new sample can be scrolled in
 * (draw_graph_step) instead of redrawing every line of the graph. */
struct NDGraphView {
    short x,y,tw,th;          /* tile */
    short gx,gy,gw,gh;        /* graph area */
    UBYTE valid;              /* painted by the plain graph-tile layout */
    UBYTE bg;                 /* interior colour it was painted on */
    UBYTE phase;              /* dotted midline position, moves with scrolls */
    ULONG painted;            /* graph_samples[] value at that paint */
};

#define ND_HOVER_CACHE_SLOTS 8
#define ND_HOVER_WIDGET_LIMIT 256
struct NDHoverScale {
    struct BitMap *bitmap,*mask,*source,*source_mask;
    ULONG used,serial;
    int index,width,height,source_width,source_height,valid;
};
struct NDHoverBank {
    struct BitMap *bitmap,*mask;
    const unsigned char *runs;
    ULONG serial;
    int size,online;
};
struct NDCaption {
    int x,baseline,width,color,tight;
    char text[ND_LABEL_LEN];
};
struct NDHoverWidget {
    struct BitMap *bitmap,*mask;
    ULONG serial,used;
    int width,height,capacity_width,capacity_height;
    int content_left,content_top,content_width,content_height;
};
/* Width of a hover label, remembered for the font it was measured with. */
struct NDLabelWidth {const void *font;int width;char text[ND_LABEL_LEN];};
struct NDArtwork;
struct NDDock {
    struct NDBackdrop background;
    struct NDTransparency transparency;
    struct NDUpdateSchedule updates;
    unsigned long alerts_started,alerts_update_tick,alerts_update_checked;
    int alerts_update_waiting;
    struct NDConfig conf;
    struct NDAlerts alerts;
    /* Only launchers that actually need a background action are retained.
     * Keeping a second complete NDConfig resident cost about 52 KB on a
     * classic machine and could leave DOpus 4 without enough contiguous Fast
     * RAM to construct its custom screen. */
    struct NDItem *action_items;
    int action_count;
    struct NDThemeSpec theme_spec;
    struct NDArtwork *plugin_artwork[ND_MAX_ITEMS];
    struct NDThumbnailCache *thumbnails[ND_MAX_ITEMS];
    ULONG plugin_art_key[ND_MAX_ITEMS];
    UWORD plugin_art_state[ND_MAX_ITEMS];
    struct DiskObject *icons[ND_MAX_ITEMS];
    struct NDHoverScale hover_icons[ND_HOVER_CACHE_SLOTS];
    struct NDHoverBank hover_banks[ND_MAX_ITEMS];
    ULONG hover_cache_clock,hover_cache_budget,hover_cache_evictions;
    ULONG frame_average_us,frame_peak_us,frame_samples;
    int timer_frame_ms;
    struct BitMap *hover_shelf,*hover_widget_mask,*hover_frame;
    int hover_frame_width,hover_frame_height;
    int painting_mask;
    struct NDHoverWidget hover_widgets[ND_MAX_ITEMS];
    struct NDCaption hover_captions[ND_MAX_ITEMS];
    ULONG hover_shelf_serial;
    struct NDLabelWidth hover_label_width[ND_MAX_ITEMS];
    /* The frame last presented by draw_hover(): what it was composed from,
     * so a pointer move recomposes only the span that changed. */
    int hover_prev_valid;
    long hover_prev_key[12];
    struct NDRect hover_prev_rects[ND_MAX_ITEMS],hover_prev_shelf;
    /* Everything drawn for each tile (artwork, label, dot, meter, badge):
     * this frame's and the presented frame's, and the presented hover. */
    struct NDRect hover_ext[ND_MAX_ITEMS],hover_prev_ext[ND_MAX_ITEMS];
    int hover_prev_index;
    /* Animated magnification: the displayed size of each tile eases toward
     * its target; draw_hover() advances one step per frame. */
    short hover_anim_w[ND_MAX_ITEMS],hover_anim_h[ND_MAX_ITEMS];
    int hover_anim_valid,hover_anim_advance,hover_animating,hover_box_paint;
    int hover_anim_row,hover_anim_pointer,timer_anim;
    struct NDFrameClock timer_clock;
    /* While set, paint_hover_wave() skips tiles outside [lo,hi). */
    int wave_span_active,wave_span_lo,wave_span_hi;
    unsigned char wave_draw[ND_MAX_ITEMS],hover_visible[ND_MAX_ITEMS];
    unsigned char hover_tile_changed[ND_MAX_ITEMS];
    int hover_shelf_width,hover_shelf_height,hover_shelf_valid;
    struct BitMap *scaled_icon_bitmaps[ND_MAX_ITEMS];
    struct BitMap *scaled_icon_masks[ND_MAX_ITEMS];
    int scaled_icon_width[ND_MAX_ITEMS];
    int scaled_icon_height[ND_MAX_ITEMS];
    char datatype_icon_paths[ND_MAX_ITEMS][ND_COMMAND_LEN];
    struct NDILBM *brushes[ND_MAX_ITEMS];
    struct NDIconPicture *picture_sources[ND_MAX_ITEMS];
    struct BitMap *brush_bitmaps[ND_MAX_ITEMS];
    struct BitMap *brush_masks[ND_MAX_ITEMS];
    LONG brush_pens[ND_MAX_ITEMS][ND_ICON_MAX_COLORS];
    struct NDGraphHistory history[ND_MAX_ITEMS];
    struct Screen *screen;
    struct Window *window;
    struct MsgPort *ipc;
    struct MsgPort *control;
    struct MsgPort *drop_port;
    struct AppWindow *app_window;
    struct Hook drop_hook;
    int drop_zones;
    struct AppMenuItem *app_menu_items[ND_MAX_ITEMS];
    struct MsgPort *hotkey_port;
    CxObj *hotkey_broker;
    struct Window *popup;
    struct Window *tooltip;
    struct Window *caption_popup;
    int caption_dirty,caption_index,caption_x,caption_y;
    char caption_text[ND_TEXT_LEN];
    struct Window *feedback;
    struct Window *calendar;
    struct Window *about;
    struct Window *folder_popup;
    struct MsgPort *rexx;
    struct MsgPort *timer_port;
    struct timerequest *timer_req;
    struct MsgPort *input_port;
    struct IOStdReq *input_req;
    int input_open;
    int timer_open, timer_pending, timer_quanta;
    int drop_attach_ticks;
    int hidden, box_pending, box_target_hidden;
    /* Button events received while box_pending, replayed on completion. */
    struct NDDeferredButtons deferred_buttons;
    /* Pointer messages generated at or before this time predate the current
     * window geometry or activation (see nd_pointer_event_stale). */
    ULONG pointer_epoch_secs, pointer_epoch_micros;
    int orphan_press_ticks;
    /* Window origin before the pending box change: button events processed
     * while box_pending were generated relative to it. */
    int box_origin_x, box_origin_y;
    /* Housekeeping ticks spent waiting for IDCMP_CHANGEWINDOW; a MENUDOWN
     * received meanwhile opens its menu once the box change completes. */
    int box_pending_ticks, deferred_menu, box_late_ack, fallback_tick_phase;
    /* Edit-mode undo: the configuration before the last move or removal. */
    struct NDConfig *undo_config;
    int undo_kind,undo_keep;
    /* Profiles offered on the dock menu, listed when it opens. */
    char menu_profiles[ND_MENU_PROFILES][ND_PROFILE_LEN];
    int menu_profile_count;
    char undo_label[ND_LABEL_LEN];
    /* Another screen in front hides the dock completely (dock_timers.c):
     * tile paints wait and plugins are throttled as while hidden. */
    int covered;
    /* Startup artwork still loading in the background (dock_icons.c). */
    int icons_pending, icon_next;
    int idle_ticks;
    int eyes_timer_ticks, eyes_blink_counter, eyes_blink;
    int volume_timer_ticks;
    int reopen_timer_ticks;   /* Workbench-reopen retries, separate from meters */
    int running_timer_ticks;
    int watchdog_timer_ticks;
    int top_timer_ticks;
    unsigned char running_items[ND_MAX_ITEMS];
    int eyes_last_x[ND_MAX_ITEMS][2], eyes_last_y[ND_MAX_ITEMS][2];
    unsigned char eyes_last_valid[ND_MAX_ITEMS];

    struct BitMap *hover_desktop;
    int hover_desktop_width, hover_desktop_height; /* allocated size */
    ULONG desktop_scene_stamp;
    int desktop_scene_valid,desktop_scene_dirty,desktop_scene_age;
    int desktop_refresh_pending,desktop_ready_ms,desktop_probe_ms;
    int canvas_view,viewport_pending;
    struct NDRect viewport,window_box_target;
    int hover_headroom,hover_pointer,painting_shelf,painting_wave;
    int hover_pad_x,hover_pad_y,hover_axis_padding,hover_plan_ready,painting_item;
    struct NDHoverLayoutItem hover_layout[ND_MAX_ITEMS];
    struct NDRect hover_rects[ND_MAX_ITEMS],hover_shelf_rect;
    struct NDRect full_box, tab_box;
    int rexx_public;
    int hover_ticks;
    int tip_item;
    int popup_item, popup_hot, popup_network_online, popup_warp_present, popup_prism_present;
    int activate_popup_pending;
    ULONG popup_opened; /* queued dock activation may land after menu open */
    int press_item, press_x, press_y, drag_to, drag_after, is_drag;
    int grip_drag,grip_start,grip_position;
    int grip_offset,grip_origin,grip_length,grip_moved;
    unsigned int music_press_action,music_hover_action;
    int drop_preview, drop_preview_item, drop_preview_after;
    int edit_mode;
    int view_start, visible_count, layout_slots, layout_rows;
    int layout_columns, overflow;
    int feedback_ticks;
    int calendar_year, calendar_month;
    /* Drawer popup: entries of folder_path, navigable below folder_root. */
    struct NDFolderEntry *folder_entries;
    struct DiskObject **folder_icons;
    struct NDFolderLayout folder_layout;
    int folder_count, folder_hot, folder_offset, folder_tile, folder_more;
    int folder_view, folder_sort;   /* session choice, from FOLDER_VIEW/SORT */
    char folder_root[ND_COMMAND_LEN];
    char folder_path[ND_COMMAND_LEN];
    char folder_typed[16];
    ULONG folder_typed_seconds;
    int control_public;
    LONG colors[ND_COLOR_COUNT];
    unsigned char color_owned[ND_COLOR_COUNT];
    struct TextFont *dock_font;
    int dock_font_owned;
    int width;
    int height;
    int tile_width;
    int tile_height;
    int gap;
    int hover;
    int hover_pick_valid,hover_pick_x,hover_pick_y,hover_pick_index;
    /* Keyboard control (DOCK_HOTKEY, cursor keys): hover follows the keys
     * until the pointer moves; KEYBOARD_MOUSE_* is where it was. */
    int keyboard;
    int keyboard_mouse_x,keyboard_mouse_y;
    /* Tiles to repaint (a byte each: ND_MAX_ITEMS exceeds 32 bits). */
    unsigned char dirty_tiles[ND_MAX_ITEMS];
    unsigned char graph_step[ND_MAX_ITEMS];  /* a new sample to scroll in */
    ULONG graph_samples[ND_MAX_ITEMS];       /* samples received per tile */
    struct NDGraphView graph_view[ND_MAX_ITEMS];
    ULONG diagnostic_started;
    ULONG diagnostic_wakes;
    ULONG diagnostic_full_paints;
    ULONG diagnostic_tile_paints;
    ULONG diagnostic_hover_full,diagnostic_hover_partial,diagnostic_hover_skipped;
    ULONG diagnostic_updates;
    ULONG diagnostic_reclaimed_replies;
    ULONG diagnostic_unchanged_updates;
    ULONG diagnostic_coalesced_updates;
    ULONG diagnostic_reload_requests;
    ULONG diagnostic_reload_applied;
    ULONG diagnostic_reload_skipped;
    ULONG diagnostic_reload_failures;
    ULONG reload_ticket_issued;  /* two-stage reload (ND_CONTROL_RELOAD_BEGIN) */
    ULONG diagnostic_reload_largest_before,diagnostic_reload_largest_after;
    ULONG diagnostic_reload_free_low;
    UWORD diagnostic_icons_reused,diagnostic_icons_loaded;
    ULONG reload_ticket_done;
    LONG reload_done_result;
    ULONG diagnostic_reload_last_ticks;
    ULONG diagnostic_reload_total_ticks;
    ULONG diagnostic_plugin_updates[ND_MAX_ITEMS];
    ULONG diagnostic_plugin_last[ND_MAX_ITEMS];
    ULONG watchdog_retry_after[ND_MAX_ITEMS];
    UWORD watchdog_failures[ND_MAX_ITEMS];
    ULONG watchdog_restarts[ND_MAX_ITEMS];
    ULONG watchdog_total_restarts;
    int running;
    int restart_requested;
    int is_public;
    /* Asynchronous ARexx Launcher Actions: replies arrive on this port. */
    struct MsgPort *rexx_reply;
    int rexx_pending;
    struct NDRexxTrack rexx_track[ND_REXX_TRACK_MAX];
    /* Plugin wake-ups: the reply port's task from the latest exchange. */
    struct Task *plugin_task[ND_MAX_ITEMS];
    struct NDPluginSupportStats plugin_support[ND_MAX_ITEMS];
    char plugin_support_id[ND_MAX_ITEMS][ND_ID_LEN];
    ULONG plugin_support_stamp[ND_MAX_ITEMS];
    struct NDPluginSampler *plugin_sampler;
    ULONG plugin_cpu_samples,plugin_cpu_hits[ND_MAX_ITEMS],plugin_cpu_stamp;
    char plugin_cpu_id[ND_MAX_ITEMS][ND_ID_LEN];

    unsigned char plugin_woken[ND_MAX_ITEMS];
    /* The plugin published ND_WAKE_PORT_PREFIX<ID> for PLUGIN_TASK: wake it
     * only through that port.  0 = legacy plugin (task-pointer wake-up). */
    unsigned char plugin_wake_port[ND_MAX_ITEMS];
    /* ND_EVENT2_* waiting for the plugin's next ABI 5 exchange. */
    ULONG plugin_event2[ND_MAX_ITEMS];
    /* NET/ROADSHOW tiles: 0 unknown, 1 online, 2 offline (event hooks). */
    unsigned char net_state[ND_MAX_ITEMS];
    /* Per-tile ID classes for the plugin packet path, recomputed only when
     * the tile's ID differs from the one they were derived from. */
    char ipc_kind_id[ND_MAX_ITEMS][ND_ID_LEN];
    unsigned char ipc_kind[ND_MAX_ITEMS];
    /* Parsed Eyes/Clock options for the paint and timer paths, which used to
     * search the option table up to nine times per animated frame.  Filled
     * on first use; invalidate_settings_cache() whenever CONF is replaced. */
    struct {
        int valid;
        int eyes_poll_ticks,eyes_sensitivity,eyes_radius;
        int eyes_show_label,eyes_blink,eyes_sclera,eyes_pupil;
        int clock_seconds,clock_analog;
    } opts;
    /* EDIT_CONFIG: the newest unsaved configuration.  SAVE_SNAPSHOT: the
     * buffer being written; it is the edit buffer itself until a later edit
     * replaces that, and is lent (not copied) to SaveConfig when SAVE_LENT. */
    struct NDConfig *edit_config,*save_snapshot;
    /* ARexx SET/THEME queue their edit in EDIT_CONFIG and apply it to the
     * live dock once the script pauses (REXX_APPLY_TICKS timer quanta), so a
     * run of SETs costs one reload instead of one each. */
    int rexx_apply_pending,rexx_apply_ticks,rexx_apply_failures;
    int save_lent;
    struct MsgPort *save_port;
    char save_port_name[64];
    struct NDSaveState save_state;
    ULONG save_started_at,save_last_ticks,save_total_ticks;
    ULONG save_env_ticks,save_envarc_ticks;
    int save_claimed,save_env_ok,save_envarc_ok;
    int save_quit_refused; /* a quit was held back by an unsaved change */
    struct Task *write_owner;
    ULONG write_deadline;
    ULONG diagnostic_live_updates,diagnostic_window_rebuilds;
    UWORD diagnostic_update_mode;
    /* Optional screennotify.library Workbench close/reopen support. */
    struct MsgPort *sn_port;
    APTR sn_handle;
    int screen_closed;
    /* ARexx SUSPEND: windows closed on request (a Workbench screen mode
     * change); reopened by RESUME, by screennotify, or after the timeout. */
    int suspended,suspend_ticks;
    int from_workbench;
    /* Removable-media tiles (runtime only, appended after configured items). */
    struct NDMediaVolume media[ND_MEDIA_MAX];
    int media_count;
    int media_scan_ticks;
    char system_volume[ND_MEDIA_NAME_LEN];
    UBYTE folder_icon_tried[ND_FOLDER_MAX];
    /* Retired taskbar tiles remain readable for compatibility. */
    struct Window *last_raised;
    char directory[256];
};
/* Which text pool an item's offsets refer to: the background-action copy
 * when the item lives there, otherwise the live configuration.  There is
 * one dock per process; TEXT_OWNER is set as soon as it is allocated. */
#define ITEM_TEXT(item,which) dock_text((item),(which))
#define ND_ICON_PATH_LEN 520

/* ITEM_TEXT() above (src/novadock.c). */
const char *dock_text(const struct NDItem *item,int which);
/* Owner of the text pool that ITEM_TEXT() reads (src/novadock.c). */
extern const struct NDDock *text_owner;

/* Icons kept across a reload (dock_icons.c, used by dock_reload.c). */
struct NDIconStash {
    struct DiskObject *icons[ND_MAX_ITEMS];
    struct NDILBM *brushes[ND_MAX_ITEMS];
    struct NDIconPicture *pictures[ND_MAX_ITEMS];
    ULONG fingerprint[ND_MAX_ITEMS];
    UBYTE usable[ND_MAX_ITEMS];
    int live;
    struct BitMap *scaled[ND_MAX_ITEMS], *scaled_masks[ND_MAX_ITEMS];
    struct BitMap *brush_maps[ND_MAX_ITEMS], *brush_masks[ND_MAX_ITEMS];
    int widths[ND_MAX_ITEMS], heights[ND_MAX_ITEMS];
    char datatype_paths[ND_MAX_ITEMS][ND_COMMAND_LEN];
};

/* Functions shared between the dock's source files. */
/* novadock.c */
APTR alloc_data(ULONG bytes);
const char *action_text_pool(const struct NDItem *items,int count);
struct NDItem *copy_background_actions(const struct NDConfig *cfg, int *out_count);
ULONG diagnostic_ticks(void);
void reset_diagnostic_counters(struct NDDock *dock);
int reopen_dock_ui(struct NDDock *dock);
void close_dock_ui(struct NDDock *dock);
/* Run the configured HOOK commands for EVENT (dock_launch.c). */
void dock_alert(struct NDDock *dock,const char *source,const char *title,const char *text,unsigned int severity);
void fire_hook(struct NDDock *dock,const char *event,const char *detail);
/* dock_draw.c */
void prepare_transparency(struct NDDock *dock);
void prepare_dock_background(struct NDDock *dock);
void desktop_scene_tick(struct NDDock *dock,int milliseconds);
void desktop_startup_tick(struct NDDock *dock,int milliseconds);
int refresh_dock_desktop(struct NDDock *dock);
void alloc_colors(struct NDDock *dock);
void free_colors(struct NDDock *dock);
void release_dock_font(struct NDDock *dock);
void apply_dock_font(struct NDDock *dock);
ULONG pen(struct NDDock *dock, int color);
int vertical(const struct NDDock *dock);
int item_visible(const struct NDDock *dock, int index);
struct Task *launcher_task(const struct NDItem *item);
struct Task *launcher_port_task(const struct NDItem *item,int *present);
struct Window *launcher_window(struct NDDock *dock, const struct NDItem *item, struct Task *task, struct Task *port_task);
int launcher_running(struct NDDock *dock,const struct NDItem *item);
void refresh_running_items(struct NDDock *dock);
void slot_coords(struct NDDock *dock, int slot, int *x, int *y);
void tile_dimensions(struct NDDock *dock,int index,int *w,int *h);
void tile_coords(struct NDDock *dock, int index, int *x, int *y);
int dock_grip_rect(struct NDDock *dock,struct NDRect *rect);
int hovered_grip(struct NDDock *dock,int mx,int my);
ULONG hover_cache_bytes(struct NDDock *dock);
int hover_cache_reserve(struct NDDock *dock,ULONG bytes,int widget,int icon);
int eyes_index(const struct NDDock *dock);
/* Cached Eyes/Clock settings (see NDDock.opts). */
void invalidate_settings_cache(struct NDDock *dock);
void load_settings_cache(struct NDDock *dock);
#define DOCK_OPTS(d) (((d)->opts.valid ? (void)0 : load_settings_cache(d)),&(d)->opts)
void eyes_positions(struct NDDock *dock,int index, int xp[2],int yp[2],int *rx,int *ry,int *pr);
int format_ram_tooltip(const char *value,char *chip_text, unsigned int chip_size,char *fast_text, unsigned int fast_size);
int queue_music_control(struct NDDock *dock,int index,int mx,int my);
int update_music_control_hover(struct NDDock *dock,int index,int mx,int my);
int begin_music_control(struct NDDock *dock,int index,int mx,int my);
void drag_music_control(struct NDDock *dock,int mx);
void finish_music_control(struct NDDock *dock,int mx,int my);
void draw_tile(struct NDDock *dock, int index);
void draw_all(struct NDDock *dock);
void draw_hover(struct NDDock *dock);
void redraw_changed_tiles(struct NDDock *dock, int a, int b, int c);
void mark_tile_dirty(struct NDDock *dock, int index);
void refresh_volume_meters(struct NDDock *dock);
void process_volume_meters(struct NDDock *dock);
void flush_dirty_tiles(struct NDDock *dock);
void draw_graph_step(struct NDDock *dock,int index);
/* dock_popups.c */
void close_calendar(struct NDDock *dock);
void open_calendar(struct NDDock *dock,int index);
void calendar_events(struct NDDock *dock);
void close_about(struct NDDock *dock);
void open_about(struct NDDock *dock);
void about_events(struct NDDock *dock);
void close_folder_popup(struct NDDock *dock);
void open_folder_popup(struct NDDock *dock,int index);
void folder_popup_events(struct NDDock *dock);
/* dock_window.c */
int open_window(struct NDDock *dock);
void update_live_window(struct NDDock *dock);
void update_grip_window(struct NDDock *dock);
void move_dock_grip(struct NDDock *dock,int coordinate);
void cancel_dock_grip(struct NDDock *dock);
void remap_brush_colors(struct NDDock *dock);
void remap_item_brush_colors(struct NDDock *dock,int index);
int hovered_tile(struct NDDock *dock, int mx, int my);
int pointer_hover_target(struct NDDock *dock,int mx,int my);
int hovered_dock_row(struct NDDock *dock,int mx,int my);
int hovered_scroll(struct NDDock *dock,int mx,int my);
void scroll_view(struct NDDock *dock,int direction);
/* dock_launch.c */
int launch_command_at(const char *cmd,const char *directory,ULONG stack);
int launch_command(const char *cmd);
int launch_command_console(const char *cmd);
ULONG icon_stack(const char *path);
void get_program_dir(struct NDDock *dock);
int launch_companion(struct NDDock *dock, const char *name, const char *args);
int has_iff_extension(const char *path);
int has_filename_extension(const char *path);
int has_info_extension(const char *path);
APTR quiet_requesters(void);
void restore_requesters(APTR old);
/* dock_icons.c */
void stash_live_icon_sources(struct NDDock *dock,struct NDIconStash *stash);
void stash_icon_sources(struct NDDock *dock,struct NDIconStash *stash);
void free_icon_stash(struct NDIconStash *stash);
void load_icons_reusing(struct NDDock *dock,struct NDIconStash *stash, const struct NDConfig *previous);
void load_icons(struct NDDock *dock);
void dock_window_box(struct NDDock *dock,int hidden,struct NDRect *box);
int sync_dock_viewport(struct NDDock *dock);
int ensure_desktop_snapshot(struct NDDock *dock,struct BitMap *friend_bitmap);
void release_desktop_snapshot(struct NDDock *dock);
void note_pointer_epoch(struct NDDock *dock,ULONG secs,ULONG micros);
void note_pointer_epoch_now(struct NDDock *dock);
void apply_dock_viewport(struct NDDock *dock);
void map_plugin_artwork(struct NDDock *dock);
void unmap_plugin_artwork(struct NDDock *dock);
void load_plugin_artwork(struct NDDock *dock,int index);
/* Cached artwork for a tile; pointer hover only shows a tooltip. */
int tile_icon_art(const struct NDDock *dock,int index,struct BitMap **bitmap,
                  struct BitMap **mask,int *width,int *height);
int load_pending_icons(struct NDDock *dock,int budget);
int planar_icon_pixel(const struct Image *image,int x,int y);
int magnify_icon(struct NDDock *dock,int index,struct BitMap **bitmap,struct BitMap **mask,
                 int *width,int *height,int zoom_width,int zoom_height);
void build_scaled_icon_bitmaps(struct NDDock *dock);
void build_brush_bitmaps(struct NDDock *dock);
void release_icons(struct NDDock *dock);
void release_hover_graphics(struct NDDock *dock);
/* dock_input.c */
void detach_appmenus(struct NDDock *dock);
void detach_appwindow(struct NDDock *dock);
void attach_appwindow(struct NDDock *dock);
void refresh_drop_zones(struct NDDock *dock);
void stop_hotkeys(struct NDDock *dock);
int start_hotkeys(struct NDDock *dock);
void rebuild_hotkeys(struct NDDock *dock);
void process_hotkeys(struct NDDock *dock);
int build_wb_path(const struct WBArg *arg, char *out, int out_size, int *kind);
void dropped_icon_path(const char *path, int kind, char *out, int out_size);
void open_drawer(const char *path);
/* dock_plugins.c */
void sync_plugin_twins(struct NDDock *dock,int source);
int launch_archive_helper(struct NDDock *dock,int index,const char *archive);
int launch_archive_drop(struct NDDock *dock,int index,const struct AppMessage *message);
int launch_plugin_helper(struct NDDock *dock,int index,const char *verb,
                         const char *path);
int is_trash_tile(const struct NDDock *dock,int index);
void open_trash(struct NDDock *dock);
void start_plugins(struct NDDock *dock, const struct NDConfig *previous);
void remember_christmas(const struct NDConfig *cfg);
void christmas_startup(struct NDDock *dock);
void plugin_watchdog_tick(struct NDDock *dock,int quantum);
struct NDItem *item_by_id(struct NDDock *dock, const char *id);
void process_ipc(struct NDDock *dock, int quitting);
int register_port(struct NDDock *dock);
void remove_port(struct NDDock *dock);
int task_exists(struct Task *task);
void wake_pending_plugins(struct NDDock *dock);
/* dock_timers.c */
void stop_timer(struct NDDock *dock);
int start_input_qualifier(struct NDDock *dock);
void stop_input_qualifier(struct NDDock *dock);
int start_timer(struct NDDock *dock);
int inside_rect(const struct NDRect *r, int x, int y);
void set_hidden(struct NDDock *dock, int hide);
void show_dock(struct NDDock *dock);
void hide_dock(struct NDDock *dock);
void always_on_top_tick(struct NDDock *dock,int quantum);
void update_drop_preview(struct NDDock *dock);
void hover_tick(struct NDDock *dock,int tip_delay);
void update_hover_pointer(struct NDDock *dock);
void process_timer(struct NDDock *dock);
void close_tooltip(struct NDDock *dock);
void close_caption_popup(struct NDDock *dock);
void visible_tile_rect(struct NDDock *dock,int index,struct NDRect *rect);
void update_caption_popup(struct NDDock *dock);
void close_feedback(struct NDDock *dock);
void show_feedback(struct NDDock *dock,const char *text,int error);
void find_system_volume(struct NDDock *dock);
int scan_media(struct NDDock *dock,struct NDMediaVolume *out);
/* dock_reload.c */
LONG reload_configuration(struct NDDock *dock);
/* Apply ARexx SETs queued in EDIT_CONFIG now (no-op when none). */
void flush_rexx_settings(struct NDDock *dock);
#define ND_REXX_APPLY_QUIET 1   /* timer quanta (~200 ms) without a SET */
void process_control(struct NDDock *dock, int quitting);
void config_dir(const char *path);
/* On success both take ownership of *FULL (an AllocVec'd NDConfig) and set
 * it to NULL; on failure the caller still owns it. */
int save_and_reload(struct NDDock *dock, struct NDConfig **full);
void capture_row_layout(struct NDDock *dock,struct NDConfig *full);
int drop_after_tile(struct NDDock *dock,int index,int mx,int my);
int do_move(struct NDDock *dock,int from,int to,int after);
int do_move_to_empty_space(struct NDDock *dock,int from,int row);
int remove_dragged_item(struct NDDock *dock, int visible);
/* dock_menu.c */
void close_popup(struct NDDock *dock);
void open_popup(struct NDDock *dock, int item, int x, int y);
int complete_window_box(struct NDDock *dock);
void fallback_recovery_tick(struct NDDock *dock);
void kick_animation_timer(struct NDDock *dock);
#define ND_UNDO_NONE 0
#define ND_UNDO_MOVE 1
#define ND_UNDO_REMOVE 2
int undo_last_edit(struct NDDock *dock);
int list_profiles(char names[][ND_PROFILE_LEN],int max,const char *except);
int save_profile(struct NDDock *dock,const char *name);
int use_profile(struct NDDock *dock,const char *name);
int delete_profile(struct NDDock *dock,const char *name);
void release_undo(struct NDDock *dock);
void process_rexx_replies(struct NDDock *dock,int report);
void stop_rexx_replies(struct NDDock *dock);
int queue_launcher(struct NDDock *dock,const struct NDItem *item);
void process_launch_jobs(struct NDDock *dock);
unsigned long launch_job_signals(void);
void close_launch_jobs(void);
void process_icon_jobs(struct NDDock *dock);
unsigned long icon_job_signals(void);
unsigned int active_icon_jobs(void);
unsigned int active_launch_jobs(void);
int launch_action(struct NDDock *dock,const struct NDItem *item);
int launch_default_music_application(struct NDDock *dock);
void folder_icons_tick(struct NDDock *dock);
void process_folder_jobs(struct NDDock *dock);
unsigned long folder_job_signals(void);
void reload_artwork(struct NDDock *dock);
void launch_item(struct NDDock *dock, int index);
void request_dopus_action(struct NDDock *dock,int index);
void set_edit_mode(struct NDDock *dock,int on);
void popup_events(struct NDDock *dock);
/* dock_events.c */
int init_settings_writer(struct NDDock *dock);
void process_settings_writer(struct NDDock *dock);
void settings_save_tick(struct NDDock *dock,int quantum,int force);
void release_saved_settings_edit(struct NDDock *dock);
int finish_settings_saves(struct NDDock *dock);
void close_settings_writer(struct NDDock *dock);
void accept_settings_save(struct NDDock *dock,struct NDConfig *copy);
int queue_settings_save(struct NDDock *dock,struct NDConfig **full);
int load_edit_configuration(struct NDDock *dock,struct NDConfig *full);
void persist_folder_choice(struct NDDock *dock);
void dropped_icons(struct NDDock *dock);
void keyboard_begin(struct NDDock *dock);
void window_events(struct NDDock *dock);
/* dock_arexx.c */
int rexx_add_launcher(struct NDDock *dock,const char *path);
void start_rexx(struct NDDock *dock);
void stop_rexx(struct NDDock *dock);
void process_rexx(struct NDDock *dock);
int receive_thumbnail(struct NDDock *dock,int index,const struct NDThumbnail *picture);
void release_thumbnail(struct NDDock *dock,int index);
struct BitMap *thumbnail_bitmap(struct NDDock *dock,int index,int tiny);
ULONG plugin_sampler_signal(struct NDDock *);
ULONG plugin_sampler_remaining(struct NDDock *);
int plugin_sampler_start(struct NDDock *);
void plugin_sampler_service(struct NDDock *);
void plugin_sampler_close(struct NDDock *);
#endif
