/* NovaDock: Context menus and their actions; launcher actions and tasks. */
#include "dock_internal.h"

/* Defined below, private to this file. */
static int plugin_can_launch(const struct NDDock *dock, const struct NDItem *item);
static int popup_count_for(const struct NDDock *dock);
static const char *popup_label_en(const struct NDDock *dock, int item);
static const char *popup_label(const struct NDDock *dock, int item);
static void draw_popup(struct NDDock *dock);
static void start_launcher_prefs(struct NDDock *dock, int visible, int icon_only);
static void start_spacer_prefs(struct NDDock *dock,int visible);
static int valid_rexx_action_port(const char *port);
static int send_rexx_action(struct NDDock *dock,const char *port, const char *command);
static int bring_launcher_to_front(struct NDDock *dock,int index);
static int raise_task(struct NDDock *dock,int index,int next);
static void signal_task_break(struct NDDock *dock,int index);
static int require_edit_mode(struct NDDock *dock);
static void flush_media(struct NDDock *dock,int index);
static int pin_media(struct NDDock *dock,int index);
static int pin_task(struct NDDock *dock,int index);
static int popup_action(struct NDDock *dock, int index, int action);
static int spacer_popup_action(struct NDDock *dock,int index,int action);
static int plugin_popup_action(struct NDDock *dock, int index, int action);
static int add_spacer_to_dock(struct NDDock *dock);
static int add_row_to_dock(struct NDDock *dock);
static int remove_row_from_dock(struct NDDock *dock);
static int dock_popup_action(struct NDDock *dock, int action);

/* Keep plugin context menus consistent with launcher menus.  A plugin only
 * gets a Launch entry when that word describes a real primary action.  DOpus
 * can start/restore its application; Music can start the configured default
 * player while it is idle.  Meter, calendar and drop-target plugins keep
 * their direct tile interactions without presenting them as "Launch". */
