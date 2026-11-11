/* Focused Add Item page used by the dock background menu.
 * One type selector owns the left column; the right side is rebuilt with
 * only the fields that make sense for the selected item type. */
#define IntuitionBase NDAddIntuitionBase
#define GadToolsBase NDAddGadToolsBase
#define GfxBase NDAddGfxBase
#define AslBase NDAddAslBase
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_layout.h"
#include "nd_control_client.h"
#include "nd_prefs_layout.h"
#include "nd_prefs_session.h"
#include "nd_plugin_discovery.h"
#include "nd_manifest.h"
#include "nd_options.h"
#include "nd_prefs_help.h"

#define A_TYPE_APP       10
#define A_TYPE_DRAWER    11
#define A_TYPE_SPACER    12
#define A_TYPE_PLUGIN    14
#define A_APP_KIND       15
#define A_NAME           20
#define A_TARGET         21
#define A_TARGET_BROWSE  22
#define A_ICON           23
#define A_ICON_BROWSE    24
#define A_MODE           25
#define A_ARGUMENTS      26
#define A_WORKDIR        27
#define A_HOTKEY         28
#define A_SPACE          29
#define A_WIDTH          30
#define A_LINE           31
#define A_THICKNESS      32
#define A_COLOR          33
#define A_CAPTION        34
#define A_INTERVAL       35
#define A_OUTPUT_LINE    36
#define A_METER          37
#define A_CLICK          38
#define A_CLICK_COMMAND  39
#define A_PLUGIN_LIST    40
#define A_PLUGIN_NAME    41
#define A_PLUGIN_PROGRAM 42
#define A_ADD            50
#define A_CANCEL         51

#define ADD_APP 0
#define ADD_DRAWER 1
#define ADD_SPACER 2
#define ADD_COMMAND 3
#define ADD_PLUGIN 4
#define ADD_GROUP 5
#define ADD_SCRIPT 6
#define ADD_TYPE_COUNT 7

struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct GfxBase *GfxBase=NULL;
struct Library *AslBase=NULL;

static STRPTR add_modes[]={"Automatic","Workbench","CLI","ARexx",NULL};
static STRPTR application_kinds[]={"Application","Command tile",
                                   "Application group","Script tile",NULL};
static STRPTR spacer_colors[]={"Subtle","Accent","Secondary","Text",NULL};
static STRPTR command_meters[]={"None","Percent",NULL};
static STRPTR command_clicks[]={"Run again","Nothing","Command",NULL};
static STRPTR script_clicks[]={"Run script","Nothing",NULL};

struct AddItemPage {
    struct NDConfig *cfg;
    struct Screen *screen;
    struct Window *win;
    APTR vi;
    struct Gadget *gadgets,*last;
    struct Gadget *app_kind,*name,*target,*icon,*mode,*arguments,*workdir,*hotkey;
    struct Gadget *space,*width,*line,*thickness,*color;
    struct Gadget *caption,*interval,*output_line,*meter,*click,*click_command_g;
    struct Gadget *plugin_list,*plugin_name,*plugin_program;
    struct NDPrefsLayout layout;
    int type,done,reopen,result;
    char names[ADD_TYPE_COUNT][ND_LABEL_LEN];
    char targets[3][ND_COMMAND_LEN];
    char icons[3][ND_COMMAND_LEN];
    char args[3][ND_COMMAND_LEN];
    char workdirs[3][ND_COMMAND_LEN];
    char hotkeys[3][ND_HOTKEY_LEN];
    int modes[3],space_meters[3];
    int spacer_width,spacer_line,spacer_thickness,spacer_color;
    char command[ND_COMMAND_LEN],command_caption[ND_LABEL_LEN];
    char command_interval[12],command_line[8],click_command[ND_COMMAND_LEN];
    int command_meter,command_click;
    char script[ND_COMMAND_LEN],script_caption[ND_LABEL_LEN];
    char script_interval[12],script_timeout[12];
    int script_click;
    struct List plugin_rows;
    struct Node plugin_nodes[ND_PLUGIN_CATALOG_MAX];
    char plugin_names[ND_PLUGIN_CATALOG_MAX+1][96];
    /* Disk catalogue (manifested plugins) scanned once per window.  Each
     * list row points at a retained configuration definition, a catalogue
     * entry, or both; -1 marks the side that does not exist. */
    struct NDPluginSummary plugin_catalog[ND_PLUGIN_CATALOG_MAX];
    int catalog_count,catalog_scanned;
    int plugin_row_cfg[ND_PLUGIN_CATALOG_MAX];
    int plugin_row_cat[ND_PLUGIN_CATALOG_MAX];
    int plugin_count,plugin_selected;
    char preselect[20];   /* PLUGIN=ID: select this plugin once */
};

static int is_app_type(int type)
{
    return type==ADD_APP || type==ADD_COMMAND || type==ADD_GROUP ||
           type==ADD_SCRIPT;
}

static int app_kind_index(int type)
{
    return type==ADD_COMMAND ? 1 : type==ADD_GROUP ? 2 :
           type==ADD_SCRIPT ? 3 : 0;
}

static int path_slot(int type)
{
    return type==ADD_GROUP ? 2 : type;
}

static void copy_text(char *out,const char *in,int size)
{
    if (!out || size<=0) return;
    if (!in) in="";
    strncpy(out,in,size-1);out[size-1]='\0';
}

