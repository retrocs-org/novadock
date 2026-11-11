/* Plugin settings page for NovaDock Preferences 0.1. Native GadTools.
 * Plugins identify settings through built-in schemas or OPTIONDEF lines in
 * the SAME novadock.cfg. Page size is bounded for 640x200 Workbench.
 */
#define IntuitionBase NDPluginIntuitionBase
#define GadToolsBase NDPluginGadToolsBase
#define AslBase NDPluginAslBase
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_config.h"
#include "nd_options.h"
#include "nd_control_client.h"
#include "nd_prefs_nav.h"
#include "nd_prefs_session.h"
#include "nd_prefs_layout.h"
#include "nd_prefs_help.h"

#define PP_ROWS 12
#define PP_PAGE_ROWS 5
#define PP_MAX 64
#define G_FIRST 100
#define G_FILE_FIRST 140
#define G_SCROLL 199
#define G_PREV 200
#define G_NEXT 201
#define G_USE 202
#define G_SAVE 203
#define G_CLOSE 204
#define G_NAV_GENERAL 205
#define G_NAV_APPEARANCE 206
#define G_NAV_ITEMS 207
#define G_NAV_DIAGNOSTICS 209
#define G_ADVANCED 210
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct Library *AslBase=NULL;
struct PluginPrefs {
    struct NDConfig *config;
    char id[20];
    struct NDOptionDef definitions[PP_MAX];
    char edits[PP_MAX][ND_OPT_VALUE_LEN];
    int count,total_count,basic_count,advanced,page,top,done,result,focused,rows;
    struct Screen *screen;
    struct Window *win;
    struct Gadget *gadgets,*last,*fields[PP_ROWS],*file_buttons[PP_ROWS],*scroll;
    struct NDPrefsLayout layout;
    int active[PP_ROWS];
    APTR visual;
    char title[90];
    char choices[PP_ROWS][96];
    STRPTR choice_labels[PP_ROWS][18];
};
static int network_option_index(const struct PluginPrefs *p,const char *key)
{
    int i;for(i=0;i<p->total_count;++i)if(!strcmp(p->definitions[i].key,key))return i;
    return -1;
}
static int sampling_override(const struct PluginPrefs *p)
{
    int i=network_option_index(p,"SAMPLING_OVERRIDE");
    return !strcmp(p->id,"NET") && i>=0 && !strcmp(p->edits[i],"YES");
}
static int sampling_field_disabled(const struct PluginPrefs *p,const struct NDOptionDef *d)
{
    return !strcmp(p->id,"NET") && (!strcmp(d->key,"DEVICE") || !strcmp(d->key,"UNIT")) &&
        !sampling_override(p);
}
static int page_rows(const struct PluginPrefs *p)
{ return p->focused ? p->rows : PP_PAGE_ROWS; }
static int visible_start(const struct PluginPrefs *p)
{
    return p->focused ? p->top : p->page*PP_PAGE_ROWS;
}
static void field(struct PluginPrefs *p,struct NewGadget *ng,
                  int x,int y,int w,int h,ULONG id,STRPTR label)
{
    memset(ng,0,sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&p->layout,h);
    ng->ng_TextAttr=p->screen->Font;ng->ng_VisualInfo=p->visual;
    ng->ng_GadgetText=label;ng->ng_GadgetID=(UWORD)id;
    ng->ng_Flags=PLACETEXT_LEFT;
}
static int button(struct PluginPrefs *p,int x,int y,ULONG id,STRPTR caption)
{
    struct NewGadget ng;
    int baseline_width=x==10 ? 118 : 86;
    field(p,&ng,x,y,baseline_width,16,id,caption);
    ng.ng_Width=nd_prefs_layout_button_width(&p->layout,baseline_width,caption);
    ng.ng_Flags=PLACETEXT_IN;
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
                         GA_Disabled,(ULONG)(caption[0]=='>'),TAG_DONE);
    return p->last!=NULL;
}
/* Network profiles and SANA-II drivers are separate Amiga drawers. Keep
 * their TEXT fields editable; the same native picker used for FILE options
 * offers detected files without changing schema or saved values. */
