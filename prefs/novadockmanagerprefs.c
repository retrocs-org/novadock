/* Installed-plugin manager for the unified NovaDock Preferences app.
 *
 * Informational: one row per installed plugin with its version and dock
 * status, and details (version, author, capabilities, program, description)
 * for the selected one. Add and settings belong to the dock's right-click
 * menus. Import installs a third-party .ndplugin package; Remove uninstalls
 * a third-party plugin that has no tiles in the dock. */
#define IntuitionBase NDManagerIntuitionBase
#define GadToolsBase NDManagerGadToolsBase
#define AslBase NDManagerAslBase
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_control_client.h"
#include "nd_prefs_layout.h"
#include "nd_prefs_nav.h"
#include "nd_prefs_session.h"
#include "nd_manifest.h"
#include "nd_options.h"
#include "nd_plugin_discovery.h"
#include "nd_package_install.h"
#include "nd_prefs_help.h"

#define M_GENERAL 601
#define M_APPEARANCE 602
#define M_ORGANIZE 603
#define M_MANAGER 604
#define M_DIAGNOSTICS 605
#define M_ABOUT 606
#define M_LIST 607
#define M_ADD 608
#define M_SETTINGS 610
#define M_USE 612
#define M_SAVE 613
#define M_IMPORT 614
#define M_REMOVE 615
#define M_DETAIL 620

#define D_VERSION 0
#define D_AUTHOR 1
#define D_PROVIDES 2
#define D_DOCK 3
#define D_PROGRAM 4
#define D_ABOUT 5
#define D_ABOUT2 6
#define D_LINES 7
#define D_TEXT 100

struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct Library *AslBase=NULL;

#define PACKAGE_FILES 3 /* program, optional icon, manifest (last) */

struct ManagerPage {
    struct NDConfig *cfg,*owned;
    struct Screen *screen;
    struct Window *win;
    struct Gadget *gadgets,*last,*list,*remove,*detail[D_LINES];
    APTR vi;
    struct NDPrefsLayout layout;
    struct List rows;
    struct Node nodes[ND_MAX_ITEMS];
    char ids[ND_MAX_ITEMS][20];
    char names[ND_MAX_ITEMS][ND_LABEL_LEN];
    char text[ND_MAX_ITEMS][64];
    struct NDPluginSummary installed[ND_PLUGIN_CATALOG_MAX];
    int installed_count;
    struct NDPluginCatalogEntry entry;   /* full manifest of the selection */
    struct NDPluginCatalogEntry other;   /* import: installed plugin with the same ID */
    struct NDPackageFile files[PACKAGE_FILES];
    char detail_text[D_LINES][D_TEXT];
    char message[120];
    int count,selected,done,result,can_add,can_remove,has_settings;
    char *output;
    int output_size;
};

static void m_field(struct ManagerPage *p,struct NewGadget *ng,
                    int x,int y,int w,int h,int id,STRPTR label)
{
    memset(ng,0,sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&p->layout,h);
    ng->ng_GadgetID=id;ng->ng_GadgetText=label;
    ng->ng_TextAttr=p->screen->Font;ng->ng_VisualInfo=p->vi;
    ng->ng_Flags=PLACETEXT_LEFT;
}

static int m_button(struct ManagerPage *p,int x,int y,int w,int id,
                    const char *label)
{
    struct NewGadget ng;
    m_field(p,&ng,x,y,w,16,id,(STRPTR)label);
    ng.ng_Flags=PLACETEXT_IN;
    ng.ng_Width=nd_prefs_layout_button_width(&p->layout,w,label);
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
        GA_Disabled,(ULONG)(label[0]=='>'),TAG_DONE);
    return p->last!=NULL;
}

static const struct NDPluginSummary *installed_summary(struct ManagerPage *p,
                                                      const char *id)
{
    int i;
    for (i=0;i<p->installed_count;++i)
        if (!strcmp(p->installed[i].id,id)) return &p->installed[i];
    return NULL;
}