static const char *gadget_text(struct Gadget *g)
{
    struct StringInfo *si;
    if (!g) return "";
    si=(struct StringInfo *)g->SpecialInfo;
    return si && si->Buffer ? (const char *)si->Buffer : "";
}

static void add_title(struct AddItemPage *p,const char *text)
{
    if (p->win) SetWindowTitles(p->win,(STRPTR)text,(STRPTR)-1);
}

static void add_ng(struct AddItemPage *p,struct NewGadget *ng,
                   int x,int y,int w,int h,int id,STRPTR label)
{
    memset(ng,0,sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&p->layout,h);
    ng->ng_GadgetID=id;ng->ng_GadgetText=label;
    ng->ng_VisualInfo=p->vi;ng->ng_TextAttr=p->screen->Font;
    ng->ng_Flags=PLACETEXT_LEFT;
}

static struct Gadget *add_button(struct AddItemPage *p,int x,int y,int w,
                                 int id,STRPTR label,int disabled)
{
    struct NewGadget ng;
    add_ng(p,&ng,x,y,w,16,id,label);
    /* Category selection is shown by the disabled/stippled state.  Keeping
     * the requested width prevents the selected button from growing into
     * the field-caption gutter when its label changes. */
    ng.ng_Flags=PLACETEXT_IN;
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
                         GA_Disabled,(ULONG)(disabled ? TRUE : FALSE),TAG_DONE);
    return p->last;
}

static struct Gadget *add_readonly_string(struct AddItemPage *p,int x,int y,
                                          int w,int id,STRPTR label,int max)
{
    struct NewGadget ng;
    add_ng(p,&ng,x,y,w,16,id,label);
    p->last=CreateGadget(STRING_KIND,p->last,&ng,GTST_MaxChars,max,
                         GA_Disabled,TRUE,TAG_DONE);
    return p->last;
}

static int catalog_find(const struct AddItemPage *p,const char *id)
{
    int i;
    for (i=0;i<p->catalog_count;++i)
        if (!strcmp(p->plugin_catalog[i].id,id)) return i;
    return -1;
}

static int config_has_plugin(const struct NDConfig *cfg,const char *id)
{
    int i;
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id,id))
            return 1;
    return 0;
}

static void add_plugin_row(struct AddItemPage *p,int cfg_index,int cat_index,
                           const char *label,const char *id)
{
    int row=p->plugin_count;
    if (row>=ND_PLUGIN_CATALOG_MAX) return;
    p->plugin_row_cfg[row]=cfg_index;p->plugin_row_cat[row]=cat_index;
    sprintf(p->plugin_names[row],"%-17.17s [%.12s]",label,id);
    memset(&p->plugin_nodes[row],0,sizeof(struct Node));
    p->plugin_nodes[row].ln_Name=p->plugin_names[row];
    AddTail(&p->plugin_rows,&p->plugin_nodes[row]);
    ++p->plugin_count;
}

/* Every installed plugin is offered: bundled plugins through the retained
 * definitions in the configuration (they carry no manifest file), third-party
 * plugins through the Plugins drawer catalogue even before they have ever
 * been merged into the configuration.  Command tiles have their own type. */
static void build_plugin_rows(struct AddItemPage *p)
{
    int i,j;
    NewList(&p->plugin_rows);
    p->plugin_count=0;
    if (!p->catalog_scanned) {
        p->catalog_count=nd_plugin_catalog_summaries(p->plugin_catalog,
                                                     ND_PLUGIN_CATALOG_MAX);
        p->catalog_scanned=1;
    }
    for (i=0;i<p->cfg->count;++i) {
        const struct NDItem *item=&p->cfg->items[i];
        int active=-1,inactive=-1,seen=0;
        if (item->kind!=ND_ITEM_PLUGIN || !strcmp(item->id,"ROADSHOW") || nd_command_plugin_path(item->command) || nd_script_plugin_path(item->command))
            continue;
        for (j=0;j<i;++j)
            if (p->cfg->items[j].kind==ND_ITEM_PLUGIN &&
                !strcmp(p->cfg->items[j].id,item->id)) {seen=1;break;}
        if (seen) continue;
        for (j=i;j<p->cfg->count;++j)
            if (p->cfg->items[j].kind==ND_ITEM_PLUGIN &&
                !strcmp(p->cfg->items[j].id,item->id)) {
                if (p->cfg->items[j].enabled) active=j;
                else if (inactive<0) inactive=j;
            }
        /* Single-instance plugins disappear while their tile is present.
         * Multi-tile plugins remain selectable; prefer restoring a retained
         * disabled definition before allocating another slot. */
        if (active>=0 && !nd_plugin_allows_multiple(&p->cfg->items[active]))
            continue;
        add_plugin_row(p,inactive>=0 ? inactive : (active>=0 ? active : i),
                       catalog_find(p,item->id),item->label,item->id);
    }
    for (i=0;i<p->catalog_count;++i) {
        const struct NDPluginSummary *e=&p->plugin_catalog[i];
        if (!strcmp(e->id,"ROADSHOW") || nd_command_plugin_path(e->command) ||
            nd_script_plugin_path(e->command) ||
            config_has_plugin(p->cfg,e->id)) continue;
        add_plugin_row(p,-1,i,e->name,e->id);
    }
    if (p->preselect[0]) {
        for (i=0;i<p->plugin_count;++i) {
            const char *row_id=p->plugin_row_cfg[i]>=0 ?
                p->cfg->items[p->plugin_row_cfg[i]].id :
                p->plugin_catalog[p->plugin_row_cat[i]].id;
            if (!strcmp(row_id,p->preselect)) {p->plugin_selected=i;break;}
        }
        p->preselect[0]='\0';
    }
    if (!p->plugin_count) {
        strcpy(p->plugin_names[0],"No plugins available");
        memset(&p->plugin_nodes[0],0,sizeof(struct Node));
        p->plugin_nodes[0].ln_Name=p->plugin_names[0];
        AddTail(&p->plugin_rows,&p->plugin_nodes[0]);
        p->plugin_selected=-1;
    } else {
        if (p->plugin_selected>=p->plugin_count)
            p->plugin_selected=p->plugin_count-1;
        if (p->plugin_selected<0) p->plugin_selected=0;
    }
}

