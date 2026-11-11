/* NovaDockPrefs: standalone classic Workbench preferences editor.
 * GadTools/Intuition only, OS 3.0+, baseline 68000. Fits a 640x200 WB.
 * ENV: is the session setting, ENVARC: the permanent setting.
 */
#include <exec/types.h>
#include <exec/lists.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <libraries/commodities.h>
#include <proto/commodities.h>
#include <stdio.h>
#include <string.h>
#include "nd_version.h"
#include "nd_config.h"
#include "nd_layout.h"
#include "nd_control_client.h"
#include "nd_protocol.h"
#include "nd_prefs_nav.h"
#include "nd_prefs_session.h"
#include "nd_numeric_gadget.h"
#include "nd_prefs_layout.h"
#include "nd_plugin_discovery.h"
#include "nd_prefs_help.h"

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("NovaDockPrefs");

struct NDConfig *nd_prefs_shared_config=NULL;
int nd_prefs_shared_dirty=0;
struct NDConfig *nd_prefs_shared_original=NULL;
int nd_prefs_preview_active=0;

int nd_launcher_prefs_run(int argc, char **argv);
int nd_plugin_prefs_run(int argc, char **argv);
int nd_pluginmanager_prefs_run(int argc,char **argv,char *plugin,int plugin_size);
int nd_appearance_prefs_run(int argc, char **argv);
int nd_dockitems_prefs_run(int argc,char **argv,char *plugin,int plugin_size,
                           int *defined_index);
int nd_spacer_prefs_run(int argc,char **argv);
int nd_diagnostics_prefs_run(int argc,char **argv);
int nd_about_prefs_run(int argc,char **argv);
int nd_layout_prefs_run(struct NDConfig *cfg,struct Screen *screen);
int nd_additem_prefs_run(int argc,char **argv);

#define G_THEME 11
#define G_WIDTH 12
#define G_HEIGHT 13
#define G_GAP 14
#define G_ITEMS 15
#define G_ENABLE 16
#define G_GRAPH 17
#define G_UP 18
#define G_DOWN 19
#define G_USE 20
#define G_SAVE 21
#define G_LAUNCHERS 24
#define G_LEFTCLICK 25
#define G_TOOLTIPS 26
#define G_AUTOHIDE 27
#define G_APPEARANCE 28
#define G_GENERAL 29
#define G_HIDE_DELAY 30
#define G_ALWAYS_ON_TOP 31
#define G_DIAGNOSTICS 32
#define G_QUIT_DOCK 33
#define G_START_DOCK 34
#define G_RESTART_DOCK 35
#define G_LAYOUTS 36
#define G_REFRESH 37
#define G_MEDIA 38
#define G_DOCK_HOTKEY 41
#define G_ABOUT 39
#define G_PLUGIN_MANAGER 40

struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct GfxBase *GfxBase = NULL;

struct Prefs {
    struct NDConfig *cfg;
    struct Screen *screen;
    struct Window *win;
    APTR vi;
    struct Gadget *gadgets;
    struct Gadget *last;
    struct Gadget *list_gad;
    struct Gadget *g_width;
    struct Gadget *g_height;
    struct Gadget *g_gap;
    struct Gadget *g_delay;
    struct Gadget *g_hotkey;
    struct Gadget *g_auto_hide;
    struct Gadget *g_refresh;
    struct Gadget *g_start_dock;
    struct Gadget *g_restart_dock;
    struct Gadget *g_quit_dock;
    struct NDPrefsLayout layout;
    struct List rows;
    struct Node nodes[ND_MAX_ITEMS];
    char names[ND_MAX_ITEMS][68];
    int selected;
    int done;
    int exit_status;
    int next_page;
    char next_plugin[20];
    int requested_visible;
};
static STRPTR clicks[] = {"Run", "Menu", NULL};
static STRPTR tooltip_modes[] = {"Off", "On", NULL};
static STRPTR hide_modes[] = {"Off", "On", NULL};
static STRPTR refresh_modes[] = {"Low mem", "Smooth", NULL};