/* Tiles of plugin ID in the dock (enabled entries), and whether its first
 * definition allows several. -1 = not defined in the configuration. */
static int dock_tiles(struct ManagerPage *p,const char *id,int *multiple,
                      const char **command)
{
    int i,tiles=0,defined=0;
    for (i=0;i<p->cfg->count;++i) {
        const struct NDItem *item=&p->cfg->items[i];
        if (item->kind!=ND_ITEM_PLUGIN || strcmp(item->id,id)) continue;
        if (!defined) {
            if (multiple) *multiple=item->multiple_instances;
            if (command) *command=item->command;
        }
        defined=1;
        if (item->enabled) ++tiles;
    }
    return defined ? tiles : -1;
}

static const char *plugin_version(struct ManagerPage *p,const char *id)
{
    static struct NDManifest builtin;
    const struct NDPluginSummary *s;
    if (nd_manifest_builtin(id,&builtin)) return builtin.version;
    s=installed_summary(p,id);
    return s && s->version[0] ? s->version : "-";
}

static const char *tile_status(int tiles,char *buffer)
{
    if (tiles<=0) return "not in dock";
    if (tiles==1) return "in dock";
    sprintf(buffer,"%d tiles",tiles);
    return buffer;
}

static void add_row(struct ManagerPage *p,const char *id,const char *name,
                    int tiles)
{
    char status[16];
    int r=p->count;
    if (r>=ND_MAX_ITEMS) return;
    strncpy(p->ids[r],id,sizeof(p->ids[r])-1);p->ids[r][sizeof(p->ids[r])-1]=0;
    strncpy(p->names[r],name,sizeof(p->names[r])-1);
    p->names[r][sizeof(p->names[r])-1]=0;
    sprintf(p->text[r],"%-20.20s %-8.8s %s",name,plugin_version(p,id),
            tile_status(tiles,status));
    memset(&p->nodes[r],0,sizeof(p->nodes[r]));
    p->nodes[r].ln_Name=p->text[r];
    AddTail(&p->rows,&p->nodes[r]);++p->count;
}

/* Every plugin defined in the configuration (command tiles excluded; they
 * have their own Add Item type), then installed plugins not defined yet. */
static void build_rows(struct ManagerPage *p)
{
    int i,j,seen;
    NewList(&p->rows);p->count=0;
    for (i=0;i<p->cfg->count;++i) {
        const struct NDItem *item=&p->cfg->items[i];
        if (item->kind!=ND_ITEM_PLUGIN || !strcmp(item->id,"ROADSHOW") || !item->id[0] ||
            nd_command_plugin_path(item->command) ||
            nd_script_plugin_path(item->command)) continue;
        for (seen=0,j=0;j<p->count;++j)
            if (!strcmp(p->ids[j],item->id)) {seen=1;break;}
        if (seen) continue;
        add_row(p,item->id,item->label[0] ? item->label : item->id,
                dock_tiles(p,item->id,NULL,NULL));
    }
    for (i=0;i<p->installed_count;++i) {
        const struct NDPluginSummary *s=&p->installed[i];
        if (!strcmp(s->id,"ROADSHOW") || nd_command_plugin_path(s->command) ||
            nd_script_plugin_path(s->command)) continue;
        for (seen=0,j=0;j<p->count;++j)
            if (!strcmp(p->ids[j],s->id)) {seen=1;break;}
        if (!seen) add_row(p,s->id,s->name,-1);
    }
    if (p->selected>=p->count) p->selected=p->count-1;
    if (p->selected<0 && p->count) p->selected=0;
}

/* Split TEXT into two lines of at most WIDTH characters at a space. */
static void wrap_two(const char *text,char *one,char *two,int width)
{
    int len=(int)strlen(text),cut=len;
    if (len>width) {
        for (cut=width;cut>0 && text[cut]!=' ';--cut) {}
        if (!cut) cut=width;
    }
    strncpy(one,text,(size_t)cut);one[cut]=0;
    while (text[cut]==' ') ++cut;
    strncpy(two,text+cut,(size_t)width);two[width]=0;
}

