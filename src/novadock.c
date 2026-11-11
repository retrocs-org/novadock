/* NovaDock - a dock for Classic AmigaOS Workbench.
 * Core: start-up, main loop, shutdown, library bases and diagnostics counters.
 * The rest of the dock lives in src/dock_*.c (see src/dock_internal.h). */
#include "dock_internal.h"

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *IconBase = NULL;
struct Library *WorkbenchBase = NULL;
struct RxsLib *RexxSysBase = NULL;
struct Device *InputBase = NULL;
struct Library *CxBase = NULL;
struct Library *DataTypesBase = NULL;
struct Library *ScreenNotifyBase = NULL;
const char nd_version[]=ND_VER_STRING("NovaDock");
long __stack=32768;
const struct NDDock *text_owner=NULL;

/* Defined below, private to this file. */
static int is_background_action(const struct NDConfig *cfg, const struct NDItem *item);
static void reset_diagnostics(struct NDDock *dock);
static LONG ask_user(const char *body,const char *gadgets);
static void report_problem(const char *body);
static void start_screennotify(struct NDDock *dock);
static void stop_screennotify(struct NDDock *dock);
static void process_screennotify(struct NDDock *dock);
static void run_loop(struct NDDock *dock);
static int dock_ports_present(void);
static int quit_running_dock(void);
static int is_quit_argument(const char *arg);

const char *dock_text(const struct NDItem *item,int which)
{
    const struct NDDock *d=text_owner;
    if (!d || !item) return "";
    if (d->action_items && item>=d->action_items &&
        item<d->action_items+d->action_count)
        return nd_pool_text(action_text_pool(d->action_items,d->action_count),
                            item->text[which]);
    return nd_pool_text(d->conf.text_pool,item->text[which]);
}


/* Prefer Fast RAM for large CPU-only state.  The public-memory fallback keeps
 * NovaDock usable on chip-only machines, while avoiding scarce display/DMA
 * memory on expanded systems. */
APTR alloc_data(ULONG bytes)
{
    APTR memory=AllocVec(bytes,MEMF_FAST|MEMF_CLEAR);
    if (!memory) memory=AllocVec(bytes,MEMF_PUBLIC|MEMF_CLEAR);
    return memory;
}

/* Hot-key and Tools-menu actions outlive the configuration they came from,
 * so they are copied with their own text pool, stored right after the
 * items in the same allocation (see action_text_pool()). */
const char *action_text_pool(const struct NDItem *items,int count)
{
    return items ? (const char *)(items+count) : NULL;
}
static int is_background_action(const struct NDConfig *cfg,
                                const struct NDItem *item)
{
    return (item->kind==ND_ITEM_LAUNCH || item->kind==ND_ITEM_DRAWER ||
            item->kind==ND_ITEM_GROUP) &&
           (nd_item_text(cfg,item,ND_TEXT_HOTKEY)[0] || item->tools_menu);
}
struct NDItem *copy_background_actions(const struct NDConfig *cfg,
                                               int *out_count)
{
    struct NDItem *items;
    char *pool;
    ULONG pool_size=1,used=1,len;
    int i,k,count=0,w=0;
    if (out_count) *out_count=0;
    if (!cfg || !out_count) return NULL;
    for (i=0;i<cfg->count;++i) {
        const struct NDItem *item=&cfg->items[i];
        if (!is_background_action(cfg,item)) continue;
        ++count;
        for (k=0;k<ND_TEXT_COUNT;++k)
            if (item->text[k]) pool_size+=strlen(nd_item_text(cfg,item,k))+1;
    }
    if (!count) return NULL;
    items=(struct NDItem *)alloc_data((ULONG)count*sizeof(*items)+pool_size);
    if (!items) { *out_count=-1;return NULL; }
    pool=(char *)(items+count);
    pool[0]='\0';
    for (i=0;i<cfg->count;++i) {
        const struct NDItem *item=&cfg->items[i];
        if (!is_background_action(cfg,item)) continue;
        items[w]=*item;
        for (k=0;k<ND_TEXT_COUNT;++k) {
            if (!item->text[k]) continue;
            len=strlen(nd_item_text(cfg,item,k))+1;
            memcpy(pool+used,nd_item_text(cfg,item,k),len);
            items[w].text[k]=(unsigned short)used;
            used+=len;
        }
        ++w;
    }
    *out_count=count;
    return items;
}

