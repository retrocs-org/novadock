/* NovaDock: The ARexx host. */
#include "dock_internal.h"

/* Defined below, private to this file. */
static int item_by_label(struct NDDock *dock,const char *label);
static int rexx_set_value(struct NDDock *dock,const struct NDRexxCommand *cmd);
static int rexx_set_badge(struct NDDock *dock,const struct NDRexxCommand *cmd);
static const char *item_kind_name(const struct NDItem *item);
static int rexx_list_stem(struct NDDock *dock,struct RexxMsg *m,const char *stem);
static char *rexx_list_text(struct NDDock *dock);
static int rexx_get(struct NDDock *dock,const char *key,char *out,unsigned int size);
static int rexx_set(struct NDDock *dock,const char *key,const char *value,char *out,unsigned int size);
static void rexx_info(struct NDDock *dock,int index,char *out);
static void rexx_status_detail(struct NDDock *dock,char *out);

/* GET key: the current value, as the configuration file stores it. */
static int rexx_get(struct NDDock *dock,const char *key,char *out,unsigned int size)
{
    const struct NDConfig *cfg=dock->edit_config ? dock->edit_config : &dock->conf;
    char stored[ND_RX_KEY_LEN+8];
    if (!nd_config_canonical_key(cfg,key,stored,sizeof(stored))) return 0;
    return nd_config_get(cfg,stored,out,size);
}
static int known_theme(const char *id)
{
    int i;
    for (i=0;i<ND_THEME_COUNT;++i) {
        const char *a=nd_theme_directory(i),*b=id;
        while (*a && *b && toupper((unsigned char)*a)==toupper((unsigned char)*b)) {++a;++b;}
        if (!*a && !*b) return 1;
    }
    return 0;
}
/* SET key value (and THEME id): change one global setting or plugin option,
 * apply it the way Preferences' Use does, and queue it for saving.  OUT gets
 * the stored value or the reason it was refused. */