static void show_details(struct ManagerPage *p)
{
    struct NDManifest *m=&p->entry.manifest;
    const char *id,*command="";
    char status[16];
    int i,tiles,multiple=0,have=0;
    struct NDOptionDef def;
    for (i=0;i<D_LINES;++i) p->detail_text[i][0]=0;
    p->can_add=0;p->can_remove=0;p->has_settings=0;
    if (p->selected>=0 && p->selected<p->count) {
        id=p->ids[p->selected];
        /* Bundled plugins describe themselves from the built-in table;
         * third-party ones from their installed manifest. */
        if (nd_manifest_builtin(id,m)) have=1;
        else if (nd_plugin_catalog_lookup(id,&p->entry)) {
            have=1;command=p->entry.command;
        } else nd_manifest_init(m,p->names[p->selected]);
        tiles=dock_tiles(p,id,&multiple,&command);
        if (tiles<0) multiple=m->multiple_instances;
        p->has_settings=tiles>=0 ? nd_option_collect(p->cfg,id,&def,1)>0 :
                                   m->option_count>0;
        p->can_add=tiles<=0 || multiple;
        /* Bundled plugins are part of NovaDock; a plugin with tiles in the
         * dock must lose them first. */
        p->can_remove=!nd_manifest_bundled_id(id) && tiles<=0;
        if (have) {
            sprintf(p->detail_text[D_VERSION],"%.15s%s%.23s%s",
                    m->version[0] ? m->version : "unknown",
                    m->license[0] ? " (" : "",m->license,
                    m->license[0] ? ")" : "");
            strncpy(p->detail_text[D_AUTHOR],m->author[0] ? m->author : "unknown",
                    D_TEXT-1);
        } else {
            strcpy(p->detail_text[D_VERSION],"not installed");
            strcpy(p->detail_text[D_AUTHOR],"unknown");
        }
        nd_manifest_describe(m->features,multiple,p->has_settings,
                             p->detail_text[D_PROVIDES],D_TEXT);
        sprintf(p->detail_text[D_DOCK],"%s (ID %.19s)",
                tile_status(tiles,status),id);
        strncpy(p->detail_text[D_PROGRAM],command,D_TEXT-1);
        wrap_two(m->description[0] ? m->description : "No description.",
                 p->detail_text[D_ABOUT],p->detail_text[D_ABOUT2],46);
    }
    if (!p->win) return;
    for (i=0;i<D_LINES;++i)
        GT_SetGadgetAttrs(p->detail[i],p->win,NULL,
                          GTTX_Text,(ULONG)p->detail_text[i],TAG_DONE);
    GT_SetGadgetAttrs(p->remove,p->win,NULL,
                      GA_Disabled,(ULONG)(p->can_remove ? FALSE : TRUE),TAG_DONE);
}

static void say(struct ManagerPage *p,const char *text,int beep)
{
    if (text!=p->message) {
        strncpy(p->message,text,sizeof(p->message)-1);
        p->message[sizeof(p->message)-1]=0;
    }
    if (p->win) SetWindowTitles(p->win,(STRPTR)p->message,(STRPTR)-1);
    if (beep) DisplayBeep(p->screen);
}

static void rescan(struct ManagerPage *p,const char *select_id)
{
    int i;
    p->installed_count=nd_plugin_catalog_summaries(p->installed,
                                                   ND_PLUGIN_CATALOG_MAX);
    GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,(ULONG)~0,TAG_DONE);
    build_rows(p);
    if (select_id)
        for (i=0;i<p->count;++i) if (!strcmp(p->ids[i],select_id)) p->selected=i;
    GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,(ULONG)&p->rows,
        GTLV_Selected,(ULONG)(p->selected<0 ? ~0UL : (ULONG)p->selected),
        GTLV_MakeVisible,(ULONG)(p->selected<0 ? 0 : p->selected),TAG_DONE);
    show_details(p);
}

