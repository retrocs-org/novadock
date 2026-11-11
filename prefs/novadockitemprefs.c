/* Active/inactive Organize page for NovaDock Preferences 0.1.
 * Plugins, launchers, drawers and spacers share one ordered dock model.
 */
#define IntuitionBase NDItemsIntuitionBase
#define GadToolsBase NDItemsGadToolsBase
#define GfxBase NDItemsGfxBase
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_control_client.h"
#include "nd_layout.h"
#include "nd_prefs_nav.h"
#include "nd_prefs_session.h"
#include "nd_numeric_gadget.h"
#include "nd_prefs_layout.h"
#include "nd_plugin_discovery.h"
#include "nd_options.h"
#include "nd_prefs_help.h"

#define G_ACTIVE 11
#define G_REMOVE 13
#define G_SETTINGS 14
#define G_ADD 15
#define G_UP 16
#define G_DOWN 17
#define G_USE 18
#define G_SAVE 19
#define G_NAV_GENERAL 21
#define G_NAV_APPEARANCE 22
#define G_NAV_ITEMS 23
#define G_NAV_DIAGNOSTICS 25
#define G_TEST 26
#define G_ROWS 27
#define G_NAV_ABOUT 28
#define G_NAV_PLUGIN_MANAGER 29
#define ACTIVE_DISPLAY_MAX (ND_MAX_ITEMS+ND_ROWS_MAX)

struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct GfxBase *GfxBase=NULL;

struct ItemPage {
    struct NDConfig *cfg;
    struct Screen *screen;
    struct Window *win;
    APTR vi;
    struct DrawInfo *draw_info;
    struct Gadget *gadgets,*last,*active_gad,*rows_gad;
    struct NDPrefsLayout layout;
    struct List active_rows;
    struct Node active_nodes[ACTIVE_DISPLAY_MAX];
    char active_names[ACTIVE_DISPLAY_MAX][64];
    int active_map[ND_MAX_ITEMS];
    int active_display_map[ACTIVE_DISPLAY_MAX];
    int active_item_display[ND_MAX_ITEMS];
    int active_grid_row[ND_MAX_ITEMS],active_grid_column[ND_MAX_ITEMS];
    int grid_rows,grid_columns;
    int active_count,active_display_count;
    int active_selected;
    int selected_side; /* Organize always operates on an item in the dock. */
    int done,result,defined_index;
    char plugin[20];
};
static STRPTR row_labels[]={"1","2","3",NULL};