static void set_plugin_details(struct AddItemPage *p)
{
    const char *name="No plugins available",*program="";
    int row=p->plugin_selected;
    if (row>=0 && row<p->plugin_count) {
        if (p->plugin_row_cfg[row]>=0) {
            const struct NDItem *item=&p->cfg->items[p->plugin_row_cfg[row]];
            name=item->label;program=item->command;
        } else {
            const struct NDPluginSummary *e=
                &p->plugin_catalog[p->plugin_row_cat[row]];
            name=e->name;program=e->command;
        }
    }
    if (p->plugin_name)
        GT_SetGadgetAttrs(p->plugin_name,p->win,NULL,GTST_String,(ULONG)name,TAG_DONE);
    if (p->plugin_program)
        GT_SetGadgetAttrs(p->plugin_program,p->win,NULL,GTST_String,(ULONG)program,TAG_DONE);
}

static struct Gadget *add_string(struct AddItemPage *p,int x,int y,int w,
                                 int id,STRPTR label,int max)
{
    struct NewGadget ng;
    add_ng(p,&ng,x,y,w,16,id,label);
    p->last=CreateGadget(STRING_KIND,p->last,&ng,GTST_MaxChars,max,TAG_DONE);
    return p->last;
}

static void capture_fields(struct AddItemPage *p)
{
    ULONG value=0;
    int slot;
    if (!p->win) return;
    if (p->type!=ADD_PLUGIN)
        copy_text(p->names[p->type],gadget_text(p->name),ND_LABEL_LEN);
    if (p->type==ADD_APP || p->type==ADD_DRAWER || p->type==ADD_GROUP) {
        slot=path_slot(p->type);
        copy_text(p->targets[slot],gadget_text(p->target),ND_COMMAND_LEN);
        copy_text(p->icons[slot],gadget_text(p->icon),ND_COMMAND_LEN);
        copy_text(p->hotkeys[slot],gadget_text(p->hotkey),ND_HOTKEY_LEN);
        if (p->type==ADD_APP) {
            copy_text(p->args[slot],gadget_text(p->arguments),ND_COMMAND_LEN);
            copy_text(p->workdirs[slot],gadget_text(p->workdir),ND_COMMAND_LEN);
            GT_GetGadgetAttrs(p->mode,p->win,NULL,
                              GTCY_Active,(ULONG)&value,TAG_DONE);
            p->modes[slot]=(int)value;
        }
        if (p->space) {
            value=0;
            GT_GetGadgetAttrs(p->space,p->win,NULL,GTCB_Checked,(ULONG)&value,TAG_DONE);
            p->space_meters[slot]=value ? 1 : 0;
        }
    } else if (p->type==ADD_SPACER) {
        p->spacer_width=atoi(gadget_text(p->width));
        p->spacer_thickness=atoi(gadget_text(p->thickness));
        value=0;GT_GetGadgetAttrs(p->line,p->win,NULL,GTCB_Checked,(ULONG)&value,TAG_DONE);
        p->spacer_line=value ? 1 : 0;
        value=0;GT_GetGadgetAttrs(p->color,p->win,NULL,GTCY_Active,(ULONG)&value,TAG_DONE);
        p->spacer_color=(int)value;
    } else if (p->type==ADD_COMMAND) {
        copy_text(p->command,gadget_text(p->target),ND_COMMAND_LEN);
        copy_text(p->command_caption,gadget_text(p->caption),ND_LABEL_LEN);
        copy_text(p->command_interval,gadget_text(p->interval),sizeof(p->command_interval));
        copy_text(p->command_line,gadget_text(p->output_line),sizeof(p->command_line));
        copy_text(p->click_command,gadget_text(p->click_command_g),ND_COMMAND_LEN);
        value=0;GT_GetGadgetAttrs(p->meter,p->win,NULL,GTCY_Active,(ULONG)&value,TAG_DONE);
        p->command_meter=(int)value;
        value=0;GT_GetGadgetAttrs(p->click,p->win,NULL,GTCY_Active,(ULONG)&value,TAG_DONE);
        p->command_click=(int)value;
    } else if (p->type==ADD_SCRIPT) {
        copy_text(p->script,gadget_text(p->target),ND_COMMAND_LEN);
        copy_text(p->script_caption,gadget_text(p->caption),ND_LABEL_LEN);
        copy_text(p->script_interval,gadget_text(p->interval),sizeof(p->script_interval));
        copy_text(p->script_timeout,gadget_text(p->output_line),sizeof(p->script_timeout));
        value=0;GT_GetGadgetAttrs(p->click,p->win,NULL,GTCY_Active,(ULONG)&value,TAG_DONE);
        p->script_click=(int)value;
    }
}