/* AmigaDOS file names compare without regard to case. */
static int same_name(const char *a,const char *b)
{
    while (*a && *b) {
        char x=*a++,y=*b++;
        if (x>='A' && x<='Z') x=(char)(x-'A'+'a');
        if (y>='A' && y<='Z') y=(char)(y-'A'+'a');
        if (x!=y) return 0;
    }
    return *a==*b;
}

static const char *file_part(const char *path)
{
    const char *p,*base=path;
    for (p=path;*p;++p) if (*p==':' || *p=='/') base=p+1;
    return base;
}

static void import_plugin(struct ManagerPage *p)
{
    struct FileRequester *fr;
    struct NDManifest *m=&p->entry.manifest;
    char drawer[256],manifest_file[108],source[300],install[256],why[80];
    char id[20];
    int n=0,tiles,result,i,owns_destination=0;
    if (!AslBase) {say(p,"NovaDock Plugins - file requester unavailable",1);return;}
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)"Select a plugin's .ndplugin manifest",
        ASLFR_Window,(ULONG)p->win,ASLFR_SleepWindow,TRUE,
        ASLFR_DoPatterns,TRUE,ASLFR_InitialPattern,(ULONG)"#?.ndplugin",TAG_DONE);
    if (!fr) return;
    if (!AslRequest(fr,NULL) || !fr->fr_File[0]) {FreeAslRequest(fr);return;}
    strncpy(drawer,fr->fr_Drawer,sizeof(drawer)-1);drawer[sizeof(drawer)-1]=0;
    strncpy(manifest_file,fr->fr_File,sizeof(manifest_file)-1);
    manifest_file[sizeof(manifest_file)-1]=0;
    FreeAslRequest(fr);
    memset(&p->entry,0,sizeof(p->entry));
    nd_manifest_init(m,manifest_file);
    if (!nd_package_join(source,sizeof(source),drawer,manifest_file) ||
        !nd_manifest_read_file(source,m)) {
        say(p,m->error[0] ? m->error : "NovaDock Plugins - manifest cannot be read",1);
        return;
    }
    if (!nd_package_check(m,manifest_file,why,sizeof(why))) {
        sprintf(p->message,"Not imported: %.90s",why);say(p,p->message,1);return;
    }
    strcpy(id,m->id);
    if (!nd_plugin_install_drawer(install,sizeof(install))) {
        say(p,"Not imported: no NovaDock Plugins drawer was found",1);return;
    }
    /* Same ID, different program: a different plugin owns that ID. */
    if (nd_plugin_catalog_lookup(id,&p->other) &&
        !same_name(file_part(p->other.command),m->program)) {
        sprintf(p->message,"Not imported: ID %.19s belongs to %.40s",id,
                p->other.manifest.name);
        say(p,p->message,1);return;
    }
    tiles=dock_tiles(p,id,NULL,NULL);
    if (tiles>0) {
        say(p,"Not imported: remove this plugin's tiles from the dock first",1);
        return;
    }
    /* Only the installed manifest at this destination can authorize an update.
     * A different ID or an unowned executable must never be overwritten. */
    if (!nd_package_join(source,sizeof(source),install,manifest_file)) return;
    if (nd_package_file_exists(source)) {
        if (!nd_manifest_read_file(source,&p->other.manifest) ||
            strcmp(p->other.manifest.id,id) ||
            !same_name(p->other.manifest.program,m->program)) {
            say(p,"Not imported: destination belongs to another or invalid package",1);return;
        }
        owns_destination=1;
    }
    /* Program, optional icon, then the manifest. */
    if (!nd_package_join(p->files[n].source,ND_PACKAGE_PATH,drawer,m->program) ||
        !nd_package_join(p->files[n].destination,ND_PACKAGE_PATH,install,m->program) ||
        !nd_package_file_exists(p->files[n].source)) {
        say(p,"Not imported: the program is not beside its manifest",1);return;
    }
    ++n;
    if (strlen(m->program)+5<sizeof(why)) {
        sprintf(why,"%s.info",m->program);
        if (nd_package_join(p->files[n].source,ND_PACKAGE_PATH,drawer,why) &&
            nd_package_file_exists(p->files[n].source) &&
            nd_package_join(p->files[n].destination,ND_PACKAGE_PATH,install,why)) ++n;
    }
    if (!nd_package_join(p->files[n].source,ND_PACKAGE_PATH,drawer,manifest_file) ||
        !nd_package_join(p->files[n].destination,ND_PACKAGE_PATH,install,manifest_file)) {
        say(p,"Not imported: path too long",1);return;
    }
    ++n;
    for (i=0;i<n;++i) if (!owns_destination &&
        nd_package_file_exists(p->files[i].destination)) {
        say(p,"Not imported: destination file is already owned or unrecognized",1);return;
    }
    {
        struct EasyStruct es={sizeof(es),0,(STRPTR)"Import plugin",
            (STRPTR)"%s %s\nID: %s\nDestination: %s\n%s",
            (STRPTR)"Import|Cancel"};
        ULONG args[5];
        args[0]=(ULONG)m->name;args[1]=(ULONG)m->version;
        args[2]=(ULONG)id;args[3]=(ULONG)p->files[0].destination;
        args[4]=(ULONG)(owns_destination ? "Update this installed package" : "Install a new package");
        if (EasyRequestArgs(p->win,&es,NULL,args)!=1) return;
    }
    result=nd_package_install(p->files,n);
    if (result!=ND_PACKAGE_OK) {
        if (result==ND_PACKAGE_RECOVERY) {
            for (i=0;i<n;++i) if (p->files[i].recovery) {
                struct EasyStruct es={sizeof(es),0,(STRPTR)"Plugin recovery required",
                    (STRPTR)"Import stopped. Recovery file:\n%s\n%s",
                    (STRPTR)"OK"};
                ULONG args[2];
                args[0]=(ULONG)(p->files[i].recovery==2 ? p->files[i].destination : p->files[i].backup);
                args[1]=(ULONG)(p->files[i].recovery==2 ?
                    "Remove the incomplete destination before retrying." :
                    "Preserve and restore this backup before retrying.");
                EasyRequestArgs(p->win,&es,NULL,args);
            }
            say(p,"Import stopped - recovery backups retained",1);
        } else say(p,"Not imported: copying into the Plugins drawer failed",1);
        return;
    }
    /* Known to this session at once (disabled until added to the dock);
     * stored with the next Use or Save. */
    (void)nd_manifest_merge(p->cfg,m,p->files[0].destination);
    ND_PREFS_MARK_DIRTY();
    rescan(p,id);
    sprintf(p->message,"Imported %.30s - right-click the dock to add it",m->name);
    say(p,p->message,0);
}

