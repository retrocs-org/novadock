#ifndef ND_CONFIG_H
#define ND_CONFIG_H
/* ANSI-C data/config layer; deliberately independent from AmigaOS. */
/* The catalogue and active dock share one conservative Classic-Amiga limit.
 * Overflow paging keeps all active entries reachable on smaller screens. */
#define ND_MAX_ITEMS 48
#define ND_MAX_DOCK_ITEMS ND_MAX_ITEMS
/* Script and Command tiles keep several settings each; 128 leaves room for
 * a few of them beside the bundled plugins' stored values. */
#define ND_MAX_OPTIONS 128
#define ND_MAX_OPTION_DEFS 32
#define ND_OPT_VALUE_LEN 96
#define ND_OPT_KEY_LEN 32
#define ND_LABEL_LEN 32
#define ND_COMMAND_LEN 240
#define ND_HOTKEY_LEN 64
#define ND_MATCH_LEN 64
/* Unrecognised KEY=value lines kept verbatim for forward compatibility. */
#define ND_MAX_UNKNOWN 8
#define ND_UNKNOWN_LEN 160
#define ND_ITEM_LAUNCH 1
#define ND_ITEM_PLUGIN 2
#define ND_ITEM_DRAWER 3
#define ND_ITEM_SPACER 4
#define ND_ITEM_GROUP 5
/* Pooled per-item texts (see struct NDItem.text and NDConfig.text_pool). */
#define ND_TEXT_ICON 0          /* icon or artwork path       ND_COMMAND_LEN */
#define ND_TEXT_ARGS 1          /* launch arguments           ND_COMMAND_LEN */
#define ND_TEXT_WORKDIR 2       /* start-in directory         ND_COMMAND_LEN */
#define ND_TEXT_HOTKEY 3        /* commodities hot key        ND_HOTKEY_LEN  */
#define ND_TEXT_MATCH_TASK 4    /* running-app task name      ND_MATCH_LEN   */
#define ND_TEXT_MATCH_WINDOW 5  /* running-app window pattern ND_MATCH_LEN   */
#define ND_TEXT_MATCH_PORT 6    /* running-app ARexx port     ND_MATCH_LEN   */
#define ND_TEXT_COUNT 7
/* Shared by every item's optional texts.  Each item used to reserve 976
 * bytes for them whether used or not (47 KB per configuration); real
 * configurations need a few KB in total. */
#define ND_TEXT_POOL_SIZE 12288
/* Runtime-only tiles appended after the configured items. */
#define ND_TRANSIENT_NONE 0
#define ND_TRANSIENT_MEDIA 1 /* inserted removable medium (drawer tile) */
#define ND_TRANSIENT_TASK 2  /* running application without a launcher */
#define ND_SIDE_BOTTOM 0
#define ND_SIDE_TOP 1
#define ND_SIDE_LEFT 2
#define ND_SIDE_RIGHT 3
#define ND_POSITION_START 0
#define ND_POSITION_CENTER 1
#define ND_POSITION_END 2
#define ND_POSITION_COUNT 3
#define ND_FONT_WORKBENCH 0
#define ND_FONT_TOPAZ 1
#define ND_FONT_BOLD 2
#define ND_FONT_COUNT 3
#define ND_LABEL_ALWAYS 0
#define ND_LABEL_HOVER 1
#define ND_LABEL_HIDDEN 2
#define ND_LABEL_COUNT 3
#define ND_ITEM_SIZE_NORMAL 0
#define ND_ITEM_SIZE_TINY 1
#define ND_ITEM_SIZE_COUNT 2
#define ND_REFRESH_LOW_MEMORY 0
#define ND_REFRESH_SMOOTH 1
#define ND_REFRESH_COUNT 2
#define ND_ROWS_MIN 1
#define ND_ROWS_MAX 3
#define ND_THEME_MOCHA 0
#define ND_THEME_LIGHT 1
#define ND_THEME_WORKBENCH 2
#define ND_THEME_GRAPHITE 3
#define ND_THEME_HIGH_CONTRAST 4
#define ND_THEME_COPPER 5
#define ND_THEME_OCEAN 6
#define ND_THEME_EMERALD 7
#define ND_THEME_NEXTSTEP 8
#define ND_THEME_BEOS 9
#define ND_THEME_NORD 10
#define ND_THEME_VAPOR 11
#define ND_THEME_COUNT 12
#define ND_THEME_ID_LEN 32
#define ND_SHAPE_ROUNDED 0
#define ND_SHAPE_SQUARE 1
#define ND_SHAPE_CHAMFER 2
#define ND_SHAPE_MINIMAL 3
#define ND_SHAPE_COUNT 4
#define ND_CLICK_LAUNCH 0
#define ND_CLICK_MENU 1
#define ND_LAUNCH_AUTO 0
#define ND_LAUNCH_WORKBENCH 1
#define ND_LAUNCH_CLI 2
#define ND_LAUNCH_AREXX 3
#define ND_LAUNCH_MODE_COUNT 4
#define ND_SPACER_COLOR_SUBTLE 0
#define ND_SPACER_COLOR_ACCENT 1
#define ND_SPACER_COLOR_SECONDARY 2
#define ND_SPACER_COLOR_TEXT 3
#define ND_SPACER_COLOR_COUNT 4