static int plugin_can_launch(const struct NDDock *dock,
                             const struct NDItem *item)
{
    const char *program;
    if (!dock || !item || item->kind!=ND_ITEM_PLUGIN) return 0;
    if (!strcmp(item->id,"DOPUS4")) return 1;
    if (strcmp(item->id,"MUSIC") || item->media_capabilities) return 0;
    program=nd_option_get(&dock->conf,"MUSIC","DEFAULT_APP");
    return program && *program;
}
/* The Trash plugin's tile (bundled, ID TRASH). */
int is_trash_tile(const struct NDDock *dock,int index)
{
    return dock && index>=0 && index<dock->conf.count &&
           dock->conf.items[index].kind==ND_ITEM_PLUGIN &&
           !strcmp(dock->conf.items[index].id,"TRASH");
}
/* Open the system Trashcan drawer, if there is one. */
void open_trash(struct NDDock *dock)
{
    APTR old=quiet_requesters();
    BPTR lock=Lock((STRPTR)"SYS:Trashcan",ACCESS_READ);
    restore_requesters(old);
    if (lock) {
        UnLock(lock);
        open_drawer("SYS:Trashcan");
    } else show_feedback(dock,"The trash is empty",0);
}
static int popup_count_for(const struct NDDock *dock)
{
    const struct NDItem *item;
    if (dock->popup_item == -2) return 5;
    if (dock->popup_item<0 || dock->popup_item>=dock->conf.count) return 0;
    item=&dock->conf.items[dock->popup_item];
    if (item->kind==ND_ITEM_SPACER) return 2;
    if (item->transient==ND_TRANSIENT_TASK) return ITEM_TEXT(item,ND_TEXT_ARGS)[0] ? 4 : 3;
    if (item->kind==ND_ITEM_DRAWER || item->kind==ND_ITEM_GROUP) return 4;
    if (item->kind==ND_ITEM_LAUNCH) return 3;
    if (item->kind==ND_ITEM_PLUGIN && is_trash_tile(dock,dock->popup_item))
        return 3;
    if(item->kind==ND_ITEM_PLUGIN && (!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW")))return 4;
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"BITSKIM"))return 6;
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"CLIPBOARD"))return 4;
    if (item->kind==ND_ITEM_PLUGIN)
        return plugin_can_launch(dock,item) ? 3 : 2;
    return 1;
}
static const char *launcher_actions[] = {
    "Launch", "Edit settings...", "Remove"
};
static const char *task_actions[] = {
    "Bring to front", "Next window", "Send Ctrl-C", "Pin to dock"
};
static const char *task_actions_nopin[] = {
    "Bring to front", "Next window", "Send Ctrl-C"
};
static const char *media_actions[] = {
    "Open", "Popup contents", "Flush (safe to remove)", "Pin to dock"
};
static const char *drawer_actions[] = {
    "Open", "Popup contents", "Edit settings...", "Remove"
};
static const char *trash_actions[] = {
    "Open trash", "Empty trash...", "Remove"
};
static const char *network_actions[]={"Details...", "Edit settings...", "Remove"};
static const char *bitskim_actions[]={"Open BitSkim", "Send file...", "Toggle receiving", "Cancel transfer", "Edit settings...", "Remove"};
static const char *clipboard_actions[]={"Clipboard history", "Manage history...", "Edit settings...", "Remove"};
static const char *plugin_actions[] = {
    "Launch", "Edit settings...", "Remove"
};
static const char *spacer_actions[] = {
    "Edit spacer...", "Remove spacer"
};
static const char *dock_actions[] = {
    "Unlock dock", "Add item...", "Preferences...", "About NovaDock...",
    "Quit NovaDock"
};
static const char *popup_label_en(const struct NDDock *dock, int item)
{
    if (dock->popup_item == -2) {
        if (item==0) return dock->edit_mode ? "Lock dock" : "Unlock dock";
        return dock_actions[item];
    }
    if (dock->popup_item>=0) {
        const struct NDItem *it=&dock->conf.items[dock->popup_item];
        if (it->kind==ND_ITEM_SPACER) return spacer_actions[item];
        if (it->transient==ND_TRANSIENT_TASK)
            return ITEM_TEXT(it,ND_TEXT_ARGS)[0] ? task_actions[item] :
                                        task_actions_nopin[item];
        if (it->kind==ND_ITEM_DRAWER || it->kind==ND_ITEM_GROUP)
            return it->transient ? media_actions[item] : drawer_actions[item];
        if (it->kind==ND_ITEM_LAUNCH) return launcher_actions[item];
        if (it->kind==ND_ITEM_PLUGIN && is_trash_tile(dock,dock->popup_item))
            return trash_actions[item];
        if(it->kind==ND_ITEM_PLUGIN && (!strcmp(it->id,"NET") || !strcmp(it->id,"ROADSHOW")))return item==0 ? (dock->popup_network_online ? "Offline":"Online"):network_actions[item-1];
        if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"BITSKIM"))return bitskim_actions[item];
        if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"CLIPBOARD"))return clipboard_actions[item];
        if (it->kind==ND_ITEM_PLUGIN)
            return plugin_actions[item + (plugin_can_launch(dock,it) ? 0 : 1)];
    }
    return "";
}
static const char *popup_label(const struct NDDock *dock, int item)
{
    return nd_tr(popup_label_en(dock,item));
}
void close_popup(struct NDDock *dock)
{
    struct IntuiMessage *m;
    if (!dock->popup) return;
    while ((m = (struct IntuiMessage *)GetMsg(dock->popup->UserPort)) != NULL)
        ReplyMsg((struct Message *)m);
    CloseWindow(dock->popup);
    dock->popup = NULL;
    dock->popup_item = -1;
    dock->popup_hot = -1;
}
static void draw_popup(struct NDDock *dock)
{
    int i, y;
    struct RastPort *rp;
    if (!dock->popup) return;
    rp = dock->popup->RPort;
    SetDrMd(rp, JAM1);
    SetAPen(rp, pen(dock, ND_BG));
    RectFill(rp, 0, 0, dock->popup->Width-1, dock->popup->Height-1);
    for (i = 0; i < popup_count_for(dock); ++i) {
        y = 4 + 19*i;
        SetAPen(rp, pen(dock, i == dock->popup_hot ? ND_HOVER : ND_TILE));
        RectFill(rp, 3, y, dock->popup->Width-4, y+17);
        SetAPen(rp, pen(dock, ND_TEXT));
        Move(rp, 9, y+13);
        Text(rp, (STRPTR)popup_label(dock,i),
             strlen(popup_label(dock,i)));
    }
}
void open_popup(struct NDDock *dock, int item, int x, int y)
{
    int px, py, popup_height, count;
    if (item < -2 || item >= dock->conf.count || !dock->window ||
        dock->hidden || dock->box_pending) return;
    /* Plugins now get their own native context menu. */
    close_tooltip(dock);
    close_popup(dock);
    close_calendar(dock);
    close_folder_popup(dock);
    /* popup_count_for() depends on the target kind.  Set it before sizing the
     * window; close_popup() deliberately cleared the previous target. */
    dock->popup_item = item;
    dock->popup_network_online=item>=0 && !strcmp(dock->conf.items[item].badge,"*");
    px = dock->window->LeftEdge + x;
    py = dock->window->TopEdge + y;
    count=popup_count_for(dock);
    popup_height=8+19*count;
    if (px + 175 > dock->screen->Width) px = dock->screen->Width - 175;
    if (py + popup_height > dock->screen->Height)
        py = dock->screen->Height - popup_height;
    if (px < 0) px = 0;
    if (py < dock->screen->BarHeight) py = dock->screen->BarHeight;
    dock->popup = OpenWindowTags(NULL,
        WA_PubScreen, (ULONG)dock->screen,
        WA_Left, px, WA_Top, py, WA_Width, 175, WA_Height, popup_height,
        WA_Borderless, TRUE, WA_Activate, TRUE, WA_RMBTrap, TRUE,
        WA_IDCMP, IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE |
                  IDCMP_REFRESHWINDOW | IDCMP_INACTIVEWINDOW | IDCMP_RAWKEY,
        WA_Flags, WFLG_REPORTMOUSE | WFLG_SIMPLE_REFRESH | WFLG_ACTIVATE,
        TAG_DONE);
    if (dock->popup) {
        /* The first action is the default: Launch for an app/drawer and the
         * primary settings action for a plugin or the dock background. */
        dock->popup_hot = 0;
        draw_popup(dock);
    } else dock->popup_item = -1;
}
static void start_launcher_prefs(struct NDDock *dock, int visible, int icon_only)
{
    char args[80];
    /* The editor converts visible tile index to a full-config row on startup. */
    sprintf(args, "PAGE=LAUNCHERS SELECT=%d FOCUSED=1%s",
            visible, icon_only ? " ICON=1" : "");
    if (!launch_companion(dock, "NovaDockPrefs", args) && dock->screen)
        DisplayBeep(dock->screen);
}
static void start_spacer_prefs(struct NDDock *dock,int visible)
{
    char args[48];
    sprintf(args,"PAGE=SPACER SELECT=%d FOCUSED=1",visible);
    if (!launch_companion(dock,"NovaDockPrefs",args) && dock->screen)
        DisplayBeep(dock->screen);
}
static int valid_rexx_action_port(const char *port)
{
    int count=0;
    unsigned char c;
    if (!port || !*port || !strcmp(port,ND_REXX_PORT)) return 0;
    while ((c=(unsigned char)*port++)!=0) {
        if (!isalnum(c) && c!='_' && c!='.') return 0;
        if (++count>=ND_COMMAND_LEN) return 0;
    }
    return 1;
}
/* User-triggered ARexx actions are sent asynchronously.  The dock never
 * waits for the target: a busy application must not freeze the dock (and
 * every plugin, whose exchanges would then time out), and a target that is
 * itself waiting on a script addressing NOVADOCK must not deadlock it.
 * Replies are collected from dock->rexx_reply in the main event loop. */