static void remove_plugin(struct ManagerPage *p)
{
    struct EasyStruct es;
    const char *id;
    char info[310];
    int installed;
    if (p->selected<0 || p->selected>=p->count || !p->can_remove) return;
    id=p->ids[p->selected];
    installed=nd_plugin_catalog_lookup(id,&p->entry);
    memset(&es,0,sizeof(es));
    es.es_StructSize=sizeof(es);
    es.es_Title=(UBYTE *)"NovaDock Plugins";
    es.es_TextFormat=(UBYTE *)(installed ?
        "Remove the plugin %s?\n\nIts files are deleted from\n%s" :
        "Forget the plugin %s?\n\nIts files are no longer installed.%s");
    es.es_GadgetFormat=(UBYTE *)"Remove|Cancel";
    {
        ULONG args[2];
        args[0]=(ULONG)p->names[p->selected];
        args[1]=(ULONG)(installed ? p->entry.command : "");
        if (EasyRequestArgs(p->win,&es,NULL,args)!=1) return;
    }
    if (installed) {
        /* The manifest goes first: a plugin that is only partly removed
         * is then never rediscovered. */
        if (p->entry.manifest_path[0] &&
            !DeleteFile((STRPTR)p->entry.manifest_path)) {
            say(p,"Not removed: its manifest could not be deleted",1);return;
        }
        if ((int)strlen(p->entry.command)+6<(int)sizeof(info)) {
            sprintf(info,"%s.info",p->entry.command);
            if (!p->entry.manifest_path[0] && !DeleteFile((STRPTR)info)) {
                say(p,"Not removed: its icon could not be deleted",1);return;
            }
            DeleteFile((STRPTR)p->entry.command);
            DeleteFile((STRPTR)info);
        }
    }
    if (!nd_remove_plugin(p->cfg,id)) {
        say(p,"Not removed: it still has tiles in the dock",1);return;
    }
    ND_PREFS_MARK_DIRTY();
    sprintf(p->message,"Removed %.40s - Use or Save keeps the change",
            p->names[p->selected]);
    rescan(p,NULL);
    say(p,p->message,0);
}

