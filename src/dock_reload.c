/* NovaDock: Configuration reload, the control port, and moving or removing tiles. */
#include "dock_internal.h"
#include "nd_reply.h"

/* Defined below, private to this file. */
static void swap_configs(struct NDConfig *a,struct NDConfig *b);
static void fill_diagnostics(struct NDDock *dock, struct NDDiagnosticsSnapshot *snapshot);
static int do_move_row(struct NDDock *dock,int from,int to,int target_row);

/* Exchange two configurations in place through a small stack window, so the
 * previous live configuration survives without a third 92 KB block. */
static void swap_configs(struct NDConfig *a,struct NDConfig *b)
{
    UBYTE chunk[256];
    UBYTE *pa=(UBYTE *)a,*pb=(UBYTE *)b;
    ULONG left=sizeof(*a),n;
    while (left) {
        n=left>sizeof(chunk) ? sizeof(chunk) : left;
        memcpy(chunk,pa,n);memcpy(pa,pb,n);memcpy(pb,chunk,n);
        pa+=n;pb+=n;left-=n;
    }
}
/* One working buffer serves the whole reload: it is loaded with the full
 * file, compacted in place, and after the swap holds the previous live
 * configuration for plugin-state transfer and rollback.  Peak use is the
 * live configuration plus this buffer (formerly plus two more). */