static int send_rexx_action(struct NDDock *dock,const char *port,
                            const char *command)
{
    struct MsgPort *target;
    struct RexxMsg *request;
    int delivered=0;
    if (!dock || !RexxSysBase || !valid_rexx_action_port(port) ||
        !command || !*command) return 0;
    if (!dock->rexx_reply) {
        dock->rexx_reply=CreateMsgPort();
        if (!dock->rexx_reply) return 0;
    }
    request=CreateRexxMsg(dock->rexx_reply,(STRPTR)"rexx",(STRPTR)ND_REXX_PORT);
    if (!request) return 0;
    request->rm_Args[0]=CreateArgstring((STRPTR)command,(LONG)strlen(command));
    if (!request->rm_Args[0]) { DeleteRexxMsg(request);return 0; }
    request->rm_Action=RXCOMM;
    Forbid();
    target=FindPort((STRPTR)port);
    if (target) { PutMsg(target,(struct Message *)request);delivered=1; }
    Permit();
    if (!delivered) {
        DeleteArgstring(request->rm_Args[0]);
        DeleteRexxMsg(request);
        return 0;
    }
    ++dock->rexx_pending;
    return 1;
}
void process_rexx_replies(struct NDDock *dock,int report)
{
    struct RexxMsg *reply;
    int failed=0;
    if (!dock->rexx_reply) return;
    while ((reply=(struct RexxMsg *)GetMsg(dock->rexx_reply))!=NULL) {
        if (reply->rm_Result1!=0) ++failed;
        DeleteArgstring(reply->rm_Args[0]);
        DeleteRexxMsg(reply);
        if (dock->rexx_pending>0) --dock->rexx_pending;
    }
    if (failed && report)
        show_feedback(dock,"ARexx Launcher Action returned an error",1);
}
/* Shutdown: give outstanding targets a few seconds to reply.  A target that
 * never replies still owns its message, so the port is then retained and
 * ignored rather than freed underneath a late ReplyMsg. */