ULONG diagnostic_ticks(void)
{
    struct DateStamp stamp;
    DateStamp(&stamp);
    /* Unsigned wrap is intentional: elapsed-time subtraction remains valid
     * for sessions shorter than the roughly 994-day ULONG tick period. */
    return (((ULONG)stamp.ds_Days*1440UL+(ULONG)stamp.ds_Minute)*60UL*
            ND_DIAGNOSTIC_TICKS_PER_SECOND)+(ULONG)stamp.ds_Tick;
}
void reset_diagnostic_counters(struct NDDock *dock)
{
    dock->diagnostic_started=diagnostic_ticks();
    dock->diagnostic_wakes=0;
    dock->diagnostic_full_paints=0;
    dock->diagnostic_tile_paints=0;
    dock->diagnostic_updates=0;
    dock->diagnostic_reclaimed_replies=0;
    dock->diagnostic_unchanged_updates=0;
    dock->diagnostic_coalesced_updates=0;
    dock->diagnostic_reload_requests=0;
    dock->diagnostic_reload_applied=0;
    dock->diagnostic_reload_skipped=0;
    dock->diagnostic_reload_failures=0;
    dock->diagnostic_reload_last_ticks=0;
    dock->diagnostic_reload_total_ticks=0;
    dock->diagnostic_live_updates=dock->diagnostic_window_rebuilds=0;
    dock->diagnostic_update_mode=0;
    dock->save_state.started=dock->save_state.completed=0;
    dock->save_state.failed=dock->save_state.coalesced=0;
    dock->save_last_ticks=dock->save_total_ticks=0;
    memset(dock->diagnostic_plugin_updates,0,
           sizeof(dock->diagnostic_plugin_updates));
    dock->watchdog_total_restarts=0;
    memset(dock->watchdog_restarts,0,sizeof(dock->watchdog_restarts));
}
static void reset_diagnostics(struct NDDock *dock)
{
    reset_diagnostic_counters(dock);
    memset(dock->diagnostic_plugin_last,0,
           sizeof(dock->diagnostic_plugin_last));
}
/* Errors that the user must see: a Workbench-started dock has no console,
 * so a requester is used whenever Intuition is available. */
static LONG ask_user(const char *body,const char *gadgets)
{
    struct EasyStruct easy;
    if (!IntuitionBase) { printf("NovaDock: %s\n",body); return 0; }
    easy.es_StructSize=sizeof(easy);
    easy.es_Flags=0;
    easy.es_Title=(STRPTR)"NovaDock";
    easy.es_TextFormat=(STRPTR)nd_tr(body);
    easy.es_GadgetFormat=(STRPTR)nd_tr(gadgets);
    return EasyRequestArgs(NULL,&easy,NULL,NULL);
}
static void report_problem(const char *body)
{
    printf("NovaDock: %s\n",body);
    (void)ask_user(body,nd_tr("OK"));
}
/* Release every Workbench-screen resource; the Exec ports stay public. */
void close_dock_ui(struct NDDock *dock)
{
    close_tooltip(dock);
    close_feedback(dock);
    close_popup(dock);
    close_calendar(dock);
    close_about(dock);
    close_folder_popup(dock);
    detach_appmenus(dock);
    detach_appwindow(dock);
    release_icons(dock);
    if (dock->window) { CloseWindow(dock->window); dock->window=NULL; }
    release_dock_font(dock);
    free_colors(dock);
    dock->screen=NULL;
}
int reopen_dock_ui(struct NDDock *dock)
{
    load_icons(dock);
    if (!open_window(dock)) { release_icons(dock); return 0; }
    attach_appwindow(dock);
    dock->screen_closed=0;
    /* Apply any Preferences change that was saved while Workbench was
     * closed (a no-op reload is detected and skipped). */
    (void)reload_configuration(dock);
    return 1;
}
/* Optional screennotify.library: ScreenMode and Font preferences can only
 * close Workbench when no other window is open on it. */