static LONG apply_configuration(struct NDDock *dock,const struct NDConfig *provided)
{
    struct NDConfig *next=NULL, *old=NULL, *buffer=NULL;
    struct NDIconStash *icon_stash=NULL;
    struct NDItem *new_action_items=NULL;
    int new_action_count=0;
    struct NDGraphHistory *saved_history=NULL;
    ULONG saved_updates[ND_MAX_ITEMS],saved_last[ND_MAX_ITEMS];
    ULONG saved_retry[ND_MAX_ITEMS],saved_restarts[ND_MAX_ITEMS];
    UWORD saved_failures[ND_MAX_ITEMS];
    struct Task *saved_tasks[ND_MAX_ITEMS];
    unsigned char saved_woken[ND_MAX_ITEMS],saved_wake[ND_MAX_ITEMS];
    struct NDLayout layout;
    struct NDThemeSpec next_theme;
    int status, i, j, found, skipped=0, base_count, live=0;
    ULONG started=diagnostic_ticks(),elapsed;
    LONG result = ND_CONTROL_OK;
    ++dock->diagnostic_reload_requests;
    dock->diagnostic_reload_largest_before=AvailMem(MEMF_LARGEST);
    dock->diagnostic_reload_free_low=AvailMem(MEMF_ANY);
    /* The loader clears the structure itself.  MEMF_REVERSE keeps this
     * short-lived block at the top of memory, away from long-lived small
     * allocations, so repeated reloads do not fragment the free list. */
    buffer = (struct NDConfig *)AllocVec(sizeof(*buffer), MEMF_PUBLIC | MEMF_REVERSE);
    if (!buffer) {
        result = ND_CONTROL_NOMEM; goto finish;
    }
    next = buffer;
    if (provided) {memcpy(next,provided,sizeof(*next));status=0;}
    else status = load_edit_configuration(dock,next);
    if (status > 0) { result = ND_CONTROL_INVALID; goto finish; }
    (void)nd_publish_config_source(ND_CONFIG_SOURCE_FILE);
    if (!dock->screen || dock->screen_closed) {
        /* Workbench is closed (screennotify): apply when it reopens. */
        result = ND_CONTROL_NOFIT; goto finish;
    }
    new_action_items=copy_background_actions(next,&new_action_count);
    if (new_action_count<0) { result=ND_CONTROL_NOMEM;goto finish; }
    if (!nd_compact_enabled(next)) { result = ND_CONTROL_NOFIT; goto finish; }
    base_count=next->count;
    (void)nd_media_append(next,dock->media,dock->media_count,
                          dock->system_volume);
    if (!nd_layout_fit(next, dock->screen->Width, dock->screen->Height,
                       dock->screen->BarHeight, &layout)) {
        /* Taskbar and removable-media tiles are optional: retry without.
         * They were only appended, so dropping them restores the compacted
         * configuration exactly. */
        next->count=base_count;
        if (!nd_layout_fit(next, dock->screen->Width, dock->screen->Height,
                           dock->screen->BarHeight, &layout)) {
            result = ND_CONTROL_NOFIT; goto finish;
        }
    }
    /* A gesture changes items, not the theme file. Explicit reloads still
     * re-read it; gestures reuse the already resolved palette without I/O. */
    if(provided && !strcmp(nd_config_theme_id(next),nd_config_theme_id(&dock->conf)))
        next_theme=dock->theme_spec;
    else nd_theme_load_config(dock->directory,next,&next_theme);
    if (nd_config_runtime_equal(next,&dock->conf) &&
        !memcmp(&next_theme,&dock->theme_spec,sizeof(next_theme)) &&
        new_action_count==dock->action_count) {
        int same=1;
        if (new_action_count &&
            (!new_action_items || !dock->action_items)) same=0;
        for (i=0;i<new_action_count;++i)
            if (same && !nd_item_runtime_equal(
                    action_text_pool(new_action_items,new_action_count),
                    &new_action_items[i],
                    action_text_pool(dock->action_items,dock->action_count),
                    &dock->action_items[i])) {
                same=0;break;
            }
        if (same) {skipped=1;goto finish;}
    }
    live=dock->window && !dock->box_pending &&
         nd_config_can_update_live(&dock->conf,next) &&
         !memcmp(&next_theme,&dock->theme_spec,sizeof(next_theme)) &&
         layout.tile_width==dock->tile_width &&
         layout.tile_height==dock->tile_height;
    icon_stash=(struct NDIconStash *)AllocVec(sizeof(*icon_stash),
                                             MEMF_PUBLIC|MEMF_CLEAR);
    if (!icon_stash) {result=ND_CONTROL_NOMEM;goto finish;}
    saved_history = (struct NDGraphHistory *)AllocVec(sizeof(dock->history), MEMF_PUBLIC | MEMF_CLEAR);
    if (!saved_history) {result=ND_CONTROL_NOMEM;goto finish;}
    /* Until the swap below, the previous configuration is still live. */
    old = &dock->conf;
    memcpy(saved_history, dock->history, sizeof(dock->history));
    memcpy(saved_tasks,dock->plugin_task,sizeof(saved_tasks));
    memcpy(saved_woken,dock->plugin_woken,sizeof(saved_woken));
    memcpy(saved_wake,dock->plugin_wake_port,sizeof(saved_wake));
    memcpy(saved_updates,dock->diagnostic_plugin_updates,
           sizeof(saved_updates));
    memcpy(saved_last,dock->diagnostic_plugin_last,sizeof(saved_last));
    memcpy(saved_retry,dock->watchdog_retry_after,sizeof(saved_retry));
    memcpy(saved_failures,dock->watchdog_failures,sizeof(saved_failures));
    memcpy(saved_restarts,dock->watchdog_restarts,sizeof(saved_restarts));
    for (i = 0; i < next->count; ++i) {
        struct NDItem *item = &next->items[i];
        if (item->kind != ND_ITEM_PLUGIN) continue;
        found = -1;
        for (j = 0; j < old->count; ++j) {
            if (old->items[j].kind == ND_ITEM_PLUGIN &&
                !strcmp(item->id, old->items[j].id)) { found = j; break; }
        }
        if (found >= 0) {
            item->online = old->items[found].online;
            item->meter = old->items[found].meter;
            item->clicks = old->items[found].clicks;
            item->media_capabilities = old->items[found].media_capabilities;
            item->media_state = old->items[found].media_state;
            item->media_volume = old->items[found].media_volume;
            item->media_event = old->items[found].media_event;
            item->media_volume_request = old->items[found].media_volume_request;
            item->settings_dirty = old->items[found].settings_dirty ||
                                   nd_options_changed(old,next,item->id);
            strncpy(item->caption, old->items[found].caption, ND_LABEL_LEN - 1);
            item->caption[ND_LABEL_LEN - 1] = '\0';
            strncpy(item->value, old->items[found].value, sizeof(item->value) - 1);
            item->value[sizeof(item->value) - 1] = '\0';
            memcpy(item->badge, old->items[found].badge, sizeof(item->badge));
        }
    }
    /* Close only the Intuition UI; retain both public Exec ports. */
    close_tooltip(dock);
    close_feedback(dock);
    close_popup(dock);
    close_calendar(dock);
    close_folder_popup(dock);
    detach_appmenus(dock);
    if (live) dock->box_pending=1; /* suppress paints until layout is installed */
    else detach_appwindow(dock);
    stop_hotkeys(dock);
    if (live) stash_live_icon_sources(dock,icon_stash);
    else stash_icon_sources(dock,icon_stash);
    release_icons(dock);
    if (!live) {
        CloseWindow(dock->window);
        dock->window = NULL;
        release_dock_font(dock);
        free_colors(dock);
        dock->screen = NULL;
    }
    /* Install the new configuration; BUFFER now holds the previous one. */
    swap_configs(&dock->conf,buffer);
    old = buffer;
    next = &dock->conf;
    dock->theme_spec=next_theme;
    /* Drawer-popup view/sort follow Preferences only when those settings
     * change; otherwise the user's choice in the popup is kept. */
    if (old->folder_view!=next->folder_view) dock->folder_view=next->folder_view;
    if (old->folder_sort!=next->folder_sort) dock->folder_sort=next->folder_sort;
    /* Remap task identities and telemetry by plugin ID, never by tile index. */
    memset(dock->plugin_task,0,sizeof(dock->plugin_task));
    memset(dock->plugin_woken,0,sizeof(dock->plugin_woken));
    refresh_volume_meters(dock);
    memset(dock->history, 0, sizeof(dock->history));
    memset(dock->diagnostic_plugin_updates,0,
           sizeof(dock->diagnostic_plugin_updates));
    memset(dock->diagnostic_plugin_last,0,
           sizeof(dock->diagnostic_plugin_last));
    memset(dock->watchdog_retry_after,0,sizeof(dock->watchdog_retry_after));
    memset(dock->watchdog_failures,0,sizeof(dock->watchdog_failures));
    memset(dock->watchdog_restarts,0,sizeof(dock->watchdog_restarts));
    memset(dock->plugin_wake_port,0,sizeof(dock->plugin_wake_port));
    memset(dock->plugin_event2,0,sizeof(dock->plugin_event2));
    memset(dock->net_state,0,sizeof(dock->net_state));
    for (i = 0; i < dock->conf.count; ++i) {
        struct NDItem *item = &dock->conf.items[i];
        if (item->kind != ND_ITEM_PLUGIN) continue;
        for (j = 0; j < old->count; ++j) {
            if (old->items[j].kind == ND_ITEM_PLUGIN &&
                !strcmp(item->id, old->items[j].id)) {
                dock->history[i] = saved_history[j];
                dock->plugin_task[i]=saved_tasks[j];
                dock->plugin_woken[i]=saved_woken[j];
                dock->plugin_wake_port[i]=saved_wake[j];
                dock->diagnostic_plugin_updates[i]=saved_updates[j];
                dock->diagnostic_plugin_last[i]=saved_last[j];
                dock->watchdog_retry_after[i]=saved_retry[j];
                dock->watchdog_failures[i]=saved_failures[j];
                dock->watchdog_restarts[i]=saved_restarts[j];
                break;
            }
        }
    }
    /* Unchanged artwork comes from the stash; the rest is read again. */
    load_icons_reusing(dock,icon_stash,old);
    {
        /* Busiest point: both configurations, the history copy and every
         * icon are held, and the unclaimed stash is not yet freed. */
        ULONG free_now=AvailMem(MEMF_ANY);
        if (free_now<dock->diagnostic_reload_free_low)
            dock->diagnostic_reload_free_low=free_now;
    }
    free_icon_stash(icon_stash);
    if (live) {
        if (dock->action_items) FreeVec(dock->action_items);
        dock->action_items=new_action_items;
        dock->action_count=new_action_count;
        new_action_items=NULL;
        update_live_window(dock);
        attach_appwindow(dock); /* existing registration retained; refresh Tools menu */
        rebuild_hotkeys(dock);
        start_plugins(dock,old);
    } else if (!open_window(dock)) {
        if (dock->window) { CloseWindow(dock->window); dock->window = NULL; }
        release_dock_font(dock);
        release_icons(dock);
        free_colors(dock);
        dock->screen = NULL;
        dock->conf = *old;
        nd_theme_load_config(dock->directory,&dock->conf,&dock->theme_spec);
        memcpy(dock->history, saved_history, sizeof(dock->history));
        memcpy(dock->plugin_task,saved_tasks,sizeof(saved_tasks));
        memcpy(dock->plugin_woken,saved_woken,sizeof(saved_woken));
        memcpy(dock->plugin_wake_port,saved_wake,sizeof(saved_wake));
        memcpy(dock->diagnostic_plugin_updates,saved_updates,
               sizeof(saved_updates));
        memcpy(dock->diagnostic_plugin_last,saved_last,sizeof(saved_last));
        memcpy(dock->watchdog_retry_after,saved_retry,sizeof(saved_retry));
        memcpy(dock->watchdog_failures,saved_failures,sizeof(saved_failures));
        memcpy(dock->watchdog_restarts,saved_restarts,sizeof(saved_restarts));
        load_icons(dock);
        if (!open_window(dock)) dock->running = 0;
        else { attach_appwindow(dock);rebuild_hotkeys(dock); }
        result = ND_CONTROL_NOFIT;
    } else {
        if (dock->action_items) FreeVec(dock->action_items);
        dock->action_items=new_action_items;
        dock->action_count=new_action_count;
        new_action_items=NULL;
        attach_appwindow(dock);
        rebuild_hotkeys(dock);
        start_plugins(dock, old);
    }
    /* The shared low-rate timer also coalesces plugin paints. Runtime
     * preferences therefore keep it alive even without auto-hide or Eyes. */
    memset(dock->eyes_last_valid,0,sizeof(dock->eyes_last_valid));
    dock->eyes_timer_ticks=0;
    dock->eyes_blink_counter=0;
    dock->eyes_blink=0;
    dock->volume_timer_ticks=0;
    dock->watchdog_timer_ticks=0;
    if (!dock->timer_port) start_timer(dock);
finish:
    /* A live plugin must get a fresh watchdog window after a blocking
     * reload. Keep telemetry timestamps intact: this is retry grace, not
     * a pretend update from the plugin. */
    elapsed=diagnostic_ticks()-started;
    for (i=0;i<dock->conf.count;++i)
        if (!skipped && elapsed>=ND_WATCHDOG_TICKS_PER_SECOND &&
            dock->conf.items[i].kind==ND_ITEM_PLUGIN &&
            dock->conf.items[i].online && dock->diagnostic_plugin_updates[i])
            dock->watchdog_retry_after[i]=diagnostic_ticks()+ND_WATCHDOG_STALE_TICKS;
    dock->diagnostic_reload_last_ticks=elapsed;
    dock->diagnostic_reload_total_ticks+=elapsed;
    if (result!=ND_CONTROL_OK) ++dock->diagnostic_reload_failures;
    else if (skipped) ++dock->diagnostic_reload_skipped;
    else ++dock->diagnostic_reload_applied;
    dock->diagnostic_update_mode=result!=ND_CONTROL_OK ? 4 : skipped ? 3 : live ? 1 : 2;
    if(result==ND_CONTROL_OK && !skipped){
        if(live)++dock->diagnostic_live_updates;else ++dock->diagnostic_window_rebuilds;
    }
    if (new_action_items) FreeVec(new_action_items);
    if (saved_history) FreeVec(saved_history);
    if (buffer) FreeVec(buffer);
    if (icon_stash) FreeVec(icon_stash);
    dock->diagnostic_reload_largest_after=AvailMem(MEMF_LARGEST);
    return result;
}
LONG reload_configuration(struct NDDock *dock)
{
    return apply_configuration(dock,NULL);
}
static void fill_diagnostics(struct NDDock *dock,
                             struct NDDiagnosticsSnapshot *snapshot)
{
    ULONG now=diagnostic_ticks();
    int i;
    memset(snapshot,0,sizeof(*snapshot));
    snapshot->uptime_ticks=now-dock->diagnostic_started;
    snapshot->wake_count=dock->diagnostic_wakes;
    snapshot->paint_count=dock->diagnostic_full_paints;
    snapshot->tile_count=dock->diagnostic_tile_paints;
    snapshot->update_count=dock->diagnostic_updates;
    snapshot->watchdog_restarts=dock->watchdog_total_restarts;
    snapshot->unchanged_updates=dock->diagnostic_unchanged_updates;
    snapshot->coalesced_updates=dock->diagnostic_coalesced_updates;
    snapshot->reload_requests=dock->diagnostic_reload_requests;
    snapshot->reload_applied=dock->diagnostic_reload_applied;
    snapshot->reload_skipped=dock->diagnostic_reload_skipped;
    snapshot->reload_failures=dock->diagnostic_reload_failures;
    snapshot->reload_last_ticks=dock->diagnostic_reload_last_ticks;
    snapshot->mem_free=AvailMem(MEMF_ANY);
    snapshot->mem_largest=AvailMem(MEMF_LARGEST);
    snapshot->mem_chip_free=AvailMem(MEMF_CHIP);
    snapshot->reload_largest_before=dock->diagnostic_reload_largest_before;
    snapshot->reload_largest_after=dock->diagnostic_reload_largest_after;
    snapshot->reload_free_low=dock->diagnostic_reload_free_low;
    snapshot->reload_icons_reused=dock->diagnostic_icons_reused;
    snapshot->reload_icons_loaded=dock->diagnostic_icons_loaded;
    snapshot->reload_total_ticks=dock->diagnostic_reload_total_ticks;
    snapshot->settings_telemetry_version=2;
    snapshot->reserved=(UWORD)(dock->diagnostic_reclaimed_replies>65535UL ?
                               65535UL : dock->diagnostic_reclaimed_replies);
    snapshot->live_updates=dock->diagnostic_live_updates;
    snapshot->window_rebuilds=dock->diagnostic_window_rebuilds;
    snapshot->update_mode=dock->diagnostic_update_mode;
    snapshot->save_state=dock->save_state.active ? 2 :
        dock->save_state.revision==dock->save_state.saved ? 0 :
        dock->save_state.last_failed ? 3 : 1;
    snapshot->saves_started=dock->save_state.started;
    snapshot->saves_completed=dock->save_state.completed;
    snapshot->saves_failed=dock->save_state.failed;
    snapshot->saves_coalesced=dock->save_state.coalesced;
    snapshot->save_last_ticks=dock->save_last_ticks;
    snapshot->save_total_ticks=dock->save_total_ticks;
    snapshot->save_env_ticks=dock->save_env_ticks;
    snapshot->save_envarc_ticks=dock->save_envarc_ticks;
    snapshot->save_env_ok=dock->save_env_ok;snapshot->save_envarc_ok=dock->save_envarc_ok;
    snapshot->hidden=(UWORD)(dock->hidden!=0);
    snapshot->timer_active=(UWORD)(dock->timer_open!=0);
    snapshot->timer_quanta=(UWORD)(dock->timer_quanta>0 ?
                                   dock->timer_quanta : 1);
    for (i=0;i<dock->conf.count;++i) {
        struct NDItem *item=&dock->conf.items[i];
        ULONG age=ND_DIAGNOSTIC_AGE_UNKNOWN;
        int stale=0;
        if (item->kind!=ND_ITEM_PLUGIN) continue;
        ++snapshot->plugin_count;
        if (item->online) ++snapshot->online_count;
        if (dock->diagnostic_plugin_updates[i])
            age=now-dock->diagnostic_plugin_last[i];
        if (item->online && age!=ND_DIAGNOSTIC_AGE_UNKNOWN &&
            age>ND_WATCHDOG_STALE_TICKS) stale=1;
        if (stale) ++snapshot->stale_count;
        if (snapshot->listed_plugins<ND_DIAGNOSTIC_MAX_PLUGINS) {
            struct NDPluginDiagnostics *plugin=
                &snapshot->plugins[snapshot->listed_plugins++];
            strncpy(plugin->id,item->id,sizeof(plugin->id)-1);
            plugin->id[sizeof(plugin->id)-1]='\0';
            plugin->updates=dock->diagnostic_plugin_updates[i];
            plugin->age_ticks=age;
            plugin->restarts=dock->watchdog_restarts[i];
            plugin->online=(UWORD)(item->online!=0);
            plugin->stale=(UWORD)stale;
        }
    }
}
void process_control(struct NDDock *dock, int quitting)
{
    struct NDControlMessage *m;
    while ((m = (struct NDControlMessage *)GetMsg(dock->control)) != NULL) {
        if (m->msg.mn_Length < sizeof(*m)) {
            dock->diagnostic_reclaimed_replies+=(ULONG)nd_reply_finish((struct Message *)m);
            continue;
        }
        if(m->magic==ND_CONTROL_MAGIC && m->version==ND_CONTROL_VERSION &&
           (m->action==ND_CONTROL_SAVE_LOCK || m->action==ND_CONTROL_SAVE_FLUSH ||
            m->action==ND_CONTROL_SAVE_UNLOCK)){
            struct Task *owner=m->msg.mn_ReplyPort ? m->msg.mn_ReplyPort->mp_SigTask : NULL;
            if(quitting){m->result=ND_CONTROL_PENDING;dock->diagnostic_reclaimed_replies+=(ULONG)nd_reply_finish((struct Message *)m);continue;}
            if(m->action==ND_CONTROL_SAVE_UNLOCK){
                if(dock->write_owner==owner)dock->write_owner=NULL;
                m->result=ND_CONTROL_OK;
            }else if(dock->write_owner && dock->write_owner!=owner)m->result=ND_CONTROL_PENDING;
            else{
                if(m->action==ND_CONTROL_SAVE_LOCK){dock->write_owner=owner;
                    dock->write_deadline=diagnostic_ticks()+1500UL;}
                settings_save_tick(dock,1,1);
                m->result=dock->save_state.revision==dock->save_state.saved ? ND_CONTROL_OK :
                    dock->save_state.last_failed && !dock->save_state.active ? ND_CONTROL_SAVE_FAILED : ND_CONTROL_PENDING;
            }
            dock->diagnostic_reclaimed_replies+=(ULONG)nd_reply_finish((struct Message *)m);continue;
        }
        if(!quitting && m->magic==ND_CONTROL_MAGIC && m->version==ND_CONTROL_VERSION &&
           (m->action==ND_CONTROL_RELOAD || m->action==ND_CONTROL_RELOAD_BEGIN)){
            struct Task *owner=m->msg.mn_ReplyPort ? m->msg.mn_ReplyPort->mp_SigTask : NULL;
            if(dock->write_owner==owner)dock->write_owner=NULL;
        }
        m->result = ND_CONTROL_INVALID;
        if (!quitting && m->magic == ND_CONTROL_MAGIC &&
            m->version == ND_CONTROL_VERSION &&
            m->action == ND_CONTROL_RELOAD_BEGIN) {
            /* Acknowledge first, so a slow rebuild is never mistaken for a
             * dock that stopped answering; the outcome is kept for
             * RELOAD_STATUS. */
            ULONG ticket=(dock->reload_ticket_issued+1)&0x7fffffffUL;
            if (!ticket) ticket=1;
            dock->reload_ticket_issued=ticket;
            m->result=(LONG)ticket;
            dock->diagnostic_reclaimed_replies+=(ULONG)nd_reply_finish((struct Message *)m);
            dock->reload_done_result=reload_configuration(dock);
            dock->reload_ticket_done=ticket;
            continue;
        }
        if (!quitting &&
            m->magic == ND_CONTROL_MAGIC && m->version == ND_CONTROL_VERSION) {
            if (m->action == ND_CONTROL_RELOAD) m->result = reload_configuration(dock);
            else if (m->action == ND_CONTROL_RELOAD_STATUS &&
                     m->msg.mn_Length >= sizeof(struct NDControlReloadMessage)) {
                struct NDControlReloadMessage *r=
                    (struct NDControlReloadMessage *)m;
                if (!r->ticket || r->ticket>dock->reload_ticket_issued)
                    m->result=ND_CONTROL_INVALID;
                else if (dock->reload_ticket_done>=r->ticket) {
                    /* A later reload supersedes an earlier ticket; either
                     * way this is the configuration now live. */
                    r->outcome=dock->reload_done_result;
                    r->elapsed_ticks=dock->diagnostic_reload_last_ticks;
                    m->result=ND_CONTROL_OK;
                } else m->result=ND_CONTROL_PENDING;
            }
            else if (m->action == ND_CONTROL_QUIT) {
                dock->running = 0;
                m->result = ND_CONTROL_OK;
            } else if (m->action==ND_CONTROL_DIAGNOSTICS &&
                       m->msg.mn_Length>=
                       offsetof(struct NDControlDiagnosticsMessage,snapshot)+
                       offsetof(struct NDDiagnosticsSnapshot,
                                unchanged_updates)) {
                struct NDControlDiagnosticsMessage *request=
                    (struct NDControlDiagnosticsMessage *)m;
                if (m->msg.mn_Length>=
                    sizeof(struct NDControlDiagnosticsMessage))
                    fill_diagnostics(dock,&request->snapshot);
                else {
                    struct NDDiagnosticsSnapshot compatible;
                    ULONG available=m->msg.mn_Length-
                        offsetof(struct NDControlDiagnosticsMessage,snapshot);
                    fill_diagnostics(dock,&compatible);
                    memcpy(&request->snapshot,&compatible,available);
                }
                m->result=ND_CONTROL_OK;
            } else if (m->action==ND_CONTROL_SHOW) {
                show_dock(dock);
                m->result=ND_CONTROL_OK;
            } else if (m->action==ND_CONTROL_HIDE) {
                hide_dock(dock);
                m->result=ND_CONTROL_OK;
            } else if (m->action==ND_CONTROL_TOGGLE) {
                if (dock->hidden) show_dock(dock); else hide_dock(dock);
                m->result=ND_CONTROL_OK;
            } else if (m->action==ND_CONTROL_DIAGNOSTICS_RESET) {
                /* Preserve liveness timestamps/backoff state: this is a
                 * telemetry baseline change, not a watchdog reset. */
                reset_diagnostic_counters(dock);
                m->result=ND_CONTROL_OK;
            }
        }
        dock->diagnostic_reclaimed_replies+=(ULONG)nd_reply_finish((struct Message *)m);
    }
}
/* Persist interactive edits from FULL config, not the compact dock copy:
 * disabled plugins must never disappear after a drag or remove operation. */
