#include "nd_control_client.h"
/* Modal named-layout manager used by the General Preferences page. */
#define IntuitionBase NDLayoutsIntuitionBase
#define GadToolsBase NDLayoutsGadToolsBase
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
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_layout_profiles.h"
#include "nd_prefs_session.h"
#include "nd_prefs_layout.h"
#include "nd_prefs_help.h"

#define L_LIST 500
#define L_NAME 501
#define L_LOAD 502
#define L_SAVE 503
#define L_RENAME 504
#define L_DELETE 505
#define L_CLOSE 506
#define L_ROOT "ENVARC:NovaDock/Layouts"

struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;

struct LayoutPage {
    struct NDConfig *cfg;
    struct Screen *screen;
    struct Window *win;
    APTR vi;
    struct Gadget *gadgets,*last,*list_gad,*name_gad;
    struct NDPrefsLayout layout;
    struct List rows;
    struct Node nodes[ND_LAYOUT_MAX_PROFILES];
    char names[ND_LAYOUT_MAX_PROFILES][ND_LAYOUT_NAME_LEN];
    int count,selected,done,loaded;
};

static void ng_init(struct LayoutPage *p,struct NewGadget *ng,int x,int y,
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
static struct Gadget *button(struct LayoutPage *p,int x,int y,int w,int id,
                             STRPTR text)
{
    struct NewGadget ng;
    ng_init(p,&ng,x,y,w,16,id,text);ng.ng_Flags=PLACETEXT_IN;
    ng.ng_Width=nd_prefs_layout_button_width(&p->layout,w,text);
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,TAG_DONE);
    return p->last;
}
static void status(struct LayoutPage *p,const char *text)
{
    SetWindowTitles(p->win,(STRPTR)text,(STRPTR)-1);
}
static int confirm(struct LayoutPage *p,const char *text)
{
    struct EasyStruct request;
    memset(&request,0,sizeof(request));request.es_StructSize=sizeof(request);
    request.es_Title=(STRPTR)"NovaDock Layouts";
    request.es_TextFormat=(STRPTR)text;
    request.es_GadgetFormat=(STRPTR)"Continue|Cancel";
    return EasyRequestArgs(p->win,&request,NULL,NULL)==1;
}
static int file_exists(const char *path)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    if (!lock) return 0;UnLock(lock);return 1;
}
static void make_dirs(void)
{
    BPTR lock=CreateDir("ENVARC:NovaDock");if (lock) UnLock(lock);
    lock=CreateDir(L_ROOT);if (lock) UnLock(lock);
}
static int has_cfg_suffix(const char *name)
{
    unsigned int n=(unsigned int)strlen(name);
    if (n<5) return 0;
    return name[n-4]=='.' &&
        (name[n-3]=='c' || name[n-3]=='C') &&
        (name[n-2]=='f' || name[n-2]=='F') &&
        (name[n-1]=='g' || name[n-1]=='G');
}
static void sort_names(struct LayoutPage *p)
{
    int i,j;
    char swap[ND_LAYOUT_NAME_LEN];
    for (i=0;i<p->count;++i) for (j=i+1;j<p->count;++j) {
        if (strcmp(p->names[i],p->names[j])>0) {
            strcpy(swap,p->names[i]);strcpy(p->names[i],p->names[j]);
            strcpy(p->names[j],swap);
        }
    }
}
static void scan_layouts(struct LayoutPage *p)
{
    BPTR lock;
    struct FileInfoBlock *fib;
    p->count=0;make_dirs();lock=Lock(L_ROOT,ACCESS_READ);
    if (!lock) return;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib && Examine(lock,fib)) while (ExNext(lock,fib)) {
        unsigned int n;
        if (fib->fib_DirEntryType>=0 || !has_cfg_suffix(fib->fib_FileName) ||
            p->count>=ND_LAYOUT_MAX_PROFILES) continue;
        n=(unsigned int)strlen(fib->fib_FileName)-4;
        if (n>=ND_LAYOUT_NAME_LEN) n=ND_LAYOUT_NAME_LEN-1;
        memcpy(p->names[p->count],fib->fib_FileName,n);
        p->names[p->count][n]=0;
        if (nd_layout_name_valid(p->names[p->count])) ++p->count;
    }
    if (fib) FreeDosObject(DOS_FIB,fib);UnLock(lock);sort_names(p);
}
static void build_rows(struct LayoutPage *p)
{
    int i;
    NewList(&p->rows);
    for (i=0;i<p->count;++i) {
        memset(&p->nodes[i],0,sizeof(struct Node));
        p->nodes[i].ln_Name=p->names[i];AddTail(&p->rows,&p->nodes[i]);
    }
    if (p->selected>=p->count) p->selected=p->count-1;
}
static void refresh_rows(struct LayoutPage *p,const char *select_name)
{
    int i;
    GT_SetGadgetAttrs(p->list_gad,p->win,NULL,GTLV_Labels,~0UL,TAG_DONE);
    scan_layouts(p);
    if (select_name) for (i=0;i<p->count;++i)
        if (!strcmp(p->names[i],select_name)) {p->selected=i;break;}
    build_rows(p);
    GT_SetGadgetAttrs(p->list_gad,p->win,NULL,
        GTLV_Labels,(ULONG)&p->rows,
        GTLV_Selected,(ULONG)(p->selected<0 ? ~0UL:p->selected),TAG_DONE);
}
static const char *entered_name(struct LayoutPage *p)
{
    STRPTR value=NULL;
    GT_GetGadgetAttrs(p->name_gad,p->win,NULL,GTST_String,(ULONG)&value,TAG_DONE);
    return value ? (const char *)value : "";
}
static void choose_row(struct LayoutPage *p,int row)
{
    if (row<0 || row>=p->count) return;
    p->selected=row;
    GT_SetGadgetAttrs(p->name_gad,p->win,NULL,
                      GTST_String,(ULONG)p->names[row],TAG_DONE);
}
static int selected_path(struct LayoutPage *p,char *path,unsigned int size)
{
    if (p->selected<0 || p->selected>=p->count) return 0;
    return nd_layout_profile_path(path,size,L_ROOT,p->names[p->selected]);
}
static void save_layout(struct LayoutPage *p)
{
    const char *value=entered_name(p);
    char name[ND_LAYOUT_NAME_LEN],path[300];
    strncpy(name,value,sizeof(name)-1);name[sizeof(name)-1]=0;
    if (!nd_layout_name_valid(name) ||
        !nd_layout_profile_path(path,sizeof(path),L_ROOT,name)) {
        status(p,"Named Layouts - use letters, numbers, spaces, - or _");
        DisplayBeep(p->screen);return;
    }
    make_dirs();
    if (file_exists(path) && !confirm(p,"Replace the existing named layout?"))
        return;
    if (!nd_control_write_config(p->cfg,path)) {
        status(p,"Named Layouts - could not save layout");
        DisplayBeep(p->screen);return;
    }
    refresh_rows(p,name);status(p,"Named Layouts - layout saved");
}
static void load_layout(struct LayoutPage *p)
{
    struct NDConfig *saved;
    char path[300];int parsed;
    if (!selected_path(p,path,sizeof(path))) {
        status(p,"Named Layouts - select a layout first");return;
    }
    if (ND_PREFS_IS_DIRTY() &&
        !confirm(p,"Replace the pending dock arrangement with this layout?"))
        return;
    saved=(struct NDConfig *)AllocVec(sizeof(*saved),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
    if (!saved) {status(p,"Named Layouts - not enough memory");return;}
    parsed=nd_load_config(saved,path);
    if (parsed!=0 || !nd_layout_apply(p->cfg,saved)) {
        status(p,"Named Layouts - layout file is invalid");
        DisplayBeep(p->screen);FreeVec(saved);return;
    }
    FreeVec(saved);ND_PREFS_MARK_DIRTY();p->loaded=1;p->done=1;
}
static void rename_layout(struct LayoutPage *p)
{
    const char *value=entered_name(p);
    char name[ND_LAYOUT_NAME_LEN],old_path[300],new_path[300];
    strncpy(name,value,sizeof(name)-1);name[sizeof(name)-1]=0;
    if (!selected_path(p,old_path,sizeof(old_path))) {
        status(p,"Named Layouts - select a layout first");return;
    }
    if (!nd_layout_name_valid(name) ||
        !nd_layout_profile_path(new_path,sizeof(new_path),L_ROOT,name)) {
        status(p,"Named Layouts - invalid new name");DisplayBeep(p->screen);return;
    }
    if (!strcmp(old_path,new_path)) return;
    if (file_exists(new_path)) {
        status(p,"Named Layouts - that name already exists");
        DisplayBeep(p->screen);return;
    }
    if (!Rename(old_path,new_path)) {
        status(p,"Named Layouts - could not rename layout");
        DisplayBeep(p->screen);return;
    }
    refresh_rows(p,name);status(p,"Named Layouts - layout renamed");
}
static void delete_layout(struct LayoutPage *p)
{
    char path[300];
    if (!selected_path(p,path,sizeof(path))) {
        status(p,"Named Layouts - select a layout first");return;
    }
    if (!confirm(p,"Delete the selected named layout?")) return;
    if (!DeleteFile(path)) {
        status(p,"Named Layouts - could not delete layout");
        DisplayBeep(p->screen);return;
    }
    p->selected=-1;refresh_rows(p,NULL);
    GT_SetGadgetAttrs(p->name_gad,p->win,NULL,GTST_String,(ULONG)"",TAG_DONE);
    status(p,"Named Layouts - layout deleted");
}
static int open_page(struct LayoutPage *p)
{
    struct NewGadget ng;
    int x,y,width,height,font_x=8,font_y=8;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,
        p->screen->Width,p->screen->Height,p->screen->BarHeight,500,175);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    x=nd_prefs_layout_left(&p->layout);y=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfo(p->screen,TAG_DONE);
    if (!p->vi || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;p->selected=-1;scan_layouts(p);build_rows(p);
    ng_init(p,&ng,15,29,195,105,L_LIST,"Saved layouts");
    ng.ng_Flags=PLACETEXT_ABOVE;
    p->list_gad=p->last=CreateGadget(LISTVIEW_KIND,p->last,&ng,
        GTLV_Labels,(ULONG)&p->rows,GTLV_Selected,~0UL,TAG_DONE);
    if (!p->last) return 0;
    ng_init(p,&ng,300,29,175,16,L_NAME,"Name:");
    p->name_gad=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)"",GTST_MaxChars,ND_LAYOUT_NAME_LEN-1,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last || !button(p,230,56,115,L_LOAD,"Load") ||
        !button(p,355,56,120,L_SAVE,"Save current") ||
        !button(p,230,82,115,L_RENAME,"Rename") ||
        !button(p,355,82,120,L_DELETE,"Delete") ||
        !button(p,355,126,120,L_CLOSE,"Close")) return 0;
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,x,WA_Top,y,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)"NovaDock Preferences - Named Layouts",
        WA_Gadgets,(ULONG)p->gadgets,WA_DragBar,TRUE,WA_Activate,TRUE,
        WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
        WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;GT_RefreshWindow(p->win,NULL);return 1;
}