static void setup_gadget(struct Prefs *p, struct NewGadget *ng,
                         int x, int y, int w, int h, UWORD id, STRPTR name)
{
    memset(ng, 0, sizeof(*ng));
    ng->ng_LeftEdge = nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge = nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width = nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height = nd_prefs_layout_h(&p->layout,h);
    ng->ng_GadgetText = name;
    ng->ng_TextAttr = p->screen->Font;
    ng->ng_GadgetID = id;
    ng->ng_Flags = PLACETEXT_LEFT;
    ng->ng_VisualInfo = p->vi;
}
static struct Gadget *create_button(struct Prefs *p, int x, int y,
                                    int w, UWORD id, STRPTR title)
{
    struct NewGadget ng;
    setup_gadget(p, &ng, x, y, w, 16, id, title);
    ng.ng_Width=nd_prefs_layout_button_width(&p->layout,w,title);
    ng.ng_Flags = PLACETEXT_IN;
    p->last = CreateGadget(BUTTON_KIND, p->last, &ng,
                           GA_Disabled,(ULONG)(title[0]=='>'),TAG_DONE);
    return p->last;
}
static void make_rows(struct Prefs *p)
{
    int i;
    NewList(&p->rows);
    for (i = 0; i < p->cfg->count; ++i) {
        struct NDItem *item = &p->cfg->items[i];
        if (item->kind == ND_ITEM_PLUGIN)
            sprintf(p->names[i], "%c %s  [%s] [%s]",
                    p->selected == i ? '>' : ' ', item->label,
                    item->enabled ? "ON" : "OFF", item->graph ? "GRAPH" : "METER");
        else if (item->kind==ND_ITEM_SPACER)
            sprintf(p->names[i],"%c --- %s --- [Spacer]",
                    p->selected==i ? '>' : ' ',item->label);
        else sprintf(p->names[i], "%c %s  [%s]",
                     p->selected == i ? '>' : ' ', item->label,
                     item->kind == ND_ITEM_DRAWER ? "Drawer" :
                     item->kind == ND_ITEM_GROUP ? "Group" : "Launcher");
        memset(&p->nodes[i], 0, sizeof(p->nodes[i]));
        p->nodes[i].ln_Name = p->names[i];
        AddTail(&p->rows, &p->nodes[i]);
    }
}
static void refresh_rows(struct Prefs *p)
{
    if (!p->list_gad || !p->win) return;
    GT_SetGadgetAttrs(p->list_gad, p->win, NULL,
                      GTLV_Labels, ~0UL, TAG_DONE);
    make_rows(p);
    GT_SetGadgetAttrs(p->list_gad, p->win, NULL,
                      GTLV_Labels, (ULONG)&p->rows,
                      GTLV_Selected, (ULONG)p->selected, TAG_DONE);
}
static void title_message(struct Prefs *p, const char *status)
{
    if (p->win) SetWindowTitles(p->win, (STRPTR)status, (STRPTR)-1);
}
static void lifecycle_message(struct Prefs *p,const char *status)
{
    char title[128];
    sprintf(title,"%s%s",status,ND_PREFS_IS_DIRTY() ? " *" : "");
    title_message(p,title);
}
static int dock_is_running(void)
{
    int running;
    Forbid();
    running=FindPort(ND_CONTROL_PORT)!=NULL;
    Permit();
    return running;
}
static int dock_state_matches(int running)
{
    int control,plugins;
    Forbid();
    control=FindPort(ND_CONTROL_PORT)!=NULL;
    plugins=FindPort(ND_PORT_NAME)!=NULL;
    Permit();
    return running ? (control && plugins) : (!control && !plugins);
}
static void refresh_dock_controls(struct Prefs *p)
{
    int running;
    if (!p || !p->win) return;
    running=dock_is_running();
    if (p->g_start_dock)
        GT_SetGadgetAttrs(p->g_start_dock,p->win,NULL,
                          GA_Disabled,(ULONG)running,TAG_DONE);
    if (p->g_restart_dock)
        GT_SetGadgetAttrs(p->g_restart_dock,p->win,NULL,
                          GA_Disabled,(ULONG)!running,TAG_DONE);
    if (p->g_quit_dock)
        GT_SetGadgetAttrs(p->g_quit_dock,p->win,NULL,
                          GA_Disabled,(ULONG)!running,TAG_DONE);
}
static int wait_for_dock(int running,int ticks)
{
    while (ticks>0) {
        if (dock_state_matches(running)) return 1;
        Delay(2);ticks-=2;
    }
    return dock_state_matches(running);
}
static void start_dock(struct Prefs *p)
{
    if (dock_is_running()) {
        lifecycle_message(p,"NovaDock Preferences - dock is already running");
    } else if (!nd_control_launch_dock()) {
        lifecycle_message(p,"NovaDock Preferences - cannot find or start NovaDock");
        DisplayBeep(p->screen);
    } else if (!wait_for_dock(1,250)) {
        lifecycle_message(p,"NovaDock Preferences - dock did not register");
        DisplayBeep(p->screen);
    } else lifecycle_message(p,"NovaDock Preferences - dock started");
    refresh_dock_controls(p);
}
static void restart_dock(struct Prefs *p)
{
    LONG rc;
    if (!dock_is_running()) {start_dock(p);return;}
    rc=nd_control_send(ND_CONTROL_QUIT);
    if (rc!=ND_CONTROL_OK || !wait_for_dock(0,250)) {
        lifecycle_message(p,rc==ND_CONTROL_TIMEOUT ?
            "NovaDock Preferences - dock control timed out" :
            "NovaDock Preferences - dock did not stop cleanly");
        DisplayBeep(p->screen);refresh_dock_controls(p);return;
    }
    if (!nd_control_launch_dock()) {
        lifecycle_message(p,"NovaDock Preferences - dock stopped; restart failed");
        DisplayBeep(p->screen);refresh_dock_controls(p);return;
    }
    if (!wait_for_dock(1,250)) {
        lifecycle_message(p,"NovaDock Preferences - replacement did not register");
        DisplayBeep(p->screen);
    } else lifecycle_message(p,"NovaDock Preferences - dock restarted");
    refresh_dock_controls(p);
}
/* Only for checking the Dock hot key text; opened briefly. */
struct Library *CxBase=NULL;

