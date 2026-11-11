/* NovaDock: Plugin processes, the plugin protocol and the watchdog. */
#include "dock_internal.h"
#include "nd_reply.h"

/* Defined below, private to this file. */
static int launch_plugin(struct NDDock *dock,int index,int restarted);
static void wake_plugin(struct NDDock *dock,int index);
static int find_wake_port(struct NDDock *dock,int index,struct Task *task);

/* Is PLUGIN_TASK the owner of the plugin's public wake port?  Called under
 * Forbid() by the caller or for a fresh task; never signals. */
static int find_wake_port(struct NDDock *dock,int index,struct Task *task)
{
    char name[ND_ID_LEN+16];
    struct MsgPort *port;
    int owned;
    if (!task) return 0;
    strcpy(name,ND_WAKE_PORT_PREFIX);
    strcat(name,dock->conf.items[index].id);
    Forbid();
    port=FindPort((STRPTR)name);
    owned=port && port->mp_SigTask==task;
    Permit();
    return owned;
}
/* Wake the plugin of tile INDEX.  A plugin with a wake port is signalled
 * only while that port still names its task, so a task that has exited is
 * never signalled even if Exec reused its memory.  Legacy plugins keep the
 * ready/wait-list check. */
static void wake_plugin(struct NDDock *dock,int index)
{
    struct Task *task=dock->plugin_task[index];
    if (!task) return;
    Forbid();
    if (dock->plugin_wake_port[index] ? find_wake_port(dock,index,task) :
                                        task_exists(task))
        Signal(task,ND_WAKE_SIGNAL);
    else {dock->plugin_task[index]=NULL;dock->plugin_wake_port[index]=0;}
    Permit();
}

static int launch_plugin(struct NDDock *dock,int index,int restarted)
{
    char cmd[1100], path[520];
    const char *p;
    size_t len;
    ULONG now=diagnostic_ticks();
    struct NDItem *it;
    if (!dock || index<0 || index>=dock->conf.count) return 0;
    it=&dock->conf.items[index];
    if (it->kind!=ND_ITEM_PLUGIN) return 0;
    if (restarted) {
        fire_hook(dock,"PLUGIN_STALE",it->id);
        ++dock->watchdog_total_restarts;
        ++dock->watchdog_restarts[index];
        if (dock->watchdog_failures[index]<65535U)
            ++dock->watchdog_failures[index];
    }
    dock->watchdog_retry_after[index]=now+
        (ULONG)nd_watchdog_backoff_ticks(dock->watchdog_failures[index]);
    p=it->command;
    /* "@..." plugin commands are passive tiles fed by ARexx SETVALUE. */
    if (p[0]=='@') return 0;
    if (strchr(p,':')!=NULL) {
        strncpy(path,p,sizeof(path)-1);
        path[sizeof(path)-1]='\0';
    } else {
        len=strlen(dock->directory);
        if (len==0 || len+strlen(p)+2>=sizeof(path)) return 0;
        strcpy(path,dock->directory);
        if (path[len-1]!=':' && path[len-1]!='/') strcat(path,"/");
        strcat(path,p);
    }
    if (!nd_quote_path(cmd,sizeof(cmd)-ND_ID_LEN-2,path)) return 0;
    strcat(cmd," ");strcat(cmd,it->id);
    dock->plugin_task[index]=NULL;
    dock->plugin_woken[index]=0;
    dock->plugin_wake_port[index]=0;
    return launch_command(cmd);
}
/* Run plugin tile INDEX's program as a helper: "<program> ID VERB [path]",
 * then wake the resident so it notices the work at once.  PATH must be
 * quoted with the AmigaDOS path encoder. Legacy drop helpers retain their
 * existing path restrictions; BitSkim accepts literal quotes and stars. */
