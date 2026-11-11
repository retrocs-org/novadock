/* Launcher page for NovaDock Preferences 0.1; GadTools, 68000/OS3.0+.
 * Supports DataTypes pictures plus classic .info and IFF/ILBM icons.
 * Plugin order and all unrelated settings are preserved verbatim in memory.
 */
#define IntuitionBase NDLauncherIntuitionBase
#define GadToolsBase NDLauncherGadToolsBase
#define GfxBase NDLauncherGfxBase
#define AslBase NDLauncherAslBase
#define WorkbenchBase NDLauncherWorkbenchBase
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <proto/wb.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_control_client.h"
#include "nd_layout.h"
#include "nd_prefs_nav.h"
#include "nd_prefs_session.h"
#include "nd_prefs_layout.h"
#include "nd_prefs_help.h"

#define G_LIST 10
#define G_NAME 11
#define G_TARGET 12
#define G_ICON 13
#define G_KIND 14
#define G_ADD_APP 15
#define G_ADD_DRAWER 16
#define G_APPLY 17
#define G_REMOVE 18
#define G_BROWSE 19
#define G_EXAMPLE 20
#define G_USE 21
#define G_CANCEL 23
#define G_SAVE 22
#define G_TARGET_BROWSE 24
#define G_NAV_GENERAL 25
#define G_NAV_APPEARANCE 26
#define G_NAV_ITEMS 27
#define G_MODE 29
#define G_ARGUMENTS 30
#define G_WORKDIR 31
#define G_HOTKEY 32
#define G_NAV_DIAGNOSTICS 34
#define G_SPACE_METER 35
#define G_MATCH_TASK 36
#define G_MATCH_WINDOW 37
#define G_MATCH_PORT 38

struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *AslBase = NULL;
struct Library *WorkbenchBase = NULL;
static STRPTR kinds[] = {"Application", "Directory", "Application group", NULL};
static STRPTR modes[] = {"Automatic", "Workbench", "CLI", "ARexx", NULL};
struct Editor {
    struct NDConfig *cfg;
    struct NDConfig *original;
    int original_dirty;
    struct Screen *screen;
    struct Window *win;
    struct MsgPort *drop_port;
    struct AppWindow *app_window;
    APTR vi;
    struct Gadget *gadgets, *last, *list, *name, *target, *icon, *kind;
    struct Gadget *mode, *arguments, *workdir, *hotkey, *space_meter;
    struct Gadget *match_task, *match_window, *match_port;
    struct NDPrefsLayout layout;
    struct List rows;
    struct Node nodes[ND_MAX_ITEMS];
    char names[ND_MAX_ITEMS][64];
    char window_title[90];
    int map[ND_MAX_ITEMS];
    int nrows, selection, done, result, focused;
    int requested_visible, request_icon;
};
static int wb_arg_path(const struct WBArg *arg, char *path, int path_size,
                       int *kind)
{
    struct FileInfoBlock *fib;
    BPTR object;
    char info[ND_COMMAND_LEN+6];
    int valid=0;
    if (!arg || !arg->wa_Lock ||
        !NameFromLock(arg->wa_Lock,path,path_size)) return 0;
    if (arg->wa_Name && *arg->wa_Name &&
        !AddPart(path,(STRPTR)arg->wa_Name,path_size)) return 0;
    object=Lock(path,ACCESS_READ);
    if (!object) {
        /* Workbench projects and command icons can legitimately consist of
         * only <name>.info. Keep the launch target suffix-free so a modern
         * workbench.library can resolve its DefaultTool when it is opened. */
        if (strlen(path)+5>=sizeof(info)) return 0;
        strcpy(info,path);strcat(info,".info");
        object=Lock(info,ACCESS_READ);
        if (!object) return 0;
        UnLock(object);*kind=ND_ITEM_LAUNCH;return 1;
    }
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib) {
        if (Examine(object,fib)) {
            *kind=fib->fib_DirEntryType>0 ? ND_ITEM_DRAWER : ND_ITEM_LAUNCH;
            valid=1;
        }
        FreeDosObject(DOS_FIB,fib);
    }
    UnLock(object);
    return valid;
}
/* Workbench stores normal object icons as <object>.info, but volume icons as
 * Disk.info in the volume root.  Store the suffix-free base path expected by
 * icon.library; the dock supplies Workbench's default icon if it has no
 * private .info file. */