/* The Dock hot key field: "" for none, otherwise a Commodities description
 * such as "ctrl alt d", checked with ParseIX() when commodities.library is
 * available.  0 (with a message) when the text is not a valid hot key. */
static int collect_hotkey(struct Prefs *p)
{
    STRPTR text=NULL;
    IX ix;
    int valid=1;
    if (!p->g_hotkey || !p->win) return 1;
    GT_GetGadgetAttrs(p->g_hotkey,p->win,NULL,GTST_String,(ULONG)&text,TAG_DONE);
    if (!text) return 1;
    if (strlen((char *)text)>=sizeof(p->cfg->dock_hotkey)) valid=0;
    else if (text[0] && (CxBase=OpenLibrary((STRPTR)"commodities.library",37))!=NULL) {
        memset(&ix,0,sizeof(ix));
        ix.ix_Version=IX_VERSION;
        valid=ParseIX(text,&ix)==0;
        CloseLibrary(CxBase);CxBase=NULL;
    }
    if (!valid) {
        title_message(p,"NovaDock Prefs - dock hot key not recognised");
        DisplayBeep(p->screen);
        return 0;
    }
    if (strcmp(p->cfg->dock_hotkey,(char *)text)) {
        strcpy(p->cfg->dock_hotkey,(char *)text);
        ND_PREFS_MARK_DIRTY();
    }
    return 1;
}