void stop_rexx_replies(struct NDDock *dock)
{
    int waited;
    if (!dock->rexx_reply) return;
    for (waited=0;dock->rexx_pending>0 && waited<50;++waited) {
        process_rexx_replies(dock,0);
        if (dock->rexx_pending>0) Delay(5);
    }
    process_rexx_replies(dock,0);
    if (dock->rexx_pending>0) {
        Forbid();
        dock->rexx_reply->mp_Flags=PA_IGNORE;
        dock->rexx_reply->mp_SigTask=NULL;
        Permit();
    } else DeleteMsgPort(dock->rexx_reply);
    dock->rexx_reply=NULL;
}
int launch_action(struct NDDock *dock,const struct NDItem *item)
{
    char command[ND_COMMAND_LEN*3+8];
    char info_path[ND_COMMAND_LEN+6];
    BPTR path_lock=0;
    int path_exists=0,workbench_object=0;
    if (!dock || !item ||
        (item->kind!=ND_ITEM_LAUNCH && item->kind!=ND_ITEM_DRAWER &&
         item->kind!=ND_ITEM_GROUP)) return 0;
    if (!strcmp(item->command,"@quit")) { dock->running=0;return 1; }
    if (item->kind==ND_ITEM_GROUP) { open_folder_popup(dock,
        (int)(item-dock->conf.items));return 1; }
    if (item->kind==ND_ITEM_DRAWER) { open_drawer(item->command);return 1; }
    if (item->launch_mode==ND_LAUNCH_AREXX)
        return send_rexx_action(dock,item->command,ITEM_TEXT(item,ND_TEXT_ARGS));
    if (!strchr(item->command,'"') && !strchr(item->command,'\n') &&
        !strchr(item->command,'\r')) {
        path_lock=Lock((STRPTR)item->command,ACCESS_READ);
        if (path_lock) {
            path_exists=1;workbench_object=1;UnLock(path_lock);
        } else if (strlen(item->command)+5<sizeof(info_path)) {
            strcpy(info_path,item->command);strcat(info_path,".info");
            path_lock=Lock((STRPTR)info_path,ACCESS_READ);
            if (path_lock) {workbench_object=1;UnLock(path_lock);}
        }
    }
    if (item->launch_mode==ND_LAUNCH_WORKBENCH ||
        (item->launch_mode==ND_LAUNCH_AUTO && workbench_object &&
         !ITEM_TEXT(item,ND_TEXT_ARGS)[0] && !ITEM_TEXT(item,ND_TEXT_WORKDIR)[0])) {
        if (WorkbenchBase && WorkbenchBase->lib_Version>=44 &&
            OpenWorkbenchObject((STRPTR)item->command,TAG_DONE)) return 1;
        if (!path_exists || strchr(item->command,'"')) return 0;
    }
    if (path_exists) {
        if (!nd_quote_path(command,sizeof(command),item->command)) return 0;
    } else {
        if (strlen(item->command)>=sizeof(command)) return 0;
        strcpy(command,item->command);
    }
    if (ITEM_TEXT(item,ND_TEXT_ARGS)[0]) {
        if (strlen(command)+strlen(ITEM_TEXT(item,ND_TEXT_ARGS))+2>=sizeof(command)) return 0;
        strcat(command," ");strcat(command,ITEM_TEXT(item,ND_TEXT_ARGS));
    }
    return launch_command_at(command,ITEM_TEXT(item,ND_TEXT_WORKDIR),
                             path_exists ? icon_stack(item->command) :
                             ND_DEFAULT_STACK);
}
int launch_default_music_application(struct NDDock *dock)
{
    struct NDItem launcher;
    const char *program;
    if (!dock) return 0;
    program=nd_option_get(&dock->conf,"MUSIC","DEFAULT_APP");
    if (!program || !*program) return 0;
    memset(&launcher,0,sizeof(launcher));
    launcher.kind=ND_ITEM_LAUNCH;
    launcher.launch_mode=ND_LAUNCH_AUTO;
    strncpy(launcher.command,program,sizeof(launcher.command)-1);
    launcher.command[sizeof(launcher.command)-1]='\0';
    if (!launch_action(dock,&launcher)) {
        show_feedback(dock,"Could not start the default music application",1);
        return 0;
    }
    show_feedback(dock,"Starting the default music application",0);
    return 1;
}
static int bring_launcher_to_front(struct NDDock *dock,int index)
{
    struct Task *task,*port_task;
    struct Window *found;
    if (!dock || !dock->conf.running_apps || index<0 ||
        index>=dock->conf.count) return 0;
    task=launcher_task(&dock->conf.items[index]);
    port_task=launcher_port_task(&dock->conf.items[index],NULL);
    found=launcher_window(dock,&dock->conf.items[index],task,port_task);
    if (!found) return 0;
    /* The application may have closed that window since the search released
     * the Intuition lock: confirm it still exists immediately before use. */
    {
        struct Screen *screen,*owner=NULL;
        struct Window *window;
        int front=0;
        ULONG lock=LockIBase(0);
        for (screen=IntuitionBase->FirstScreen;screen && !owner;
             screen=screen->NextScreen)
            for (window=screen->FirstWindow;window;window=window->NextWindow)
                if (window==found) {owner=screen;break;}
        front=owner && owner==IntuitionBase->FirstScreen;
        UnlockIBase(lock);
        if (!owner) return 0;
        /* Applications on their own screen (DOpus, browsers, players) need
         * that screen brought forward as well. */
        if (!front) ScreenToFront(owner);
    }
    WindowToFront(found);
    ActivateWindow(found);
    dock->running_items[index]=1;
    return 1;
}
/* Bring a running application's window forward.  With NEXT set, successive
 * clicks cycle through all of that task's windows. */