struct NDItem {
    int kind;
    char label[ND_LABEL_LEN];
    char caption[ND_LABEL_LEN];
    char command[ND_COMMAND_LEN];
    /* Optional launcher texts live in the configuration's text pool; these
     * are offsets into it (0 = empty).  Read them with nd_item_text() and
     * change them only with nd_item_set_text(s).  ND_TEXT_MATCH_*: optional
     * running-application identities; empty fields retain automatic
     * command-basename matching, and window patterns accept '*' and '?'. */
    unsigned short text[ND_TEXT_COUNT];
    char id[20];
    char value[32];
    unsigned short meter;
    unsigned short clicks;
    unsigned short media_capabilities; /* transient ABI v3 player controls */
    unsigned short media_state;
    unsigned short media_volume;
    unsigned short media_event;
    unsigned short media_volume_request;
    int online;
    int settings_dirty;   /* pending options-changed event for running plugin */
    int graph;            /* 1: sparkline; 0: conventional progress meter */
    int enabled;          /* plugin tile enabled; disabled plugins remain editable */
    int multiple_instances; /* plugin may have several tiles sharing one process */
    int launch_mode;      /* ND_LAUNCH_*; shared by dock, hotkey and Tools menu */
    int tools_menu;       /* expose this action in Workbench's Tools menu */
    int show_volume_meter;/* drawers: sample and show their volume's usage meter */
    int spacer_width;     /* 0: half the normal tile; otherwise 20..140 pixels */
    int spacer_line;      /* 1: draw the divider, 0: use empty space only */
    int spacer_line_width;/* divider thickness in pixels, 1..8 */
    int spacer_line_color;/* one of ND_SPACER_COLOR_*; follows the dock theme */
    int transient;        /* ND_TRANSIENT_*: runtime-only tile, never saved */
    char badge[8];        /* runtime plugin badge (ND_MSG_BADGE); never saved */
};
/* All dock and plugin settings reside in the SAME novadock.cfg file.
 * OPTION.<plugin-id>.<key>=value entries are preserved across editor saves.
 * Third-party descriptors (OPTIONDEF lines) are also stored in that file.
 */
/* HOOK=EVENT|command: a command run when the dock event happens. */
#define ND_MAX_HOOKS 8
#define ND_HOOK_EVENT_LEN 16
#define ND_HOOK_COMMAND_LEN 96
struct NDHook { char event[ND_HOOK_EVENT_LEN]; char command[ND_HOOK_COMMAND_LEN]; };
struct NDOption { char id[20]; char key[ND_OPT_KEY_LEN]; char value[ND_OPT_VALUE_LEN]; };
struct NDOptionDef { char id[20]; char key[ND_OPT_KEY_LEN]; char label[32];
    char type[12]; char choices[96]; char default_value[ND_OPT_VALUE_LEN]; };
