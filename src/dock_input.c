/* NovaDock: Workbench and Commodities: AppWindow, AppMenus, hot keys, paths. */
#include "dock_internal.h"

/* Defined below, private to this file. */
static void attach_appmenus(struct NDDock *dock);
static int workbench_task_running(void);

/* AmigaOS 2.0+ AppWindow: Workbench sends WBArg lists via our
 * private message port. Workbench owns the WBArg locks until ReplyMsg.
 * The actual insertion is done only after all fields are copied. */
void detach_appmenus(struct NDDock *dock)
{
    int i;
    if (!WorkbenchBase) return;
    for (i=0;i<ND_MAX_ITEMS;++i) {
        if (dock->app_menu_items[i]) {
            RemoveAppMenuItem(dock->app_menu_items[i]);
            dock->app_menu_items[i]=NULL;
        }
    }
}
void detach_appwindow(struct NDDock *dock)
{
    if (dock->app_window && WorkbenchBase) {
        RemoveAppWindow(dock->app_window);
        dock->app_window = NULL;
    }
}
static void attach_appmenus(struct NDDock *dock)
{
    int i;
    if (!WorkbenchBase || !dock->drop_port || !workbench_task_running()) return;
    for (i=0;i<dock->action_count;++i) {
        struct NDItem *item=&dock->action_items[i];
        if (!item->tools_menu ||
            (item->kind!=ND_ITEM_LAUNCH && item->kind!=ND_ITEM_DRAWER) ||
            dock->app_menu_items[i]) continue;
        dock->app_menu_items[i]=AddAppMenuItemA((ULONG)(100+i),(ULONG)i,
                                 (STRPTR)item->label,dock->drop_port,NULL);
    }
}
static int workbench_task_running(void)
{
    struct Task *task;
    Forbid();
    task = FindTask("Workbench");
    Permit();
    return task != NULL;
}
void attach_appwindow(struct NDDock *dock)
{
    /* AddAppWindowA can only be used after LoadWB.  Opening
     * workbench.library is not proof that the Workbench task is ready, and an
     * early registration can otherwise leave a non-working target behind. */
    if (!WorkbenchBase)
        WorkbenchBase = OpenLibrary("workbench.library", 36);
    if (!dock->window || !dock->drop_port || !WorkbenchBase ||
        !workbench_task_running()) return;
    if (!dock->app_window)
        dock->app_window = AddAppWindowA(1UL, 0UL, dock->window,
                                         dock->drop_port, NULL);
    attach_appmenus(dock);
}
void stop_hotkeys(struct NDDock *dock)
{
    CxMsg *message;
    if (dock->hotkey_broker && CxBase) {
        DeleteCxObjAll(dock->hotkey_broker);
        dock->hotkey_broker=NULL;
    }
    if (dock->hotkey_port) {
        while ((message=(CxMsg *)GetMsg(dock->hotkey_port))!=NULL)
            ReplyMsg((struct Message *)message);
        DeleteMsgPort(dock->hotkey_port);
        dock->hotkey_port=NULL;
    }
}
int start_hotkeys(struct NDDock *dock)
{
    struct NewBroker broker;
    CxObj *action,*filter,*sender;
    IX right_button;
    LONG error=0;
    int i;
    if (!CxBase) return 0;
    dock->hotkey_port=CreateMsgPort();
    if (!dock->hotkey_port) return 0;
    memset(&broker,0,sizeof(broker));
    broker.nb_Version=NB_VERSION;
    broker.nb_Name=(STRPTR)"NovaDock";
    broker.nb_Title=(STRPTR)"NovaDock";
    broker.nb_Descr=(STRPTR)"Workbench dock - Show/Hide reveals or collapses it";
    broker.nb_Unique=NBU_UNIQUE|NBU_NOTIFY;
    broker.nb_Flags=COF_SHOW_HIDE;
    broker.nb_Port=dock->hotkey_port;
    dock->hotkey_broker=CxBroker(&broker,&error);
    if (!dock->hotkey_broker || error!=CBERR_OK) {
        stop_hotkeys(dock);return 0;
    }
    /* Intuition can consume the button event that activates an inactive
     * window.  Observe (but do not swallow) right-button-down through the
     * Commodities input network, then let the normal dock task open its menu.
     * This is more reliable than depending on ACTIVEWINDOW qualifiers alone,
     * particularly with relative mouse input. */
    memset(&right_button,0,sizeof(right_button));
    right_button.ix_Version=IX_VERSION;
    right_button.ix_Class=IECLASS_RAWMOUSE;
    right_button.ix_Code=IECODE_RBUTTON;
    right_button.ix_CodeMask=0;
    right_button.ix_QualMask=IX_NORMALQUALS;
    filter=CreateCxObj(CX_FILTER,0,0);
    sender=CxSender(dock->hotkey_port,ND_CX_RBUTTON_ID);
    if (filter && sender) {
        SetFilterIX(filter,&right_button);
        AttachCxObj(filter,sender);
        AttachCxObj(dock->hotkey_broker,filter);
    } else {
        if (filter) DeleteCxObjAll(filter);
        if (sender) DeleteCxObjAll(sender);
    }
    /* Disk insert/remove input events trigger a prompt media rescan; the
     * periodic scan still covers drivers that mount without such events. */
    {
        static const UBYTE disk_classes[2]={IECLASS_DISKINSERTED,
                                            IECLASS_DISKREMOVED};
        int c;
        for (c=0;c<2;++c) {
            IX disk;
            CxObj *disk_filter=CreateCxObj(CX_FILTER,0,0);
            CxObj *disk_sender=CxSender(dock->hotkey_port,ND_CX_DISK_ID);
            memset(&disk,0,sizeof(disk));
            disk.ix_Version=IX_VERSION;
            disk.ix_Class=disk_classes[c];
            if (disk_filter && disk_sender) {
                SetFilterIX(disk_filter,&disk);
                AttachCxObj(disk_filter,disk_sender);
                AttachCxObj(dock->hotkey_broker,disk_filter);
            } else {
                if (disk_filter) DeleteCxObjAll(disk_filter);
                if (disk_sender) DeleteCxObjAll(disk_sender);
            }
        }
    }
    for (i=0;i<dock->action_count;++i) {
        struct NDItem *item=&dock->action_items[i];
        if (!ITEM_TEXT(item,ND_TEXT_HOTKEY)[0] ||
            (item->kind!=ND_ITEM_LAUNCH && item->kind!=ND_ITEM_DRAWER)) continue;
        action=HotKey((STRPTR)ITEM_TEXT(item,ND_TEXT_HOTKEY),dock->hotkey_port,(LONG)(i+1));
        if (!action) continue;
        if (CxObjError(action)) DeleteCxObjAll(action);
        else AttachCxObj(dock->hotkey_broker,action);
    }
    /* The dock's own hot key: keyboard control of the tiles. */
    if (dock->conf.dock_hotkey[0]) {
        action=HotKey((STRPTR)dock->conf.dock_hotkey,dock->hotkey_port,
                      ND_CX_FOCUS_ID);
        if (action && CxObjError(action)) DeleteCxObjAll(action);
        else if (action) AttachCxObj(dock->hotkey_broker,action);
    }
    ActivateCxObj(dock->hotkey_broker,1L);
    return 1;
}
void rebuild_hotkeys(struct NDDock *dock)
{
    stop_hotkeys(dock);
    (void)start_hotkeys(dock);
}
void process_hotkeys(struct NDDock *dock)
{
    CxMsg *message;
    while (dock->hotkey_port &&
           (message=(CxMsg *)GetMsg(dock->hotkey_port))!=NULL) {
        ULONG type=CxMsgType(message);
        LONG id=CxMsgID(message);
        struct InputEvent *input_event=type==CXM_IEVENT ?
            (struct InputEvent *)CxMsgData(message) : NULL;
        if (type==CXM_IEVENT && id==ND_CX_RBUTTON_ID && input_event &&
            nd_right_button_down((unsigned int)input_event->ie_Class,
                                 (unsigned int)input_event->ie_Code) &&
            dock->screen &&
            dock->window && !dock->hidden && !dock->box_pending &&
            !(dock->window->Flags&WFLG_WINDOWACTIVE) && !dock->popup &&
            inside_rect(&dock->full_box,
                         dock->screen->MouseX,dock->screen->MouseY)) {
            /* The Commodities filter is only an inactive-window fallback.
             * Revalidate its attached InputEvent so pointer movement can
             * never arm a context menu, even with unusual input drivers. */
            dock->activate_popup_pending=1;
            ActivateWindow(dock->window);
        }
        ReplyMsg((struct Message *)message);
        if (type==CXM_IEVENT && id==ND_CX_DISK_ID) {
            if (dock->media_scan_ticks>ND_MEDIA_EVENT_TICKS)
                dock->media_scan_ticks=ND_MEDIA_EVENT_TICKS;
        } else if (type==CXM_IEVENT && id==ND_CX_FOCUS_ID) {
            keyboard_begin(dock);
        } else if (type==CXM_IEVENT && id>0 && id<=dock->action_count) {
            if (!launch_action(dock,&dock->action_items[id-1]))
                show_feedback(dock,"Hot-key Launcher Action failed",1);
        } else if (type==CXM_COMMAND && id==CXCMD_KILL) dock->running=0;
        else if (type==CXM_COMMAND &&
                (id==CXCMD_APPEAR || id==CXCMD_UNIQUE)) show_dock(dock);
        else if (type==CXM_COMMAND && id==CXCMD_DISAPPEAR) hide_dock(dock);
        else if (type==CXM_COMMAND && id==CXCMD_DISABLE)
            ActivateCxObj(dock->hotkey_broker,0L);
        else if (type==CXM_COMMAND && id==CXCMD_ENABLE)
            ActivateCxObj(dock->hotkey_broker,1L);
    }
}
int build_wb_path(const struct WBArg *arg, char *out, int out_size,
                         int *kind)
{
    struct FileInfoBlock *fib;
    BPTR obj;
    char info[ND_COMMAND_LEN+6];
    int good = 0;
    if (!arg || !arg->wa_Lock || !NameFromLock(arg->wa_Lock, out, out_size))
        return 0;
    if (arg->wa_Name && *arg->wa_Name &&
        !AddPart(out, (STRPTR)arg->wa_Name, out_size)) return 0;
    obj = Lock(out, ACCESS_READ);
    if (!obj) {
        /* Some Workbench tools are represented only by a project/tool icon.
         * Accept that valid DiskObject and retain its suffix-free base path. */
        if (strlen(out)+5>=sizeof(info)) return 0;
        strcpy(info,out);strcat(info,".info");
        obj=Lock(info,ACCESS_READ);
        if (!obj) return 0;
        UnLock(obj);*kind=ND_ITEM_LAUNCH;return 1;
    }
    fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);
    if (fib) {
        if (Examine(obj, fib)) {
            *kind = fib->fib_DirEntryType > 0 ? ND_ITEM_DRAWER : ND_ITEM_LAUNCH;
            good = 1;
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(obj);
    return good;
}
/* Select the icon base Workbench uses for the dropped object.  Ordinary
 * objects keep their icon beside them as <object>.info; a volume root is the
 * special case and keeps it as Disk.info.  icon.library wants the path
 * without the .info suffix, so that is what is stored in the configuration.
 * The loader supplies Workbench's default icon when no private icon exists. */
void dropped_icon_path(const char *path, int kind, char *out,
                              int out_size)
{
    size_t length;
    if (!path || !*path || !out || out_size <= 0) return;
    out[0] = '\0';
    length = strlen(path);
    if (length >= (size_t)out_size) return;
    strcpy(out, path);
    if (kind == ND_ITEM_DRAWER && path[length - 1] == ':' &&
        !AddPart(out, (STRPTR)"Disk", out_size)) out[0] = '\0';
}
/* Workbench 3.5+ opens drawers natively through workbench.library V44.
 * For earlier versions users can install C:WBRUN (if available).
 * Only configured local drawer paths are accepted, never shell syntax. */
void open_drawer(const char *path)
{
    char quoted[ND_COMMAND_LEN*2+4], command[ND_COMMAND_LEN*2+32];
    if (!path || !*path || strlen(path) >= ND_COMMAND_LEN) return;
    if (WorkbenchBase && WorkbenchBase->lib_Version >= 44 &&
        OpenWorkbenchObject((STRPTR)path, TAG_DONE)) return;
    /* The command is optional on early OS 3.x installations. */
    if (!nd_quote_path(quoted, sizeof(quoted), path)) return;
    sprintf(command, "C:WBRUN %s", quoted);
    launch_command(command);
}