static int raise_task(struct NDDock *dock,int index,int next)
{
    struct Task *task=(struct Task *)nd_task_tile_key(&dock->conf.items[index]);
    struct Screen *screen,*screens[16];
    struct Window *window,*windows[16];
    int count=0,pick=0,i,front;
    ULONG lock;
    if (!task) return 0;
    /* Collect and use the windows under one lock acquisition, immediately
     * before acting, so a closed window is never touched. */
    lock=LockIBase(0);
    for (screen=IntuitionBase->FirstScreen;screen;screen=screen->NextScreen)
        for (window=screen->FirstWindow;window && count<16;
             window=window->NextWindow)
            if (window->UserPort && window->UserPort->mp_SigTask==task &&
                window->Title) {
                windows[count]=window;screens[count]=screen;++count;
            }
    if (next)
        for (i=0;i<count;++i)
            if (windows[i]==dock->last_raised) {pick=(i+1)%count;break;}
    front=count && screens[pick]==IntuitionBase->FirstScreen;
    UnlockIBase(lock);
    if (!count) return 0;
    if (!front) ScreenToFront(screens[pick]);
    WindowToFront(windows[pick]);
    ActivateWindow(windows[pick]);
    dock->last_raised=windows[pick];
    return 1;
}
static void signal_task_break(struct NDDock *dock,int index)
{
    struct Task *task=(struct Task *)nd_task_tile_key(&dock->conf.items[index]);
    char message[200];
    int sent=0;
    Forbid();
    if (task && task_exists(task)) {Signal(task,SIGBREAKF_CTRL_C);sent=1;}
    Permit();
    sprintf(message,sent ? nd_trf("Sent Ctrl-C to %.28s") :
                           nd_trf("%.28s is no longer running"),
            dock->conf.items[index].label);
    show_feedback(dock,message,!sent);
}
void launch_item(struct NDDock *dock, int index)
{
    if (index < 0 || index >= dock->conf.count) return;
    if (dock->conf.items[index].transient==ND_TRANSIENT_TASK) {
        if (!raise_task(dock,index,1))
            show_feedback(dock,"Application has no open window",1);
        return;
    }
    if (bring_launcher_to_front(dock,index)) return;
    if (dock->conf.running_apps &&
        dock->conf.items[index].kind==ND_ITEM_LAUNCH &&
        launcher_running(dock,&dock->conf.items[index])) {
        dock->running_items[index]=1;
        show_feedback(dock,"Application is running; no window matched",0);
        return;
    }
    if (!launch_action(dock,&dock->conf.items[index]))
        show_feedback(dock,"Launcher Action could not be started",1);
}
static int require_edit_mode(struct NDDock *dock)
{
    if (dock->edit_mode) return 1;
    show_feedback(dock,"Dock locked - right-click the background to unlock",1);
    return 0;
}
void request_dopus_action(struct NDDock *dock,int index)
{
    struct NDItem *item;
    if (!dock || index<0 || index>=dock->conf.count) return;
    item=&dock->conf.items[index];
    if (strcmp(item->id,"DOPUS4")) return;
    if (item->clicks<3) ++item->clicks;
    show_feedback(dock,!strcmp(item->value,"READY") ?
                  "DOpus 4: bring to front" : "DOpus 4: starting...",0);
}
/* Returns true if the main window was replaced by an action. */
/* Inhibit forces the filesystem to write everything and release the
 * medium; uninhibiting at once lets it remount normally.  Afterwards the
 * disk is safe to remove until something writes to it again. */