void config_dir(const char *path)
{
    BPTR lock = CreateDir((STRPTR)path);
    if (lock) UnLock(lock);
}
int save_and_reload(struct NDDock *dock, struct NDConfig **full)
{
    struct NDLayout layout;
    if (!full || !*full || !dock->screen || dock->write_owner ||
        !dock->save_port) return 0;
    if (!nd_layout_fit_enabled(*full,dock->screen->Width,dock->screen->Height,
                               dock->screen->BarHeight,&layout)) return 0;
    if(apply_configuration(dock,*full)!=ND_CONTROL_OK)return 0;
    /* The caller's buffer becomes the pending edit: no 58 KB copy. */
    accept_settings_save(dock,*full);*full=NULL;
    return 1;
}
void capture_row_layout(struct NDDock *dock,struct NDConfig *full)
{
    int i,row,column,span,persistent;
    if (!dock || !full || vertical(dock) || dock->layout_rows<1) return;
    memset(full->row_counts,0,sizeof(full->row_counts));
    persistent=nd_media_persistent_count(&dock->conf);
    for (i=0;i<persistent;++i)
        if (nd_layout_item_cell(&dock->conf,i,dock->layout_columns,
                                &row,&column,&span) &&
            row>=0 && row<ND_ROWS_MAX) ++full->row_counts[row];
    full->rows=dock->conf.rows;
    full->row_layout=1;
}
/* Return a real persistence result for ARexx, rather than always saying OK. */
static int do_move_row(struct NDDock *dock,int from,int to,int target_row)
{
    struct NDConfig *full;
    int rc, success=0;
    char moved[ND_LABEL_LEN],message[200];
    if (from < 0 || to < 0) return 0;
    if (from>=nd_media_persistent_count(&dock->conf) ||
        to>=nd_media_persistent_count(&dock->conf)) {
        show_feedback(dock,"Temporary tiles stay at the end - pin to keep one",1);
        return 0;
    }
    full=(struct NDConfig *)AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
    if (!full) return 0;
    strncpy(moved,dock->conf.items[from].label,sizeof(moved)-1);
    moved[sizeof(moved)-1]='\0';
    rc=load_edit_configuration(dock,full);
    if (rc<=0) capture_row_layout(dock,full);
    if (rc<=0 && nd_move_visible_to_row(full,from,to,target_row))
        success=save_and_reload(dock,&full);
    if (!success && dock->screen) DisplayBeep(dock->screen);
    if (full) FreeVec(full);
    if (success) {
        sprintf(message,nd_trf("Moved %s"),moved);
        show_feedback(dock,message,0);
    }
    return success;
}
int drop_after_tile(struct NDDock *dock,int index,int mx,int my)
{
    int x,y,w,h;
    if (!dock || index<0 || index>=dock->conf.count) return 0;
    tile_coords(dock,index,&x,&y);
    tile_dimensions(dock,index,&w,&h);
    return vertical(dock) ? nd_drop_after_item(my-y,h) :
                            nd_drop_after_item(mx-x,w);
}
int do_move(struct NDDock *dock,int from,int to,int after)
{
    int row,column,span,destination;
    if (from==to) return 1;
    row=0;
    if (!vertical(dock) && dock->layout_rows>1 &&
        !nd_layout_item_cell(&dock->conf,to,dock->layout_columns,
                             &row,&column,&span)) return 0;
    /* nd_move_full_item's destination is the final array position. Moving
     * forward naturally lands after TO, while moving backward lands before
     * it, so compensate when the pointer requests the opposite edge. */
    destination=nd_drop_destination(from,to,after);
    if (destination<0) destination=0;
    if (destination>=dock->conf.count) destination=dock->conf.count-1;
    return do_move_row(dock,from,destination,row);
}
int do_move_to_empty_space(struct NDDock *dock,int from,int row)
{
    int i,target,count=nd_media_persistent_count(&dock->conf);
    int item_row,column,span,last=-1,first_later=-1;
    if (!dock || from<0 || row<0 || row>=dock->layout_rows || count<1)
        return 0;
    for (i=0;i<count;++i)
        if (nd_layout_item_cell(&dock->conf,i,dock->layout_columns,
                                &item_row,&column,&span) && item_row==row)
            last=i;
    if (last>=0) {
        /* nd_move_full_item inserts before TO when moving left and after TO
         * when moving right. Adjust so an empty-area drop always appends to
         * the selected row instead of displacing its final tile. */
        target=from>last ? last+1 : last;
    } else {
        for (i=0;i<count;++i) {
            if (nd_layout_item_cell(&dock->conf,i,dock->layout_columns,
                                    &item_row,&column,&span) && item_row>row) {
                first_later=i;break;
            }
        }
        if (first_later<0) target=count-1;
        else target=from<first_later ? first_later-1 : first_later;
    }
    if (target<0) target=0;
    if (target>=count) target=count-1;
    return do_move_row(dock,from,target,row);
}
int remove_dragged_item(struct NDDock *dock, int visible)
{
    struct NDConfig *full;
    int rc, success=0;
    char removed[ND_LABEL_LEN],message[200];
    if (visible<0 || visible>=dock->conf.count) return 0;
    if (dock->conf.items[visible].transient) {
        show_feedback(dock,"Temporary tiles follow their disk or application",1);
        return 0;
    }
    strncpy(removed,dock->conf.items[visible].label,sizeof(removed)-1);
    removed[sizeof(removed)-1]='\0';
    full=(struct NDConfig *)AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
    if (!full) return 0;
    rc=load_edit_configuration(dock,full);
    if (rc<=0 && nd_remove_visible_item(full,visible))
        success=save_and_reload(dock,&full);
    if (!success && dock->screen) DisplayBeep(dock->screen);
    if (full) FreeVec(full);
    if (success) {
        sprintf(message,nd_trf("Removed %s"),removed);
        show_feedback(dock,message,0);
    }
    return success;
}