static void set_fields(struct AddItemPage *p)
{
    int slot;
    if (!p->win) return;
    if (p->type!=ADD_PLUGIN)
        GT_SetGadgetAttrs(p->name,p->win,NULL,GTST_String,(ULONG)p->names[p->type],TAG_DONE);
    if (p->type==ADD_APP || p->type==ADD_DRAWER || p->type==ADD_GROUP) {
        slot=path_slot(p->type);
        GT_SetGadgetAttrs(p->target,p->win,NULL,GTST_String,(ULONG)p->targets[slot],TAG_DONE);
        GT_SetGadgetAttrs(p->icon,p->win,NULL,GTST_String,(ULONG)p->icons[slot],TAG_DONE);
        if (p->type==ADD_APP) {
            GT_SetGadgetAttrs(p->mode,p->win,NULL,
                              GTCY_Active,(ULONG)p->modes[slot],TAG_DONE);
            GT_SetGadgetAttrs(p->arguments,p->win,NULL,
                              GTST_String,(ULONG)p->args[slot],TAG_DONE);
            GT_SetGadgetAttrs(p->workdir,p->win,NULL,
                              GTST_String,(ULONG)p->workdirs[slot],TAG_DONE);
        }
        GT_SetGadgetAttrs(p->hotkey,p->win,NULL,GTST_String,(ULONG)p->hotkeys[slot],TAG_DONE);
        if (p->space) GT_SetGadgetAttrs(p->space,p->win,NULL,
            GTCB_Checked,(ULONG)(p->space_meters[slot] ? TRUE : FALSE),TAG_DONE);
    } else if (p->type==ADD_SPACER) {
        char number[16];
        sprintf(number,"%d",p->spacer_width);
        GT_SetGadgetAttrs(p->width,p->win,NULL,GTST_String,(ULONG)number,TAG_DONE);
        sprintf(number,"%d",p->spacer_thickness);
        GT_SetGadgetAttrs(p->thickness,p->win,NULL,GTST_String,(ULONG)number,TAG_DONE);
        GT_SetGadgetAttrs(p->line,p->win,NULL,GTCB_Checked,(ULONG)(p->spacer_line?TRUE:FALSE),TAG_DONE);
        GT_SetGadgetAttrs(p->color,p->win,NULL,GTCY_Active,(ULONG)p->spacer_color,TAG_DONE);
    } else if (p->type==ADD_COMMAND) {
        GT_SetGadgetAttrs(p->target,p->win,NULL,GTST_String,(ULONG)p->command,TAG_DONE);
        GT_SetGadgetAttrs(p->caption,p->win,NULL,GTST_String,(ULONG)p->command_caption,TAG_DONE);
        GT_SetGadgetAttrs(p->interval,p->win,NULL,GTST_String,(ULONG)p->command_interval,TAG_DONE);
        GT_SetGadgetAttrs(p->output_line,p->win,NULL,GTST_String,(ULONG)p->command_line,TAG_DONE);
        GT_SetGadgetAttrs(p->meter,p->win,NULL,GTCY_Active,(ULONG)p->command_meter,TAG_DONE);
        GT_SetGadgetAttrs(p->click,p->win,NULL,GTCY_Active,(ULONG)p->command_click,TAG_DONE);
        GT_SetGadgetAttrs(p->click_command_g,p->win,NULL,GTST_String,(ULONG)p->click_command,TAG_DONE);
    } else if (p->type==ADD_SCRIPT) {
        GT_SetGadgetAttrs(p->target,p->win,NULL,GTST_String,(ULONG)p->script,TAG_DONE);
        GT_SetGadgetAttrs(p->caption,p->win,NULL,GTST_String,(ULONG)p->script_caption,TAG_DONE);
        GT_SetGadgetAttrs(p->interval,p->win,NULL,GTST_String,(ULONG)p->script_interval,TAG_DONE);
        GT_SetGadgetAttrs(p->output_line,p->win,NULL,GTST_String,(ULONG)p->script_timeout,TAG_DONE);
        GT_SetGadgetAttrs(p->click,p->win,NULL,GTCY_Active,(ULONG)p->script_click,TAG_DONE);
    } else set_plugin_details(p);
}