static void ng_init(struct ItemPage *p,struct NewGadget *ng,int x,int y,
                    int w,int h,int id,STRPTR text)
{
    memset(ng,0,sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&p->layout,h);
    ng->ng_GadgetID=id;ng->ng_GadgetText=text;ng->ng_VisualInfo=p->vi;
    ng->ng_TextAttr=p->screen->Font;ng->ng_Flags=PLACETEXT_LEFT;
}
static struct Gadget *button(struct ItemPage *p,int x,int y,int w,int id,STRPTR text)
{
    struct NewGadget ng;
    ng_init(p,&ng,x,y,w,16,id,text);ng.ng_Flags=PLACETEXT_IN;
    ng.ng_Width=nd_prefs_layout_button_width(&p->layout,w,text);
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
                         GA_Disabled,(ULONG)(text[0]=='>'),TAG_DONE);
    return p->last;
}
static int discovery_reported=0; /* discovery note shown once per session */
static const char *kind_name(const struct NDItem *it)
{
    if (it->kind==ND_ITEM_PLUGIN) {
        if (nd_command_plugin_path(it->command)) return "Command Tile";
        if (nd_script_plugin_path(it->command)) return "Script Tile";
        return nd_discovery_is_new(it->id) ? "New Plugin" : "Plugin";
    }
    if (it->kind==ND_ITEM_DRAWER)
        return nd_item_is_volume(it) ? "Volume" : "Drawer";
    if (it->kind==ND_ITEM_GROUP) return "Application Group";
    if (it->kind==ND_ITEM_SPACER) return "Spacer";
    return "Application";
}
static void build_grid_map(struct ItemPage *p)
{
    struct NDLayout layout;
    int i;
    p->grid_rows=1;p->grid_columns=1;
    for (i=0;i<ND_MAX_ITEMS;++i) {
        p->active_grid_row[i]=0;
        p->active_grid_column[i]=i;
    }
    if (!p->screen || p->cfg->side==ND_SIDE_LEFT ||
        p->cfg->side==ND_SIDE_RIGHT) return;
    if (nd_layout_fit_enabled(p->cfg,p->screen->Width,p->screen->Height,
                              p->screen->BarHeight,&layout)) {
        int visible=0;
        p->grid_rows=layout.rows;
        p->grid_columns=layout.columns;
        for (i=0;i<p->cfg->count;++i) if (p->cfg->items[i].enabled) ++visible;
        for (i=0;i<visible;++i)
            nd_layout_item_cell_enabled(p->cfg,i,layout.columns,
                &p->active_grid_row[i],&p->active_grid_column[i],NULL);
    }
}
static void build_rows(struct ItemPage *p)
{
    int i,b=0,d=0,dock_row,display_row,organizer_rows;
    build_grid_map(p);
    NewList(&p->active_rows);
    for (i=0;i<ND_MAX_ITEMS;++i) p->active_item_display[i]=-1;
    /* Present the dock as explicit row groups.  Headers are deliberately
     * non-actions: the item-to-display maps keep selection and editing tied
     * to real dock entries rather than to LISTVIEW line numbers. */
    organizer_rows=(p->cfg->side==ND_SIDE_LEFT || p->cfg->side==ND_SIDE_RIGHT) ?
                   1 : p->cfg->rows;
    for (dock_row=0;dock_row<organizer_rows && dock_row<ND_ROWS_MAX;++dock_row) {
        display_row=d++;
        p->active_display_map[display_row]=-1;
        sprintf(p->active_names[display_row],"  -- Dock Row %d --",dock_row+1);
        memset(&p->active_nodes[display_row],0,sizeof(struct Node));
        p->active_nodes[display_row].ln_Name=p->active_names[display_row];
        AddTail(&p->active_rows,&p->active_nodes[display_row]);
        for (i=0;i<p->cfg->count;++i) {
            struct NDItem *it=&p->cfg->items[i];
            int visible=-1,j,row;
            if (!it->enabled) continue;
            for (j=0;j<=i;++j) if (p->cfg->items[j].enabled) ++visible;
            if (visible<0 || p->active_grid_row[visible]!=dock_row) continue;
            row=b++;p->active_map[row]=i;
            display_row=d++;p->active_display_map[display_row]=row;
            p->active_item_display[row]=display_row;
            if (it->kind==ND_ITEM_SPACER) {
                int n=0;
                for (j=0;j<=i;++j)
                    if (p->cfg->items[j].kind==ND_ITEM_SPACER) ++n;
                sprintf(p->active_names[display_row],"%c  Spacer %d [Space]",
                        p->selected_side==1 && p->active_selected==row ? '>' : ' ',n);
            } else
                sprintf(p->active_names[display_row],"%c  %.11s [%s]",
                        p->selected_side==1 && p->active_selected==row ? '>' : ' ',
                        it->label,kind_name(it));
            memset(&p->active_nodes[display_row],0,sizeof(struct Node));
            p->active_nodes[display_row].ln_Name=p->active_names[display_row];
            AddTail(&p->active_rows,&p->active_nodes[display_row]);
        }
    }
    p->active_count=b;p->active_display_count=d;
    if (p->active_selected>=b) p->active_selected=b-1;
    if (p->active_selected<0 && b) p->active_selected=0;
    /* Do not rely on palette-dependent GadTools highlighting alone. */
    for (i=0;i<d;++i) if (p->active_display_map[i]>=0)
        p->active_names[i][0]=p->selected_side==1 &&
            p->active_selected==p->active_display_map[i] ? '>' : ' ';
}
static int selected_full(struct ItemPage *p);
static void draw_selected_row(struct ItemPage *p)
{
    struct Gadget *gad;
    char *name;
    LONG top=0;
    int selected,row_height,x,y,w;
    struct RastPort *rp;
    if (!p->win || !p->draw_info) return;
    gad=p->active_gad;
    selected=p->active_selected>=0 && p->active_selected<p->active_count ?
             p->active_item_display[p->active_selected] : -1;
    if (selected<0 || selected>=p->active_display_count) return;
    name=p->active_names[selected];
    GT_GetGadgetAttrs(gad,p->win,NULL,GTLV_Top,(ULONG)&top,TAG_DONE);
    row_height=p->screen->RastPort.Font->tf_YSize;
    if (row_height<6) row_height=8;
    if (selected<top || selected>=top+(gad->Height-4)/row_height) return;
    x=gad->LeftEdge+3;
    y=gad->TopEdge+2+(selected-(int)top)*row_height;
    w=gad->Width-22;
    rp=p->win->RPort;
    SetAPen(rp,p->draw_info->dri_Pens[FILLPEN]);
    RectFill(rp,x,y,x+w-1,y+row_height-1);
    SetAPen(rp,p->draw_info->dri_Pens[FILLTEXTPEN]);
    SetDrMd(rp,JAM1);Move(rp,x+2,y+row_height-2);
    Text(rp,(STRPTR)name,strlen(name));
}
static void refresh_rows(struct ItemPage *p)
{
    GT_SetGadgetAttrs(p->active_gad,p->win,NULL,GTLV_Labels,~0UL,TAG_DONE);
    build_rows(p);
    GT_SetGadgetAttrs(p->active_gad,p->win,NULL,
        GTLV_Labels,(ULONG)&p->active_rows,
        GTLV_Selected,(ULONG)(p->active_selected<0 ? ~0UL:
            p->active_item_display[p->active_selected]),TAG_DONE);
    GT_SetGadgetAttrs(p->rows_gad,p->win,NULL,
        GTCY_Active,(ULONG)(p->cfg->rows-1),TAG_DONE);
    draw_selected_row(p);
}
static void status(struct ItemPage *p,const char *text)
{
    SetWindowTitles(p->win,(STRPTR)text,(STRPTR)-1);
}
static void make_dir(const char *path)
{
    BPTR lock=CreateDir((STRPTR)path);if (lock) UnLock(lock);
}
static int apply_dockbar(struct ItemPage *p,int permanent,int finish)
{
    struct NDLayout layout;
    LONG rc;
    /* An empty dock has no enabled items, so it never fits either. */
    if (!nd_layout_fit_enabled(p->cfg,p->screen->Width,p->screen->Height,
                               p->screen->BarHeight,&layout)) {
        status(p,"NovaDock Organize - active items do not fit");
        DisplayBeep(p->screen);return 0;
    }
    make_dir("ENV:NovaDock");
    if (permanent) {
        make_dir("ENVARC:NovaDock");
        if (!nd_control_write_config(p->cfg,"ENVARC:NovaDock/novadock.cfg")) return 0;
    }
    if (!nd_control_write_config(p->cfg,"ENV:NovaDock/novadock.cfg")) return 0;
    rc=nd_control_reload();
    if (rc!=ND_CONTROL_OK && rc!=ND_CONTROL_NOHOST &&
        rc!=ND_CONTROL_PENDING) {
        status(p,rc==ND_CONTROL_TIMEOUT ?
            "Saved, but NovaDock reload timed out" :
            "Saved, but NovaDock rejected reload");return 0;
    }
    if (finish) {
        ND_PREFS_MARK_CLEAN();ND_PREFS_CLEAR_PREVIEW();
        p->done=1;p->result=0;
    } else {
        ND_PREFS_MARK_PREVIEW();
        status(p,"NovaDock Preferences - Organize (test active)");
    }
    return 1;
}
static int active_full(struct ItemPage *p)
{
    return p->active_selected>=0 && p->active_selected<p->active_count ?
           p->active_map[p->active_selected] : -1;
}
static int selected_full(struct ItemPage *p)
{
    return active_full(p);
}
static void remove_selected(struct ItemPage *p)
{
    int full=active_full(p);
    if (full<0) return;
    nd_set_item_enabled(p->cfg,full,0);ND_PREFS_MARK_DIRTY();
    refresh_rows(p);
}
static void capture_grid_distribution(struct ItemPage *p)
{
    int i,row;
    if (!p || p->cfg->row_layout) return;
    memset(p->cfg->row_counts,0,sizeof(p->cfg->row_counts));
    for (i=0;i<p->active_count;++i) {
        row=p->active_grid_row[i];
        if (row<0) row=0;
        if (row>=p->cfg->rows) row=p->cfg->rows-1;
        if (row>=0 && row<ND_ROWS_MAX) ++p->cfg->row_counts[row];
    }
    p->cfg->row_layout=1;
}
static void move_selected(struct ItemPage *p,int direction)
{
    int current,adjacent,target;
    if (p->active_selected<0 || p->active_selected>=p->active_count) return;
    if (direction!=1 && direction!=-1) return;
    current=p->active_grid_row[p->active_selected];
    adjacent=p->active_selected+direction;
    /* Inside a row, Up/Down swaps with the adjacent item.  At a row edge,
     * the same buttons transfer the item to the adjacent row while keeping
     * its order: Down makes it the first item below; Up makes it the last
     * item above.  This also permits moving into an intentionally empty row. */
    if (adjacent>=0 && adjacent<p->active_count &&
        p->active_grid_row[adjacent]==current) {
        if (!nd_move_visible(p->cfg,p->active_selected,adjacent)) return;
        p->active_selected=adjacent;
    } else {
        if (p->cfg->side==ND_SIDE_LEFT || p->cfg->side==ND_SIDE_RIGHT) return;
        target=current+direction;
        if (target<0 || target>=p->cfg->rows || target>=ND_ROWS_MAX) return;
        capture_grid_distribution(p);
        if (!nd_move_visible_to_row(p->cfg,p->active_selected,
                                    p->active_selected,target)) {
            status(p,"NovaDock Organize - item could not change row");
            DisplayBeep(p->screen);return;
        }
    }
    ND_PREFS_MARK_DIRTY();refresh_rows(p);
}
static void settings_selected(struct ItemPage *p)
{
    int full=selected_full(p);
    struct NDItem *it;
    if (full<0) return;
    it=&p->cfg->items[full];
    if (it->kind==ND_ITEM_PLUGIN) {
        strncpy(p->plugin,it->id,sizeof(p->plugin)-1);
        p->plugin[sizeof(p->plugin)-1]=0;
        p->done=1;p->result=ND_PREF_NAV_PLUGINS;
    } else if (it->kind==ND_ITEM_LAUNCH || it->kind==ND_ITEM_DRAWER ||
               it->kind==ND_ITEM_GROUP) {
        p->defined_index=full;p->done=1;p->result=ND_PREF_NAV_LAUNCHER;
    } else {
        p->defined_index=full;
        p->done=1;p->result=ND_PREF_NAV_SPACER;
    }
}
static int open_ui(struct ItemPage *p)
{
    struct NewGadget ng;
    int x,y,width,height,font_x=8,font_y=8;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,
        p->screen->Width,p->screen->Height,p->screen->BarHeight,620,219);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    x=nd_prefs_layout_left(&p->layout);y=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfo(p->screen,TAG_DONE);
    p->draw_info=GetScreenDrawInfo(p->screen);
    if (!p->vi || !p->draw_info || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;build_rows(p);
    if (!button(p,10,21,118,G_NAV_GENERAL,"General") ||
        !button(p,10,45,118,G_NAV_APPEARANCE,"Appearance") ||
        !button(p,10,69,118,G_NAV_ITEMS,"> Organize") ||
        !button(p,10,93,118,G_NAV_PLUGIN_MANAGER,"Plugins") ||
        !button(p,10,117,118,G_NAV_DIAGNOSTICS,"Diagnostics") ||
        !button(p,10,141,118,G_NAV_ABOUT,"About")) return 0;
    ng_init(p,&ng,140,30,465,86,G_ACTIVE,"In dock");
    ng.ng_Flags=PLACETEXT_ABOVE;
    p->active_gad=p->last=CreateGadget(LISTVIEW_KIND,p->last,&ng,
        GTLV_Labels,(ULONG)&p->active_rows,
        GTLV_Selected,(ULONG)(p->active_selected<0 ? ~0UL:
            p->active_item_display[p->active_selected]),TAG_DONE);
    if (!p->last) return 0;
    ng_init(p,&ng,290,125,55,16,G_ROWS,"Dock rows:");
    p->rows_gad=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)row_labels,GTCY_Active,p->cfg->rows-1,
        GA_Disabled,(ULONG)(p->cfg->side==ND_SIDE_LEFT ||
                            p->cfg->side==ND_SIDE_RIGHT),TAG_DONE);
    if (!p->last) return 0;
    if (!button(p,365,125,84,G_UP,"Up") ||
        !button(p,461,125,84,G_DOWN,"Down") ||
        !button(p,218,151,92,G_ADD,"Add...") ||
        !button(p,320,151,92,G_SETTINGS,"Edit...") ||
        !button(p,422,151,92,G_REMOVE,"Remove") ||
        !button(p,335,188,80,G_TEST,"Test") ||
        !button(p,427,188,86,G_USE,"Use") ||
        !button(p,519,188,86,G_SAVE,"Save")) return 0;
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,x,WA_Top,y,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)(ND_PREFS_IS_DIRTY() ?
            "NovaDock Preferences - Organize *" :
            "NovaDock Preferences - Organize"),
        WA_Gadgets,(ULONG)p->gadgets,WA_DragBar,TRUE,WA_Activate,TRUE,
        WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
        WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;GT_RefreshWindow(p->win,NULL);
    draw_selected_row(p);
    /* Report discovery once; new plugins are added through Add item rather
     * than through a second inactive-items catalogue on this page. */
    if (!discovery_reported) {
        static char note[120];
        discovery_reported=1;
        if (nd_prefs_discovery.added) {
            sprintf(note,"New plugin%s: %.20s%s - use Add item to add it",
                    nd_prefs_discovery.added==1 ? "" : "s",
                    nd_prefs_discovery.added_names[0],
                    nd_prefs_discovery.added>1 ? ", ..." : "");
            status(p,note);
        } else if (nd_prefs_discovery.problem[0])
            status(p,nd_prefs_discovery.problem);
    }
    return 1;
}
static void on_gadget(struct ItemPage *p,UWORD id,UWORD code)
{
    switch(id) {
    case G_NAV_GENERAL:p->done=1;p->result=ND_PREF_NAV_GENERAL;break;
    case G_NAV_APPEARANCE:p->done=1;p->result=ND_PREF_NAV_APPEARANCE;break;
    case G_NAV_ITEMS:break;
    case G_NAV_PLUGIN_MANAGER:p->done=1;p->result=ND_PREF_NAV_PLUGIN_MANAGER;break;
    case G_NAV_DIAGNOSTICS:p->done=1;p->result=ND_PREF_NAV_DIAGNOSTICS;break;
    case G_NAV_ABOUT:p->done=1;p->result=ND_PREF_NAV_ABOUT;break;
    case G_ACTIVE:
        if (code<p->active_display_count && p->active_display_map[code]>=0) {
            p->active_selected=p->active_display_map[code];p->selected_side=1;
            refresh_rows(p);
        } else refresh_rows(p);
        break;
    case G_REMOVE:remove_selected(p);break;
    case G_SETTINGS:settings_selected(p);break;
    case G_ADD:p->done=1;p->result=ND_PREF_NAV_ADD_ITEM;break;
    case G_UP:move_selected(p,-1);break;
    case G_DOWN:move_selected(p,1);break;
    case G_ROWS:
        {
            int old_rows=p->cfg->rows,new_rows=(old_rows%ND_ROWS_MAX)+1,i;
            if (p->cfg->row_layout && new_rows<old_rows) {
                for (i=new_rows;i<old_rows && i<ND_ROWS_MAX;++i) {
                    p->cfg->row_counts[new_rows-1]+=p->cfg->row_counts[i];
                    p->cfg->row_counts[i]=0;
                }
            } else if (p->cfg->row_layout && new_rows>old_rows) {
                for (i=old_rows;i<new_rows && i<ND_ROWS_MAX;++i)
                    p->cfg->row_counts[i]=0;
            }
            p->cfg->rows=new_rows;ND_PREFS_MARK_DIRTY();refresh_rows(p);
        }
        break;
    case G_TEST:apply_dockbar(p,0,0);break;
    case G_USE:apply_dockbar(p,0,1);break;
    case G_SAVE:apply_dockbar(p,1,1);break;
    default:break;
    }
}
int nd_dockitems_prefs_run(int argc,char **argv,char *plugin,int plugin_size,
                           int *defined_index)
{
    struct ItemPage *p;
    struct IntuiMessage *msg;
    ULONG cls;UWORD id,code;int result=20,parse,requested=-1,a;
    if (plugin && plugin_size>0) plugin[0]=0;
    if (defined_index) *defined_index=-1;
    p=(struct ItemPage *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 20;
    p->active_selected=-1;p->defined_index=-1;
    p->selected_side=1;
    p->cfg=ND_PREFS_SHARED_CONFIG;
    if (!p->cfg)
        p->cfg=(struct NDConfig *)AllocVec(sizeof(*p->cfg),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p->cfg) goto done;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    if (!IntuitionBase || !GfxBase || !GadToolsBase) goto done;
    parse=0;
    if (!ND_PREFS_SHARED_CONFIG) parse=nd_load_effective(p->cfg);
    if (parse>0) goto done;
    for (a=1;a<argc;++a) if (!strncmp(argv[a],"SELECT=",7)) {
        char tail=0;int n=-1;
        if (sscanf(argv[a]+7,"%d%c",&n,&tail)==1 && n>=0 && n<ND_MAX_ITEMS)
            requested=n;
    }
    if (requested>=0) {
        p->active_selected=requested;
        p->selected_side=1;
    }
    p->screen=LockPubScreen("Workbench");if (!p->screen || !open_ui(p)) goto done;
    while(!p->done) {
        Wait(1UL<<p->win->UserPort->mp_SigBit);
        while((msg=GT_GetIMsg(p->win->UserPort))!=NULL) {
            cls=msg->Class;code=msg->Code;
            id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID:0;
            GT_ReplyIMsg(msg);
            if (cls==IDCMP_CLOSEWINDOW) {p->done=1;p->result=0;}
            else if (cls==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);
                draw_selected_row(p);
            } else if (cls==IDCMP_GADGETUP) on_gadget(p,id,code);
        }
    }
    result=p->result;
    if (plugin && plugin_size>0) {
        strncpy(plugin,p->plugin,plugin_size-1);plugin[plugin_size-1]=0;
    }
    if (defined_index) *defined_index=p->defined_index;
done:
    if (p->win) CloseWindow(p->win);
    if (p->gadgets) FreeGadgets(p->gadgets);
    if (p->vi) FreeVisualInfo(p->vi);
    if (p->draw_info) FreeScreenDrawInfo(p->screen,p->draw_info);
    if (p->screen) UnlockPubScreen(NULL,p->screen);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (p->cfg && !ND_PREFS_SHARED_CONFIG) FreeVec(p->cfg);
    FreeVec(p);return result;
}