static int commit(struct ManagerPage *p,int permanent)
{
    LONG rc;
    if (permanent && !nd_control_write_config(p->cfg,"ENVARC:NovaDock/novadock.cfg"))
        return 0;
    if (!nd_control_write_config(p->cfg,"ENV:NovaDock/novadock.cfg")) return 0;
    rc=nd_control_reload();
    if (rc!=ND_CONTROL_OK && rc!=ND_CONTROL_PENDING) return 0;
    if (permanent) ND_PREFS_MARK_CLEAN();
    else ND_PREFS_MARK_PREVIEW();
    return 1;
}

static struct Gadget *detail_line(struct ManagerPage *p,int y,int index,
                                  const char *label)
{
    struct NewGadget ng;
    m_field(p,&ng,232,y,373,11,M_DETAIL+index,(STRPTR)label);
    p->last=CreateGadget(TEXT_KIND,p->last,&ng,
        GTTX_Text,(ULONG)p->detail_text[index],GTTX_Clipped,TRUE,TAG_DONE);
    return p->detail[index]=p->last;
}

static int open_manager(struct ManagerPage *p)
{
    struct NewGadget ng;
    int left,top,width,height,font_x=8,font_y=8;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,p->screen->Width,
        p->screen->Height,p->screen->BarHeight,620,240);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    left=nd_prefs_layout_left(&p->layout);top=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfo(p->screen,TAG_DONE);
    if (!p->vi || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;
    if (!m_button(p,10,21,118,M_GENERAL,"General") ||
        !m_button(p,10,45,118,M_APPEARANCE,"Appearance") ||
        !m_button(p,10,69,118,M_ORGANIZE,"Organize") ||
        !m_button(p,10,93,118,M_MANAGER,"> Plugins") ||
        !m_button(p,10,117,118,M_DIAGNOSTICS,"Diagnostics") ||
        !m_button(p,10,141,118,M_ABOUT,"About")) return 0;
    m_field(p,&ng,150,21,455,60,M_LIST,NULL);
    p->list=p->last=CreateGadget(LISTVIEW_KIND,p->last,&ng,
        GTLV_Labels,(ULONG)&p->rows,
        GTLV_Selected,(ULONG)(p->selected<0 ? ~0UL : (ULONG)p->selected),
        GTLV_ShowSelected,NULL,TAG_DONE);
    if (!p->last ||
        !detail_line(p,87,D_VERSION,"Version:") ||
        !detail_line(p,99,D_AUTHOR,"Author:") ||
        !detail_line(p,111,D_PROVIDES,"Provides:") ||
        !detail_line(p,123,D_DOCK,"Dock:") ||
        !detail_line(p,135,D_PROGRAM,"Program:") ||
        !detail_line(p,147,D_ABOUT,"About:") ||
        !detail_line(p,159,D_ABOUT2,NULL)) return 0;
    if (!m_button(p,386,181,100,M_IMPORT,"Import...") ||
        !m_button(p,492,181,100,M_REMOVE,"Remove...")) return 0;
    p->remove=p->last;
    if (!m_button(p,414,205,90,M_USE,"Use") ||
        !m_button(p,510,205,90,M_SAVE,"Save")) return 0;
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,left,WA_Top,top,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)"NovaDock Preferences - Plugins",
        WA_Gadgets,(ULONG)p->gadgets,WA_DragBar,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,WA_DepthGadget,TRUE,
        WA_IDCMP,LISTVIEWIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;
    GT_RefreshWindow(p->win,NULL);
    show_details(p);
    return 1;
}