struct NDConfig {
    int option_count;
    int def_count;
    struct NDOption options[ND_MAX_OPTIONS];
    struct NDOptionDef defs[ND_MAX_OPTION_DEFS];
    int side;
    int position; /* alignment along the selected screen edge */
    int font_mode; /* Workbench, fixed Topaz 8, or Workbench bold */
    int label_mode; /* launcher/drawer names: always, hover, or hidden */
    int item_size; /* normal detail, or half-size compact tile rendering */
    int theme;
    /* Stable folder identifier below Themes/.  The numeric value remains for
     * source compatibility with the bundled catalogue. */
    char theme_id[ND_THEME_ID_LEN];
    int shape;
    int tile_width;
    int tile_height;
    int gap;
    int rows; /* requested horizontal rows; vertical docks remain one column */
    /* Optional manual horizontal distribution. Counts refer to visible,
     * persistent items in row order; inactive keeps legacy auto-packing. */
    int row_layout;
    int row_counts[ND_ROWS_MAX];
    int left_click; /* default: open/launch; optional popup */
    int tooltips; /* draw a small hover label after a short delay */
    int always_on_top; /* periodically keep the borderless dock layer in front */
    int running_apps; /* show running marks and bring an existing app forward */
    int refresh_mode; /* low-memory simple refresh, or smoother smart refresh */
    int auto_hide; /* collapse to a reveal tab when pointer leaves */
    int hide_delay; /* idle seconds (1..60), checked by timer.device */
    int removable_media; /* show inserted floppies/cards/USB volumes as tiles */
    /* Commodities hot key for keyboard control of the dock ("" = none),
     * for example "ctrl alt d". */
    char dock_hotkey[ND_HOTKEY_LEN];
    int running_section; /* legacy config field; always disabled */
    int folder_view;     /* drawer popups: 0 icon grid, 1 list (ND_FOLDER_VIEW_*) */
    int folder_sort;     /* drawer popups: ND_FOLDER_SORT_* */
    int hook_count;
    struct NDHook hooks[ND_MAX_HOOKS];
    int count;
    struct NDItem items[ND_MAX_ITEMS];
    int unknown_count;   /* preserved lines; never affect the running dock */
    int unknown_dropped; /* unknown lines that did not fit and were dropped */
    char unknown[ND_MAX_UNKNOWN][ND_UNKNOWN_LEN];
    /* Text pool for NDItem.text offsets.  text_pool[0] is always '\0', so a
     * zeroed configuration is valid; text_used counts bytes in use. */
    unsigned int text_used;
    char text_pool[ND_TEXT_POOL_SIZE];
};
/* Pooled item texts.  Setters always append (never rewrite in place), so
 * copies of an item made within the same configuration stay valid until the
 * pool is compacted; values longer than the field's limit are truncated
 * like the former fixed fields.  nd_item_set_texts() changes all of its
 * fields or none of them.  0 = the pool is full even after compacting. */
const char *nd_pool_text(const char *pool, unsigned short offset);
#define nd_item_text(cfg,item,which) \
    nd_pool_text((cfg)->text_pool,(item)->text[(which)])
unsigned int nd_text_limit(int which);
int nd_item_set_text(struct NDConfig *cfg, struct NDItem *item, int which,
                     const char *value);
int nd_item_set_texts(struct NDConfig *cfg, struct NDItem *item, int count,
                      const int *which, const char *const *values);
/* Drops unreferenced texts.  EXTRA is an item being built beyond count
 * (or NULL); its offsets are kept and updated too. */
void nd_text_pool_compact(struct NDConfig *cfg, struct NDItem *extra);
/* Bound-checked option access and editable plugin descriptors. */
const char *nd_option_get(const struct NDConfig *cfg, const char *id, const char *key);
int nd_options_changed(const struct NDConfig *old, const struct NDConfig *next, const char *id);
int nd_option_set(struct NDConfig *cfg, const char *id, const char *key, const char *value);
int nd_option_define(struct NDConfig *cfg, const char *id, const char *key,
                     const char *label, const char *type, const char *choices,
                     const char *default_value);
/* Safe launcher editing API, used by NovaDockLauncherPrefs and unit tests. */
int nd_add_launcher(struct NDConfig *config, int kind, const char *name,
                    const char *target, const char *icon);
int nd_add_spacer(struct NDConfig *config, const char *name);
int nd_edit_launcher(struct NDConfig *config, int index, int kind,
                     const char *name, const char *target, const char *icon);
int nd_set_launcher_action(struct NDConfig *config, int index, int mode,
                           const char *arguments, const char *work_directory,
                           const char *hotkey, int tools_menu);
int nd_set_launcher_match(struct NDConfig *config, int index,
                          const char *task_name,const char *window_pattern,
                          const char *arexx_port);
int nd_remove_launcher(struct NDConfig *config, int index);
/* Drop every definition, option schema and value of plugin ID; refused (0)
 * while one of its tiles is enabled in the dock. */