static int open_add_ui(struct AddItemPage *p)
{
    struct NewGadget ng;
    int font_x=8,font_y=8,width,height,x,y;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,p->screen->Width,
                         p->screen->Height,p->screen->BarHeight,660,330);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    x=nd_prefs_layout_left(&p->layout);y=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfoA(p->screen,NULL);
    if (!p->vi) return 0;
    add_ng(p,&ng,0,0,1,1,0,NULL);
    p->gadgets=p->last=CreateContext(&p->gadgets);
    if (!p->last) return 0;
    /* Application, Command tile and Application group share one sidebar
     * entry; the Type cycle on the right selects between them. */
    if (!add_button(p,10,28,132,A_TYPE_APP,"Application",
                    is_app_type(p->type)) ||
        !add_button(p,10,54,132,A_TYPE_DRAWER,"Drawer / disk",
                    p->type==ADD_DRAWER) ||
        !add_button(p,10,80,132,A_TYPE_PLUGIN,"Plugins",
                    p->type==ADD_PLUGIN) ||
        !add_button(p,10,106,132,A_TYPE_SPACER,"Spacer",
                    p->type==ADD_SPACER)) return 0;
    if (p->type==ADD_PLUGIN) {
        build_plugin_rows(p);
        add_ng(p,&ng,220,48,410,120,A_PLUGIN_LIST,"Installed plugins");
        ng.ng_Flags=PLACETEXT_ABOVE;
        p->plugin_list=p->last=CreateGadget(LISTVIEW_KIND,p->last,&ng,
            GTLV_Labels,(ULONG)&p->plugin_rows,
            /* On V39+ NULL means draw the selected row with the native
             * highlight bar.  Without this tag GadTools tracks the ordinal
             * but gives no visible feedback inside the list. */
            GTLV_ShowSelected,(ULONG)NULL,
            GTLV_Selected,(ULONG)(p->plugin_selected<0 ? ~0UL:p->plugin_selected),
            TAG_DONE);
        p->plugin_name=add_readonly_string(p,300,188,330,A_PLUGIN_NAME,
                                           "Name:",ND_LABEL_LEN-1);
        p->plugin_program=add_readonly_string(p,300,220,330,A_PLUGIN_PROGRAM,
                                              "Program:",ND_COMMAND_LEN-1);
        if (!p->plugin_list || !p->plugin_name || !p->plugin_program) return 0;
    } else if (is_app_type(p->type)) {
        add_ng(p,&ng,300,28,330,16,A_APP_KIND,"Type:");
        p->app_kind=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
            GTCY_Labels,(ULONG)application_kinds,
            GTCY_Active,(ULONG)app_kind_index(p->type),TAG_DONE);
        p->name=add_string(p,300,56,330,A_NAME,"Name:",ND_LABEL_LEN-1);
        if (!p->app_kind || !p->name) return 0;
    } else {
        p->name=add_string(p,300,28,330,A_NAME,"Name:",ND_LABEL_LEN-1);
        if (!p->name) return 0;
    }
    if (p->type==ADD_APP) {
        p->target=add_string(p,300,84,220,A_TARGET,"Target:",ND_COMMAND_LEN-1);
        if (!p->target || !add_button(p,530,84,100,A_TARGET_BROWSE,"Browse...",0)) return 0;
        p->icon=add_string(p,300,112,220,A_ICON,"Icon:",ND_COMMAND_LEN-1);
        if (!p->icon || !add_button(p,530,112,100,A_ICON_BROWSE,"Browse...",0)) return 0;
        add_ng(p,&ng,300,140,330,16,A_MODE,"Run as:");
        p->mode=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
                                     GTCY_Labels,(ULONG)add_modes,TAG_DONE);
        p->arguments=add_string(p,300,168,330,A_ARGUMENTS,
                                "Arguments:",ND_COMMAND_LEN-1);
        p->workdir=add_string(p,300,196,330,A_WORKDIR,
                              "Start in:",ND_COMMAND_LEN-1);
        p->hotkey=add_string(p,300,224,330,A_HOTKEY,
                             "Hot key:",ND_HOTKEY_LEN-1);
    } else if (p->type==ADD_DRAWER || p->type==ADD_GROUP) {
        /* An application group sits below the Type cycle, one row lower. */
        int top=p->type==ADD_GROUP ? 84 : 56;
        p->target=add_string(p,300,top,220,A_TARGET,"Target:",ND_COMMAND_LEN-1);
        if (!p->target || !add_button(p,530,top,100,A_TARGET_BROWSE,"Browse...",0)) return 0;
        p->icon=add_string(p,300,top+28,220,A_ICON,"Icon:",ND_COMMAND_LEN-1);
        if (!p->icon || !add_button(p,530,top+28,100,A_ICON_BROWSE,"Browse...",0)) return 0;
        /* Drawers and volumes are always opened by Workbench.  Launcher
         * execution mode, arguments and working directory do not apply. */
        p->hotkey=add_string(p,300,top+64,330,A_HOTKEY,
                             "Hot key:",ND_HOTKEY_LEN-1);
        if (p->type==ADD_DRAWER) {
            add_ng(p,&ng,610,154,20,16,A_SPACE,"Show space usage:");
            p->space=p->last=CreateGadget(CHECKBOX_KIND,p->last,&ng,TAG_DONE);
        }
    } else if (p->type==ADD_SPACER) {
        p->width=add_string(p,300,74,160,A_WIDTH,"Width:",3);
        add_ng(p,&ng,440,108,20,16,A_LINE,"Show divider:");
        p->line=p->last=CreateGadget(CHECKBOX_KIND,p->last,&ng,TAG_DONE);
        p->thickness=add_string(p,300,142,160,A_THICKNESS,"Line width:",1);
        add_ng(p,&ng,300,176,160,16,A_COLOR,"Line colour:");
        p->color=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,GTCY_Labels,(ULONG)spacer_colors,TAG_DONE);
    } else if (p->type==ADD_COMMAND) {
        p->target=add_string(p,300,84,330,A_TARGET,"Command:",ND_COMMAND_LEN-1);
        p->caption=add_string(p,300,112,330,A_CAPTION,"Caption:",ND_LABEL_LEN-1);
        p->interval=add_string(p,300,148,130,A_INTERVAL,"Refresh seconds:",10);
        p->output_line=add_string(p,300,176,130,A_OUTPUT_LINE,"Output line:",2);
        add_ng(p,&ng,300,208,150,16,A_METER,"Meter:");
        p->meter=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,GTCY_Labels,(ULONG)command_meters,TAG_DONE);
        add_ng(p,&ng,525,208,105,16,A_CLICK,"Click:");
        p->click=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,GTCY_Labels,(ULONG)command_clicks,TAG_DONE);
        p->click_command_g=add_string(p,300,240,330,A_CLICK_COMMAND,"Click command:",ND_COMMAND_LEN-1);
    } else if (p->type==ADD_SCRIPT) {
        p->target=add_string(p,300,84,220,A_TARGET,"ARexx script:",ND_COMMAND_LEN-1);
        if (!p->target || !add_button(p,530,84,100,A_TARGET_BROWSE,"Browse...",0)) return 0;
        p->caption=add_string(p,300,112,330,A_CAPTION,"Caption:",ND_LABEL_LEN-1);
        p->interval=add_string(p,300,148,130,A_INTERVAL,"Refresh seconds:",10);
        p->output_line=add_string(p,300,176,130,A_OUTPUT_LINE,"Timeout seconds:",3);
        add_ng(p,&ng,300,208,150,16,A_CLICK,"Click:");
        p->click=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,GTCY_Labels,(ULONG)script_clicks,TAG_DONE);
    }
    /* Adding persists the new item.  Cancel follows the same caller-aware
     * path as the close gadget: an embedded page returns to Organize while a
     * window opened directly from the dock simply closes. */
    if (!p->last ||
        !add_button(p,340,292,140,A_ADD,"Add item",0) ||
        !add_button(p,490,292,140,A_CANCEL,"Cancel",0)) return 0;
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,x,WA_Top,y,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)"NovaDock Preferences - Add Item",
        WA_Gadgets,(ULONG)p->gadgets,WA_DragBar,TRUE,WA_Activate,TRUE,
        WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
        WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;
    GT_RefreshWindow(p->win,NULL);set_fields(p);
    return 1;
}