int nd_layout_prefs_run(struct NDConfig *cfg,struct Screen *screen)
{
    struct LayoutPage *p;struct IntuiMessage *msg;
    ULONG cls;UWORD id,code;int loaded=0;
    p=(struct LayoutPage *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 0;p->cfg=cfg;p->screen=screen;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    if (!IntuitionBase || !GadToolsBase || !open_page(p)) goto done;
    while (!p->done) {
        Wait(1UL<<p->win->UserPort->mp_SigBit);
        while ((msg=GT_GetIMsg(p->win->UserPort))!=NULL) {
            cls=msg->Class;code=msg->Code;
            id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID:0;
            GT_ReplyIMsg(msg);
            if (cls==IDCMP_CLOSEWINDOW) p->done=1;
            else if (cls==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(p->win);GT_EndRefresh(p->win,TRUE);
            } else if (cls==IDCMP_GADGETUP) {
                if (id==L_LIST) choose_row(p,code);
                else if (id==L_LOAD) load_layout(p);
                else if (id==L_SAVE) save_layout(p);
                else if (id==L_RENAME) rename_layout(p);
                else if (id==L_DELETE) delete_layout(p);
                else if (id==L_CLOSE) p->done=1;
            }
        }
    }
    loaded=p->loaded;
done:
    if (p->win) CloseWindow(p->win);
    if (p->gadgets) FreeGadgets(p->gadgets);
    if (p->vi) FreeVisualInfo(p->vi);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    FreeVec(p);GadToolsBase=NULL;IntuitionBase=NULL;return loaded;
}