static int rexx_set(struct NDDock *dock,const char *key,const char *value,char *out,unsigned int size)
{
    struct NDConfig *full;
    char line[ND_RX_KEY_LEN+ND_RX_TEXT_LEN+10],name[ND_RX_KEY_LEN+8];
    int ok=0;
    strcpy(out,"INVALID");
    if (strlen(key)+strlen(value)+2>sizeof(line)) return 0;
    full=(struct NDConfig *)AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
    if (!full) {strcpy(out,"NO MEMORY");return 0;}
    if (!strcmp(key,"THEME") && !known_theme(value)) {
        /* The file keeps unknown theme ids for newer versions; ARexx only
         * switches to a theme this version has. */
        FreeVec(full);strcpy(out,"UNKNOWN THEME");return 0;
    }
    if (load_edit_configuration(dock,full)<=0 &&
        nd_config_canonical_key(full,key,name,sizeof(name)) && nd_config_settable(full,name)) {
        sprintf(line,"%s=%s",name,value);
        if (nd_parse_line(full,line) && nd_config_get(full,name,out,size)) {
            char stored[ND_CONFIG_VALUE_LEN];
            struct NDLayout layout;
            strcpy(stored,out);
            /* Say why a valid value could not be applied. */
            if (!dock->screen || dock->write_owner || !dock->save_port)
                strcpy(out,"BUSY");
            else if (!nd_layout_fit_enabled(full,dock->screen->Width,dock->screen->Height,
                                            dock->screen->BarHeight,&layout))
                strcpy(out,"DOES NOT FIT");
            else if ((ok=queue_settings_save(dock,&full))!=0) {
                /* Validated and fitted above; the live reload is batched
                 * with any SETs that follow (see flush_rexx_settings). */
                strcpy(out,stored);
                dock->rexx_apply_pending=1;
                dock->rexx_apply_ticks=ND_REXX_APPLY_QUIET;
                dock->rexx_apply_failures=0;
            }
            else strcpy(out,"FAILED");
        } else strcpy(out,"INVALID");
    } else strcpy(out,"UNKNOWN");
    if (full) FreeVec(full);
    return ok;
}
/* INFO n: KIND|LABEL|COMMAND|ID|STATE for tile n (1-based, as LIST). */
static void rexx_info(struct NDDock *dock,int index,char *out)
{
    const struct NDItem *item=&dock->conf.items[index];
    const char *state="-";
    if (item->kind==ND_ITEM_PLUGIN) state=item->online ? "ONLINE":"OFFLINE";
    else if (item->kind==ND_ITEM_LAUNCH || item->transient==ND_TRANSIENT_TASK)
        state=dock->running_items[index] ? "RUNNING":"IDLE";
    sprintf(out,"%s|%.31s|%.255s|%.19s|%s",item_kind_name(item),item->label,
            item->kind==ND_ITEM_SPACER ? "" : item->command,
            item->kind==ND_ITEM_PLUGIN ? item->id : "",state);
}
/* STATUS DETAIL: paint and wake counters for measuring on real hardware. */
static void rexx_status_detail(struct NDDock *dock,char *out)
{
    sprintf(out,"RUNNING=%d FULL_PAINTS=%lu TILE_PAINTS=%lu HOVER_FULL=%lu HOVER_PARTIAL=%lu "
            "HOVER_SKIPPED=%lu WAKES=%lu UPDATES=%lu COALESCED=%lu TIMER_HZ=%d SECONDS=%lu",
            dock->conf.count,(unsigned long)dock->diagnostic_full_paints,
            (unsigned long)dock->diagnostic_tile_paints,(unsigned long)dock->diagnostic_hover_full,
            (unsigned long)dock->diagnostic_hover_partial,(unsigned long)dock->diagnostic_hover_skipped,
            (unsigned long)dock->diagnostic_wakes,(unsigned long)dock->diagnostic_updates,
            (unsigned long)dock->diagnostic_coalesced_updates,
            !dock->timer_open ? 10 : dock->timer_anim ? 25 : dock->timer_quanta>1 ? 1:5,
            (unsigned long)((diagnostic_ticks()-dock->diagnostic_started)/50UL));
}
static const char *item_kind_name(const struct NDItem *item)
{
    if (item->transient==ND_TRANSIENT_MEDIA) return "DISK";
    if (item->transient==ND_TRANSIENT_TASK) return "TASK";
    switch (item->kind) {
    case ND_ITEM_LAUNCH: return "LAUNCHER";
    case ND_ITEM_DRAWER: return "DRAWER";
    case ND_ITEM_GROUP: return "GROUP";
    case ND_ITEM_PLUGIN: return "PLUGIN";
    case ND_ITEM_SPACER: return "SPACER";
    }
    return "UNKNOWN";
}
/* LIST STEM name.: name.COUNT, and name.n.LABEL/KIND/ID/VALUE for every
 * visible tile (1-based, the same numbers OPEN/MOVE/CLICK use).  The
 * variables are set in the calling script while it waits for this reply. */
static int rexx_list_stem(struct NDDock *dock,struct RexxMsg *m,const char *stem)
{
    char name[ND_RX_STEM_LEN+16],number[12];
    int i;
    sprintf(number,"%d",dock->conf.count);
    sprintf(name,"%sCOUNT",stem);
    if (SetRexxVar(m,name,number,(LONG)strlen(number))) return 0;
    for (i=0;i<dock->conf.count;++i) {
        const struct NDItem *item=&dock->conf.items[i];
        const char *id=item->kind==ND_ITEM_PLUGIN ? item->id : "";
        const char *value=item->kind==ND_ITEM_PLUGIN ? item->value : "";
        const char *kind=item_kind_name(item);
        sprintf(name,"%s%d.LABEL",stem,i+1);
        if (SetRexxVar(m,name,(STRPTR)item->label,(LONG)strlen(item->label)))
            return 0;
        sprintf(name,"%s%d.KIND",stem,i+1);
        if (SetRexxVar(m,name,(STRPTR)kind,(LONG)strlen(kind))) return 0;
        sprintf(name,"%s%d.ID",stem,i+1);
        if (SetRexxVar(m,name,(STRPTR)id,(LONG)strlen(id))) return 0;
        sprintf(name,"%s%d.VALUE",stem,i+1);
        if (SetRexxVar(m,name,(STRPTR)value,(LONG)strlen(value))) return 0;
    }
    return 1;
}
/* Plain LIST: "n:label;..." for every tile, in a buffer sized for the full
 * dock (the 550-byte result buffer used to truncate it silently). */
