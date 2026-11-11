/* NovaDock: The ARexx host. */
#include "dock_internal.h"

/* Defined below, private to this file. */
static int item_by_label(struct NDDock *dock,const char *label);
static int rexx_set_value(struct NDDock *dock,const struct NDRexxCommand *cmd);
static int rexx_set_badge(struct NDDock *dock,const struct NDRexxCommand *cmd);
static const char *item_kind_name(const struct NDItem *item);
static int rexx_list_stem(struct NDDock *dock,struct RexxMsg *m,const char *stem);
static char *rexx_list_text(struct NDDock *dock);

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
        if (!valid) { m->rm_Result1=10; strcpy(result,"Invalid command"); }
        else switch (cmd.action) {
        case ND_RX_PING: strcpy(result,"PONG"); break;
        case ND_RX_VERSION: strcpy(result,ND_VERSION); break;
        case ND_RX_STATUS: sprintf(result,"RUNNING %d",dock->conf.count); break;
        case ND_RX_HELP:
            strcpy(result,"PING VERSION STATUS LIST [STEM var] OPEN n|label MOVE a b "
                   "CLICK n REMOVE n ADD path SETVALUE id caption value [meter] "
                   "SETBADGE id [text] "
                   "SHOW HIDE TOGGLE LOCK UNLOCK SUSPEND RESUME RELOAD PREFS QUIT"); break;
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
            else if (FindPort((STRPTR)"Workbench") && reopen_dock_ui(dock))
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