/* Focused editor opened by a spacer's contextual Edit action. */
#define S_WIDTH 300
#define S_LINE 301
#define S_COLOR 302
#define S_THICKNESS 303
#define S_USE 304
#define S_SAVE 305
#define S_NAV_GENERAL 307
#define S_NAV_APPEARANCE 308
#define S_NAV_ITEMS 309
#define S_NAV_DIAGNOSTICS 311
struct SpacerPage {
    struct NDConfig *cfg;
    int full,done,result,line,color,focused;
    struct Screen *screen;
    struct Window *win;
    APTR vi;
    struct Gadget *gadgets,*last,*width_gad,*thickness_gad;
    struct NDPrefsLayout layout;
    char title[90];
};
static STRPTR spacer_line_labels[]={"Hidden","Visible",NULL};
static STRPTR spacer_color_labels[]={"Subtle","Accent","Secondary","Text",NULL};
static void sp_field(struct SpacerPage *p,struct NewGadget *ng,int x,int y,
                     int w,int id,STRPTR label)
{
    memset(ng,0,sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&p->layout,16);ng->ng_GadgetID=id;
    ng->ng_GadgetText=label;ng->ng_VisualInfo=p->vi;
    ng->ng_TextAttr=p->screen->Font;ng->ng_Flags=PLACETEXT_LEFT;
}
static int sp_button(struct SpacerPage *p,int x,int y,int w,int id,STRPTR text)
{
    struct NewGadget ng;sp_field(p,&ng,x,y,w,id,text);ng.ng_Flags=PLACETEXT_IN;
    ng.ng_Width=nd_prefs_layout_button_width(&p->layout,w,text);
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
                         GA_Disabled,(ULONG)(text[0]=='>'),TAG_DONE);
    return p->last!=NULL;
}
static int sp_stage(struct SpacerPage *p)
{
    LONG width=0,thickness=0;
    struct NDItem *item;
    if (p->full<0 || p->full>=p->cfg->count) return 0;
    item=&p->cfg->items[p->full];
    if (item->kind!=ND_ITEM_SPACER ||
        !nd_numeric_gadget_read(p->width_gad,p->win,&width) ||
        !nd_numeric_gadget_read(p->thickness_gad,p->win,&thickness) ||
        width<20 || width>140 || thickness<1 || thickness>8 ||
        p->line<0 || p->line>1 || p->color<0 ||
        p->color>=ND_SPACER_COLOR_COUNT) return 0;
    item->spacer_width=(int)width;
    item->spacer_line=p->line;
    item->spacer_line_width=(int)thickness;
    item->spacer_line_color=p->color;
    ND_PREFS_MARK_DIRTY();return 1;
}
static int sp_commit(struct SpacerPage *p,int permanent)
{
    BPTR lock;LONG rc;
    if (!sp_stage(p)) return 0;
    lock=CreateDir("ENV:NovaDock");if (lock) UnLock(lock);
    if (permanent) {
        lock=CreateDir("ENVARC:NovaDock");if (lock) UnLock(lock);
        if (!nd_control_write_config(p->cfg,"ENVARC:NovaDock/novadock.cfg")) return 0;
    }
    if (!nd_control_write_config(p->cfg,"ENV:NovaDock/novadock.cfg")) return 0;
    rc=nd_control_reload();
    if (rc!=ND_CONTROL_OK && rc!=ND_CONTROL_NOHOST &&
        rc!=ND_CONTROL_PENDING) return 0;
    ND_PREFS_MARK_CLEAN();ND_PREFS_CLEAR_PREVIEW();
    p->done=1;p->result=0;return 1;
}
static int sp_open(struct SpacerPage *p)
{
    struct NewGadget ng;
    struct NDItem *item=&p->cfg->items[p->full];
    int base_width=p->focused ? 420 : 620;
    int base_height=p->focused ? 160 : 183;
    int width,height,font_x=8,font_y=8;
    int field_x=p->focused ? 190 : 315;
    char width_text[8],thickness_text[8];
    int i,spacer_no=0,x,y;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,
        p->screen->Width,p->screen->Height,p->screen->BarHeight,
        base_width,base_height);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    x=nd_prefs_layout_left(&p->layout);y=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfo(p->screen,TAG_DONE);
    if (!p->vi || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;
    sprintf(width_text,"%d",item->spacer_width ? item->spacer_width :
                         (p->cfg->tile_width+1)/2);
    sprintf(thickness_text,"%d",item->spacer_line_width ?
                             item->spacer_line_width : 2);
    if (!p->focused && (!sp_button(p,10,21,118,S_NAV_GENERAL,"General") ||
        !sp_button(p,10,45,118,S_NAV_APPEARANCE,"Appearance") ||
        !sp_button(p,10,69,118,S_NAV_ITEMS,"> Organize") ||
        !sp_button(p,10,93,118,S_NAV_DIAGNOSTICS,"Diagnostics"))) return 0;
    sp_field(p,&ng,field_x,24,90,S_WIDTH,"Spacer width:");
    p->width_gad=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)width_text,GTST_MaxChars,3,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    sp_field(p,&ng,field_x,52,145,S_LINE,"Divider line:");
    p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)spacer_line_labels,GTCY_Active,p->line,TAG_DONE);
    if (!p->last) return 0;
    sp_field(p,&ng,field_x,80,145,S_COLOR,"Line color:");
    p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)spacer_color_labels,GTCY_Active,p->color,TAG_DONE);
    if (!p->last) return 0;
    sp_field(p,&ng,field_x,108,90,S_THICKNESS,"Line width:");
    p->thickness_gad=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)thickness_text,GTST_MaxChars,1,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    if (p->focused) {
        if (!sp_button(p,235,132,80,S_USE,"Use") ||
            !sp_button(p,325,132,80,S_SAVE,"Save")) return 0;
    } else if (!sp_button(p,335,151,86,S_USE,"Use") ||
               !sp_button(p,427,151,86,S_SAVE,"Save")) return 0;
    for (i=0;i<=p->full;++i)
        if (p->cfg->items[i].kind==ND_ITEM_SPACER) ++spacer_no;
    if (p->focused)
        sprintf(p->title,"Edit spacer %d - %s",spacer_no,item->label);
    else
        sprintf(p->title,"NovaDock Preferences - Spacer %d: %s%s",spacer_no,
                item->label,ND_PREFS_IS_DIRTY() ? " *" : "");
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,x,WA_Top,y,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)p->title,WA_Gadgets,(ULONG)p->gadgets,
        WA_DragBar,TRUE,WA_Activate,TRUE,WA_DepthGadget,TRUE,
        WA_CloseGadget,TRUE,
        WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;GT_RefreshWindow(p->win,NULL);return 1;
}
int nd_spacer_prefs_run(int argc,char **argv)
{
    struct SpacerPage *p;struct IntuiMessage *msg;
    ULONG cls;UWORD id,code;int a,parse=0,visible=-1,full=-1,result=20;
    p=(struct SpacerPage *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 20;
    p->cfg=ND_PREFS_SHARED_CONFIG;
    if (!p->cfg) p->cfg=(struct NDConfig *)AllocVec(sizeof(*p->cfg),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p->cfg) goto done;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    if (!IntuitionBase || !GfxBase || !GadToolsBase) goto done;
    if (!ND_PREFS_SHARED_CONFIG) parse=nd_load_effective(p->cfg);
    if (parse>0) goto done;
    for (a=1;a<argc;++a) {
        char tail=0;int n=-1;
        if (!strncmp(argv[a],"SELECTFULL=",11) &&
            sscanf(argv[a]+11,"%d%c",&n,&tail)==1) full=n;
        else if (!strncmp(argv[a],"SELECT=",7) &&
                 sscanf(argv[a]+7,"%d%c",&n,&tail)==1) visible=n;
        else if (!strcmp(argv[a],"FOCUSED=1")) p->focused=1;
    }
    if (full<0 && visible>=0) full=nd_visible_to_full(p->cfg,visible);
    if (full<0 || full>=p->cfg->count ||
        p->cfg->items[full].kind!=ND_ITEM_SPACER) goto done;
    p->full=full;p->line=p->cfg->items[full].spacer_line ? 1 : 0;
    p->color=p->cfg->items[full].spacer_line_color;
    if (p->color<0 || p->color>=ND_SPACER_COLOR_COUNT)
        p->color=ND_SPACER_COLOR_SUBTLE;
    p->screen=LockPubScreen("Workbench");if (!p->screen || !sp_open(p)) goto done;
    while(!p->done) {
        Wait(1UL<<p->win->UserPort->mp_SigBit);
        while((msg=GT_GetIMsg(p->win->UserPort))!=NULL) {
            cls=msg->Class;code=msg->Code;
            id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID:0;
            GT_ReplyIMsg(msg);
            if (cls==IDCMP_CLOSEWINDOW) {p->done=1;p->result=0;}
            else if (cls==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);
            } else if (cls==IDCMP_GADGETUP) {
                if (id==S_LINE) p->line=(int)code;
                else if (id==S_COLOR) p->color=(int)code;
                else if (id==S_USE) {if (!sp_commit(p,0)) DisplayBeep(p->screen);}
                else if (id==S_SAVE) {if (!sp_commit(p,1)) DisplayBeep(p->screen);}
                else if (id>=S_NAV_GENERAL && id<=S_NAV_DIAGNOSTICS) {
                    if (!sp_stage(p)) {DisplayBeep(p->screen);continue;}
                    if (id==S_NAV_GENERAL) p->result=ND_PREF_NAV_GENERAL;
                    else if (id==S_NAV_APPEARANCE) p->result=ND_PREF_NAV_APPEARANCE;
                    else if (id==S_NAV_ITEMS) p->result=ND_PREF_NAV_ORGANIZE;
                    else p->result=ND_PREF_NAV_DIAGNOSTICS;
                    p->done=1;
                }
            }
        }
    }
    result=p->result;
done:
    if (p->win) CloseWindow(p->win);
    if (p->gadgets) FreeGadgets(p->gadgets);
    if (p->vi) FreeVisualInfo(p->vi);
    if (p->screen) UnlockPubScreen(NULL,p->screen);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (p->cfg && !ND_PREFS_SHARED_CONFIG) FreeVec(p->cfg);
    FreeVec(p);return result;
}