static void close_add_ui(struct AddItemPage *p)
{
    if (p->win) { CloseWindow(p->win);p->win=NULL; }
    if (p->gadgets) { FreeGadgets(p->gadgets);p->gadgets=NULL;p->last=NULL; }
    if (p->vi) { FreeVisualInfo(p->vi);p->vi=NULL; }
    p->app_kind=p->name=p->target=p->icon=p->mode=p->arguments=p->workdir=p->hotkey=NULL;
    p->space=p->width=p->line=p->thickness=p->color=NULL;
    p->caption=p->interval=p->output_line=p->meter=p->click=p->click_command_g=NULL;
    p->plugin_list=p->plugin_name=p->plugin_program=NULL;
}

static void choose_file(struct AddItemPage *p,int icon)
{
    struct FileRequester *fr;
    char path[ND_COMMAND_LEN];
    int drawer=(p->type==ADD_DRAWER || p->type==ADD_GROUP) && !icon;
    if (!AslBase) { add_title(p,"Add Item - file requester unavailable");return; }
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)(icon ? "Select icon artwork" :
          (drawer ? "Select drawer or disk" :
           p->type==ADD_SCRIPT ? "Select ARexx script" : "Select application")),
        ASLFR_DrawersOnly,drawer ? TRUE : FALSE,TAG_DONE);
    if (!fr) return;
    if (AslRequest(fr,NULL)) {
        copy_text(path,fr->fr_Drawer,sizeof(path));
        if (!drawer && !AddPart(path,fr->fr_File,sizeof(path)))
            add_title(p,"Add Item - selected path is too long");
        else GT_SetGadgetAttrs(icon ? p->icon : p->target,p->win,NULL,
                              GTST_String,(ULONG)path,TAG_DONE);
    }
    FreeAslRequest(fr);
}

static void make_dir(const char *path)
{
    BPTR lock=CreateDir((STRPTR)path);
    if (lock) UnLock(lock);
}

static int fresh_tile_id(const struct NDConfig *cfg,const char *prefix,
                         char *id,int size)
{
    int n,i,used;
    for (n=1;n<1000;++n) {
        sprintf(id,"%s%d",prefix,n);used=0;
        for (i=0;i<cfg->count;++i)
            if (cfg->items[i].kind==ND_ITEM_PLUGIN && !strcmp(cfg->items[i].id,id)) used=1;
        if (!used) return 1;
    }
    if (size>0) id[0]='\0';
    return 0;
}