int nd_remove_plugin(struct NDConfig *config, const char *id);
int nd_set_item_enabled(struct NDConfig *config, int index, int enabled);
/* Plugins normally have one dock tile.  A manifest may opt into repeated
 * tiles; those tiles share the plugin process, live state and settings. */
int nd_plugin_allows_multiple(const struct NDItem *item);
int nd_add_plugin_instance(struct NDConfig *config, int index);
int nd_move_full_item(struct NDConfig *config, int from, int to);
/* Operations below use VISIBLE indices, skipping disabled plugin entries.
 * They preserve every disabled entry and work directly on full config files. */
int nd_visible_to_full(const struct NDConfig *cfg, int visible);
int nd_move_visible(struct NDConfig *cfg, int from, int to);
int nd_move_visible_to_row(struct NDConfig *cfg,int from,int to,int row);
int nd_remove_visible_launcher(struct NDConfig *cfg, int visible);
/* Dragging a visible launcher out deletes it. Dragging a plugin out disables
 * it so its settings remain available in Preferences. */
int nd_remove_visible_item(struct NDConfig *cfg, int visible);
int nd_insert_launcher_visible(struct NDConfig *cfg, int visible, int kind,
                               const char *name, const char *target, const char *icon);
int nd_insert_spacer_visible(struct NDConfig *cfg, int visible, const char *name);
void nd_label_from_path(const char *path, char *label, unsigned int size);
/* A dropped Workbench volume is represented by a drawer target ending in a
 * colon (for example System:). Drawers inside a volume contain a path after
 * the colon and deliberately do not qualify. */
int nd_item_is_volume(const struct NDItem *item);
/* Every drawer resolves to a backing volume and may display its usage. */
int nd_item_supports_space_meter(const struct NDItem *item);
/* Filesystem-backed application groups reuse the drawer popup, but are kept
 * distinct so Preferences can describe and edit them as groups. */
int nd_item_is_group(const struct NDItem *item);
unsigned int nd_used_percent(unsigned long used, unsigned long total);
void nd_default_config(struct NDConfig *config);
const char *nd_theme_name(int theme);
const char *nd_config_theme_id(const struct NDConfig *config);
int nd_config_set_theme(struct NDConfig *config,const char *theme_id);
const char *nd_shape_name(int shape);
const char *nd_position_name(int position);
const char *nd_font_name(int mode);
const char *nd_label_mode_name(int mode);
const char *nd_item_size_name(int mode);
const char *nd_refresh_mode_name(int mode);
const char *nd_spacer_color_name(int color);
const char *nd_launch_mode_name(int mode);
int nd_parse_line(struct NDConfig *config, const char *line);
/* Store a well-formed but unrecognised KEY=value line (returns 0 only when
 * the key itself is malformed). Used by nd_parse_line. */
int nd_preserve_unknown(struct NDConfig *config,const char *key,const char *value);
int nd_load_config(struct NDConfig *config, const char *filename);
/* In-memory on/off filtering for the runtime dock; editor retains definitions. */
int nd_compact_enabled(struct NDConfig *config);
/* Compare persistent runtime inputs while ignoring live captions/meters/events. */
int nd_item_runtime_equal(const char *pool_a,const struct NDItem *a,
                          const char *pool_b,const struct NDItem *b);
/* Window/font/palette compatibility for an in-place item/layout update.
 * The caller also compares the resolved theme and fitted tile dimensions. */
int nd_config_can_update_live(const struct NDConfig *a,const struct NDConfig *b);
int nd_config_runtime_equal(const struct NDConfig *a,const struct NDConfig *b);
/* Dock event names for HOOK= lines; returns 1 for a known name. */
int nd_hook_event_valid(const char *event);
/* Stage in the destination directory, then replace by rename. Filesystems
 * that cannot rename over an existing file use a recoverable backup swap;
 * the destination is never rewritten in place. */
int nd_write_config(const struct NDConfig *config, const char *filename);
/* ENV: session, then ENVARC: permanent, with built-in defaults as fallback. */
int nd_load_effective(struct NDConfig *config);
/* Fold legacy Roadshow tiles into the unified Network tile on effective load. */
void nd_upgrade_network_config(struct NDConfig *config);
/* Path selected by the most recent nd_load_effective call in this process. */
const char *nd_effective_config_source(void);
/* Write that path to TARGET for plugin processes (removed for built-in
 * defaults). Only the running dock calls this. */
int nd_publish_config_source(const char *target);
int nd_config_low_resource(struct NDConfig *cfg);
#endif