static int collect_numbers(struct Prefs *p)
{
    LONG delay;
    if (!nd_numeric_gadget_read(p->g_delay,p->win,&delay)) return 0;
    if (delay<1 || delay>60) {
        title_message(p, "NovaDock Prefs - invalid hide delay");
        DisplayBeep(p->screen);
        return 0;
    }
    p->cfg->hide_delay=(int)delay;
    return 1;
}
static void navigate_to_page(struct Prefs *p,int page)
{
    /* Page navigation must never depend on an unrelated editor field.  Keep
     * a valid delay edit, but still open the requested section when the
     * delay field is temporarily empty or invalid. */
    if (collect_numbers(p)) ND_PREFS_MARK_DIRTY();
    (void)collect_hotkey(p);
    p->next_page=page;
    p->done=1;
    p->exit_status=0;
}
static void create_settings_dir(const char *path)
{
    BPTR lock = CreateDir((STRPTR)path);
    if (lock) UnLock(lock);  /* existing directories are fine */
}
static int apply_preferences(struct Prefs *p, int permanent)
{
    LONG rc;
    struct NDLayout layout;
    int fits;
    if (!collect_numbers(p) || !collect_hotkey(p)) return 0;
    fits = nd_layout_fit_enabled(p->cfg, p->screen->Width, p->screen->Height,
                                 p->screen->BarHeight, &layout);
    if (!fits) {
        title_message(p, "NovaDock Prefs - dock does not fit screen");
        DisplayBeep(p->screen);
        return 0;
    }
    create_settings_dir("ENV:NovaDock");
    if (permanent) {
        create_settings_dir("ENVARC:NovaDock");
        if (!nd_control_write_config(p->cfg, "ENVARC:NovaDock/novadock.cfg")) {
            title_message(p, "NovaDock Prefs - cannot write ENVARC:");
            DisplayBeep(p->screen);
            return 0;
        }
    }
    if (!nd_control_write_config(p->cfg, "ENV:NovaDock/novadock.cfg")) {
        title_message(p, "NovaDock Prefs - cannot write ENV:");
        DisplayBeep(p->screen);
        return 0;
    }
    /* No dock running is not an error: next startup loads these settings. */
    rc = nd_control_reload();
    if (rc != ND_CONTROL_OK && rc != ND_CONTROL_NOHOST &&
        rc != ND_CONTROL_PENDING) {
        title_message(p,rc==ND_CONTROL_TIMEOUT ?
            "NovaDock Prefs - saved; dock reload timed out" :
            "NovaDock Prefs - saved; dock rejected live reload");
        DisplayBeep(p->screen);
        return 0;
    }
    ND_PREFS_MARK_CLEAN();
    ND_PREFS_CLEAR_PREVIEW();
    p->done = 1;
    p->exit_status = 0;
    return 1;
}
static void quit_dock(struct Prefs *p)
{
    LONG rc=nd_control_send(ND_CONTROL_QUIT);
    if (rc==ND_CONTROL_OK) {
        if (wait_for_dock(0,250))
            lifecycle_message(p,"NovaDock Preferences - dock stopped");
        else {
            lifecycle_message(p,"NovaDock Preferences - dock cleanup timed out");
            DisplayBeep(p->screen);
        }
    }
    else if (rc==ND_CONTROL_NOHOST)
        lifecycle_message(p,"NovaDock Preferences - dock is not running");
    else if (rc==ND_CONTROL_TIMEOUT) {
        lifecycle_message(p,"NovaDock Preferences - dock control timed out");
        DisplayBeep(p->screen);
    }
    else {
        lifecycle_message(p,"NovaDock Preferences - dock did not accept quit");
        DisplayBeep(p->screen);
    }
    refresh_dock_controls(p);
}
static const char *general_title(void)
{
    static char title[96];
    if (nd_prefs_discovery.added) {
        sprintf(title,"NovaDock Preferences - %d new plugin%s in Organize%s",
                nd_prefs_discovery.added,nd_prefs_discovery.added==1 ? "" : "s",
                ND_PREFS_IS_DIRTY() ? " *" : "");
        return title;
    }
    return ND_PREFS_IS_DIRTY() ? "NovaDock Preferences - General *" :
                                 "NovaDock Preferences - General";
}
static int open_editor(struct Prefs *p)
{
    struct NewGadget ng;
    char delay_text[8];
    int left,top,width,height,font_x=8,font_y=8;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,
        p->screen->Width,p->screen->Height,p->screen->BarHeight,620,208);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    left=nd_prefs_layout_left(&p->layout);
    top=nd_prefs_layout_top(&p->layout);
    p->vi = GetVisualInfo(p->screen, TAG_DONE);
    if (!p->vi || !CreateContext(&p->gadgets)) return 0;
    p->last = p->gadgets;
    sprintf(delay_text,"%d",p->cfg->hide_delay);
    if (!create_button(p,10,21,118,G_GENERAL,"> General") ||
        !create_button(p,10,45,118,G_APPEARANCE,"Appearance") ||
        !create_button(p,10,69,118,G_LAUNCHERS,"Organize") ||
        !create_button(p,10,93,118,G_PLUGIN_MANAGER,"Plugins") ||
        !create_button(p,10,117,118,G_DIAGNOSTICS,"Diagnostics") ||
        !create_button(p,10,141,118,G_ABOUT,"About")) return 0;
    /* Visual placement, font and labels live with their theme in Appearance. */
    /* Keep a fixed gutter between the sidebar and GadTools' left-hand
     * captions.  In particular, the longer Keep in front label must never
     * extend beneath the navigation buttons. */
    setup_gadget(p, &ng, 270, 21, 100, 16, G_LEFTCLICK, "Click action:");
    p->last = CreateGadget(CYCLE_KIND, p->last, &ng,
                           GTCY_Labels, (ULONG)clicks,
                           GTCY_Active, p->cfg->left_click, TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p, &ng, 520, 21, 80, 16, G_TOOLTIPS, "Tooltips:");
    p->last = CreateGadget(CYCLE_KIND, p->last, &ng,
                           GTCY_Labels, (ULONG)tooltip_modes,
                           GTCY_Active, p->cfg->tooltips ? 1 : 0, TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,270,49,100,16,G_AUTOHIDE,"Auto hide:");
    p->g_auto_hide=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)hide_modes,GTCY_Active,p->cfg->auto_hide,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,550,49,50,16,G_HIDE_DELAY,"Hide delay:");
    p->g_delay=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)delay_text,GTST_MaxChars,2,
        GA_TabCycle,TRUE,GA_Disabled,(ULONG)!p->cfg->auto_hide,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,270,77,100,16,G_ALWAYS_ON_TOP,"Keep in front:");
    p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)hide_modes,
        GTCY_Active,p->cfg->always_on_top ? 1 : 0,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,520,77,80,16,G_REFRESH,"Refresh:");
    p->g_refresh=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)refresh_modes,
        GTCY_Active,p->cfg->refresh_mode,TAG_DONE);
    if (!p->last) return 0;
    /* Lifecycle actions have their own row; commit actions remain separate. */
    p->g_start_dock=create_button(p,150,109,70,G_START_DOCK,"Start");
    p->g_restart_dock=create_button(p,228,109,80,G_RESTART_DOCK,"Restart");
    p->g_quit_dock=create_button(p,316,109,70,G_QUIT_DOCK,"Quit");
    if (!p->g_start_dock || !p->g_restart_dock || !p->g_quit_dock ||
        !create_button(p,500,109,100,G_LAYOUTS,"Layouts...")) return 0;
    /* Keyboard control: a Commodities hot key such as "ctrl alt d". */
    setup_gadget(p,&ng,270,141,190,16,G_DOCK_HOTKEY,"Dock hot key:");
    p->g_hotkey=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)p->cfg->dock_hotkey,
        GTST_MaxChars,(ULONG)(sizeof(p->cfg->dock_hotkey)-1),
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    /* Removable-media tiles share the commit row's free left-hand space. */
    setup_gadget(p,&ng,230,176,60,16,G_MEDIA,"Media:");
    p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)hide_modes,
        GTCY_Active,p->cfg->removable_media ? 1 : 0,TAG_DONE);
    if (!p->last ||
        !create_button(p, 427, 176, 86, G_USE, "Use") ||
        !create_button(p, 519, 176, 86, G_SAVE, "Save")) return 0;
    p->win = OpenWindowTags(NULL,
                  WA_PubScreen, (ULONG)p->screen,
                  WA_Left, left, WA_Top, top,
                  WA_Width, width, WA_Height, height,
                  WA_Title, (ULONG)general_title(),
                  WA_Gadgets, (ULONG)p->gadgets,
                  WA_DragBar, TRUE, WA_CloseGadget, TRUE,
                  WA_Activate, TRUE, WA_DepthGadget, TRUE,
                  WA_IDCMP, IDCMP_GADGETUP | IDCMP_CLOSEWINDOW |
                            IDCMP_REFRESHWINDOW | IDCMP_ACTIVEWINDOW | IDCMP_RAWKEY,
                  WA_Flags, WFLG_SMART_REFRESH | WFLG_ACTIVATE,
                  TAG_DONE);
    if (!p->win) return 0;
    GT_RefreshWindow(p->win, NULL);
    refresh_dock_controls(p);
    return 1;
}
static void on_gadget(struct Prefs *p, UWORD id, UWORD code)
{
    struct NDItem tmp;
    switch (id) {
    case G_GENERAL: break;
    case G_LEFTCLICK: if (code <= ND_CLICK_MENU) {p->cfg->left_click = code;ND_PREFS_MARK_DIRTY();} break;
    case G_TOOLTIPS: if (code <= 1) {p->cfg->tooltips = code;ND_PREFS_MARK_DIRTY();} break;
    case G_ALWAYS_ON_TOP:
        if (code <= 1) {p->cfg->always_on_top = code;ND_PREFS_MARK_DIRTY();}
        break;
    case G_MEDIA:
        if (code<=1) {p->cfg->removable_media=(int)code;ND_PREFS_MARK_DIRTY();}
        break;
    case G_REFRESH:
        if (code<ND_REFRESH_COUNT) {
            p->cfg->refresh_mode=code;ND_PREFS_MARK_DIRTY();
        }
        break;
    case G_AUTOHIDE:
        if (code <= 1) {
            p->cfg->auto_hide=code;
            GT_SetGadgetAttrs(p->g_delay,p->win,NULL,
                              GA_Disabled,(ULONG)!code,TAG_DONE);
            ND_PREFS_MARK_DIRTY();
        }
        break;
    case G_HIDE_DELAY:
        if (collect_numbers(p)) ND_PREFS_MARK_DIRTY();
        break;
    case G_DOCK_HOTKEY:
        (void)collect_hotkey(p);
        break;
    case G_LAYOUTS:
        if (nd_layout_prefs_run(p->cfg,p->screen))
            lifecycle_message(p,"NovaDock Preferences - layout loaded; choose Use or Save");
        break;
    case G_START_DOCK: start_dock(p);break;
    case G_RESTART_DOCK: restart_dock(p);break;
    case G_QUIT_DOCK: quit_dock(p); break;
    case G_ITEMS:
        if (code < p->cfg->count) { p->selected = code; refresh_rows(p); }
        break;
    case G_ENABLE:
        if (p->cfg->items[p->selected].kind == ND_ITEM_PLUGIN) {
            p->cfg->items[p->selected].enabled = !p->cfg->items[p->selected].enabled;
            refresh_rows(p);
        }
        break;
    case G_GRAPH:
        if (p->cfg->items[p->selected].kind == ND_ITEM_PLUGIN) {
            p->cfg->items[p->selected].graph = !p->cfg->items[p->selected].graph;
            refresh_rows(p);
        }
        break;
    case G_UP:
        if (p->selected > 0) {
            tmp = p->cfg->items[p->selected - 1];
            p->cfg->items[p->selected - 1] = p->cfg->items[p->selected];
            p->cfg->items[p->selected] = tmp;
            --p->selected;
            refresh_rows(p);
        }
        break;
    case G_DOWN:
        if (p->selected + 1 < p->cfg->count) {
            tmp = p->cfg->items[p->selected + 1];
            p->cfg->items[p->selected + 1] = p->cfg->items[p->selected];
            p->cfg->items[p->selected] = tmp;
            ++p->selected;
            refresh_rows(p);
        }
        break;
    case G_LAUNCHERS:
        /* Switch pages inside the unified NovaDockPrefs executable. */
        navigate_to_page(p,1);
        break;
    case G_APPEARANCE:
        navigate_to_page(p,3);
        break;
    case G_PLUGIN_MANAGER:
        navigate_to_page(p,9);
        break;
    case G_DIAGNOSTICS:
        navigate_to_page(p,6);break;
    case G_ABOUT:
        navigate_to_page(p,8);break;
    case G_USE: apply_preferences(p, 0); break;
    case G_SAVE: apply_preferences(p, 1); break;
    default: break;
    }
}
static void editor_loop(struct Prefs *p)
{
    struct IntuiMessage *msg;
    ULONG cls;
    UWORD code, id;
    while (!p->done) {
        Wait(1UL << p->win->UserPort->mp_SigBit);
        while ((msg = GT_GetIMsg(p->win->UserPort)) != NULL) {
            cls = msg->Class;
            code = msg->Code;
            id = msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(msg);
            if (cls == IDCMP_CLOSEWINDOW) { p->done = 1; p->exit_status = 0; }
            else if (cls==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (cls == IDCMP_REFRESHWINDOW) GT_BeginRefresh(p->win);
            else if (cls == IDCMP_ACTIVEWINDOW) refresh_dock_controls(p);
            /* An EMPTY refresh is correct: all UI belongs to GadTools. */
            else if (cls == IDCMP_GADGETUP) on_gadget(p, id, code);
            if (cls == IDCMP_REFRESHWINDOW) GT_EndRefresh(p->win, TRUE);
        }
    }
}
static int command_page(int argc, char **argv, char *plugin, int plugin_size)
{
    int i, page=0;
    if (plugin_size>0) plugin[0]=0;
    for (i=1;i<argc;++i) {
        if (!strcmp(argv[i],"PAGE=ITEMS")) page=1;
        else if (!strcmp(argv[i],"PAGE=LAUNCHERS") ||
                 !strcmp(argv[i],"PAGE=DEFINED")) page=2;
        else if (!strcmp(argv[i],"PAGE=PLUGIN")) page=4;
        else if (!strcmp(argv[i],"PAGE=SPACER")) page=5;
        else if (!strcmp(argv[i],"PAGE=APPEARANCE")) page=3;
        else if (!strcmp(argv[i],"PAGE=DIAGNOSTICS")) page=6;
        else if (!strcmp(argv[i],"PAGE=ADD")) page=7;
        else if (!strcmp(argv[i],"PAGE=ABOUT")) page=8;
        else if (!strcmp(argv[i],"PAGE=PLUGINMANAGER") ||
                 !strcmp(argv[i],"PAGE=PLUGINS")) page=9;
        else if (!strncmp(argv[i],"PLUGIN=",7) && plugin_size>0) {
            strncpy(plugin,argv[i]+7,plugin_size-1);
            plugin[plugin_size-1]=0;
        }
    }
    return page;
}

static int run_child_pages(int page,int argc,char **argv,char *plugin)
{
    int result,defined_index=-1,return_from_add=0,ran_page;
    int add_return_page=1;
    char select_arg[32],plugin_arg[32];
    char *plugin_argv[3],*launcher_argv[3],*spacer_argv[3],*add_argv[3];
    while (page) {
        ran_page=page;
        if (page==1) {
            result=nd_dockitems_prefs_run(argc,argv,plugin,20,&defined_index);
        } else if (page==2) {
            launcher_argv[0]=argv[0];
            if (defined_index>=0) {
                sprintf(select_arg,"SELECTFULL=%d",defined_index);
                launcher_argv[1]=select_arg;
                launcher_argv[2]=(char *)"FOCUSED=1";
                result=nd_launcher_prefs_run(3,launcher_argv);
            } else result=nd_launcher_prefs_run(argc,argv);
            defined_index=-1;
        } else if (page==4) {
            if (!plugin[0]) strcpy(plugin,"CLOCK");
            plugin_argv[0]=argv[0];plugin_argv[1]=plugin;
            plugin_argv[2]=(char *)"FOCUSED=1";
            result=nd_plugin_prefs_run(3,plugin_argv);
        } else if (page==5) {
            spacer_argv[0]=argv[0];
            if (defined_index>=0) {
                sprintf(select_arg,"SELECTFULL=%d",defined_index);
                spacer_argv[1]=select_arg;
                spacer_argv[2]=(char *)"FOCUSED=1";
                result=nd_spacer_prefs_run(3,spacer_argv);
            } else result=nd_spacer_prefs_run(argc,argv);
            defined_index=-1;
        } else if (page==3) result=nd_appearance_prefs_run(argc,argv);
        else if (page==6) result=nd_diagnostics_prefs_run(argc,argv);
        else if (page==7) {
            if (add_return_page==9 && plugin[0]) {
                /* From the Plugin Manager: open Add Item on that plugin. */
                sprintf(plugin_arg,"PLUGIN=%.19s",plugin);
                add_argv[0]=argv[0];add_argv[1]=plugin_arg;
                add_argv[2]=(char *)"FOCUSED=1";
                result=nd_additem_prefs_run(3,add_argv);
            } else result=nd_additem_prefs_run(argc,argv);
        }
        else if (page==8) result=nd_about_prefs_run(argc,argv);
        else if (page==9)
            result=nd_pluginmanager_prefs_run(argc,argv,plugin,20);
        else return 20;
        if (ran_page==7 && return_from_add && result==0) {
            return_from_add=0;
            page=add_return_page;
            add_return_page=1;
            continue;
        }
        if (result==ND_PREF_NAV_GENERAL) return result;
        if (result==ND_PREF_NAV_APPEARANCE) page=3;
        else if (result==ND_PREF_NAV_ORGANIZE) page=1;
        else if (result==ND_PREF_NAV_LAUNCHER) page=2;
        else if (result==ND_PREF_NAV_DIAGNOSTICS) page=6;
        else if (result==ND_PREF_NAV_ABOUT) page=8;
        else if (result==ND_PREF_NAV_PLUGIN_MANAGER) page=9;
        else if (result==ND_PREF_NAV_PLUGINS) {
            page=4;
        } else if (result==ND_PREF_NAV_SPACER) {
            page=5;
        } else if (result==ND_PREF_NAV_ADD_ITEM) {
            return_from_add=1;
            add_return_page=ran_page==9 ? 9 : 1;
            page=7;
        } else return result;
    }
    return 20;
}

static int run_general_page(int argc,char **argv,struct NDConfig *session,
                            int *next_page,char *next_plugin,int plugin_size)
{
    struct Prefs *p;
    int result=20;
    p = (struct Prefs *)AllocVec(sizeof(*p), MEMF_PUBLIC | MEMF_CLEAR);
    if (!p) return 20;
    p->requested_visible=-1;
    { int a;
      for (a=1;a<argc;++a) if (!strncmp(argv[a],"SELECT=",7)) {
          char tail=0;int n=-1;
          if (sscanf(argv[a]+7,"%d%c",&n,&tail)==1 && n>=0 && n<ND_MAX_ITEMS)
              p->requested_visible=n;
      }
    }
    p->cfg=session;
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    if (!IntuitionBase) goto finish;
    GadToolsBase = OpenLibrary("gadtools.library", 39);
    if (!GadToolsBase) goto finish;
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39);
    if (!GfxBase) goto finish;
    if (p->requested_visible>=0) {
        int full=nd_visible_to_full(p->cfg,p->requested_visible);
        if (full>=0) p->selected=full;
    }
    p->screen = LockPubScreen("Workbench");
    if (!p->screen) goto finish;
    if (!open_editor(p)) {
        Printf("NovaDockPrefs requires a Workbench screen >=630x195.\n");
        goto finish;
    }
    editor_loop(p);
    result = p->exit_status;
finish:
    if (next_page) *next_page=p->next_page;
    if (next_plugin && plugin_size>0) {
        strncpy(next_plugin,p->next_plugin,plugin_size-1);
        next_plugin[plugin_size-1]=0;
    }
    if (p->win) CloseWindow(p->win);
    if (p->gadgets) FreeGadgets(p->gadgets);
    if (p->vi) FreeVisualInfo(p->vi);
    if (p->screen) UnlockPubScreen(NULL, p->screen);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    FreeVec(p);
    GfxBase=NULL;GadToolsBase=NULL;IntuitionBase=NULL;
    return result;
}