static char *rexx_list_text(struct NDDock *dock)
{
    char *text=(char *)AllocVec((ULONG)ND_MAX_ITEMS*(ND_LABEL_LEN+8)+1,MEMF_PUBLIC);
    int i;
    if (!text) return NULL;
    text[0]='\0';
    for (i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i)
        sprintf(text+strlen(text),"%d:%s%s",i+1,dock->conf.items[i].label,
                i+1==dock->conf.count ? "" : ";");
    return text;
}

static int item_by_label(struct NDDock *dock,const char *label)
{
    int i;
    for (i=0;i<dock->conf.count;++i)
        if (!nd_text_matches_pattern(label,dock->conf.items[i].label)) continue;
        else return i;
    return -1;
}
/* ADD path: append an application or drawer launcher, as a Workbench drop. */
int rexx_add_launcher(struct NDDock *dock,const char *path)
{
    struct NDConfig *full;
    struct FileInfoBlock *fib;
    char name[ND_LABEL_LEN],icon[ND_COMMAND_LEN];
    BPTR lock;
    APTR requesters;
    int kind=ND_ITEM_LAUNCH,ok=0;
    if (!path || strlen(path)>=ND_COMMAND_LEN) return 0;
    /* A path on an absent volume fails quietly instead of stopping the dock. */
    requesters=quiet_requesters();
    lock=Lock((STRPTR)path,ACCESS_READ);
    restore_requesters(requesters);
    if (!lock) return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib) {
        if (Examine(lock,fib) && fib->fib_DirEntryType>0) kind=ND_ITEM_DRAWER;
        FreeDosObject(DOS_FIB,fib);
    }
    UnLock(lock);
    nd_label_from_path(path,name,sizeof(name));
    if (!*name) return 0;
    dropped_icon_path(path,kind,icon,sizeof(icon));
    full=(struct NDConfig *)AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
    if (!full) return 0;
    if (load_edit_configuration(dock,full)<=0 &&
        nd_insert_launcher_visible(full,nd_media_persistent_count(&dock->conf),
                                   kind,name,path,icon))
        ok=save_and_reload(dock,&full);
    if (full) FreeVec(full);
    return ok;
}
/* SETVALUE drives passive tiles: PLUGIN=Name|@rexx|ID|METER|ON.  Real
 * plugin processes keep ownership of their own tiles. */
/* SETBADGE: a badge on any plugin tile (shared by its twins).  A plugin
 * that sets its own badges replaces it with its next change; goodbye and
 * watchdog restarts clear it, as for plugin badges. */