static void start_screennotify(struct NDDock *dock)
{
    ScreenNotifyBase=OpenLibrary((STRPTR)SCREENNOTIFY_NAME,SCREENNOTIFY_VERSION);
    if (!ScreenNotifyBase) return;
    dock->sn_port=CreateMsgPort();
    if (dock->sn_port) dock->sn_handle=ND_AddWorkbenchClient(dock->sn_port,0);
    if (!dock->sn_handle) {
        if (dock->sn_port) { DeleteMsgPort(dock->sn_port); dock->sn_port=NULL; }
        CloseLibrary(ScreenNotifyBase); ScreenNotifyBase=NULL;
    }
}
static void stop_screennotify(struct NDDock *dock)
{
    struct Message *message;
    if (dock->sn_handle) {
        /* RemWorkbenchClient fails while a notification is outstanding. */
        while (!ND_RemWorkbenchClient(dock->sn_handle)) {
            while ((message=GetMsg(dock->sn_port))!=NULL) ReplyMsg(message);
            Delay(5);
        }
        dock->sn_handle=NULL;
    }
    if (dock->sn_port) {
        while ((message=GetMsg(dock->sn_port))!=NULL) ReplyMsg(message);
        DeleteMsgPort(dock->sn_port); dock->sn_port=NULL;
    }
    if (ScreenNotifyBase) { CloseLibrary(ScreenNotifyBase); ScreenNotifyBase=NULL; }
}
static void process_screennotify(struct NDDock *dock)
{
    struct ScreenNotifyMessage *message;
    int reopen=0;
    while (dock->sn_port &&
           (message=(struct ScreenNotifyMessage *)GetMsg(dock->sn_port))!=NULL) {
        if (message->snm_Type==SCREENNOTIFY_TYPE_WORKBENCH) {
            if (!message->snm_Value) {
                /* Close before replying: Workbench closes after the reply. */
                if (dock->window) close_dock_ui(dock);
                dock->screen_closed=1;
            } else {reopen=1;dock->suspended=0;}
        }
        ReplyMsg((struct Message *)message);
    }
    if (reopen && dock->screen_closed) (void)reopen_dock_ui(dock);
}
static void run_loop(struct NDDock *dock)
{
    ULONG signals, win_sig, ipc_sig, ctl_sig, drop_sig, popup_sig, rx_sig, timer_sig;
    ULONG calendar_sig, about_sig, folder_sig, hotkey_sig, rexx_reply_sig, sn_sig, save_sig;
    ipc_sig = 1UL << dock->ipc->mp_SigBit;
    ctl_sig = 1UL << dock->control->mp_SigBit;
    dock->running = 1;
    while (dock->running) {
        /* Window UserPort can change after a live window recreation. */
        win_sig = dock->window ? (1UL << dock->window->UserPort->mp_SigBit) : 0;
        drop_sig = dock->drop_port ? (1UL << dock->drop_port->mp_SigBit) : 0;
        popup_sig = dock->popup ? (1UL << dock->popup->UserPort->mp_SigBit) : 0;
        calendar_sig = dock->calendar ?
                       (1UL << dock->calendar->UserPort->mp_SigBit) : 0;
        about_sig = dock->about ?
                    (1UL << dock->about->UserPort->mp_SigBit) : 0;
        folder_sig = dock->folder_popup ?
                     (1UL << dock->folder_popup->UserPort->mp_SigBit) : 0;
        rx_sig = dock->rexx ? (1UL << dock->rexx->mp_SigBit) : 0;
        hotkey_sig = dock->hotkey_port ?
                     (1UL << dock->hotkey_port->mp_SigBit) : 0;
        timer_sig = dock->timer_port ? (1UL << dock->timer_port->mp_SigBit) : 0;
        rexx_reply_sig = dock->rexx_reply ?
                         (1UL << dock->rexx_reply->mp_SigBit) : 0;
        sn_sig = dock->sn_port ? (1UL << dock->sn_port->mp_SigBit) : 0;
        save_sig=dock->save_port ? 1UL<<dock->save_port->mp_SigBit : 0;
        signals = Wait(win_sig | ipc_sig | ctl_sig | drop_sig | popup_sig |
                       calendar_sig | about_sig | folder_sig | rx_sig | hotkey_sig | timer_sig |
                       rexx_reply_sig | sn_sig | save_sig | SIGBREAKF_CTRL_C);
        ++dock->diagnostic_wakes;
        if(signals&save_sig)process_settings_writer(dock);
        if (signals & popup_sig) popup_events(dock);
        if (signals & calendar_sig) calendar_events(dock);
        if (signals & about_sig) about_events(dock);
        if (signals & folder_sig) folder_popup_events(dock);
        if ((signals & win_sig) && dock->window) window_events(dock);
        if (signals & timer_sig) process_timer(dock);
        if (signals & rx_sig) process_rexx(dock);
        if (signals & hotkey_sig) process_hotkeys(dock);
        if (signals & drop_sig) dropped_icons(dock);
        if (signals & ctl_sig) process_control(dock, 0);
        if (signals & ipc_sig) process_ipc(dock, 0);
        if (signals & rexx_reply_sig) process_rexx_replies(dock, 1);
        if (signals & sn_sig) process_screennotify(dock);
        if (signals & SIGBREAKF_CTRL_C) dock->running = 0;
        wake_pending_plugins(dock);
    }
}
/* "NovaDock QUIT": ask a running dock to quit and wait until it has gone
 * (used by the installer before it replaces the program files).  Needs no
 * ARexx: it uses the dock's own control port.  0 = not running or stopped,
 * 5 = still running after 10 seconds, 10 = it did not answer. */