static int network_picker(const struct PluginPrefs *p,const struct NDOptionDef *d)
{
    if(strcmp(p->id,"NET") && strcmp(p->id,"ROADSHOW"))return 0;
    if(!strcmp(d->key,"INTERFACE"))return 1;
    if(!strcmp(d->key,"DEVICE"))return 2;
    return 0;
}
static int has_picker(const struct PluginPrefs *p,const struct NDOptionDef *d)
{return !strcmp(d->type,"FILE") || network_picker(p,d);}
static int choice_index(struct PluginPrefs *p,int row,const char *choice)
{
    int j;
    for (j=0;p->choice_labels[row][j] && j<17;++j)
        if (!strcmp(p->choice_labels[row][j],choice)) return j;
    return 0;
}
static void open_choices(struct PluginPrefs *p,int row,const char *values)
{
    char *start,*comma;
    int n=0;
    strncpy(p->choices[row],values,sizeof(p->choices[row])-1);
    p->choices[row][sizeof(p->choices[row])-1]=0;
    start=p->choices[row];
    while (*start && n<17) {
        comma=strchr(start,',');
        if (comma) *comma=0;
        p->choice_labels[row][n++]=start;
        if (!comma) break;
        start=comma+1;
    }
    p->choice_labels[row][n]=NULL;
}
static int collect_page(struct PluginPrefs *p)
{
    int i,at=visible_start(p);
    for (i=0;i<page_rows(p) && at+i<p->count;++i) {
        const struct NDOptionDef *def=&p->definitions[at+i];
        if (!strcmp(def->type,"BOOL") || !strcmp(def->type,"CHOICE")) {
            int ix=p->active[i];
            if (ix<0 || ix>=17 || !p->choice_labels[i][ix]) return 0;
            strcpy(p->edits[at+i],p->choice_labels[i][ix]);
        } else {
            struct StringInfo *si;
            STRPTR s;
            if (!p->fields[i] || !p->fields[i]->SpecialInfo) return 0;
            si=(struct StringInfo *)p->fields[i]->SpecialInfo;
            s=si->Buffer;
            if (!s ||
                strlen((char *)s)>=ND_OPT_VALUE_LEN) return 0;
            strcpy(p->edits[at+i],s);
        }
        if (!nd_option_validate(def,p->edits[at+i])) return 0;
    }
    return 1;
}
static void close_ui(struct PluginPrefs *p)
{
    if (p->win) {CloseWindow(p->win);p->win=NULL;}
    if (p->gadgets) {FreeGadgets(p->gadgets);p->gadgets=NULL;}
    if (p->visual) {FreeVisualInfo(p->visual);p->visual=NULL;}
    p->last=NULL;p->scroll=NULL;
    memset(p->fields,0,sizeof(p->fields));
    memset(p->file_buttons,0,sizeof(p->file_buttons));
}
static int open_ui(struct PluginPrefs *p)
{
    int i,at=visible_start(p),left,top,visible_rows,button_y;
    int base_width=p->focused ? 500 : 620;
    int base_height=183,width,height,font_x=8,font_y=8;
    int field_x=p->focused ? 190 : 300;
    int field_w=p->focused ? 295 : 305;
    struct NewGadget ng;
    visible_rows=p->count-at;
    if (visible_rows>page_rows(p)) visible_rows=page_rows(p);
    if (visible_rows<1) visible_rows=1;
    button_y=16+visible_rows*24+9;
    if (p->focused) {
        base_height=button_y+32;
        if (p->count>page_rows(p)) field_w=270;
        /* GadTools places labels outside the gadget. Reserve their width
         * across every page so scrolling cannot shift the field column. */
        for (i=0;i<p->count;++i) {
            int label_edge=22+(int)strlen(p->definitions[i].label)*8;
            if (label_edge>field_x) field_x=label_edge;
        }
        if (field_x>base_width-150) field_x=base_width-150;
        field_w=(p->count>page_rows(p) ? 460 : 485)-field_x;
        if (!p->count) base_height=92;
    }
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,
        p->screen->Width,p->screen->Height,p->screen->BarHeight,
        base_width,base_height);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    left=nd_prefs_layout_left(&p->layout);
    top=nd_prefs_layout_top(&p->layout);
    p->visual=GetVisualInfo(p->screen,TAG_DONE);
    if (!p->visual || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;
    if (!p->focused && (!button(p,10,21,G_NAV_GENERAL,"General") ||
        !button(p,10,45,G_NAV_APPEARANCE,"Appearance") ||
        !button(p,10,69,G_NAV_ITEMS,"> Organize") ||
        !button(p,10,93,G_NAV_DIAGNOSTICS,"Diagnostics"))) return 0;
    if (p->focused && !p->count) {
        field(p,&ng,28,22,444,16,0,NULL);
        ng.ng_Flags=PLACETEXT_IN;
        p->last=CreateGadget(TEXT_KIND,p->last,&ng,
            GTTX_Text,(ULONG)"This plugin has no configurable settings.",
            GTTX_Border,FALSE,TAG_DONE);
        if (!p->last || !button(p,207,55,G_CLOSE,"Close")) return 0;
    }
    for (i=0;i<page_rows(p) && at+i<p->count;++i) {
        int current=at+i;
        int row_field_w=field_w;
        const struct NDOptionDef *d=&p->definitions[current];
        if (has_picker(p,d))
            row_field_w=p->focused ? 400-field_x : 210;
        field(p,&ng,field_x,16+i*24,row_field_w,16,G_FIRST+i,(STRPTR)d->label);
        if (!strcmp(d->type,"BOOL") || !strcmp(d->type,"CHOICE")) {
            open_choices(p,i,!strcmp(d->type,"BOOL") ? "NO,YES" : d->choices);
            p->active[i]=choice_index(p,i,p->edits[current]);
            p->fields[i]=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
                       GTCY_Labels,(ULONG)p->choice_labels[i],
                       GTCY_Active,p->active[i],TAG_DONE);
        } else {
            p->fields[i]=p->last=CreateGadget(STRING_KIND,p->last,&ng,
                       GTST_String,(ULONG)p->edits[current],
                       GTST_MaxChars,ND_OPT_VALUE_LEN-1,
                       GA_Disabled,(ULONG)sampling_field_disabled(p,d),TAG_DONE);
            if (p->last && has_picker(p,d)) {
                field(p,&ng,p->focused ? 405 : 515,16+i*24,
                      p->focused ? 60 : 90,16,G_FILE_FIRST+i,(STRPTR)"Select");
                ng.ng_Flags=PLACETEXT_IN;
                p->file_buttons[i]=p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
                    GA_Disabled,(ULONG)sampling_field_disabled(p,d),TAG_DONE);
            }
        }
        if (!p->last) return 0;
    }
    if (p->focused) {
        if (p->count>page_rows(p)) {
            field(p,&ng,470,16,15,visible_rows*24-8,G_SCROLL,NULL);
            ng.ng_Flags=0;
            p->scroll=p->last=CreateGadget(SCROLLER_KIND,p->last,&ng,
                GTSC_Top,p->top,GTSC_Total,p->count,
                GTSC_Visible,page_rows(p),GTSC_Arrows,10,
                PGA_Freedom,FREEVERT,TAG_DONE);
            if (!p->last) return 0;
        }
        if(!strcmp(p->id,"NET") && !button(p,20,button_y,G_ADVANCED,
            p->advanced ? "Basic":sampling_override(p) ? "Advanced *":"Advanced..."))return 0;
        if (p->count && (!button(p,315,button_y,G_USE,"Use") ||
            !button(p,405,button_y,G_SAVE,"Save"))) return 0;
        sprintf(p->title,"%s settings",p->id);
    } else {
        if(!strcmp(p->id,"NET") && !button(p,319,151,G_ADVANCED,
            p->advanced ? "Basic":sampling_override(p) ? "Advanced *":"Advanced..."))return 0;
        if (!button(p,135,151,G_PREV,"Previous") ||
            !button(p,227,151,G_NEXT,"Next") ||
            !button(p,427,151,G_USE,"Use") ||
            !button(p,519,151,G_SAVE,"Save")) return 0;
        sprintf(p->title,"NovaDock Preferences - %s (page %d)%s",p->id,p->page+1,
                ND_PREFS_IS_DIRTY() ? " *" : "");
    }
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
           WA_Left,left,WA_Top,top,WA_Width,width,WA_Height,height,
           WA_Title,(ULONG)p->title,WA_Gadgets,(ULONG)p->gadgets,
           WA_DragBar,TRUE,WA_Activate,TRUE,WA_CloseGadget,TRUE,
           WA_DepthGadget,TRUE,WA_IDCMP,ARROWIDCMP|SCROLLERIDCMP|
           IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
           WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;
    GT_RefreshWindow(p->win,NULL);
    return 1;
}
#include "nd_network_picker.h"
static void choose_file(struct PluginPrefs *p,int row)
{
    struct FileRequester *fr;
    int at=visible_start(p)+row,kind;
    char path[ND_OPT_VALUE_LEN],drawer[ND_OPT_VALUE_LEN]="",file[ND_OPT_VALUE_LEN]="";
    const char *title="Select file",*pattern="#?";
    if(row<0 || row>=PP_ROWS || at>=p->count || !has_picker(p,&p->definitions[at]))return;
    kind=network_picker(p,&p->definitions[at]);
    if(kind==1){choose_network_profile(p,row);return;}
    if(!AslBase){
        SetWindowTitles(p->win,(STRPTR)"ASL requester unavailable; type the path",(STRPTR)-1);
        DisplayBeep(p->screen);return;
    }
    if(kind==2){
        const char *name=FilePart(p->edits[at]);
        title="Select sampling driver";pattern="#?.device";
        strcpy(drawer,"DEVS:Networks");strcpy(file,name);
        if(name>p->edits[at]){
            unsigned int n=(unsigned int)(name-p->edits[at]);
            memcpy(drawer,p->edits[at],n);drawer[n]=0;
        }
    }else if(!strcmp(p->id,"DOPUS4"))title="Select DOpus executable";
    else if(!strcmp(p->id,"MUSIC"))title="Select default music application";
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)title,ASLFR_InitialDrawer,(ULONG)drawer,
        ASLFR_InitialFile,(ULONG)file,ASLFR_DoPatterns,kind ? TRUE:FALSE,
        ASLFR_InitialPattern,(ULONG)pattern,
        ASLFR_Window,(ULONG)p->win,ASLFR_SleepWindow,TRUE,TAG_DONE);
    if(!fr)return;
    if(AslRequest(fr,NULL)){
        strncpy(path,fr->fr_Drawer,sizeof(path)-1);path[sizeof(path)-1]=0;
        if(AddPart(path,fr->fr_File,sizeof(path))){
            strcpy(p->edits[at],path);
            GT_SetGadgetAttrs(p->fields[row],p->win,NULL,GTST_String,(ULONG)path,TAG_DONE);
        }else{SetWindowTitles(p->win,(STRPTR)"Selected path is too long",(STRPTR)-1);DisplayBeep(p->screen);}
    }
    FreeAslRequest(fr);
}
static int stage_options(struct PluginPrefs *p)
{
    int i;
    if (!p->count) return 1;
    if(!strcmp(p->id,"NET") && !sampling_override(p)){
        int device=network_option_index(p,"DEVICE"),unit=network_option_index(p,"UNIT");
        if(device>=0)p->edits[device][0]=0;
        if(unit>=0)strcpy(p->edits[unit],"0");
    }
    for (i=0;i<p->total_count;++i) {
        if (!nd_option_set(p->config,p->id,p->definitions[i].key,p->edits[i]))
            return 0;
    }
    ND_PREFS_MARK_DIRTY();
    return 1;
}
static int save(struct PluginPrefs *p,int permanent)
{
    BPTR lock;
    /* Stage changes in memory. No per-plugin configuration files are used. */
    if (!stage_options(p)) return 0;
    lock=CreateDir("ENV:NovaDock");if (lock) UnLock(lock);
    if (permanent) {
        lock=CreateDir("ENVARC:NovaDock");if (lock) UnLock(lock);
        if (!nd_control_write_config(p->config,"ENVARC:NovaDock/novadock.cfg")) return 0;
    }
    if (!nd_control_write_config(p->config,"ENV:NovaDock/novadock.cfg")) return 0;
    ND_PREFS_MARK_CLEAN();ND_PREFS_CLEAR_PREVIEW();
    return 1;
}
static int start(struct PluginPrefs *p,const char *id)
{
    int i,count,found=0;
    if (!id || !*id || strlen(id)>=sizeof(p->id)) return 0;
    strcpy(p->id,id);
    count=0;
    if (!ND_PREFS_SHARED_CONFIG) count=nd_load_effective(p->config);
    if (count>0) return 0;
    for (i=0;i<p->config->count;++i)
        if (p->config->items[i].kind==ND_ITEM_PLUGIN &&
            !strcmp(p->config->items[i].id,id)) {found=1;break;}
    if (!found) return 0;
    p->count=nd_option_collect(p->config,id,p->definitions,PP_MAX);
    p->total_count=p->basic_count=p->count;
    p->basic_count=nd_option_group_settings(id,p->definitions,p->total_count);
    for (i=0;i<p->total_count;++i) {
        const char *value=nd_option_get(p->config,id,p->definitions[i].key);
        if (!value) value=p->definitions[i].default_value;
        strncpy(p->edits[i],value,ND_OPT_VALUE_LEN-1);
        p->edits[i][ND_OPT_VALUE_LEN-1]=0;
    }
    p->count=p->basic_count;
    if(!strcmp(p->id,"NET") && !sampling_override(p)){
        int device=network_option_index(p,"DEVICE"),unit=network_option_index(p,"UNIT");
        if(device>=0)p->edits[device][0]=0;
        if(unit>=0)strcpy(p->edits[unit],"0");
    }
    return 1;
}
int nd_plugin_prefs_run(int argc,char **argv)
{
    struct PluginPrefs *p;
    struct IntuiMessage *im;
    ULONG kind; UWORD id,code; int result=20,notify_reload=0;
    if (argc<2) {Printf("Usage: NovaDockPrefs PAGE=PLUGIN PLUGIN=ID\n");return 20;}
    p=(struct PluginPrefs *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 20;
    { int a;for (a=2;a<argc;++a)
        if (!strcmp(argv[a],"FOCUSED=1")) p->focused=1; }
    p->config=ND_PREFS_SHARED_CONFIG;
    if (!p->config)
        p->config=(struct NDConfig *)AllocVec(sizeof(*p->config),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p->config) goto done;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    AslBase=OpenLibrary("asl.library",37);
    if (!IntuitionBase || !GadToolsBase || !start(p,argv[1])) goto done;
    p->screen=LockPubScreen("Workbench");
    if (!p->screen) goto done;
    p->rows=((p->screen->Height-p->screen->BarHeight-8)*8/
        (p->screen->RastPort.Font->tf_YSize<8 ? 8 : p->screen->RastPort.Font->tf_YSize)-57)/24;
    if(p->rows<1)p->rows=1;
    if(p->rows>PP_ROWS)p->rows=PP_ROWS;
    if (!open_ui(p)) goto done;
    while (!p->done) {
        Wait(1UL<<p->win->UserPort->mp_SigBit);
        while (p->win && (im=GT_GetIMsg(p->win->UserPort))!=NULL) {
            kind=im->Class;
            id=kind==IDCMP_GADGETUP && im->IAddress ? ((struct Gadget *)im->IAddress)->GadgetID : 0;
            code=im->Code;
            GT_ReplyIMsg(im);
            if (kind==IDCMP_CLOSEWINDOW) {p->done=1;result=0;}
            else if (kind==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (kind==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);
            } else if (kind==IDCMP_GADGETUP) {
                if (id==G_CLOSE) {p->done=1;result=0;}
                else if(id==G_ADVANCED && !strcmp(p->id,"NET")){
                    if(!collect_page(p)){DisplayBeep(p->screen);continue;}
                    p->advanced=!p->advanced;p->count=p->advanced ? p->total_count:p->basic_count;
                    p->top=p->page=0;close_ui(p);if(!open_ui(p))p->done=1;
                }
                else if (id==G_NAV_GENERAL || id==G_NAV_APPEARANCE ||
                    id==G_NAV_ITEMS ||
                    id==G_NAV_DIAGNOSTICS) {
                    if (!collect_page(p) || !stage_options(p)) {
                        DisplayBeep(p->screen);continue;
                    }
                    if (id==G_NAV_GENERAL) result=ND_PREF_NAV_GENERAL;
                    else if (id==G_NAV_APPEARANCE) result=ND_PREF_NAV_APPEARANCE;
                    else if (id==G_NAV_ITEMS) result=ND_PREF_NAV_ORGANIZE;
                    else result=ND_PREF_NAV_DIAGNOSTICS;
                    p->done=1;
                } else if (id>=G_FIRST && id<G_FIRST+PP_ROWS) {
                    int row=(int)id-G_FIRST;
                    int at=visible_start(p)+row;
                    if (at<p->count &&
                        (!strcmp(p->definitions[at].type,"BOOL") ||
                         !strcmp(p->definitions[at].type,"CHOICE")) &&
                        code<17 && p->choice_labels[row][code]) {
                        p->active[row]=(int)code;
                        strcpy(p->edits[at],p->choice_labels[row][code]);
                        if(!strcmp(p->id,"NET") && !strcmp(p->definitions[at].key,"SAMPLING_OVERRIDE")){
                            if(!collect_page(p)){DisplayBeep(p->screen);continue;}
                            close_ui(p);if(!open_ui(p))p->done=1;
                        }
                    }
                } else if (id>=G_FILE_FIRST && id<G_FILE_FIRST+PP_ROWS) {
                    choose_file(p,(int)id-G_FILE_FIRST);
                } else if (id==G_SCROLL && p->focused && p->scroll) {
                    ULONG new_top=(ULONG)p->top;
                    if (!collect_page(p)) {DisplayBeep(p->screen);continue;}
                    GT_GetGadgetAttrs(p->scroll,p->win,NULL,
                                      GTSC_Top,(ULONG)&new_top,TAG_DONE);
                    if ((int)new_top>p->count-page_rows(p))
                        new_top=(ULONG)(p->count-page_rows(p));
                    if ((int)new_top<0) new_top=0;
                    if ((int)new_top!=p->top) {
                        p->top=(int)new_top;
                        close_ui(p);
                        if (!open_ui(p)) p->done=1;
                    }
                } else if (id==G_NEXT || id==G_PREV) {
                    if (!collect_page(p)) {DisplayBeep(p->screen);continue;}
                    if (id==G_NEXT && (p->page+1)*PP_PAGE_ROWS<p->count) ++p->page;
                    else if (id==G_PREV && p->page>0) --p->page;
                    close_ui(p);
                    if (!open_ui(p)) p->done=1;
                } else if (id==G_USE || id==G_SAVE) {
                    if (!collect_page(p)) {
                        SetWindowTitles(p->win,(STRPTR)"NovaDock - invalid setting",(STRPTR)-1);
                        DisplayBeep(p->screen);
                    } else if (!save(p,id==G_SAVE)) {
                        SetWindowTitles(p->win,(STRPTR)"NovaDock - cannot write settings",(STRPTR)-1);
                        DisplayBeep(p->screen);
                    } else {p->done=1;result=0;notify_reload=1;}
                }
            }
        }
    }
done:
    /* Do not keep the preferences window open while waiting for NovaDock to
     * recreate its window and plugins. Saved settings are authoritative even
     * when no dock is currently running. */
    close_ui(p);
    if (p->screen) {UnlockPubScreen(NULL,p->screen);p->screen=NULL;}
    if (notify_reload) {
        LONG rc=nd_control_reload();
        if (rc!=ND_CONTROL_OK && rc!=ND_CONTROL_NOHOST &&
        rc!=ND_CONTROL_PENDING) result=10;
    }
    if (AslBase) CloseLibrary(AslBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (p->config && !ND_PREFS_SHARED_CONFIG) FreeVec(p->config);
    FreeVec(p);
    return result;
}
#ifndef ND_PREFS_EMBEDDED
int main(int argc,char **argv)
{
    return nd_plugin_prefs_run(argc,argv);
}
#endif
