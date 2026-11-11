/* Diagnostics section for live telemetry, counter reset and text snapshots. */
#define IntuitionBase NDDiagnosticsIntuitionBase
#define GadToolsBase NDDiagnosticsGadToolsBase
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_version.h"
#include "nd_control_client.h"
#include "nd_prefs_nav.h"
#include "nd_prefs_session.h"
#include "nd_prefs_layout.h"
#include "nd_protocol.h"
#include "nd_prefs_help.h"

#define D_GENERAL 401
#define D_APPEARANCE 402
#define D_DOCKBAR 404
#define D_DIAGNOSTICS 405
#define D_LIST 406
#define D_REFRESH 407
#define D_CLOSE 408
#define D_RESET 409
#define D_SAVE_REPORT 410
#define D_ABOUT 411
#define D_PLUGIN_MANAGER 412
#define D_RECOVERY 413
#define D_LOW_RESOURCE 414
#define D_ROWS 72
#define D_REPORT_PATH "RAM:NovaDock-Diagnostics.txt"

struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;

struct DiagnosticsPage {
    struct NDConfig *cfg;
    struct Screen *screen;
    struct Window *win;
    APTR vi;
    struct Gadget *gadgets,*last,*list;
    struct NDPrefsLayout layout;
    struct List rows;
    struct Node nodes[D_ROWS];
    char text[D_ROWS][96];
    int count,done,result;
};