static int dock_ports_present(void)
{
    int present;
    Forbid();
    present=FindPort((STRPTR)ND_PORT_NAME)!=NULL ||
            FindPort((STRPTR)ND_CONTROL_PORT)!=NULL;
    Permit();
    return present;
}
static int quit_running_dock(void)
{
    int waited;
    if (!dock_ports_present()) {
        PutStr("NovaDock is not running.\n");
        return 0;
    }
    if (nd_control_send(ND_CONTROL_QUIT)!=ND_CONTROL_OK) {
        PutStr("NovaDock did not answer the quit request.\n");
        return 10;
    }
    for (waited=0;waited<500;waited+=5) {
        if (!dock_ports_present()) {
            /* Give its plugins a moment to finish their last update. */
            Delay(50);
            PutStr("NovaDock stopped.\n");
            return 0;
        }
        Delay(5);
    }
    PutStr("NovaDock is still shutting down.\n");
    return 5;
}
static int is_quit_argument(const char *arg)
{
    return arg && (arg[0]=='Q' || arg[0]=='q') && (arg[1]=='U' || arg[1]=='u') &&
           (arg[2]=='I' || arg[2]=='i') && (arg[3]=='T' || arg[3]=='t') && !arg[4];
}

int main(int argc, char **argv)
{
    struct NDDock *dock;
    int result = 10, parsing, running_elsewhere;
    /* argc is 0 when started from Workbench (argv is then a WBStartup). */
    if (argc>1 && is_quit_argument(argv[1])) return quit_running_dock();
    dock = (struct NDDock *)alloc_data(sizeof(*dock));
    text_owner = dock;
    if (!dock) return 20;
    { int x, y;
      for (x = 0; x < ND_COLOR_COUNT; ++x) {
          dock->colors[x] = -1;
          dock->color_owned[x] = 0;
      }
      for (x = 0; x < ND_MAX_ITEMS; ++x)
          for (y = 0; y < ND_ICON_MAX_COLORS; ++y) dock->brush_pens[x][y] = -1;
    }
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    if (!IntuitionBase) goto cleanup;
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39);
    if (!GfxBase) goto cleanup;
    DataTypesBase = OpenLibrary("datatypes.library", 39); /* optional artwork */
    IconBase = OpenLibrary("icon.library", 36); /* optional */
    WorkbenchBase = OpenLibrary("workbench.library", 36); /* optional */
    RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0); /* optional */
    CxBase = OpenLibrary("commodities.library", 37); /* optional global hot keys */
    nd_locale_open(); /* optional catalogs */
    dock->from_workbench = argc == 0;
    /* A second start reveals the running dock instead of failing. */
    Forbid();
    running_elsewhere = FindPort((STRPTR)ND_PORT_NAME) != NULL;
    Permit();
    if (running_elsewhere) {
        if (nd_control_send(ND_CONTROL_SHOW) != ND_CONTROL_OK)
            report_problem("Another NovaDock is running but did not respond.");
        result = 0;
        goto cleanup;
    }
    (void)start_input_qualifier(dock);
    parsing = nd_load_effective(&dock->conf);
    if (parsing > 0 &&
        !ask_user("No valid settings file was found.\n"
                  "ENV:, ENVARC: and the packaged novadock.cfg (and their\n"
                  ".bak copies) all contain errors.\n\n"
                  "Start with the built-in defaults? Your files are\n"
                  "left untouched until you save from Preferences.",
                  "Use defaults|Quit")) goto cleanup;
    (void)nd_publish_config_source(ND_CONFIG_SOURCE_FILE);
    dock->action_items=copy_background_actions(&dock->conf,
                                               &dock->action_count);
    if (dock->action_count<0) {
        report_problem("Not enough memory for launcher actions.");
        goto cleanup;
    }
    if (!nd_compact_enabled(&dock->conf)) {
        report_problem("The configuration has no enabled tiles."); goto cleanup;
    }
    find_system_volume(dock);
    dock->folder_view = dock->conf.folder_view;
    dock->folder_sort = dock->conf.folder_sort;
    dock->media_count = scan_media(dock, dock->media);
    (void)nd_media_append(&dock->conf, dock->media, dock->media_count,
                          dock->system_volume);
    dock->media_scan_ticks = ND_MEDIA_SCAN_TICKS;
    get_program_dir(dock);
    nd_theme_load_config(dock->directory,&dock->conf,&dock->theme_spec);
    /* Artwork is read after the window opens (load_pending_icons), so the
     * dock appears at once even on a 7 MHz machine reading from floppy. */
    dock->icons_pending=1;dock->icon_next=0;
    refresh_volume_meters(dock);
    if (!register_port(dock)) {
        report_problem("Could not register NovaDock's message ports\n"
                       "(another dock started at the same moment, or no free signal).");
        goto cleanup;
    }
    if (!open_window(dock)) {
        report_problem("The configured dock does not fit on the Workbench screen.\n"
                       "Reduce tile size, gap or items in Preferences.");
        goto cleanup;
    }
    dock->drop_port = CreateMsgPort(); /* optional if Exec has no signals */
    /* Delayed in the timer/IntuiTicks path so early User-Startup launches do
     * not register against Workbench's transient startup state. */
    dock->drop_attach_ticks = 0;
    (void)start_hotkeys(dock);
    start_plugins(dock, NULL);
    start_rexx(dock);
    start_screennotify(dock);
    reset_diagnostics(dock);
    if (!start_timer(dock)) {
        printf("NovaDock: timer.device unavailable; using IntuiTicks.\n");
        /* No timer to drive background loading: load everything now. */
        while (load_pending_icons(dock,ND_MAX_ITEMS)) {}
        flush_dirty_tiles(dock);
    }
    (void)init_settings_writer(dock);
    fire_hook(dock,"STARTUP",ND_VERSION);
    do {run_loop(dock);} while(!finish_settings_saves(dock));
    fire_hook(dock,"QUIT","");
    result = 0;
cleanup:
    close_settings_writer(dock);
    stop_screennotify(dock);
    stop_rexx_replies(dock);
    stop_timer(dock);
    stop_input_qualifier(dock);
    stop_rexx(dock);
    close_tooltip(dock);
    close_feedback(dock);
    close_popup(dock);
    close_calendar(dock);
    close_about(dock);
    close_folder_popup(dock);
    detach_appmenus(dock);
    detach_appwindow(dock);
    stop_hotkeys(dock);
    if (dock->drop_port) {
        struct Message *pending;
        while ((pending = GetMsg(dock->drop_port)) != NULL) ReplyMsg(pending);
        DeleteMsgPort(dock->drop_port);
        dock->drop_port = NULL;
    }
    remove_port(dock);
    if (dock->window) CloseWindow(dock->window);
    release_dock_font(dock);
    release_icons(dock);
    free_colors(dock);
    dock->screen = NULL;
    if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
    if (CxBase) CloseLibrary(CxBase);
    nd_locale_close();
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (IconBase) CloseLibrary(IconBase);
    if (DataTypesBase) CloseLibrary(DataTypesBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (dock->action_items) FreeVec(dock->action_items);
    FreeVec(dock);
    return result;
}