static int add_selected(struct AddItemPage *p)
{
    int index,ok=0,interval,line,row_counted=0;
    char id[20],definition[128],number[16];
    capture_fields(p);
    /* Installed plugins already have retained catalogue definitions.  They
     * can be enabled even when that catalogue is full because doing so does
     * not allocate another definition. */
    if (p->type!=ADD_PLUGIN && p->cfg->count>=ND_MAX_ITEMS) return 0;
    if (p->type==ADD_APP || p->type==ADD_DRAWER || p->type==ADD_GROUP) {
        int slot=path_slot(p->type);
        int kind=p->type==ADD_DRAWER ? ND_ITEM_DRAWER :
                 (p->type==ADD_GROUP ? ND_ITEM_GROUP : ND_ITEM_LAUNCH);
        ok=nd_add_launcher(p->cfg,kind,
                           p->names[p->type],p->targets[slot],p->icons[slot]);
        if (ok) {
            index=p->cfg->count-1;
            ok=nd_set_launcher_action(p->cfg,index,p->modes[slot],p->args[slot],
                                      p->workdirs[slot],p->hotkeys[slot],0);
            if (ok && p->type==ADD_DRAWER)
                p->cfg->items[index].show_volume_meter=p->space_meters[slot];
        }
    } else if (p->type==ADD_SPACER) {
        ok=p->spacer_width>=0 && p->spacer_width<=140 &&
           p->spacer_thickness>=1 && p->spacer_thickness<=8 &&
           p->spacer_color>=0 && p->spacer_color<ND_SPACER_COLOR_COUNT &&
           nd_add_spacer(p->cfg,p->names[p->type]);
        if (ok) {
            index=p->cfg->count-1;
            p->cfg->items[index].spacer_width=p->spacer_width;
            p->cfg->items[index].spacer_line=p->spacer_line;
            p->cfg->items[index].spacer_line_width=p->spacer_thickness;
            p->cfg->items[index].spacer_line_color=p->spacer_color;
        }
    } else if (p->type==ADD_COMMAND) {
        interval=atoi(p->command_interval);line=atoi(p->command_line);
        ok=p->names[p->type][0] && p->command[0] && interval>=1 && interval<=86400 &&
           line>=1 && line<=20 && fresh_tile_id(p->cfg,"CMD",id,sizeof(id));
        if (ok) {
            sprintf(definition,"PLUGIN=%s|Plugins/Command|%s|METER|ON",
                    p->names[p->type],id);
            ok=nd_parse_line(p->cfg,definition);
        }
        if (ok) ok=nd_option_set(p->cfg,id,"COMMAND",p->command);
        if (ok) ok=nd_option_set(p->cfg,id,"CAPTION",p->command_caption);
        sprintf(number,"%d",interval);
        if (ok) ok=nd_option_set(p->cfg,id,"INTERVAL",number);
        sprintf(number,"%d",line);
        if (ok) ok=nd_option_set(p->cfg,id,"LINE",number);
        if (ok) ok=nd_option_set(p->cfg,id,"METER",p->command_meter ? "PERCENT" : "NONE");
        if (ok) ok=nd_option_set(p->cfg,id,"CLICK",p->command_click==0 ? "RERUN" :
                                  (p->command_click==1 ? "NONE" : "COMMAND"));
        if (ok) ok=nd_option_set(p->cfg,id,"CLICK_COMMAND",p->click_command);
    } else if (p->type==ADD_SCRIPT) {
        int timeout=atoi(p->script_timeout);
        interval=atoi(p->script_interval);
        ok=p->names[p->type][0] && p->script[0] && interval>=1 && interval<=86400 &&
           timeout>=1 && timeout<=600 && fresh_tile_id(p->cfg,"SCRIPT",id,sizeof(id));
        if (ok) {
            sprintf(definition,"PLUGIN=%s|Plugins/Script|%s|METER|ON",
                    p->names[p->type],id);
            ok=nd_parse_line(p->cfg,definition);
        }
        if (ok) ok=nd_option_set(p->cfg,id,"SCRIPT",p->script);
        if (ok) ok=nd_option_set(p->cfg,id,"CAPTION",p->script_caption);
        sprintf(number,"%d",interval);
        if (ok) ok=nd_option_set(p->cfg,id,"INTERVAL",number);
        sprintf(number,"%d",timeout);
        if (ok) ok=nd_option_set(p->cfg,id,"TIMEOUT",number);
        if (ok) ok=nd_option_set(p->cfg,id,"CLICK",p->script_click ? "NONE" : "RUN");
    } else if (p->type==ADD_PLUGIN && p->plugin_selected>=0 &&
               p->plugin_selected<p->plugin_count) {
        index=p->plugin_row_cfg[p->plugin_selected];
        if (index<0) {
            /* Catalogue-only plugin: merge its manifest (adds a disabled
             * definition with its settings schema at the end), then enable
             * it exactly like a retained definition. */
            /* The list keeps only summaries; the full manifest (settings
             * schema included) is read again just for the plugin added. */
            const struct NDPluginSummary *e=
                &p->plugin_catalog[p->plugin_row_cat[p->plugin_selected]];
            struct NDPluginCatalogEntry *full=(struct NDPluginCatalogEntry *)
                AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR);
            ok=full && nd_plugin_catalog_lookup(e->id,full) &&
               nd_manifest_merge(p->cfg,&full->manifest,full->command)==
                   ND_MANIFEST_ADDED;
            if (full) FreeVec(full);
            if (ok) {
                index=p->cfg->count-1;
                ok=nd_set_item_enabled(p->cfg,index,1);
                if (ok) row_counted=1;
            }
        } else if (p->cfg->items[index].enabled) {
            ok=nd_add_plugin_instance(p->cfg,index);
            if (ok) row_counted=1;
        } else {
            ok=nd_set_item_enabled(p->cfg,index,1);
            if (ok) {
                row_counted=1;
                ok=nd_move_full_item(p->cfg,index,p->cfg->count-1);
            }
        }
    }
    if (ok && !row_counted && p->cfg->row_layout && p->cfg->rows>0)
        ++p->cfg->row_counts[p->cfg->rows-1];
    return ok;
}

static int commit_add(struct AddItemPage *p,int permanent)
{
    struct NDConfig *backup;
    struct NDLayout fit;
    LONG rc;
    int ok;
    backup=(struct NDConfig *)AllocVec(sizeof(*backup),MEMF_PUBLIC|MEMF_REVERSE);
    if (!backup) return 0;
    *backup=*p->cfg;
    if (!add_selected(p)) {
        *p->cfg=*backup;FreeVec(backup);
        add_title(p,"NovaDock Add Item - check the required fields");
        DisplayBeep(p->screen);return 0;
    }
    ok=nd_layout_fit_enabled(p->cfg,p->screen->Width,p->screen->Height,
                             p->screen->BarHeight,&fit);
    if (!ok) {
        *p->cfg=*backup;FreeVec(backup);
        add_title(p,"NovaDock Add Item - item does not fit this screen");
        DisplayBeep(p->screen);return 0;
    }
    make_dir("ENV:NovaDock");
    if (permanent) {
        make_dir("ENVARC:NovaDock");
        if (!nd_control_write_config(p->cfg,"ENVARC:NovaDock/novadock.cfg")) {
            *p->cfg=*backup;FreeVec(backup);return 0;
        }
    }
    if (!nd_control_write_config(p->cfg,"ENV:NovaDock/novadock.cfg")) {
        *p->cfg=*backup;FreeVec(backup);return 0;
    }
    FreeVec(backup);
    rc=nd_control_reload();
    ND_PREFS_MARK_CLEAN();ND_PREFS_CLEAR_PREVIEW();
    p->done=1;p->result=0;
    if (rc!=ND_CONTROL_OK && rc!=ND_CONTROL_NOHOST &&
        rc!=ND_CONTROL_PENDING) DisplayBeep(p->screen);
    return 1;
}