static void flush_media(struct NDDock *dock,int index)
{
    char message[200];
    const char *drive=dock->conf.items[index].id;
    int ok;
    if (!drive[0]) return;
    ok=Inhibit((STRPTR)drive,DOSTRUE)!=0;
    if (ok) ok=Inhibit((STRPTR)drive,DOSFALSE)!=0;
    else (void)Inhibit((STRPTR)drive,DOSFALSE);
    sprintf(message,ok ? nd_trf("%.20s flushed - safe to remove") :
                         nd_trf("Could not flush %.20s"),drive);
    show_feedback(dock,message,!ok);
    /* The remount briefly removes the volume; do not report it as removed. */
    dock->media_scan_ticks=ND_MEDIA_SCAN_TICKS;
}
static int pin_media(struct NDDock *dock,int index)
{
    char target[ND_COMMAND_LEN],message[200],label[ND_LABEL_LEN];
    strcpy(target,dock->conf.items[index].command);
    strcpy(label,dock->conf.items[index].label);
    if (!rexx_add_launcher(dock,target)) {
        if (dock->screen) DisplayBeep(dock->screen);
        show_feedback(dock,"Could not pin - dock may be full",1);
        return 0;
    }
    sprintf(message,nd_trf("Pinned %.28s"),label);
    show_feedback(dock,message,0);
    return 1;
}
static int pin_task(struct NDDock *dock,int index)
{
    char path[ND_COMMAND_LEN],label[ND_LABEL_LEN],message[200];
    strcpy(path,ITEM_TEXT(&dock->conf.items[index],ND_TEXT_ARGS));
    strcpy(label,dock->conf.items[index].label);
    if (!path[0] || !rexx_add_launcher(dock,path)) {
        if (dock->screen) DisplayBeep(dock->screen);
        show_feedback(dock,"Could not pin - dock may be full",1);
        return 0;
    }
    sprintf(message,nd_trf("Pinned %.28s"),label);
    show_feedback(dock,message,0);
    return 1;
}
static int popup_action(struct NDDock *dock, int index, int action)
{
    int drawer=dock->conf.items[index].kind==ND_ITEM_DRAWER ||
               dock->conf.items[index].kind==ND_ITEM_GROUP;
    if (dock->conf.items[index].transient==ND_TRANSIENT_TASK) {
        if (action==0) {
            if (!raise_task(dock,index,0))
                show_feedback(dock,"Application has no open window",1);
        } else if (action==1) {
            if (!raise_task(dock,index,1))
                show_feedback(dock,"Application has no open window",1);
        } else if (action==2) signal_task_break(dock,index);
        else if (action==3 && ITEM_TEXT(&dock->conf.items[index],ND_TEXT_ARGS)[0])
            return pin_task(dock,index);
        return 0;
    }
    if (dock->conf.items[index].transient) {
        if (action==0) launch_item(dock,index);
        else if (action==1) open_folder_popup(dock,index);
        else if (action==2) flush_media(dock,index);
        else if (action==3) return pin_media(dock,index);
        return 0;
    }
    switch (action) {
    case 0: launch_item(dock, index); break;
    case 1:
        if (drawer) open_folder_popup(dock,index);
        else start_launcher_prefs(dock,index,0);
        break;
    case 2:
        if (drawer) start_launcher_prefs(dock,index,0);
        else return remove_dragged_item(dock,index);
        break;
    case 3:
        if (drawer) return remove_dragged_item(dock,index);
        break;
    default: break;
    }
    return 0;
}
static int spacer_popup_action(struct NDDock *dock,int index,int action)
{
    if (action==0) start_spacer_prefs(dock,index);
    else if (action==1) return remove_dragged_item(dock,index);
    return 0;
}
static int plugin_popup_action(struct NDDock *dock, int index, int action)
{
    int launchable;
    char args[80];
    const struct NDItem *item=&dock->conf.items[index];
    if (is_trash_tile(dock,index)) {
        if (action==0) open_trash(dock);
        else if (action==1) {
            /* The helper asks before deleting anything. */
            if (!launch_plugin_helper(dock,index,"EMPTY",NULL))
                show_feedback(dock,"Could not start the Trash plugin",1);
        } else if (action==2) return remove_dragged_item(dock,index);
        return 0;
    }
    if(!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW")){
        if(action==0){
            dock->conf.items[index].media_event&=~(ND_EVENT_NETWORK_ONLINE|ND_EVENT_NETWORK_OFFLINE);
            dock->conf.items[index].media_event|=dock->popup_network_online ? ND_EVENT_NETWORK_OFFLINE:ND_EVENT_NETWORK_ONLINE;
        }else if(action==1){
            if(!launch_plugin_helper(dock,index,"DETAILS",NULL))
                show_feedback(dock,"Could not open Network details",1);
        }else if(action==2)(void)launch_companion(dock,"NovaDockPrefs","PAGE=PLUGIN PLUGIN=NET FOCUSED=1");
        else if(action==3)return remove_dragged_item(dock,index);
        return 0;
    }
    if(!strcmp(item->id,"BITSKIM")){
        if(action==0)launch_item(dock,index);
        else if(action==1){
            if(!launch_plugin_helper(dock,index,"SEND",NULL))
                show_feedback(dock,"Could not open BitSkim helper",1);
        }
        else if(action==2)dock->conf.items[index].media_event|=ND_EVENT_BITSKIM_RECEIVING;
        else if(action==3)dock->conf.items[index].media_event|=ND_EVENT_BITSKIM_CANCEL;
        else if(action==4)(void)launch_companion(dock,"NovaDockPrefs","PAGE=PLUGIN PLUGIN=BITSKIM FOCUSED=1");
        else if(action==5)return remove_dragged_item(dock,index);
        return 0;
    }
    if(!strcmp(item->id,"CLIPBOARD")){
        if(action==0)launch_item(dock,index);
        else if(action==1)dock->conf.items[index].media_event|=ND_EVENT_MANAGE_HISTORY;
        else if(action==2){
            sprintf(args,"PAGE=PLUGIN PLUGIN=%s FOCUSED=1",item->id);
            (void)launch_companion(dock,"NovaDockPrefs",args);
        }else if(action==3)return remove_dragged_item(dock,index);
        return 0;
    }
    launchable=plugin_can_launch(dock,item);
    if (launchable && action==0) {
        if (!strcmp(item->id,"DOPUS4")) request_dopus_action(dock,index);
        else if (!strcmp(item->id,"MUSIC"))
            (void)launch_default_music_application(dock);
        return 0;
    }
    if (action==(launchable ? 1 : 0)) {
        sprintf(args,"PAGE=PLUGIN PLUGIN=%s FOCUSED=1",item->id);
        if (!launch_companion(dock,"NovaDockPrefs",args) && dock->screen)
            DisplayBeep(dock->screen);
        return 0;
    }
    if (action==(launchable ? 2 : 1))
        return remove_dragged_item(dock,index);
    return 0;
}
static int add_spacer_to_dock(struct NDDock *dock)
{
    struct NDConfig *full;
    struct NDThemeSpec spec;
    int ok=0,parsed,visible=0,i,inserted;
    if (!require_edit_mode(dock)) return 0;
    full = (struct NDConfig *)AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
    if (!full) return 0;
    parsed = load_edit_configuration(dock,full);
    if (parsed <= 0) {
        for (i=0;i<full->count;++i) if (full->items[i].enabled) ++visible;
        if (nd_insert_spacer_visible(full,visible,"Spacer")) {
            inserted=nd_visible_to_full(full,visible);
            nd_theme_load_config(dock->directory,full,&spec);
            if (inserted>=0) nd_theme_apply_spacer(&full->items[inserted],&spec);
            ok=save_and_reload(dock,&full);
        }
    }
    if (full) FreeVec(full);
    if (ok) show_feedback(dock,"Spacer added - drag it into position",0);
    else if (dock->screen) {
        DisplayBeep(dock->screen);
        show_feedback(dock,"Could not add spacer - dock may be full",1);
    }
    return ok;
}
static int add_row_to_dock(struct NDDock *dock)
{
    struct NDConfig *full;
    int ok=0,parsed,target_rows;
    char message[200];
    if (dock->conf.side==ND_SIDE_LEFT || dock->conf.side==ND_SIDE_RIGHT) {
        show_feedback(dock,"Rows apply to top and bottom docks",1);
        return 0;
    }
    if (dock->conf.rows>=ND_ROWS_MAX) {
        show_feedback(dock,"Dock already has the maximum rows",0);
        return 0;
    }
    full=(struct NDConfig *)AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
    if (!full) return 0;
    parsed=load_edit_configuration(dock,full);
    target_rows=dock->conf.rows+1;
    if (parsed<=0 && target_rows<=ND_ROWS_MAX) {
        /* The running layout may be a live Preferences preview and therefore
         * newer than the saved file. Apply the popup action to what the user
         * can actually see rather than incrementing a stale saved value. */
        capture_row_layout(dock,full);
        full->rows=target_rows;
        full->row_counts[target_rows-1]=0;
        ok=save_and_reload(dock,&full);
    }
    if (full) FreeVec(full);
    if (ok) {
        sprintf(message,nd_trf("Dock now uses %d rows"),dock->conf.rows);
        show_feedback(dock,message,0);
    } else if (dock->screen) {
        DisplayBeep(dock->screen);
        show_feedback(dock,"Could not add row at this screen size",1);
    }
    return ok;
}
static int remove_row_from_dock(struct NDDock *dock)
{
    struct NDConfig *full;
    int ok=0,parsed,target_rows;
    char message[200];
    if (dock->conf.side==ND_SIDE_LEFT || dock->conf.side==ND_SIDE_RIGHT) {
        show_feedback(dock,"Rows apply to top and bottom docks",1);
        return 0;
    }
    if (dock->conf.rows<=ND_ROWS_MIN) {
        show_feedback(dock,"Dock already has the minimum rows",0);
        return 0;
    }
    full=(struct NDConfig *)AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
    if (!full) return 0;
    parsed=load_edit_configuration(dock,full);
    target_rows=dock->conf.rows-1;
    if (parsed<=0 && target_rows>=ND_ROWS_MIN) {
        /* See add_row_to_dock(): a previewed row count is valid live state,
         * even when ENV:/ENVARC: still contain the previous value. */
        capture_row_layout(dock,full);
        full->row_counts[target_rows-1]+=full->row_counts[target_rows];
        full->row_counts[target_rows]=0;
        full->rows=target_rows;
        ok=save_and_reload(dock,&full);
    }
    if (full) FreeVec(full);
    if (ok) {
        sprintf(message,nd_trf("Dock now uses %d rows"),dock->conf.rows);
        show_feedback(dock,message,0);
    } else if (dock->screen) {
        DisplayBeep(dock->screen);
        show_feedback(dock,"Could not remove dock row",1);
    }
    return ok;
}
void set_edit_mode(struct NDDock *dock,int on)
{
    dock->edit_mode=on ? 1 : 0;
    dock->press_item=-1;dock->drag_to=-1;dock->drag_after=0;dock->is_drag=0;
    draw_all(dock);
    show_feedback(dock,dock->edit_mode ?
                  "Dock unlocked - drag and drop enabled" :
                  "Dock locked",0);
}
static int dock_popup_action(struct NDDock *dock, int action)
{
    switch (action) {
    case 0:
        set_edit_mode(dock,!dock->edit_mode);
        break;
    case 1:
        if (!launch_companion(dock,"NovaDockPrefs","PAGE=ADD FOCUSED=1") &&
            dock->screen) DisplayBeep(dock->screen);
        break;
    case 2:
        if (!launch_companion(dock,"NovaDockPrefs",NULL) && dock->screen)
            DisplayBeep(dock->screen);
        break;
    case 3:
        open_about(dock);
        break;
    case 4: dock->running = 0; return 1;
    default: break;
    }
    return 0;
}
void popup_events(struct NDDock *dock)
{
    struct IntuiMessage *m;
    ULONG cls, code;
    WORD my;
    int row, index;
    while (dock->popup && (m = (struct IntuiMessage *)GetMsg(dock->popup->UserPort)) != NULL) {
        cls = m->Class; code = m->Code; my = m->MouseY;
        ReplyMsg((struct Message *)m);
        if (cls == IDCMP_REFRESHWINDOW) {
            BeginRefresh(dock->popup); draw_popup(dock);
            EndRefresh(dock->popup, TRUE);
        } else if (cls == IDCMP_MOUSEMOVE) {
            row = (my - 4) / 19;
            if (my < 4 || row >= popup_count_for(dock)) row = -1;
            if (row != dock->popup_hot) {
                dock->popup_hot = row;
                draw_popup(dock);
            }
        } else if (cls == IDCMP_RAWKEY && (code & IECODE_UP_PREFIX)) {
            /* Key releases (such as the M that opened this menu) are not
             * a reason to close it. */
        } else if (cls == IDCMP_RAWKEY && (code == 0x4C || code == 0x4D)) {
            int count = popup_count_for(dock);
            if (count > 0) {
                dock->popup_hot = dock->popup_hot < 0 ? 0 :
                    (dock->popup_hot + (code == 0x4D ? 1 : count - 1)) % count;
                draw_popup(dock);
            }
        } else if ((cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN) ||
                   (cls == IDCMP_RAWKEY && (code == 0x44 || code == 0x43) &&
                    dock->popup_hot >= 0)) {
            /* A click on a row, or Return/Enter on the highlighted one. */
            row = cls == IDCMP_RAWKEY ? dock->popup_hot :
                  my < 4 ? -1 : (my - 4) / 19;
            index = dock->popup_item;
            { int count = popup_count_for(dock);
            close_popup(dock);
            if (row >= 0 && row < count) {
                if (index == -2) {
                    if (dock_popup_action(dock, row)) return;
                } else if (dock->conf.items[index].kind == ND_ITEM_PLUGIN) {
                    if (plugin_popup_action(dock,index,row)) return;
                } else if (dock->conf.items[index].kind == ND_ITEM_SPACER) {
                    if (spacer_popup_action(dock,index,row)) return;
                } else if (popup_action(dock, index, row)) return;
            } }
            return;
        } else if (cls == IDCMP_INACTIVEWINDOW || cls == IDCMP_RAWKEY ||
                   (cls == IDCMP_MOUSEBUTTONS && code == MENUDOWN)) {
            close_popup(dock);
            return;
        }
    }
}