static int rexx_set_badge(struct NDDock *dock,const struct NDRexxCommand *cmd)
{
    struct NDItem *item=item_by_id(dock,cmd->id);
    int index;
    if (!item || item->kind!=ND_ITEM_PLUGIN) return 0;
    index=(int)(item-dock->conf.items);
    if (strcmp(item->badge,cmd->text)) {
        strncpy(item->badge,cmd->text,sizeof(item->badge)-1);
        item->badge[sizeof(item->badge)-1]='\0';
        mark_tile_dirty(dock,index);
        sync_plugin_twins(dock,index);
        flush_dirty_tiles(dock);
    }
    return 1;
}
static int rexx_set_value(struct NDDock *dock,const struct NDRexxCommand *cmd)
{
    struct NDItem *item=item_by_id(dock,cmd->id);
    int index;
    unsigned short meter;
    if (!item || item->command[0]!='@') return 0;
    index=(int)(item-dock->conf.items);
    meter=cmd->meter>=0 ? (unsigned short)cmd->meter : ND_NO_METER;
    if (strcmp(item->caption,cmd->text) || strcmp(item->value,cmd->text2) ||
        item->meter!=meter || !item->online) {
        strncpy(item->caption,cmd->text,sizeof(item->caption)-1);
        item->caption[sizeof(item->caption)-1]='\0';
        strncpy(item->value,cmd->text2,sizeof(item->value)-1);
        item->value[sizeof(item->value)-1]='\0';
        item->meter=meter;
        item->online=1;
        ++dock->graph_samples[index];
        nd_graph_push(&dock->history[index],
                      cmd->meter>=0 ? (unsigned int)cmd->meter :
                      ND_GRAPH_NO_SAMPLE,ND_GRAPH_NO_SAMPLE);
        mark_tile_dirty(dock,index);
        flush_dirty_tiles(dock);
    }
    return 1;
}
void start_rexx(struct NDDock *dock)
{
    if (!RexxSysBase) return;
    dock->rexx=CreateMsgPort();
    if (!dock->rexx) return;
    dock->rexx->mp_Node.ln_Name=(STRPTR)ND_REXX_PORT;
    Forbid();
    if (!FindPort(ND_REXX_PORT)) {
        AddPort(dock->rexx);
        dock->rexx_public=1;
    }
    Permit();
    if (!dock->rexx_public) {
        DeleteMsgPort(dock->rexx); dock->rexx=NULL;
    }
}
void stop_rexx(struct NDDock *dock)
{
    struct RexxMsg *m;
    if (!dock->rexx) return;
    if (dock->rexx_public) {
        Forbid(); RemPort(dock->rexx); Permit();
        dock->rexx_public=0;
    }
    while ((m=(struct RexxMsg *)GetMsg(dock->rexx))!=NULL) {
        m->rm_Result1=20;
        m->rm_Result2=0;
        ReplyMsg((struct Message *)m);
    }
    DeleteMsgPort(dock->rexx); dock->rexx=NULL;
}
void process_rexx(struct NDDock *dock)
{
    struct RexxMsg *m;
    struct NDRexxCommand cmd;
    char result[550], *long_result;
    int valid;
    while (dock->rexx &&
           (m=(struct RexxMsg *)GetMsg(dock->rexx))!=NULL) {
        result[0]='\0';
        long_result=NULL;
        m->rm_Result1=0;
        m->rm_Result2=0;
        valid=IsRexxMsg(m) &&
              ((m->rm_Action & RXCODEMASK) == RXCOMM) &&
              nd_parse_rexx(m->rm_Args[0],&cmd);
        /* Commands that read or change the live dock see queued SETs first. */
        if (valid && cmd.action!=ND_RX_SET && cmd.action!=ND_RX_THEME &&
            cmd.action!=ND_RX_GET && cmd.action!=ND_RX_PING &&
            cmd.action!=ND_RX_VERSION && cmd.action!=ND_RX_HELP)
            flush_rexx_settings(dock);
        if (!valid) { m->rm_Result1=10; strcpy(result,"Invalid command"); }
        else switch (cmd.action) {
        case ND_RX_PING: strcpy(result,"PONG"); break;
        case ND_RX_VERSION: strcpy(result,ND_VERSION); break;
        case ND_RX_STATUS:
            if (cmd.first) rexx_status_detail(dock,result);
            else sprintf(result,"RUNNING %d",dock->conf.count);
            break;
        case ND_RX_GET:
            if (!rexx_get(dock,cmd.text2,result,sizeof(result))) {m->rm_Result1=5;strcpy(result,"UNKNOWN");}
            break;
        case ND_RX_SET:
        case ND_RX_THEME:
            if (!rexx_set(dock,cmd.text2,cmd.text,result,sizeof(result))) m->rm_Result1=10;
            break;
        case ND_RX_INFO:
            if (cmd.first>dock->conf.count) {m->rm_Result1=5;strcpy(result,"NOT FOUND");}
            else rexx_info(dock,cmd.first-1,result);
            break;
        case ND_RX_SAVE:
            if (dock->save_state.revision==dock->save_state.saved && !dock->save_state.active)
                strcpy(result,"OK");
            else {settings_save_tick(dock,0,1);strcpy(result,"QUEUED");}
            break;
        case ND_RX_PROFILE:
            if (cmd.first==ND_RX_PROFILE_LIST) {
                char names[20][ND_PROFILE_LEN];
                int count=list_profiles(names,20,NULL),k;
                result[0]='\0';
                for (k=0;k<count;++k) {if (k) strcat(result,";");strcat(result,names[k]);}
            } else if (!nd_profile_name_valid(cmd.text)) {m->rm_Result1=10;strcpy(result,"INVALID");}
            else if (cmd.first==ND_RX_PROFILE_SAVE) {
                if (save_profile(dock,cmd.text)) strcpy(result,"OK"); else m->rm_Result1=10;
            } else if (cmd.first==ND_RX_PROFILE_DELETE) {
                if (delete_profile(dock,cmd.text)) strcpy(result,"OK");
                else {m->rm_Result1=5;strcpy(result,"NOT FOUND");}
            } else {
                char path[ND_COMMAND_LEN];
                BPTR lock;
                (void)nd_profile_path(cmd.text,path,sizeof(path));
                lock=Lock((STRPTR)path,ACCESS_READ);
                if (!lock) {m->rm_Result1=5;strcpy(result,"NOT FOUND");}
                else {UnLock(lock);if (use_profile(dock,cmd.text)) strcpy(result,"OK"); else m->rm_Result1=10;}
            }
            break;
        case ND_RX_UNDO:
            if (undo_last_edit(dock)) strcpy(result,"OK");
            else {m->rm_Result1=5;strcpy(result,"NOTHING");}
            break;
        case ND_RX_HELP:
            strcpy(result,"PING VERSION STATUS LIST [STEM var] OPEN n|label MOVE a b "
                   "CLICK n REMOVE n ADD path SETVALUE id caption value [meter] "
                   "SETBADGE id [text] GET key SET key value THEME id INFO n "
                   "SAVE UNDO STATUS DETAIL PROFILE [USE|SAVE|DELETE] name PROFILE LIST "
                   "SHOW HIDE TOGGLE LOCK UNLOCK SUSPEND RESUME RELOAD PREFS RESTART QUIT"); break;
        case ND_RX_LIST:
            if (cmd.text[0]) {
                if (rexx_list_stem(dock,m,cmd.text))
                    sprintf(result,"%d",dock->conf.count);
                else m->rm_Result1=10;
            } else if (!(long_result=rexx_list_text(dock))) m->rm_Result1=20;
            break;
        case ND_RX_OPEN:
            if (cmd.first==0) {
                cmd.first=item_by_label(dock,cmd.text)+1;
                if (cmd.first<=0) { m->rm_Result1=5; strcpy(result,"NOT FOUND"); break; }
            }
            if (cmd.first > dock->conf.count ||
                dock->conf.items[cmd.first-1].kind == ND_ITEM_PLUGIN ||
                dock->conf.items[cmd.first-1].kind == ND_ITEM_SPACER)
                m->rm_Result1=10;
            else { launch_item(dock,cmd.first-1); strcpy(result,"REQUESTED"); }
            break;
        case ND_RX_MOVE:
            if (cmd.first > dock->conf.count || cmd.second > dock->conf.count)
                m->rm_Result1=10;
            else if (!do_move(dock,cmd.first-1,cmd.second-1,
                              cmd.first<cmd.second)) m->rm_Result1=10;
            else strcpy(result,"OK");
            break;
        case ND_RX_RELOAD:
            if (reload_configuration(dock)!=ND_CONTROL_OK) m->rm_Result1=10;
            else strcpy(result,"OK");
            break;
        case ND_RX_PREFS:
            if (launch_companion(dock,"NovaDockPrefs",NULL))
                strcpy(result,"REQUESTED");
            else { m->rm_Result1=10; strcpy(result,"NOT FOUND"); }
            break;
        case ND_RX_RESTART:
            dock->restart_requested=1;dock->running=0;strcpy(result,"OK");break;
        case ND_RX_QUIT:
            dock->running=0; strcpy(result,"OK"); break;
        case ND_RX_SHOW: show_dock(dock); strcpy(result,"OK"); break;
        case ND_RX_HIDE: hide_dock(dock); strcpy(result,"OK"); break;
        case ND_RX_TOGGLE:
            if (dock->hidden) show_dock(dock); else hide_dock(dock);
            strcpy(result,dock->box_target_hidden ? "HIDDEN" : "SHOWN"); break;
        case ND_RX_SUSPEND:
            /* Workbench can only change its screen mode while no other
             * window is open on it.  Ports and plugins keep running. */
            if (dock->window) close_dock_ui(dock);
            dock->screen_closed=1;dock->suspended=1;
            dock->suspend_ticks=ND_SUSPEND_TICKS;
            strcpy(result,"OK");
            break;
        case ND_RX_RESUME:
            dock->suspended=0;
            if (dock->window) strcpy(result,"OK");
            else if (FindPort((STRPTR)"WORKBENCH") && reopen_dock_ui(dock))
                strcpy(result,"OK");
            else {m->rm_Result1=5;strcpy(result,"PENDING");} /* timer retries */
            break;
        case ND_RX_LOCK: set_edit_mode(dock,0); strcpy(result,"OK"); break;
        case ND_RX_UNLOCK: set_edit_mode(dock,1); strcpy(result,"OK"); break;
        case ND_RX_CLICK:
            if (cmd.first > dock->conf.count ||
                dock->conf.items[cmd.first-1].kind != ND_ITEM_PLUGIN ||
                dock->conf.items[cmd.first-1].command[0]=='@')
                m->rm_Result1=10;
            else {
                struct NDItem *it=&dock->conf.items[cmd.first-1];
                if (it->clicks<3) ++it->clicks;
                strcpy(result,"QUEUED");
            }
            break;
        case ND_RX_REMOVE:
            if (cmd.first > dock->conf.count ||
                !remove_dragged_item(dock,cmd.first-1)) m->rm_Result1=10;
            else strcpy(result,"OK");
            break;
        case ND_RX_ADD:
            if (!rexx_add_launcher(dock,cmd.text)) m->rm_Result1=10;
            else strcpy(result,"OK");
            break;
        case ND_RX_SETBADGE:
            if (rexx_set_badge(dock,&cmd)) strcpy(result,"OK");
            else {m->rm_Result1=5;strcpy(result,"NOT FOUND");}
            break;
        case ND_RX_SETVALUE:
            if (!rexx_set_value(dock,&cmd)) m->rm_Result1=10;
            else strcpy(result,"OK");
            break;
        default: m->rm_Result1=10; break;
        }
        if ((m->rm_Action & RXFF_RESULT) && m->rm_Result1 == 0) {
            const char *text=long_result ? long_result : result;
            m->rm_Result2=(LONG)CreateArgstring((STRPTR)text,(ULONG)strlen(text));
            if (!m->rm_Result2) m->rm_Result1=20;
        }
        if (long_result) FreeVec(long_result);
        ReplyMsg((struct Message *)m);
    }
}