int nd_pluginmanager_prefs_run(int argc,char **argv,char *plugin,int plugin_size)
{
    struct ManagerPage *p;struct IntuiMessage *msg;
    ULONG cls;UWORD id;LONG code;int result=20,i;
    (void)argc;(void)argv;
    p=(struct ManagerPage *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 20;
    p->cfg=ND_PREFS_SHARED_CONFIG;
    if (!p->cfg) {
        /* Standalone: load our own copy (0 means loaded successfully). */
        p->owned=(struct NDConfig *)AllocVec(sizeof(*p->owned),MEMF_PUBLIC);
        if (!p->owned || nd_load_effective(p->owned)>0) goto done;
        p->cfg=p->owned;
    }
    p->installed_count=nd_plugin_catalog_summaries(p->installed,
                                                   ND_PLUGIN_CATALOG_MAX);
    p->selected=-1;p->output=plugin;p->output_size=plugin_size;
    /* Return to the plugin that was being looked at. */
    build_rows(p);
    if (plugin && plugin[0])
        for (i=0;i<p->count;++i) if (!strcmp(p->ids[i],plugin)) p->selected=i;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    AslBase=OpenLibrary("asl.library",37); /* optional: Import... only */
    if (!IntuitionBase || !GadToolsBase) goto done;
    p->screen=LockPubScreen("Workbench");
    if (!p->screen || !open_manager(p)) goto done;
    while (!p->done) {
        Wait(1UL<<p->win->UserPort->mp_SigBit);
        while ((msg=GT_GetIMsg(p->win->UserPort))!=NULL) {
            cls=msg->Class;code=msg->Code;
            id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(msg);
            if (cls==IDCMP_CLOSEWINDOW) {p->done=1;p->result=0;}
            else if (cls==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);
            } else if (cls==IDCMP_GADGETUP) {
                if (id==M_LIST) {
                    if ((int)code<p->count) {p->selected=(int)code;show_details(p);}
                } else if (id==M_IMPORT) import_plugin(p);
                else if (id==M_REMOVE) remove_plugin(p);
                else if (id==M_USE) {
                    if (commit(p,0)) {p->done=1;p->result=0;}
                    else DisplayBeep(p->screen);
                } else if (id==M_SAVE) {
                    if (commit(p,1)) {p->done=1;p->result=0;}
                    else DisplayBeep(p->screen);
                } else if (id==M_GENERAL) {p->done=1;p->result=ND_PREF_NAV_GENERAL;}
                else if (id==M_APPEARANCE) {p->done=1;p->result=ND_PREF_NAV_APPEARANCE;}
                else if (id==M_ORGANIZE) {p->done=1;p->result=ND_PREF_NAV_ORGANIZE;}
                else if (id==M_DIAGNOSTICS) {p->done=1;p->result=ND_PREF_NAV_DIAGNOSTICS;}
                else if (id==M_ABOUT) {p->done=1;p->result=ND_PREF_NAV_ABOUT;}
            }
        }
    }
    result=p->result;
done:
    if (p->win) CloseWindow(p->win);
    if (p->gadgets) FreeGadgets(p->gadgets);
    if (p->vi) FreeVisualInfo(p->vi);
    if (p->screen) UnlockPubScreen(NULL,p->screen);
    if (AslBase) CloseLibrary(AslBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (p->owned) FreeVec(p->owned);
    FreeVec(p);AslBase=NULL;GadToolsBase=NULL;IntuitionBase=NULL;return result;
}