static void d_field(struct DiagnosticsPage *p,struct NewGadget *ng,
                    int x,int y,int w,int h,int id,STRPTR label)
{
    memset(ng,0,sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&p->layout,h);
    ng->ng_GadgetID=id;ng->ng_GadgetText=label;ng->ng_TextAttr=p->screen->Font;
    ng->ng_VisualInfo=p->vi;ng->ng_Flags=PLACETEXT_LEFT;
}
static int d_button(struct DiagnosticsPage *p,int x,int y,int w,int id,
                    STRPTR label)
{
    struct NewGadget ng;
    d_field(p,&ng,x,y,w,16,id,label);ng.ng_Flags=PLACETEXT_IN;
    ng.ng_Width=nd_prefs_layout_button_width(&p->layout,w,label);
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
        GA_Disabled,(ULONG)(label[0]=='>'),TAG_DONE);
    return p->last!=NULL;
}
static ULONG rate_tenths(ULONG count,ULONG ticks)
{
    ULONG seconds=ticks/ND_DIAGNOSTIC_TICKS_PER_SECOND;
    if (!seconds) seconds=1;
    if (count<=0xffffffffUL/10UL)
        return (count*10UL+seconds/2UL)/seconds;
    return (count/seconds)*10UL;
}
static void counter_text(char *text,ULONG count,ULONG ticks)
{
    ULONG rate=rate_tenths(count,ticks);
    sprintf(text,"%lu total (%lu.%lu/sec)",count,rate/10UL,rate%10UL);
}
static void age_text(char *text,ULONG ticks)
{
    ULONG seconds;
    if (ticks==ND_DIAGNOSTIC_AGE_UNKNOWN) {strcpy(text,"never");return;}
    seconds=ticks/ND_DIAGNOSTIC_TICKS_PER_SECOND;
    if (seconds<60UL) sprintf(text,"%lus",seconds);
    else if (seconds<3600UL) sprintf(text,"%lum %lus",seconds/60UL,seconds%60UL);
    else sprintf(text,"%luh %lum",seconds/3600UL,(seconds/60UL)%60UL);
}
static void add_row(struct DiagnosticsPage *p,const char *label,
                    const char *value)
{
    int i=p->count;
    if (i>=D_ROWS) return;
    /* The native 8-pixel Workbench font gives this list about 55 readable
       characters.  Keep every row inside the list rather than relying on a
       horizontal scroll bar or clipping the last words. */
    sprintf(p->text[i],"%-20s %.34s",label,value);
    memset(&p->nodes[i],0,sizeof(p->nodes[i]));
    p->nodes[i].ln_Name=p->text[i];
    AddTail(&p->rows,&p->nodes[i]);++p->count;
}
static void build_rows(struct DiagnosticsPage *p)
{
    struct NDDiagnosticsSnapshot snapshot;
    char value[70];
    char label[28],age[24];
    LONG status;
    int i,enabled_items=0;
    NewList(&p->rows);p->count=0;
    status=nd_control_diagnostics(&snapshot);
    if (status==ND_CONTROL_OK) {
        ULONG seconds=snapshot.uptime_ticks/ND_DIAGNOSTIC_TICKS_PER_SECOND;
        ULONG oldest=0;
        int have_oldest=0;
        add_row(p,"Dock runtime:","Running - live counters");
        if (seconds<3600UL)
            sprintf(value,"%lum %lus",seconds/60UL,seconds%60UL);
        else sprintf(value,"%luh %lum",seconds/3600UL,(seconds/60UL)%60UL);
        add_row(p,"Telemetry age:",value);
        counter_text(value,snapshot.wake_count,snapshot.uptime_ticks);
        add_row(p,"Event wakes:",value);
        counter_text(value,snapshot.paint_count,snapshot.uptime_ticks);
        add_row(p,"Full redraws:",value);
        counter_text(value,snapshot.tile_count,snapshot.uptime_ticks);
        add_row(p,"Tile redraws:",value);
        counter_text(value,snapshot.update_count,snapshot.uptime_ticks);
        add_row(p,"Plugin messages:",value);
        counter_text(value,snapshot.unchanged_updates,snapshot.uptime_ticks);
        add_row(p,"Unchanged IPC:",value);
        counter_text(value,snapshot.coalesced_updates,snapshot.uptime_ticks);
        add_row(p,"Coalesced paints:",value);
        sprintf(value,"%lu applied, %lu skipped, %lu failed",
                snapshot.reload_applied,snapshot.reload_skipped,
                snapshot.reload_failures);
        add_row(p,"Update outcomes:",value);
        if (snapshot.reload_requests) {
            ULONG average=snapshot.reload_total_ticks/snapshot.reload_requests;
            sprintf(value,"last %lu.%02lus; avg %lu.%02lus",
                    snapshot.reload_last_ticks/50UL,
                    (snapshot.reload_last_ticks%50UL)*2UL,
                    average/50UL,(average%50UL)*2UL);
        } else strcpy(value,"No update requested");
        add_row(p,"Update time:",value);
        if(snapshot.settings_telemetry_version){
            const char *mode=snapshot.update_mode==1 ? "Live: window reused" :
                snapshot.update_mode==2 ? "Window rebuilt" :
                snapshot.update_mode==3 ? "Unchanged: skipped" :
                snapshot.update_mode==4 ? "Update failed" : "No update yet";
            add_row(p,"Last update:",mode);
            sprintf(value,"%lu live / %lu rebuilt",snapshot.live_updates,snapshot.window_rebuilds);
            add_row(p,"Update paths:",value);
            add_row(p,"Settings save:",snapshot.save_state==2 ? "Writing in background" :
                snapshot.save_state==1 ? "Queued; waiting for idle" :
                snapshot.save_state==3 ? "Failed; changes kept for retry" : "Saved; no pending changes");
            sprintf(value,"%lu saved / %lu coalesced",snapshot.saves_completed,snapshot.saves_coalesced);
            add_row(p,"Save batches:",value);
            sprintf(value,"%lu failures",snapshot.saves_failed);add_row(p,"Save failures:",value);
            if(snapshot.saves_completed || snapshot.saves_failed){
                sprintf(value,"last %lu.%02lus",snapshot.save_last_ticks/50UL,
                        (snapshot.save_last_ticks%50UL)*2UL);
                add_row(p,"Save time:",value);
                sprintf(value,"%s %lu.%02lus",snapshot.save_env_ok ? "OK" : "FAILED",
                        snapshot.save_env_ticks/50UL,(snapshot.save_env_ticks%50UL)*2UL);
                add_row(p,"Last ENV write:",value);
                sprintf(value,"%s %lu.%02lus",snapshot.save_envarc_ok ? "OK" : "FAILED",
                        snapshot.save_envarc_ticks/50UL,(snapshot.save_envarc_ticks%50UL)*2UL);
                add_row(p,"Last ENVARC write:",value);
            }
        }else add_row(p,"Settings telemetry:","Unavailable in this dock build");
        if(snapshot.settings_telemetry_version>=2) {
            sprintf(value,"%u late replies reclaimed",(unsigned int)snapshot.reserved);
            add_row(p,"IPC recovery:",value);
        }
        /* Zero means a dock that predates the memory fields. */
        if (snapshot.mem_free) {
            sprintf(value,"%luK, chip %luK, max %luK",
                    snapshot.mem_free>>10,snapshot.mem_chip_free>>10,
                    snapshot.mem_largest>>10);
            add_row(p,"Free memory:",value);
        }
        if (snapshot.reload_largest_before) {
            sprintf(value,"max %luK -> %luK; low %luK",
                    snapshot.reload_largest_before>>10,
                    snapshot.reload_largest_after>>10,
                    snapshot.reload_free_low>>10);
            add_row(p,"Update memory:",value);
            sprintf(value,"%u reused, %u loaded",
                    (unsigned)snapshot.reload_icons_reused,
                    (unsigned)snapshot.reload_icons_loaded);
            add_row(p,"Update icons:",value);
        }
        sprintf(value,"%d online, %d offline, %d stale",
                (int)snapshot.online_count,
                (int)(snapshot.plugin_count-snapshot.online_count),
                (int)snapshot.stale_count);
        add_row(p,"Plugin state:",value);
        sprintf(value,"Active; %lu automatic restarts",
                snapshot.watchdog_restarts);
        add_row(p,"Plugin watchdog:",value);
        if (!snapshot.timer_active)
            sprintf(value,"%s; coalescing timer OFF",
                    snapshot.hidden ? "Hidden" : "Visible");
        else sprintf(value,"%s; adaptive timer %s",
                     snapshot.hidden ? "Hidden" : "Visible",
                     snapshot.timer_quanta>=ND_TIMER_IDLE_QUANTA ?
                     "idle 1Hz" : "responsive 5Hz");
        add_row(p,"Dock state:",value);
        add_row(p,"Plugin pacing:",snapshot.hidden ?
                "Hidden backoff (2-10 seconds)" : "Adaptive activity backoff");
        for (i=0;i<(int)snapshot.listed_plugins;++i) {
            struct NDPluginDiagnostics *plugin=&snapshot.plugins[i];
            if (plugin->online &&
                plugin->age_ticks!=ND_DIAGNOSTIC_AGE_UNKNOWN &&
                (!have_oldest || plugin->age_ticks>oldest)) {
                oldest=plugin->age_ticks;have_oldest=1;
            }
        }
        if (have_oldest) age_text(value,oldest);else strcpy(value,"No update yet");
        add_row(p,"Oldest update:",value);
        for (i=0;i<(int)snapshot.listed_plugins;++i) {
            struct NDPluginDiagnostics *plugin=&snapshot.plugins[i];
            ULONG rate=rate_tenths(plugin->updates,snapshot.uptime_ticks);
            age_text(age,plugin->age_ticks);
            sprintf(label,"%s plugin:",plugin->id[0] ? plugin->id : "?");
            if (!plugin->online)
                sprintf(value,"offline; %lu updates; R%lu",plugin->updates,
                        plugin->restarts);
            else sprintf(value,"%s %lu.%lu/sec; age %s; R%lu",
                         plugin->stale ? "STALE" : "online",
                         rate/10UL,rate%10UL,age,plugin->restarts);
            add_row(p,label,value);
        }
    } else if (status==ND_CONTROL_NOHOST)
        add_row(p,"Dock runtime:","Not running");
    else if (status==ND_CONTROL_TIMEOUT)
        add_row(p,"Dock runtime:","Request timed out after 5 seconds");
    else add_row(p,"Dock runtime:","Diagnostics request failed");
    add_row(p,(strstr(nd_effective_config_source(),".good") ||
               strstr(nd_effective_config_source(),".bak")) ?
            "Recovered config:" : "Active config:",
            nd_effective_config_source());
    /* By ID: imported themes have no built-in number. */
    sprintf(value,"%.20s / %s shape",nd_config_theme_id(p->cfg),
            nd_shape_name(p->cfg->shape));
    add_row(p,"Appearance:",value);
    sprintf(value,"%s / %s / labels %s",
            nd_position_name(p->cfg->position),nd_font_name(p->cfg->font_mode),
            nd_label_mode_name(p->cfg->label_mode));
    add_row(p,"Dock text:",value);
    add_row(p,"Running apps:",p->cfg->running_apps ?
            "Configured launchers: marks + click-to-front" : "Disabled");
    add_row(p,"App browser:","Optional Running Apps plugin; on-demand scan");
    sprintf(value,"%s, sorted by %s",p->cfg->folder_view ? "List" : "Icon grid",
            p->cfg->folder_sort==2 ? "size" : p->cfg->folder_sort==1 ? "date" : "name");
    add_row(p,"Drawer popups:",value);
    add_row(p,"Removable media:",p->cfg->removable_media ?
            "Temporary tiles for inserted disks" : "Disabled");
    add_row(p,"Refresh mode:",p->cfg->refresh_mode==ND_REFRESH_SMOOTH ?
            "Smooth (smart refresh)" : "Low memory (simple refresh)");
    sprintf(value,"%d%s",p->cfg->rows,
            (p->cfg->side==ND_SIDE_LEFT || p->cfg->side==ND_SIDE_RIGHT) ?
            " configured (horizontal only)" : " active");
    add_row(p,"Dock rows:",value);
    for (i=0;i<p->cfg->count;++i)
        if (p->cfg->items[i].enabled) ++enabled_items;
    sprintf(value,"%d enabled / %d set / %d max",
            enabled_items,p->cfg->count,ND_MAX_ITEMS);
    add_row(p,"Dock capacity:",value);
    add_row(p,"Graph time axis:","Samples; adaptive spacing, not fixed seconds");
    add_row(p,"CPU sampler:","Scout idle task means free CPU");
    sprintf(value,"%u-second bounded reply wait",
            (unsigned int)ND_PLUGIN_IPC_TIMEOUT_SECONDS);
    add_row(p,"Plugin IPC:",value);
    add_row(p,"Preferences:",ND_PREFS_IS_DIRTY() ?
            "Unsaved changes in this session" : "No pending changes");
}
static void refresh_rows(struct DiagnosticsPage *p)
{
    GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,(ULONG)~0UL,TAG_DONE);
    build_rows(p);
    GT_SetGadgetAttrs(p->list,p->win,NULL,GTLV_Labels,(ULONG)&p->rows,TAG_DONE);
}
static int save_report(struct DiagnosticsPage *p)
{
    FILE *f;
    int i,ok=1;
    f=fopen(D_REPORT_PATH,"w");
    if (!f) return 0;
    if (fprintf(f,"NovaDock " ND_VERSION " diagnostics snapshot\n\n")<0) ok=0;
    for (i=0;i<p->count;++i)
        if (fprintf(f,"%s\n",p->text[i])<0) ok=0;
    if (fclose(f)!=0) ok=0;
    return ok;
}
static void reset_counters(struct DiagnosticsPage *p)
{
    LONG status=nd_control_send(ND_CONTROL_DIAGNOSTICS_RESET);
    if (status==ND_CONTROL_OK) {
        refresh_rows(p);
        SetWindowTitles(p->win,(STRPTR)
            "NovaDock Diagnostics - counters reset",(STRPTR)-1);
    } else if (status==ND_CONTROL_TIMEOUT)
        SetWindowTitles(p->win,(STRPTR)
            "NovaDock Diagnostics - reset timed out",(STRPTR)-1);
    else if (status==ND_CONTROL_NOHOST)
        SetWindowTitles(p->win,(STRPTR)
            "NovaDock Diagnostics - dock is not running",(STRPTR)-1);
    else SetWindowTitles(p->win,(STRPTR)
            "NovaDock Diagnostics - reset not supported",(STRPTR)-1);
}
/* Recovery edits the Preferences session; Use/Save remains the commit step. */
static void recover_settings(struct DiagnosticsPage *p)
{
    struct EasyStruct storage={sizeof(storage),0,(STRPTR)"Recover settings",
        (STRPTR)"Choose the saved settings location to inspect.",
        (STRPTR)"Session (ENV)|Permanent (ENVARC)|Cancel"};
    struct EasyStruct generation={sizeof(generation),0,(STRPTR)"Recover settings",
        (STRPTR)"Choose a recovery generation. It will be validated before loading.",
        (STRPTR)"Known-good|Previous backup|Cancel"};
    struct EasyStruct confirm={sizeof(confirm),0,(STRPTR)"Recover settings",
        (STRPTR)"Validated: %s\nReplace the current Preferences session?\nUse or Save applies the recovered settings to the dock.",
        (STRPTR)"Load|Cancel"};
    struct NDConfig *copy;
    char path[96];ULONG args[1];int location,which;
    location=(int)EasyRequestArgs(p->win,&storage,NULL,NULL);
    if(location!=1 && location!=2)return;
    which=(int)EasyRequestArgs(p->win,&generation,NULL,NULL);
    if(which!=1 && which!=2)return;
    sprintf(path,"%s:NovaDock/novadock.cfg.%s",location==1 ? "ENV" : "ENVARC",
            which==1 ? "good" : "bak");
    copy=(struct NDConfig *)AllocVec(sizeof(*copy),MEMF_PUBLIC);
    if(!copy)return;
    if(nd_load_config(copy,path)!=0) {
        SetWindowTitles(p->win,(STRPTR)"Recovery file missing or invalid - session unchanged",(STRPTR)-1);
        FreeVec(copy);return;
    }
    args[0]=(ULONG)path;
    if(EasyRequestArgs(p->win,&confirm,NULL,args)==1) {
        nd_upgrade_network_config(copy);
        memcpy(p->cfg,copy,sizeof(*copy));ND_PREFS_MARK_DIRTY();refresh_rows(p);
        SetWindowTitles(p->win,(STRPTR)"Recovered settings in Preferences - Use or Save to apply",(STRPTR)-1);
    }
    FreeVec(copy);
}
static void low_resource(struct DiagnosticsPage *p)
{
    struct EasyStruct es={sizeof(es),0,(STRPTR)"Low-resource preset",
        (STRPTR)"Use simple refresh, list views and meters.\nDisable launcher scans and always-on-top.\nSample CPU/RAM/Network every 2 seconds.\nUse or Save applies these Preferences changes.",
        (STRPTR)"Apply preset|Cancel"};
    if(EasyRequestArgs(p->win,&es,NULL,NULL)!=1)return;
    if(!nd_config_low_resource(p->cfg)) {
        SetWindowTitles(p->win,(STRPTR)"Preset could not be applied - session unchanged",(STRPTR)-1);return;
    }
    ND_PREFS_MARK_DIRTY();refresh_rows(p);
}
static int open_page(struct DiagnosticsPage *p)
{
    struct NewGadget ng;
    int x,y,width,height,font_x=8,font_y=8;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,
        p->screen->Width,p->screen->Height,p->screen->BarHeight,620,207);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    x=nd_prefs_layout_left(&p->layout);y=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfo(p->screen,TAG_DONE);
    if (!p->vi || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;build_rows(p);
    if (!d_button(p,10,21,118,D_GENERAL,"General") ||
        !d_button(p,10,45,118,D_APPEARANCE,"Appearance") ||
        !d_button(p,10,69,118,D_DOCKBAR,"Organize") ||
        !d_button(p,10,93,118,D_PLUGIN_MANAGER,"Plugins") ||
        !d_button(p,10,117,118,D_DIAGNOSTICS,"> Diagnostics") ||
        !d_button(p,10,141,118,D_ABOUT,"About")) return 0;
    d_field(p,&ng,145,24,460,112,D_LIST,"Runtime performance");
    ng.ng_Flags=PLACETEXT_ABOVE;
    p->list=p->last=CreateGadget(LISTVIEW_KIND,p->last,&ng,
        GTLV_Labels,(ULONG)&p->rows,GTLV_ReadOnly,TRUE,TAG_DONE);
    if (!p->last || !d_button(p,145,147,220,D_RECOVERY,"Recover settings") ||
        !d_button(p,375,147,230,D_LOW_RESOURCE,"Low-resource preset") ||
        !d_button(p,229,175,80,D_RESET,"Reset") ||
        !d_button(p,315,175,106,D_SAVE_REPORT,"Save report") ||
        !d_button(p,427,175,86,D_REFRESH,"Refresh") ||
        !d_button(p,519,175,86,D_CLOSE,"Close")) return 0;
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,x,WA_Top,y,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)(ND_PREFS_IS_DIRTY() ?
            "NovaDock Preferences - Diagnostics *" :
            "NovaDock Preferences - Diagnostics"),
        WA_Gadgets,(ULONG)p->gadgets,WA_DragBar,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,WA_DepthGadget,TRUE,
        WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;GT_RefreshWindow(p->win,NULL);return 1;
}
int nd_diagnostics_prefs_run(int argc,char **argv)
{
    struct DiagnosticsPage *p;struct IntuiMessage *msg;
    ULONG cls;UWORD id,rawcode;int result=20;
    (void)argc;(void)argv;
    p=(struct DiagnosticsPage *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 20;
    p->cfg=ND_PREFS_SHARED_CONFIG;
    if (!p->cfg) goto done;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    if (!IntuitionBase || !GadToolsBase) goto done;
    p->screen=LockPubScreen("Workbench");
    if (!p->screen || !open_page(p)) goto done;
    while (!p->done) {
        Wait(1UL<<p->win->UserPort->mp_SigBit);
        while ((msg=GT_GetIMsg(p->win->UserPort))!=NULL) {
            cls=msg->Class;rawcode=msg->Code;
            id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(msg);
            if (cls==IDCMP_CLOSEWINDOW) {p->done=1;p->result=0;}
            else if (cls==IDCMP_RAWKEY && rawcode==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);
            } else if (cls==IDCMP_GADGETUP) {
                if (id==D_GENERAL) {p->done=1;p->result=ND_PREF_NAV_GENERAL;}
                else if (id==D_APPEARANCE) {p->done=1;p->result=ND_PREF_NAV_APPEARANCE;}
                else if (id==D_DOCKBAR) {p->done=1;p->result=ND_PREF_NAV_ORGANIZE;}
                else if (id==D_PLUGIN_MANAGER) {p->done=1;p->result=ND_PREF_NAV_PLUGIN_MANAGER;}
                else if (id==D_ABOUT) {p->done=1;p->result=ND_PREF_NAV_ABOUT;}
                else if (id==D_RECOVERY) recover_settings(p);
                else if (id==D_LOW_RESOURCE) low_resource(p);
                else if (id==D_RESET) reset_counters(p);
                else if (id==D_SAVE_REPORT) {
                    if (save_report(p))
                        SetWindowTitles(p->win,(STRPTR)
                            "Diagnostics saved: RAM:NovaDock-Diagnostics.txt",
                            (STRPTR)-1);
                    else {
                        SetWindowTitles(p->win,(STRPTR)
                            "NovaDock Diagnostics - could not save report",
                            (STRPTR)-1);
                        DisplayBeep(p->screen);
                    }
                }
                else if (id==D_REFRESH) refresh_rows(p);
                else if (id==D_CLOSE) {p->done=1;p->result=0;}
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
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    FreeVec(p);GadToolsBase=NULL;IntuitionBase=NULL;return result;
}
