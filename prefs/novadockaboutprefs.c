/* About section for the unified NovaDock Preferences application. */
#define IntuitionBase NDAboutIntuitionBase
#define GadToolsBase NDAboutGadToolsBase
#define GfxBase NDAboutGfxBase
#include <exec/types.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <string.h>
#include "nd_about.h"
#include "nd_prefs_layout.h"
#include "nd_prefs_nav.h"
#include "nd_prefs_help.h"

#define B_GENERAL 501
#define B_APPEARANCE 502
#define B_ORGANIZE 503
#define B_DIAGNOSTICS 504
#define B_ABOUT 505
#define B_PLUGIN_MANAGER 506
#define B_HELP 507

struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct GfxBase *GfxBase=NULL;

struct AboutPage {
    struct Screen *screen;
    struct Window *win;
    APTR vi;
    struct DrawInfo *draw_info;
    struct Gadget *gadgets,*last;
    struct NDPrefsLayout layout;
    int done,result;
};

static void about_field(struct AboutPage *p,struct NewGadget *ng,
                        int x,int y,int w,int h,int id,STRPTR text)
{
    memset(ng,0,sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&p->layout,h);
    ng->ng_GadgetID=id;ng->ng_GadgetText=text;ng->ng_TextAttr=p->screen->Font;
    ng->ng_VisualInfo=p->vi;
    ng->ng_Flags=PLACETEXT_IN;
}

static struct Gadget *about_button(struct AboutPage *p,int x,int y,int w,
                                   int id,const char *text)
{
    struct NewGadget ng;
    about_field(p,&ng,x,y,w,16,id,(STRPTR)text);
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
        GA_Disabled,(ULONG)(text[0]=='>'),TAG_DONE);
    return p->last;
}

static void about_text(struct AboutPage *p,const char *text,int x,int y)
{
    struct RastPort *rp=p->win->RPort;
    SetAPen(rp,p->draw_info ? p->draw_info->dri_Pens[TEXTPEN] : 1);
    SetDrMd(rp,JAM1);
    Move(rp,nd_prefs_layout_x(&p->layout,x),nd_prefs_layout_y(&p->layout,y));
    Text(rp,(STRPTR)text,strlen(text));
}

static void draw_about(struct AboutPage *p)
{
    about_text(p,ND_ABOUT_NAME,170,35);
    about_text(p,"Version " ND_ABOUT_VERSION,170,53);
    about_text(p,ND_ABOUT_DESCRIPTION_1,170,77);
    about_text(p,ND_ABOUT_DESCRIPTION_2,170,93);
    about_text(p,"License: " ND_ABOUT_LICENSE,170,117);
    about_text(p,"Website: " ND_ABOUT_WEBSITE,170,141);
    about_text(p,"GitHub: " ND_ABOUT_GITHUB,170,157);
}

static int open_about_page(struct AboutPage *p)
{
    int x,y,width,height,font_x=8,font_y=8;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,p->screen->Width,
        p->screen->Height,p->screen->BarHeight,620,184);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    x=nd_prefs_layout_left(&p->layout);y=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfo(p->screen,TAG_DONE);
    p->draw_info=GetScreenDrawInfo(p->screen);
    if (!p->vi || !p->draw_info || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;
    if (!about_button(p,10,21,118,B_GENERAL,"General") ||
        !about_button(p,10,45,118,B_APPEARANCE,"Appearance") ||
        !about_button(p,10,69,118,B_ORGANIZE,"Organize") ||
        !about_button(p,10,93,118,B_PLUGIN_MANAGER,"Plugins") ||
        !about_button(p,10,117,118,B_DIAGNOSTICS,"Diagnostics") ||
        !about_button(p,10,141,118,B_ABOUT,"> About") ||
        /* The manual; the Help key opens it from every window too. */
        !about_button(p,170,163,118,B_HELP,"Help...")) return 0;
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,x,WA_Top,y,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)"NovaDock Preferences - About",
        WA_Gadgets,(ULONG)p->gadgets,WA_DragBar,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,WA_DepthGadget,TRUE,
        WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;
    GT_RefreshWindow(p->win,NULL);draw_about(p);return 1;
}

int nd_about_prefs_run(int argc,char **argv)
{
    struct AboutPage *p;struct IntuiMessage *msg;
    ULONG cls;UWORD id,rawcode;int result=20;
    (void)argc;(void)argv;
    p=(struct AboutPage *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 20;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    if (!IntuitionBase || !GadToolsBase || !GfxBase) goto done;
    p->screen=LockPubScreen("Workbench");
    if (!p->screen || !open_about_page(p)) goto done;
    while (!p->done) {
        Wait(1UL<<p->win->UserPort->mp_SigBit);
        while ((msg=GT_GetIMsg(p->win->UserPort))!=NULL) {
            cls=msg->Class;rawcode=msg->Code;
            id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(msg);
            if (cls==IDCMP_CLOSEWINDOW) {p->done=1;p->result=0;}
            else if (cls==IDCMP_RAWKEY && rawcode==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(p->win);draw_about(p);GT_EndRefresh(p->win,TRUE);
            } else if (cls==IDCMP_GADGETUP) {
                if (id==B_GENERAL) {p->done=1;p->result=ND_PREF_NAV_GENERAL;}
                else if (id==B_APPEARANCE) {p->done=1;p->result=ND_PREF_NAV_APPEARANCE;}
                else if (id==B_ORGANIZE) {p->done=1;p->result=ND_PREF_NAV_ORGANIZE;}
                else if (id==B_PLUGIN_MANAGER) {p->done=1;p->result=ND_PREF_NAV_PLUGIN_MANAGER;}
                else if (id==B_HELP) nd_prefs_help(p->screen);
                else if (id==B_DIAGNOSTICS) {p->done=1;p->result=ND_PREF_NAV_DIAGNOSTICS;}
            }
        }
    }
    result=p->result;
done:
    if (p->win) CloseWindow(p->win);
    if (p->gadgets) FreeGadgets(p->gadgets);
    if (p->draw_info && p->screen) FreeScreenDrawInfo(p->screen,p->draw_info);
    if (p->vi) FreeVisualInfo(p->vi);
    if (p->screen) UnlockPubScreen(NULL,p->screen);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    FreeVec(p);GfxBase=NULL;GadToolsBase=NULL;IntuitionBase=NULL;
    return result;
}