static void dropped_icon_path(const char *path,int kind,char *out,int out_size)
{
    size_t length;
    if (!path || !*path || !out || out_size<=0) return;
    out[0]='\0';
    length=strlen(path);
    if (length>=(size_t)out_size) return;
    strcpy(out,path);
    if (kind==ND_ITEM_DRAWER && path[length-1]==':' &&
        !AddPart(out,(STRPTR)"Disk",out_size)) out[0]='\0';
}
static void title(struct Editor *e, const char *s)
{
    if (e->win) SetWindowTitles(e->win, (STRPTR)s, (STRPTR)-1);
}
static void ng_init(struct Editor *e, struct NewGadget *ng, int x, int y,
                    int w, int h, int id, STRPTR caption)
{
    memset(ng, 0, sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&e->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&e->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&e->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&e->layout,h);
    ng->ng_GadgetID=id; ng->ng_GadgetText=caption;
    ng->ng_VisualInfo=e->vi; ng->ng_TextAttr=e->screen->Font;
    ng->ng_Flags=PLACETEXT_LEFT;
}
static struct Gadget *button(struct Editor *e, int x, int y, int w,
                             int id, STRPTR label)
{
    struct NewGadget ng;
    ng_init(e,&ng,x,y,w,16,id,label);
    ng.ng_Width=nd_prefs_layout_button_width(&e->layout,w,label);
    ng.ng_Flags=PLACETEXT_IN;
    e->last=CreateGadget(BUTTON_KIND,e->last,&ng,
                         GA_Disabled,(ULONG)(label[0]=='>'),TAG_DONE);
    return e->last;
}
static void build_list(struct Editor *e)
{
    int i;
    NewList(&e->rows);
    e->nrows=0;
    for (i=0; i<e->cfg->count; ++i) {
        struct NDItem *it=&e->cfg->items[i];
        int row;
        if (it->kind != ND_ITEM_LAUNCH && it->kind != ND_ITEM_DRAWER &&
            it->kind != ND_ITEM_GROUP) continue;
        row=e->nrows++;
        e->map[row]=i;
        sprintf(e->names[row], "%c%c %c %.15s", ' ',
                it->enabled ? '*' : ' ',
                it->kind==ND_ITEM_GROUP ? 'G' : it->kind==ND_ITEM_DRAWER ?
                (nd_item_is_volume(it) ? 'V' : 'D') : 'A',
                it->label);
        memset(&e->nodes[row],0,sizeof(struct Node));
        e->nodes[row].ln_Name=e->names[row];
        AddTail(&e->rows,&e->nodes[row]);
    }
    if (e->selection >= e->nrows) e->selection=e->nrows-1;
    if (e->selection < 0 && e->nrows) e->selection=0;
    for (i=0;i<e->nrows;++i)
        e->names[i][0]=i==e->selection ? '>' : ' ';
}
static void set_fields(struct Editor *e)
{
    struct NDItem *it;
    int k;
    if (e->selection<0 || e->selection>=e->nrows) return;
    it=&e->cfg->items[e->map[e->selection]];
    GT_SetGadgetAttrs(e->name,e->win,NULL, GTST_String,(ULONG)it->label,TAG_DONE);
    GT_SetGadgetAttrs(e->target,e->win,NULL, GTST_String,(ULONG)it->command,TAG_DONE);
    GT_SetGadgetAttrs(e->icon,e->win,NULL, GTST_String,(ULONG)nd_item_text(e->cfg,it,ND_TEXT_ICON),TAG_DONE);
    k=it->kind==ND_ITEM_GROUP ? 2 : (it->kind==ND_ITEM_DRAWER ? 1 : 0);
    GT_SetGadgetAttrs(e->kind,e->win,NULL, GTCY_Active,(ULONG)k,TAG_DONE);
    GT_SetGadgetAttrs(e->mode,e->win,NULL,
                      GTCY_Active,(ULONG)it->launch_mode,TAG_DONE);
    GT_SetGadgetAttrs(e->arguments,e->win,NULL,
                      GTST_String,(ULONG)nd_item_text(e->cfg,it,ND_TEXT_ARGS),TAG_DONE);
    GT_SetGadgetAttrs(e->workdir,e->win,NULL,
                      GTST_String,(ULONG)nd_item_text(e->cfg,it,ND_TEXT_WORKDIR),TAG_DONE);
    GT_SetGadgetAttrs(e->hotkey,e->win,NULL,
                      GTST_String,(ULONG)nd_item_text(e->cfg,it,ND_TEXT_HOTKEY),TAG_DONE);
    GT_SetGadgetAttrs(e->match_task,e->win,NULL,
                      GTST_String,(ULONG)nd_item_text(e->cfg,it,ND_TEXT_MATCH_TASK),TAG_DONE);
    GT_SetGadgetAttrs(e->match_window,e->win,NULL,
                      GTST_String,(ULONG)nd_item_text(e->cfg,it,ND_TEXT_MATCH_WINDOW),TAG_DONE);
    GT_SetGadgetAttrs(e->match_port,e->win,NULL,
                      GTST_String,(ULONG)nd_item_text(e->cfg,it,ND_TEXT_MATCH_PORT),TAG_DONE);
    if (e->space_meter)
        GT_SetGadgetAttrs(e->space_meter,e->win,NULL,
                          GTCB_Checked,(ULONG)(it->show_volume_meter ? TRUE : FALSE),
                          GA_Disabled,(ULONG)(nd_item_supports_space_meter(it) ?
                                              FALSE : TRUE),
                          TAG_DONE);
}
static void refresh_list(struct Editor *e)
{
    GT_SetGadgetAttrs(e->list,e->win,NULL,GTLV_Labels,~0UL,TAG_DONE);
    build_list(e);
    GT_SetGadgetAttrs(e->list,e->win,NULL,GTLV_Labels,(ULONG)&e->rows,
                      GTLV_Selected,(ULONG)(e->selection<0 ? ~0UL : e->selection),TAG_DONE);
    set_fields(e);
}
static const char *field(struct Gadget *g)
{
    struct StringInfo *si=(struct StringInfo *)g->SpecialInfo;
    return si && si->Buffer ? (const char *)si->Buffer : "";
}
/* The editor's undo keeps the item's texts as strings: pooled offsets in a
 * saved copy of the item would go stale if the pool compacted mid-edit. */