int launch_plugin_helper(struct NDDock *dock,int index,const char *verb,
                         const char *path)
{
    const struct NDItem *item;
    const char *configured;
    char executable[520],command[2400];
    size_t length;
    int launched;
    if (!dock || index<0 || index>=dock->conf.count || !verb) return 0;
    item=&dock->conf.items[index];
    if (item->kind!=ND_ITEM_PLUGIN) return 0;
    if(path && strcmp(item->id,"BITSKIM") && !nd_archive_safe_path(path))return 0;
    configured=item->command;
    if (strchr(configured,':')) {
        if (strlen(configured)>=sizeof(executable)) return 0;
        strcpy(executable,configured);
    } else {
        length=strlen(dock->directory);
        if (!length || length+strlen(configured)+2>=sizeof(executable)) return 0;
        strcpy(executable,dock->directory);
        if (executable[length-1]!=':' && executable[length-1]!='/')
            strcat(executable,"/");
        strcat(executable,configured);
    }
    if (!nd_quote_path(command,sizeof(command)-80,executable))return 0;
    length=strlen(command);
    if(length+strlen(item->id)+strlen(verb)+4>=sizeof(command))return 0;
    sprintf(command+length," %s %s",item->id,verb);
    if(path){length=strlen(command);command[length++]=' ';command[length]=0;
        if(!nd_quote_path(command+length,sizeof(command)-length,path))return 0;}
    launched=launch_command(command);
    /* The resident normally sleeps for five seconds while idle. Wake it as
     * soon as a helper is launched so EXTRACTING appears without busy polls. */
    if (launched && dock->plugin_task[index]) wake_plugin(dock,index);
    return launched;
}
int launch_archive_helper(struct NDDock *dock,int index,const char *archive)
{
    if (!dock || index<0 || index>=dock->conf.count ||
        !nd_archive_safe_path(archive) ||
        nd_archive_kind(archive)==ND_ARCHIVE_NONE ||
        strcmp(dock->conf.items[index].id,"ARCHIVE")) return 0;
    return launch_plugin_helper(dock,index,"EXTRACT",archive);
}
void start_plugins(struct NDDock *dock, const struct NDConfig *previous)
{
    int i,j;
    for (i = 0; i < dock->conf.count; ++i) {
        struct NDItem *it = &dock->conf.items[i];
        if (it->kind != ND_ITEM_PLUGIN) continue;
        /* Repeated tiles are views of one logical plugin.  Starting a second
         * process with the same ID would make both clients race for one IPC
         * state and one settings namespace. */
        for (j=0;j<i;++j)
            if (dock->conf.items[j].kind==ND_ITEM_PLUGIN &&
                !strcmp(dock->conf.items[j].id,it->id)) break;
        if (j<i) continue;
        if (previous) {
            int j, known = 0;
            for (j = 0; j < previous->count; ++j) {
                const struct NDItem *old = &previous->items[j];
                if (old->kind == ND_ITEM_PLUGIN && !strcmp(old->id, it->id)) {
                    /* A changed executable with the SAME ID cannot be
                     * hot-replaced safely by ABI v1/v2; restart dock. */
                    known = 1;
                    break;
                }
            }
            if (known) continue;
        }
        (void)launch_plugin(dock,i,0);
    }
}
void plugin_watchdog_tick(struct NDDock *dock,int quantum)
{
    ULONG now;
    int i;
    dock->watchdog_timer_ticks+=quantum;
    if (dock->watchdog_timer_ticks<ND_WATCHDOG_CHECK_TIMER_TICKS) return;
    dock->watchdog_timer_ticks=0;
    now=diagnostic_ticks();
    for (i=0;i<dock->conf.count;++i) {
        struct NDItem *item=&dock->conf.items[i];
        int stale=0,j;
        if (item->kind!=ND_ITEM_PLUGIN || item->command[0]=='@') continue;
        for (j=0;j<i;++j)
            if (dock->conf.items[j].kind==ND_ITEM_PLUGIN &&
                !strcmp(dock->conf.items[j].id,item->id)) break;
        if (j<i) continue;
        if (item->online && dock->diagnostic_plugin_updates[i])
            stale=nd_watchdog_elapsed(now,dock->diagnostic_plugin_last[i],
                                      ND_WATCHDOG_STALE_TICKS);
        if (item->online && !stale) continue;
        if (dock->watchdog_retry_after[i] &&
            !nd_watchdog_deadline_reached(now,dock->watchdog_retry_after[i]))
            continue;
        if (item->online) {
            item->online=0;
            /* Recovery is normally shorter than one redraw.  Preserve the
             * last valid value, meter and graph rather than flashing STALE
             * and clearing useful history while the replacement starts.
             * The badge belongs to the stalled process, though: the new one
             * starts without and sends its own, so clear it on every tile. */
            item->badge[0]='\0';
            sync_plugin_twins(dock,i);
            mark_tile_dirty(dock,i);
        }
        (void)launch_plugin(dock,i,1);
    }
}
struct NDItem *item_by_id(struct NDDock *dock, const char *id)
{
    int i;
    for (i = 0; i < dock->conf.count; ++i)
        if (dock->conf.items[i].kind == ND_ITEM_PLUGIN &&
            !strcmp(dock->conf.items[i].id, id)) return &dock->conf.items[i];
    return NULL;
}
void sync_plugin_twins(struct NDDock *dock,int source)
{
    const struct NDItem *from=&dock->conf.items[source];
    int i,changed;
    for (i=0;i<dock->conf.count;++i) {
        struct NDItem *to=&dock->conf.items[i];
        if (i==source || to->kind!=ND_ITEM_PLUGIN || strcmp(to->id,from->id))
            continue;
        changed=to->online!=from->online || to->meter!=from->meter ||
                strcmp(to->caption,from->caption) || strcmp(to->value,from->value) ||
                to->media_capabilities!=from->media_capabilities ||
                to->media_state!=from->media_state ||
                to->media_volume!=from->media_volume ||
                strcmp(to->badge,from->badge);
        strcpy(to->badge,from->badge);
        strcpy(to->caption,from->caption);
        strcpy(to->value,from->value);
        to->meter=from->meter;
        to->online=from->online;
        to->media_capabilities=from->media_capabilities;
        to->media_state=from->media_state;
        to->media_volume=from->media_volume;
        dock->history[i]=dock->history[source];
        dock->graph_samples[i]=dock->graph_samples[source];
        if (dock->graph_step[source]) dock->graph_step[i]=1;
        dock->diagnostic_plugin_updates[i]=dock->diagnostic_plugin_updates[source];
        dock->diagnostic_plugin_last[i]=dock->diagnostic_plugin_last[source];
        if (changed) mark_tile_dirty(dock,i);
    }
}
void process_ipc(struct NDDock *dock, int quitting)
{
    struct NDMessage *m;
    struct NDItem *item;
    struct NDGraphMessage *gm;
    struct NDMediaMessage *media;
    struct NDExtMessage *ext;
    struct Task *task;
    unsigned int sample_a, sample_b;
    unsigned int new_meter;
    int index, changed,i,packet_abandoned,graph_only,has_graph,has_media;
    unsigned char abandoned[ND_MAX_ITEMS];
    memset(abandoned,0,sizeof(abandoned));
    while ((m = (struct NDMessage *)GetMsg(dock->ipc)) != NULL) {
        if (m->exec_msg.mn_Length < sizeof(struct NDMessage)) {
            dock->diagnostic_reclaimed_replies+=(ULONG)nd_reply_finish((struct Message *)m);
            continue;
        }
        m->event = ND_EVENT_QUIT;
        if (!quitting && m->magic == ND_MAGIC &&
            (m->version == ND_ABI_VERSION ||
             (m->version == ND_GRAPH_ABI_VERSION &&
              m->exec_msg.mn_Length >= sizeof(struct NDGraphMessage)) ||
             (m->version == ND_MEDIA_ABI_VERSION &&
              m->exec_msg.mn_Length >= sizeof(struct NDMediaMessage)) ||
             (m->version == ND_EXT_ABI_VERSION &&
              m->exec_msg.mn_Length >= sizeof(struct NDExtMessage)))) {
            m->id[ND_ID_LEN - 1] = '\0';
            m->caption[ND_TEXT_LEN - 1] = '\0';
            m->value[ND_TEXT_LEN - 1] = '\0';
            item = item_by_id(dock, m->id);
            ext = m->version==ND_EXT_ABI_VERSION ? (struct NDExtMessage *)m : NULL;
            if (ext) ext->event2=0;
            has_graph = m->version==ND_GRAPH_ABI_VERSION ||
                        (ext && (ext->flags&ND_EXT_HAS_GRAPH));
            has_media = m->version==ND_MEDIA_ABI_VERSION ||
                        (ext && (ext->flags&ND_EXT_HAS_MEDIA));
            if (item) {
                m->event = ND_EVENT_NONE;
                if (m->command == ND_MSG_UPDATE) {
                    packet_abandoned=0;
                    index = (int)(item - dock->conf.items);
                    /* Repeated tiles share this first logical client.  Fold
                     * actions raised on any view into the client reply so a
                     * settings change made from the second Eyes tile is not
                     * stranded on a tile which owns no process.  This runs
                     * before the media branch below, so a control pressed on
                     * a second view goes out in this reply, not the next. */
                    for (i=0;i<dock->conf.count;++i) {
                        struct NDItem *view=&dock->conf.items[i];
                        if (i==index || view->kind!=ND_ITEM_PLUGIN ||
                            strcmp(view->id,item->id)) continue;
                        item->clicks+=view->clicks;
                        view->clicks=0;
                        dock->plugin_event2[index]|=dock->plugin_event2[i];
                        dock->plugin_event2[i]=0;
                        if (view->settings_dirty) {
                            item->settings_dirty=1;
                            view->settings_dirty=0;
                        }
                        if (view->media_event) {
                            item->media_event|=view->media_event;
                            if ((view->media_event&ND_EVENT_MEDIA_VOLUME) &&
                                view->media_volume_request<=100U)
                                item->media_volume_request=
                                    view->media_volume_request;
                            view->media_event=0;
                            view->media_volume_request=
                                ND_MEDIA_VOLUME_UNKNOWN;
                        }
                    }
                    changed = !item->online;
                    /* A graph tile already online with the same meter only
                     * needs its new sample scrolled in (draw_graph_step). */
                    graph_only = item->online;
                    if (m->caption[0]) {
                        if (strcmp(item->caption, m->caption)) changed = 1;
                        strncpy(item->caption, m->caption, sizeof(item->caption) - 1);
                        item->caption[sizeof(item->caption) - 1] = '\0';
                    }
                    if (strcmp(item->value, m->value)) changed = 1;
                    strncpy(item->value, m->value, sizeof(item->value) - 1);
                    item->value[sizeof(item->value) - 1] = '\0';
                    item->online = 1;
                    new_meter = m->meter <= 100 ? m->meter : ND_NO_METER;
                    if (item->meter != new_meter) changed = 1;
                    if (item->meter != new_meter) graph_only = 0;
                    item->meter = new_meter;
                    sample_a = item->meter;
                    sample_b = ND_GRAPH_NO_SAMPLE;
                    if (has_graph && ext) {
                        sample_a = ext->graph_a;
                        sample_b = ext->graph_b;
                    } else if (has_graph) {
                        gm = (struct NDGraphMessage *)m;
                        sample_a = gm->graph_a;
                        sample_b = gm->graph_b;
                    } else if (has_media) {
                        media = (struct NDMediaMessage *)m;
                        if (item->media_capabilities != media->capabilities ||
                            item->media_state != media->state ||
                            item->media_volume != media->volume) changed = 1;
                        item->media_capabilities = media->capabilities;
                        item->media_state = media->state;
                        item->media_volume = media->volume <= 100U ?
                            media->volume : ND_MEDIA_VOLUME_UNKNOWN;
                        media->requested_volume = ND_MEDIA_VOLUME_UNKNOWN;
                        if (item->media_event) {
                            m->event |= item->media_event;
                            if ((item->media_event & ND_EVENT_MEDIA_VOLUME) &&
                                item->media_volume_request <= 100U) {
                                media->requested_volume = item->media_volume_request;
                                item->media_volume = item->media_volume_request;
                            }
                            if (item->media_event & ND_EVENT_MEDIA_PLAY_PAUSE)
                                item->media_state ^= ND_MEDIA_STATE_PLAYING;
                            item->media_event = 0;
                            item->media_volume_request = ND_MEDIA_VOLUME_UNKNOWN;
                        }
                    }
                    task=m->exec_msg.mn_ReplyPort ?
                        m->exec_msg.mn_ReplyPort->mp_SigTask : NULL;
                    /* Look for its wake port only when the process changed. */
                    if (task!=dock->plugin_task[index])
                        dock->plugin_wake_port[index]=
                            (unsigned char)find_wake_port(dock,index,task);
                    dock->plugin_task[index]=task;
                    /* Second-word events go out only in a v5 reply; an
                     * older client could never receive them. */
                    if (ext) ext->event2=dock->plugin_event2[index];
                    dock->plugin_event2[index]=0;
                    if (!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW")) {
                        /* OFFLINE, or packet rates / ONLINE.  Transitions
                         * only: the first report after start-up sets the
                         * state without running a hook. */
                        unsigned char state=!strcmp(item->value,"OFFLINE") ? 2 :
                            (strstr(item->value,"p/s") ||
                             !strcmp(item->value,"ONLINE")) ? 1 : 0;
                        if (state && dock->net_state[index] &&
                            state!=dock->net_state[index])
                            fire_hook(dock,state==1 ? "NET_ONLINE" : "NET_OFFLINE",
                                      item->id);
                        if (state) dock->net_state[index]=state;
                    }
                    /* A timed-out client deliberately leaves an inert reply
                     * port (PA_IGNORE/no signal task) so this queued packet
                     * remains memory-safe.  Its process is exiting, however,
                     * so do not mistake the late packet for a healthy client
                     * and wait for the stale watchdog before replacing it. */
                    if (!m->exec_msg.mn_ReplyPort ||
                        m->exec_msg.mn_ReplyPort->mp_Flags==PA_IGNORE ||
                        !dock->plugin_task[index]) {
                        abandoned[index]=1;
                        packet_abandoned=1;
                    }
                    dock->plugin_woken[index]=0;
                    ++dock->diagnostic_updates;
                    ++dock->diagnostic_plugin_updates[index];
                    if (!packet_abandoned) {
                        dock->diagnostic_plugin_last[index]=diagnostic_ticks();
                        dock->watchdog_failures[index]=0;
                        dock->watchdog_retry_after[index]=0;
                    }
                    nd_graph_push(&dock->history[index], sample_a, sample_b);
                    ++dock->graph_samples[index];
                    if (item->graph && strcmp(item->id,"DOPUS4") &&
                        strcmp(item->id,"ARCHIVE")) {
                        if (graph_only) {
                            dock->graph_step[index]=1;
                            changed=0;
                        } else changed = 1;
                    } else graph_only = 0;
                    if(!strcmp(item->id,"BITSKIM")){
                        m->event|=item->media_event&ND_EVENT_BITSKIM_ACTIONS;
                        item->media_event&=~ND_EVENT_BITSKIM_ACTIONS;
                    }
                    if(item->media_event&(ND_EVENT_NETWORK_ONLINE|ND_EVENT_NETWORK_OFFLINE)){
                        m->event|=item->media_event&(ND_EVENT_NETWORK_ONLINE|ND_EVENT_NETWORK_OFFLINE);
                        item->media_event&=~(ND_EVENT_NETWORK_ONLINE|ND_EVENT_NETWORK_OFFLINE);
                    }
                    if (item->media_event&ND_EVENT_MANAGE_HISTORY) {
                        m->event|=ND_EVENT_MANAGE_HISTORY;
                        item->media_event&=~ND_EVENT_MANAGE_HISTORY;
                    }
                    if (item->clicks) {
                        --item->clicks;
                        m->event |= ND_EVENT_CLICK;
                    }
                    if (item->settings_dirty) {
                        item->settings_dirty=0;
                        m->event |= ND_EVENT_SETTINGS;
                    }
                    if(changed && (!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW")) &&
                       (!strcmp(item->value,"ACTION FAILED") || !strcmp(item->value,"WORKER FAILED")))
                        show_feedback(dock,"Could not change network state - check Network settings",1);
                    if (changed) {
                        if (dock->dirty_tiles[index])
                            ++dock->diagnostic_coalesced_updates;
                        mark_tile_dirty(dock,index);
                    } else if (!graph_only) ++dock->diagnostic_unchanged_updates;
                    sync_plugin_twins(dock,index);
                } else if (m->command == ND_MSG_BADGE) {
                    char badge[ND_BADGE_LEN];
                    strncpy(badge,m->value,ND_BADGE_LEN-1);
                    badge[ND_BADGE_LEN-1]='\0';
                    index = (int)(item - dock->conf.items);
                    if (strcmp(item->badge,badge)) {
                        strcpy(item->badge,badge);
                        mark_tile_dirty(dock,index);
                    }
                    sync_plugin_twins(dock,index);
                } else if (m->command == ND_MSG_GOODBYE) {
                    changed = item->online || strcmp(item->value, "OFFLINE") ||
                              dock->history[(int)(item - dock->conf.items)].count;
                    item->online = 0;
                    item->badge[0] = '\0';
                    strcpy(item->value, "OFFLINE");
                    item->meter = ND_NO_METER;
                    item->media_capabilities = 0;
                    item->media_state = 0;
                    item->media_volume = ND_MEDIA_VOLUME_UNKNOWN;
                    item->media_event = 0;
                    item->media_volume_request = ND_MEDIA_VOLUME_UNKNOWN;
                    index = (int)(item - dock->conf.items);
                    dock->plugin_task[index]=NULL;
                    dock->plugin_woken[index]=0;
                    dock->plugin_wake_port[index]=0;
                    dock->plugin_event2[index]=0;
                    dock->watchdog_retry_after[index]=diagnostic_ticks()+
                        ND_WATCHDOG_GOODBYE_TICKS;
                    nd_graph_reset(&dock->history[index]);
                    if (changed) mark_tile_dirty(dock, index);
                    sync_plugin_twins(dock,index);
                } else m->event = ND_EVENT_QUIT;
                if ((dock->hidden || dock->covered) && !(m->event&ND_EVENT_QUIT))
                    m->event|=ND_EVENT_THROTTLE;
                /* Tell plugins this dock understands ND_MSG_BADGE. */
                if (!(m->event&ND_EVENT_QUIT))
                    m->event|=ND_EVENT_HOST_BADGES;
            }
        }
        dock->diagnostic_reclaimed_replies+=(ULONG)nd_reply_finish((struct Message *)m);
    }
    if (!quitting) for (i=0;i<dock->conf.count;++i)
        if (abandoned[i] &&
            dock->conf.items[i].kind==ND_ITEM_PLUGIN) {
            dock->plugin_task[i]=NULL;
            dock->plugin_wake_port[i]=0;
            /* Do not immediately relaunch a client which has timed out.  Its
             * existing retry deadline and the stale watchdog provide bounded
             * recovery with backoff; immediate replacement created an
             * unbounded restart storm when a late timer signal was observed. */
        }
    /* A running timer coalesces simultaneous plugin messages into one paint
     * pass. If timer.device is unavailable, retain the immediate fallback. */
    if (!dock->timer_open && !quitting) flush_dirty_tiles(dock);
}
int register_port(struct NDDock *dock)
{
    int registered = 0;
    dock->ipc = CreateMsgPort();
    if (!dock->ipc) return 0;
    dock->ipc->mp_Node.ln_Name = (STRPTR)ND_PORT_NAME;
    /* Advertises the highest plugin ABI understood (see nd_protocol.h). */
    dock->ipc->mp_Node.ln_Pri = ND_HOST_ABI;
    Forbid();
    if (!FindPort(ND_PORT_NAME)) {
        AddPort(dock->ipc);
        registered = 1;
    }
    Permit();
    dock->is_public = registered;
    if (!registered) return 0;
    dock->control = CreateMsgPort();
    if (!dock->control) return 0;
    dock->control->mp_Node.ln_Name = (STRPTR)ND_CONTROL_PORT;
    Forbid();
    if (!FindPort(ND_CONTROL_PORT)) {
        AddPort(dock->control);
        dock->control_public = 1;
    }
    Permit();
    return dock->control_public;
}
void remove_port(struct NDDock *dock)
{
    if (dock->control) {
        if (dock->control_public) {
            Forbid(); RemPort(dock->control); Permit();
            dock->control_public = 0;
        }
        process_control(dock, 1);
        DeleteMsgPort(dock->control);
        dock->control = NULL;
    }
    if (!dock->ipc) return;
    if (dock->is_public) {
        Forbid();
        RemPort(dock->ipc);
        Permit();
        dock->is_public = 0;
    }
    /* No new writers after RemPort; drain queued sends before freeing port. */
    process_ipc(dock, 1);
    DeleteMsgPort(dock->ipc);
    dock->ipc = NULL;
}
/* Standard Classic Amiga ARexx host: no MUI or external Rexx GUI needed.
 * If rexxsyslib.library is absent, the dock runs normally without ARexx. */
/* A plugin's task is only signalled while it still exists: the scan runs
 * under Forbid/Disable so it cannot exit between the check and Signal(). */
int task_exists(struct Task *task)
{
    struct List *lists[2];
    struct Node *node;
    int list,found=0;
    if (!task) return 0;
    lists[0]=&SysBase->TaskReady;
    lists[1]=&SysBase->TaskWait;
    Disable();
    for (list=0;list<2 && !found;++list)
        for (node=lists[list]->lh_Head;node->ln_Succ;node=node->ln_Succ)
            if (node==(struct Node *)task) {found=1;break;}
    Enable();
    return found;
}
void wake_pending_plugins(struct NDDock *dock)
{
    int i;
    for (i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i) {
        struct NDItem *item=&dock->conf.items[i];
        if (item->kind!=ND_ITEM_PLUGIN || !dock->plugin_task[i] ||
            dock->plugin_woken[i] ||
            !(item->clicks || item->media_event || item->settings_dirty ||
              dock->plugin_event2[i]))
            continue;
        wake_plugin(dock,i);
        dock->plugin_woken[i]=1;
    }
}