/* vbcc's startup code grows the stack to this size when launched with less.
 * GadTools, ASL and the plugin catalogue scan all run on the main task. */
long __stack=32768;

/* Leaving Preferences after Test restores the settings it started with.
 * They are kept as text in T: (a few KB of RAM) rather than as a second
 * 92 KB configuration; only if that file cannot be written is the old
 * in-memory copy used. */
#define ND_REVERT_PATH "T:NovaDockPrefs.revert"
static void revert_files(int remove_them,int *written,const struct NDConfig *cfg)
{
    static const char *suffixes[]={"",".bak",".old",".new",NULL};
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR requesters=self->pr_WindowPtr;
    char path[64];
    int i;
    self->pr_WindowPtr=(APTR)-1L; /* T: may not be assigned */
    if (remove_them) {
        for (i=0;suffixes[i];++i) {
            sprintf(path,"%s%s",ND_REVERT_PATH,suffixes[i]);
            (void)DeleteFile((STRPTR)path);
        }
    } else *written=nd_control_write_config(cfg,ND_REVERT_PATH);
    self->pr_WindowPtr=requesters;
}

int main(int argc, char **argv)
{
    struct NDConfig *session,*original=NULL;
    int result=20,parsing,page,snapshot=0;
    char plugin[20];
    session=(struct NDConfig *)AllocVec(sizeof(*session),MEMF_PUBLIC|MEMF_CLEAR);
    if (!session) return 20;
    (void)nd_control_sync_settings();
    parsing=nd_load_effective(session);
    if (parsing>0) {
        Printf("NovaDockPrefs: configuration has %ld invalid line(s).\n",(LONG)parsing);
        FreeVec(session);return 20;
    }
    /* Self-describing plugins in the Plugins drawer join Organize's
     * Available list (disabled) for this session; nothing is written until
     * the user adds one and chooses Use or Save. */
    (void)nd_discover_plugins(session,&nd_prefs_discovery);
    revert_files(0,&snapshot,session);
    if (!snapshot) {
        original=(struct NDConfig *)AllocVec(sizeof(*original),MEMF_PUBLIC);
        if (!original) {FreeVec(session);return 20;}
        *original=*session;
    }
    nd_prefs_shared_config=session;
    nd_prefs_shared_original=original;
    nd_prefs_shared_dirty=0;
    nd_prefs_preview_active=0;
    page=command_page(argc,argv,plugin,sizeof(plugin));
    for (;;) {
        if (page) {
            result=run_child_pages(page,argc,argv,plugin);
            if (result==ND_PREF_NAV_GENERAL) {page=0;continue;}
            break;
        }
        page=0;
        result=run_general_page(argc,argv,session,&page,plugin,sizeof(plugin));
        if (page) continue;
        break;
    }
    if (nd_prefs_preview_active) {
        /* The session is finished with, so its buffer can hold the
         * restored settings. */
        const struct NDConfig *restore=original;
        if (!restore && nd_load_config(session,ND_REVERT_PATH)==0)
            restore=session;
        create_settings_dir("ENV:NovaDock");
        if (restore && nd_control_write_config(restore,"ENV:NovaDock/novadock.cfg"))
            nd_control_reload();
    }
    if (snapshot) revert_files(1,&snapshot,NULL);
    nd_prefs_shared_config=NULL;nd_prefs_shared_original=NULL;
    nd_prefs_preview_active=0;
    if (original) FreeVec(original);
    FreeVec(session);
    return result;
}