static void select_type(struct AddItemPage *p,int type)
{
    if (type==p->type) return;
    capture_fields(p);p->type=type;p->reopen=1;
}

static void handle_add_gadget(struct AddItemPage *p,UWORD id,UWORD code)
{
    switch (id) {
    case A_TYPE_APP: select_type(p,ADD_APP);break;
    case A_TYPE_DRAWER: select_type(p,ADD_DRAWER);break;
    case A_TYPE_PLUGIN: select_type(p,ADD_PLUGIN);break;
    case A_TYPE_SPACER: select_type(p,ADD_SPACER);break;
    case A_APP_KIND:
        select_type(p,code==1 ? ADD_COMMAND : code==2 ? ADD_GROUP :
                      code==3 ? ADD_SCRIPT : ADD_APP);
        break;
    case A_TARGET_BROWSE: choose_file(p,0);break;
    case A_ICON_BROWSE: choose_file(p,1);break;
    case A_PLUGIN_LIST:
        if ((int)code<p->plugin_count) {
            p->plugin_selected=(int)code;
            GT_SetGadgetAttrs(p->plugin_list,p->win,NULL,
                              GTLV_Selected,(ULONG)p->plugin_selected,
                              GTLV_MakeVisible,(ULONG)p->plugin_selected,
                              TAG_DONE);
            set_plugin_details(p);
        }
        break;
    case A_ADD: commit_add(p,1);break;
    case A_CANCEL:p->done=1;p->result=0;break;
    default: break;
    }
}

int nd_additem_prefs_run(int argc,char **argv)
{
    struct AddItemPage *p;
    struct IntuiMessage *msg;
    ULONG cls;
    UWORD id,code;
    int result=20;
    int a;
    p=(struct AddItemPage *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 20;
    /* PLUGIN=ID (from the Plugin Manager) opens on that plugin. */
    for (a=1;a<argc;++a)
        if (!strncmp(argv[a],"PLUGIN=",7) && argv[a][7]) {
            strncpy(p->preselect,argv[a]+7,sizeof(p->preselect)-1);
            p->type=ADD_PLUGIN;
        }
    strcpy(p->names[ADD_APP],"New App");strcpy(p->names[ADD_DRAWER],"New Drawer");
    strcpy(p->names[ADD_GROUP],"Applications");
    strcpy(p->names[ADD_SPACER],"Spacer");strcpy(p->names[ADD_COMMAND],"Command");
    strcpy(p->command_interval,"60");strcpy(p->command_line,"1");
    strcpy(p->names[ADD_SCRIPT],"Script");
    strcpy(p->script,"NovaDock:Rexx/ScriptTile.rexx");
    strcpy(p->script_interval,"30");strcpy(p->script_timeout,"30");
    p->spacer_width=0;p->spacer_line=1;p->spacer_thickness=2;
    p->plugin_selected=-1;
    p->cfg=ND_PREFS_SHARED_CONFIG;
    if (!p->cfg) p->cfg=(struct NDConfig *)AllocVec(sizeof(*p->cfg),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p->cfg) goto done;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    AslBase=OpenLibrary("asl.library",37);
    if (!IntuitionBase || !GfxBase || !GadToolsBase) goto done;
    if (!ND_PREFS_SHARED_CONFIG && nd_load_effective(p->cfg)>0) goto done;
    p->screen=LockPubScreen("Workbench");
    if (!p->screen) goto done;
    while (!p->done) {
        p->reopen=0;
        if (!open_add_ui(p)) goto done;
        while (!p->done && !p->reopen) {
            Wait(1UL<<p->win->UserPort->mp_SigBit);
            while ((msg=GT_GetIMsg(p->win->UserPort))!=NULL) {
                cls=msg->Class;
                id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID : 0;
                code=msg->Code;
                GT_ReplyIMsg(msg);
                if (cls==IDCMP_CLOSEWINDOW) {p->done=1;p->result=0;}
                else if (cls==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(p->screen);
                else if (cls==IDCMP_REFRESHWINDOW) {
                    GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);
                } else if (cls==IDCMP_GADGETUP) handle_add_gadget(p,id,code);
            }
        }
        close_add_ui(p);
    }
    result=p->result;
done:
    close_add_ui(p);
    if (p->screen) UnlockPubScreen(NULL,p->screen);
    if (AslBase) CloseLibrary(AslBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (p->cfg && !ND_PREFS_SHARED_CONFIG) FreeVec(p->cfg);
    FreeVec(p);
    return result;
}

#ifndef ND_PREFS_EMBEDDED
int main(int argc,char **argv) { return nd_additem_prefs_run(argc,argv); }
#endif