static char saved_texts[ND_TEXT_COUNT][ND_COMMAND_LEN];
static void save_texts(const struct NDConfig *cfg,const struct NDItem *item)
{
    int k;
    for (k=0;k<ND_TEXT_COUNT;++k) {
        strncpy(saved_texts[k],nd_item_text(cfg,item,k),ND_COMMAND_LEN-1);
        saved_texts[k][ND_COMMAND_LEN-1]='\0';
    }
}
static int texts_changed(const struct NDConfig *cfg,const struct NDItem *item)
{
    int k;
    for (k=0;k<ND_TEXT_COUNT;++k)
        if (strcmp(saved_texts[k],nd_item_text(cfg,item,k))) return 1;
    return 0;
}
static void restore_item(struct NDConfig *cfg,int index,const struct NDItem *old)
{
    static const int which[ND_TEXT_COUNT]={ND_TEXT_ICON,ND_TEXT_ARGS,
        ND_TEXT_WORKDIR,ND_TEXT_HOTKEY,ND_TEXT_MATCH_TASK,
        ND_TEXT_MATCH_WINDOW,ND_TEXT_MATCH_PORT};
    const char *values[ND_TEXT_COUNT];
    int k;
    cfg->items[index]=*old;
    for (k=0;k<ND_TEXT_COUNT;++k) {
        cfg->items[index].text[k]=0; /* old offsets may predate compaction */
        values[k]=saved_texts[k];
    }
    /* These texts fitted before the edit, so they fit again. */
    (void)nd_item_set_texts(cfg,&cfg->items[index],ND_TEXT_COUNT,which,values);
}
static int same_fields(const struct NDItem *a,const struct NDItem *b)
{
    struct NDItem x=*a,y=*b;
    memset(x.text,0,sizeof(x.text));memset(y.text,0,sizeof(y.text));
    return memcmp(&x,&y,sizeof(x))==0;
}
static int update_current(struct Editor *e)
{
    ULONG cy=0,mode=0,space_meter=1;
    int success;
    struct NDItem old;
    int index;
    if (e->selection<0 || e->selection>=e->nrows) return 0;
    GT_GetGadgetAttrs(e->kind,e->win,NULL,GTCY_Active,(ULONG)&cy,TAG_DONE);
    GT_GetGadgetAttrs(e->mode,e->win,NULL,GTCY_Active,(ULONG)&mode,TAG_DONE);
    index=e->map[e->selection];
    old=e->cfg->items[index];
    save_texts(e->cfg,&old);
    space_meter=(ULONG)(old.show_volume_meter ? TRUE : FALSE);
    if (e->space_meter)
        GT_GetGadgetAttrs(e->space_meter,e->win,NULL,
                          GTCB_Checked,(ULONG)&space_meter,TAG_DONE);
    success=nd_edit_launcher(e->cfg,index,
                             cy==2 ? ND_ITEM_GROUP :
                             (cy==1 ? ND_ITEM_DRAWER : ND_ITEM_LAUNCH),
                             field(e->name),field(e->target),field(e->icon));
    if (success)
        success=nd_set_launcher_action(e->cfg,index,(int)mode,
                   field(e->arguments),field(e->workdir),field(e->hotkey),
                   old.tools_menu);
    if (success)
        success=nd_set_launcher_match(e->cfg,index,field(e->match_task),
                   field(e->match_window),field(e->match_port));
    if (success && e->space_meter)
        e->cfg->items[index].show_volume_meter=space_meter ? 1 : 0;
    if (!success) {
        restore_item(e->cfg,index,&old);
        title(e,"NovaDock Items - invalid or too-long Launcher Action field");
        DisplayBeep(e->screen);
        return 0;
    }
    if (!same_fields(&old,&e->cfg->items[index]) ||
        texts_changed(e->cfg,&e->cfg->items[index]))
        ND_PREFS_MARK_DIRTY();
    if (e->list) refresh_list(e);
    return 1;
}
static void add_launcher(struct Editor *e, int kind)
{
    const char *name=kind==ND_ITEM_GROUP ? "Applications" :
                     kind==ND_ITEM_DRAWER ? "New Drawer" : "New App";
    const char *target=kind==ND_ITEM_LAUNCH ? "SYS:Tools/MultiView" :
                       "SYS:Utilities";
    const char *icon=kind==ND_ITEM_LAUNCH ? "Icons/Workbench.iff" :
                     "Icons/Drawer.iff";
    if (!nd_add_launcher(e->cfg,kind,name,target,icon)) {
        title(e,"NovaDock Launchers - list full");
        DisplayBeep(e->screen);
        return;
    }
    /* Definitions are reusable; Organize decides when they become active. */
    e->cfg->items[e->cfg->count-1].enabled=0;
    ND_PREFS_MARK_DIRTY();
    /* The new item is last in config, and hence last of launcher rows. */
    e->selection=e->nrows;
    refresh_list(e);
}
static void dropped_icons(struct Editor *e)
{
    struct AppMessage *message;
    int i,kind,added;
    char path[ND_COMMAND_LEN],icon[ND_COMMAND_LEN],name[ND_LABEL_LEN];
    /* A Workbench drop rebuilds and selects the list.  Preserve any text the
       user was editing first; invalid fields keep their focus and reject the
       drop rather than being overwritten. */
    if (e->selection>=0 && !update_current(e)) {
        while (e->drop_port &&
               (message=(struct AppMessage *)GetMsg(e->drop_port))!=NULL)
            ReplyMsg((struct Message *)message);
        return;
    }
    while (e->drop_port &&
           (message=(struct AppMessage *)GetMsg(e->drop_port))!=NULL) {
        added=0;
        for (i=0;i<message->am_NumArgs;++i) {
            if (!wb_arg_path(&message->am_ArgList[i],path,sizeof(path),&kind))
                continue;
            nd_label_from_path(path,name,sizeof(name));
            if (!*name) continue;
            dropped_icon_path(path,kind,icon,sizeof(icon));
            if (!nd_add_launcher(e->cfg,kind,name,path,icon)) break;
            e->cfg->items[e->cfg->count-1].enabled=0;
            ND_PREFS_MARK_DIRTY();
            ++added;
        }
        ReplyMsg((struct Message *)message);
        if (added) {
            e->selection=e->nrows+added-1;
            refresh_list(e);
            title(e,"NovaDock Items - dropped definition added");
        } else {
            title(e,"NovaDock Items - dropped item could not be added");
            DisplayBeep(e->screen);
        }
    }
}
static void choose_icon(struct Editor *e)
{
    struct FileRequester *fr;
    char path[ND_COMMAND_LEN];
    if (!AslBase) {
        title(e,"ASL requester not available; type path manually");
        return;
    }
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
                                        ASLFR_TitleText,(ULONG)"Select icon or picture artwork",
                                        TAG_DONE);
    if (!fr) return;
    if (AslRequest(fr,NULL)) {
        strncpy(path,fr->fr_Drawer,sizeof(path)-1);
        path[sizeof(path)-1]='\0';
        if (AddPart(path,fr->fr_File,sizeof(path)))
            GT_SetGadgetAttrs(e->icon,e->win,NULL,GTST_String,(ULONG)path,TAG_DONE);
    }
    FreeAslRequest(fr);
}
static void choose_target(struct Editor *e)
{
    struct FileRequester *fr;
    ULONG kind=0;
    char path[ND_COMMAND_LEN];
    if (!AslBase) {
        title(e,"ASL requester not available; type target manually");
        return;
    }
    GT_GetGadgetAttrs(e->kind,e->win,NULL,GTCY_Active,(ULONG)&kind,TAG_DONE);
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)(kind ? "Select Workbench directory" :
                                       "Select Amiga application"),
        ASLFR_DrawersOnly,kind ? TRUE : FALSE,TAG_DONE);
    if (!fr) return;
    if (AslRequest(fr,NULL)) {
        strncpy(path,fr->fr_Drawer,sizeof(path)-1);
        path[sizeof(path)-1]='\0';
        if (!kind && !AddPart(path,fr->fr_File,sizeof(path))) {
            FreeAslRequest(fr);
            title(e,"Target path too long");
            return;
        }
        GT_SetGadgetAttrs(e->target,e->win,NULL,GTST_String,(ULONG)path,TAG_DONE);
    }
    FreeAslRequest(fr);
}
static void mk_dir(const char *p)
{
    BPTR lock=CreateDir((STRPTR)p);
    if (lock) UnLock(lock);
}
static int commit(struct Editor *e, int permanent)
{
    LONG rc;
    struct NDLayout fit;
    if (e->selection>=0 && !update_current(e)) return 0;
    if (!nd_layout_fit_enabled(e->cfg,e->screen->Width,e->screen->Height,
                               e->screen->BarHeight,&fit)) {
        title(e,"NovaDock Launchers - too many tiles for screen");
        DisplayBeep(e->screen);
        return 0;
    }
    mk_dir("ENV:NovaDock");
    if (permanent) {
        mk_dir("ENVARC:NovaDock");
        if (!nd_control_write_config(e->cfg,"ENVARC:NovaDock/novadock.cfg")) {
            title(e,"NovaDock Launchers - could not save ENVARC:");
            return 0;
        }
    }
    if (!nd_control_write_config(e->cfg,"ENV:NovaDock/novadock.cfg")) {
        title(e,"NovaDock Launchers - could not write ENV:");
        return 0;
    }
    rc=nd_control_reload();
    if (rc!=ND_CONTROL_OK && rc!=ND_CONTROL_NOHOST &&
        rc!=ND_CONTROL_PENDING) {
        title(e,rc==ND_CONTROL_TIMEOUT ?
            "Saved, but NovaDock reload timed out" :
            "Saved, but NovaDock rejected reload");
        return 0;
    }
    ND_PREFS_MARK_CLEAN();ND_PREFS_CLEAR_PREVIEW();
    e->done=1; e->result=0;
    return 1;
}
static int open_ui(struct Editor *e)
{
    struct NewGadget ng;
    int base_width=620,base_height=e->focused ? 301 : 320;
    int width,height,x,y,font_x=8,font_y=8;
    if (e->screen->RastPort.Font) {
        font_x=e->screen->RastPort.Font->tf_XSize;
        font_y=e->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&e->layout,font_x,font_y,
        e->screen->Width,e->screen->Height,e->screen->BarHeight,
        base_width,base_height);
    if (!nd_prefs_layout_fits(&e->layout,10,12)) return 0;
    width=e->layout.window_width;height=e->layout.window_height;
    x=nd_prefs_layout_left(&e->layout);y=nd_prefs_layout_top(&e->layout);
    e->vi=GetVisualInfo(e->screen,TAG_DONE);
    if (!e->vi || !CreateContext(&e->gadgets)) return 0;
    e->last=e->gadgets;
    build_list(e);
    if (e->requested_visible >= 0) {
        int wanted = nd_visible_to_full(e->cfg, e->requested_visible);
        int row;
        for (row = 0; row < e->nrows; ++row)
            if (e->map[row] == wanted) { e->selection = row; break; }
        for (row=0;row<e->nrows;++row)
            e->names[row][0]=row==e->selection ? '>' : ' ';
    }
    if (e->focused) {
        struct NDItem *item;
        if (e->selection<0 || e->selection>=e->nrows) return 0;
        item=&e->cfg->items[e->map[e->selection]];
        ng_init(e,&ng,125,18,480,16,G_NAME,"Name:");
        e->name=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_LABEL_LEN-1,TAG_DONE);
        if (!e->last) return 0;
        ng_init(e,&ng,125,43,380,16,G_TARGET,"Target:");
        e->target=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_COMMAND_LEN-1,TAG_DONE);
        if (!e->last || !button(e,515,43,90,G_TARGET_BROWSE,"Target...")) return 0;
        ng_init(e,&ng,125,68,380,16,G_ICON,"Icon:");
        e->icon=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_COMMAND_LEN-1,TAG_DONE);
        if (!e->last || !button(e,515,68,90,G_BROWSE,"Icon...")) return 0;
        ng_init(e,&ng,125,93,150,16,G_KIND,"Type:");
        e->kind=e->last=CreateGadget(CYCLE_KIND,e->last,&ng,
                                 GTCY_Labels,(ULONG)kinds,TAG_DONE);
        if (!e->last) return 0;
        ng_init(e,&ng,365,93,150,16,G_MODE,"Run as:");
        e->mode=e->last=CreateGadget(CYCLE_KIND,e->last,&ng,
                                 GTCY_Labels,(ULONG)modes,TAG_DONE);
        ng_init(e,&ng,125,118,480,16,G_ARGUMENTS,"Arguments:");
        e->arguments=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_COMMAND_LEN-1,TAG_DONE);
        ng_init(e,&ng,125,143,480,16,G_WORKDIR,"Start in:");
        e->workdir=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_COMMAND_LEN-1,TAG_DONE);
        ng_init(e,&ng,125,168,205,16,G_HOTKEY,"Hot key:");
        e->hotkey=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_HOTKEY_LEN-1,TAG_DONE);
        /* Focused item editing keeps the final row uncluttered: the global
         * Tools-menu flag remains preserved in the action definition, while
         * every directory can opt into its backing volume's usage meter. */
        ng_init(e,&ng,585,168,20,16,G_SPACE_METER,"Show space usage:");
        e->space_meter=e->last=CreateGadget(CHECKBOX_KIND,e->last,&ng,TAG_DONE);
        ng_init(e,&ng,125,193,480,16,G_MATCH_TASK,"Running task:");
        e->match_task=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_MATCH_LEN-1,TAG_DONE);
        ng_init(e,&ng,125,218,480,16,G_MATCH_WINDOW,"Window pattern:");
        e->match_window=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_MATCH_LEN-1,TAG_DONE);
        ng_init(e,&ng,125,243,480,16,G_MATCH_PORT,"ARexx port:");
        e->match_port=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                                 GTST_MaxChars,ND_MATCH_LEN-1,TAG_DONE);
        if (!e->last || !button(e,325,273,90,G_CANCEL,"Cancel") ||
            !button(e,425,273,80,G_USE,"Use") ||
            !button(e,515,273,90,G_SAVE,"Save")) return 0;
        sprintf(e->window_title,"Launcher Action - %s",item->label);
    } else {
    if (!button(e,10,21,118,G_NAV_GENERAL,"General") ||
        !button(e,10,45,118,G_NAV_APPEARANCE,"Appearance") ||
        !button(e,10,69,118,G_NAV_ITEMS,"Organize") ||
        !button(e,10,93,118,G_NAV_DIAGNOSTICS,"Diagnostics")) return 0;
    ng_init(e,&ng,135,30,190,163,G_LIST,"Actions (> pick, * dock)");
    ng.ng_Flags=PLACETEXT_ABOVE;
    e->list=e->last=CreateGadget(LISTVIEW_KIND,e->last,&ng,
                  GTLV_Labels,(ULONG)&e->rows,GTLV_Selected,(ULONG)e->selection,TAG_DONE);
    if (!e->last) return 0;
    ng_init(e,&ng,415,24,190,16,G_NAME,"Name:");
    e->name=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_LABEL_LEN-1,TAG_DONE);
    if (!e->last) return 0;
    ng_init(e,&ng,415,48,190,16,G_TARGET,"Target:");
    e->target=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_COMMAND_LEN-1,TAG_DONE);
    if (!e->last) return 0;
    ng_init(e,&ng,415,72,190,16,G_ICON,"Icon:");
    e->icon=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_COMMAND_LEN-1,TAG_DONE);
    if (!e->last) return 0;
    /* Give each cycle its own complete row. This remains readable with the
     * classic 8-pixel Workbench font and longer localized labels. */
    ng_init(e,&ng,415,96,190,16,G_KIND,"Type:");
    e->kind=e->last=CreateGadget(CYCLE_KIND,e->last,&ng,
                             GTCY_Labels,(ULONG)kinds,TAG_DONE);
    if (!e->last) return 0;
    ng_init(e,&ng,415,120,190,16,G_MODE,"Run as:");
    e->mode=e->last=CreateGadget(CYCLE_KIND,e->last,&ng,
                             GTCY_Labels,(ULONG)modes,TAG_DONE);
    ng_init(e,&ng,415,144,190,16,G_ARGUMENTS,"Arguments:");
    e->arguments=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_COMMAND_LEN-1,TAG_DONE);
    ng_init(e,&ng,415,168,190,16,G_WORKDIR,"Start in:");
    e->workdir=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_COMMAND_LEN-1,TAG_DONE);
    ng_init(e,&ng,415,192,190,16,G_HOTKEY,"Hot key:");
    e->hotkey=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_HOTKEY_LEN-1,TAG_DONE);
    /* Compact captions keep the matching fields clear of the two-column
     * action buttons on an unscaled 620-pixel Preferences window. */
    ng_init(e,&ng,415,216,190,16,G_MATCH_TASK,"Task:");
    e->match_task=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_MATCH_LEN-1,TAG_DONE);
    ng_init(e,&ng,415,240,190,16,G_MATCH_WINDOW,"Window:");
    e->match_window=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_MATCH_LEN-1,TAG_DONE);
    ng_init(e,&ng,415,264,190,16,G_MATCH_PORT,"ARexx:");
    e->match_port=e->last=CreateGadget(STRING_KIND,e->last,&ng,
                             GTST_MaxChars,ND_MATCH_LEN-1,TAG_DONE);
    /* The Workbench Tools-menu flag is a legacy config field. Preserve it on
     * edits, but do not expose a vague checkbox in the primary editor. */
    if (!e->last || !button(e,135,216,90,G_ADD_APP,"Add App") ||
        !button(e,235,216,90,G_ADD_DRAWER,"Add Dir") ||
        !button(e,135,240,90,G_REMOVE,"Remove") ||
        !button(e,235,240,90,G_APPLY,"Update") ||
        !button(e,135,264,90,G_BROWSE,"Icon...") ||
        !button(e,235,264,90,G_TARGET_BROWSE,"Target...") ||
        !button(e,331,294,90,G_CANCEL,"Cancel") ||
        !button(e,427,294,86,G_USE,"Use") ||
        !button(e,519,294,86,G_SAVE,"Save")) return 0;
    }
    e->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)e->screen,
               WA_Left,x,WA_Top,y,WA_Width,width,WA_Height,height,
               WA_Title,(ULONG)(e->focused ? e->window_title :
                   (ND_PREFS_IS_DIRTY() ?
                    "NovaDock Preferences - Launcher Actions *" :
                    "NovaDock Preferences - Launcher Actions")),
               WA_Gadgets,(ULONG)e->gadgets,
               WA_DragBar,TRUE,WA_Activate,TRUE,WA_DepthGadget,TRUE,
               WA_CloseGadget,TRUE,
               WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
               WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!e->win) return 0;
    if (!e->focused && WorkbenchBase && e->drop_port)
        e->app_window=AddAppWindowA(2UL,0UL,e->win,e->drop_port,NULL);
    GT_RefreshWindow(e->win,NULL);
    set_fields(e);
    if (e->request_icon && e->selection >= 0) choose_icon(e);
    return 1;
}
static void cancel_editor(struct Editor *e)
{
    if (e->original) {
        *e->cfg=*e->original;
        if (e->original_dirty) ND_PREFS_MARK_DIRTY();
        else ND_PREFS_MARK_CLEAN();
    }
    e->done=1;e->result=0;
}
static void on_gadget(struct Editor *e,UWORD id,UWORD code)
{
    int old_selection;
    switch(id) {
        case G_CANCEL: cancel_editor(e); break;
        case G_NAV_GENERAL:
            if (e->selection<0 || update_current(e)) {e->done=1;e->result=ND_PREF_NAV_GENERAL;}break;
        case G_NAV_APPEARANCE:
            if (e->selection<0 || update_current(e)) {e->done=1;e->result=ND_PREF_NAV_APPEARANCE;}break;
        case G_NAV_ITEMS:
            if (e->selection<0 || update_current(e)) {e->done=1;e->result=ND_PREF_NAV_ORGANIZE;}break;
        case G_NAV_DIAGNOSTICS:
            if (e->selection<0 || update_current(e)) {e->done=1;e->result=ND_PREF_NAV_DIAGNOSTICS;}break;
        case G_LIST:
            if (code<e->nrows && code!=e->selection) {
                old_selection=e->selection;
                if (old_selection<0 || update_current(e)) {
                    e->selection=code;
                    refresh_list(e);
                } else {
                    /* GadTools has already highlighted the requested row.
                       Restore the old selection without replacing the user's
                       invalid field contents. */
                    GT_SetGadgetAttrs(e->list,e->win,NULL,
                                      GTLV_Selected,(ULONG)old_selection,
                                      TAG_DONE);
                }
            }
            break;
        case G_KIND:
            if (e->space_meter)
                GT_SetGadgetAttrs(e->space_meter,e->win,NULL,
                                  GA_Disabled,(ULONG)(code==1 ? FALSE : TRUE),
                                  TAG_DONE);
            break;
        case G_ADD_APP:
            if (e->selection<0 || update_current(e))
                add_launcher(e,ND_ITEM_LAUNCH);
            break;
        case G_ADD_DRAWER:
            if (e->selection<0 || update_current(e))
                add_launcher(e,ND_ITEM_DRAWER);
            break;
        case G_APPLY: update_current(e); break;
        case G_REMOVE:
            if (e->selection>=0 && e->selection<e->nrows) {
                nd_remove_launcher(e->cfg,e->map[e->selection]);
                ND_PREFS_MARK_DIRTY();
                refresh_list(e);
            }
            break;
        case G_BROWSE: choose_icon(e); break;
        case G_TARGET_BROWSE: choose_target(e); break;
        case G_USE: commit(e,0); break;
        case G_SAVE: commit(e,1); break;
        default: break;
    }
}
int nd_launcher_prefs_run(int argc,char **argv)
{
    struct Editor *e;
    struct IntuiMessage *msg;
    ULONG cls,signals,win_signal,drop_signal;
    UWORD id,code;
    int parse,result=20;
    e=(struct Editor *)AllocVec(sizeof(*e),MEMF_PUBLIC|MEMF_CLEAR);
    if (!e) return 20;
    e->requested_visible = -1;
    /* Launching from the dock passes the visible tile index, not the full
     * configuration index (disabled plugins retain their rows). */
    { int a;
      for (a=1; a<argc; ++a) {
          if (!strncmp(argv[a],"SELECT=",7)) {
              char tail=0;
              int n=-1;
              if (sscanf(argv[a]+7,"%d%c",&n,&tail)==1 && n>=0 && n<ND_MAX_ITEMS)
                  e->requested_visible=n;
          } else if (!strncmp(argv[a],"SELECTFULL=",11)) {
              char tail=0;int n=-1;
              if (sscanf(argv[a]+11,"%d%c",&n,&tail)==1 && n>=0 && n<ND_MAX_ITEMS)
                  e->requested_visible=-(n+2); /* tagged full configuration index */
          } else if (!strcmp(argv[a],"ICON=1")) e->request_icon=1;
          else if (!strcmp(argv[a],"FOCUSED=1")) e->focused=1;
      }
    }
    e->cfg=ND_PREFS_SHARED_CONFIG;
    if (!e->cfg)
        e->cfg=(struct NDConfig *)AllocVec(sizeof(*e->cfg),MEMF_PUBLIC|MEMF_CLEAR);
    if (!e->cfg) goto done;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    AslBase=OpenLibrary("asl.library",37);
    WorkbenchBase=OpenLibrary("workbench.library",36);
    if (!IntuitionBase || !GfxBase || !GadToolsBase) goto done;
    parse=0;
    if (!ND_PREFS_SHARED_CONFIG) parse=nd_load_effective(e->cfg);
    if (parse>0) {
        Printf("NovaDockPrefs Launchers: configuration contains invalid lines.\\n");
        goto done;
    }
    e->original=(struct NDConfig *)AllocVec(sizeof(*e->original),MEMF_PUBLIC);
    if (!e->original) goto done;
    *e->original=*e->cfg;
    e->original_dirty=ND_PREFS_IS_DIRTY();
    if (e->requested_visible<=-2) {
        int full=-e->requested_visible-2,row;
        e->requested_visible=-1;
        build_list(e);
        for (row=0;row<e->nrows;++row)
            if (e->map[row]==full) { e->selection=row;break; }
    }
    e->screen=LockPubScreen("Workbench");
    e->drop_port=CreateMsgPort();
    if (!e->screen || !open_ui(e)) goto done;
    win_signal=1UL<<e->win->UserPort->mp_SigBit;
    drop_signal=e->drop_port ? 1UL<<e->drop_port->mp_SigBit : 0;
    while(!e->done) {
        signals=Wait(win_signal|drop_signal);
        if (signals&drop_signal) dropped_icons(e);
        while((msg=GT_GetIMsg(e->win->UserPort))!=NULL) {
            cls=msg->Class;code=msg->Code;
            id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(msg);
            if (cls==IDCMP_CLOSEWINDOW) cancel_editor(e);
            else if (cls==IDCMP_RAWKEY && code==0x45) cancel_editor(e);
            else if (cls==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(e->screen);
            else if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(e->win); GT_EndRefresh(e->win,TRUE);
            } else if (cls==IDCMP_GADGETUP) on_gadget(e,id,code);
        }
    }
    result=e->result;
done:
    if (e->original) FreeVec(e->original);
    if (e->app_window && WorkbenchBase) {
        RemoveAppWindow(e->app_window);
        e->app_window=NULL;
    }
    if (e->drop_port) {
        struct Message *pending;
        while((pending=GetMsg(e->drop_port))!=NULL) ReplyMsg(pending);
        DeleteMsgPort(e->drop_port);
        e->drop_port=NULL;
    }
    if (e->win) CloseWindow(e->win);
    if (e->gadgets) FreeGadgets(e->gadgets);
    if (e->vi) FreeVisualInfo(e->vi);
    if (e->screen) UnlockPubScreen(NULL,e->screen);
    if (AslBase) CloseLibrary(AslBase);
    if (WorkbenchBase) CloseLibrary(WorkbenchBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (e->cfg && !ND_PREFS_SHARED_CONFIG) FreeVec(e->cfg);
    FreeVec(e);
    return result;
}
#ifndef ND_PREFS_EMBEDDED
int main(int argc,char **argv)
{
    return nd_launcher_prefs_run(argc,argv);
}
#endif
