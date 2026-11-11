/* Appearance page for the unified NovaDock Preferences application.
 * Native GadTools/Intuition, OS 3.0+, baseline 68000.  Editing updates only
 * the local sample; Test is the explicit reversible live-preview action.
 */
#define IntuitionBase NDAppearanceIntuitionBase
#define GadToolsBase NDAppearanceGadToolsBase
#define GfxBase NDAppearanceGfxBase
#define AslBase NDAppearanceAslBase
#include <exec/types.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_layout.h"
#include "nd_control_client.h"
#include "nd_prefs_nav.h"
#include "nd_prefs_session.h"
#include "nd_numeric_gadget.h"
#include "nd_theme.h"
#include "nd_prefs_layout.h"
#include "nd_prefs_help.h"

#define A_THEME 101
#define A_SIDE 102
#define A_WIDTH 103
#define A_HEIGHT 104
#define A_GAP 105
#define A_REVERT 107
#define A_GENERAL 108
#define A_USE 109
#define A_SAVE 110
#define A_SHAPE 112
#define A_APPEARANCE 113
#define A_ITEMS 114
#define A_TEST 116
#define A_DIAGNOSTICS 119
#define A_ABOUT 125
#define A_PLUGIN_MANAGER 126
#define A_RUNNING 120
#define A_POSITION 121
#define A_FONT 122
#define A_LABELS 123
#define A_SIZE 124
#define A_IMPORT_THEME 127
#define A_COLOR_COUNT 8
#define RGBBYTE(n) ((ULONG)(n) * 0x01010101UL)

struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *AslBase = NULL;

struct AppearanceTheme {
    char id[ND_THEME_ID_LEN];
    char label[64];
};

struct AppearancePrefs {
    struct NDConfig *cfg;
    struct NDConfig *baseline;
    struct Screen *screen;
    struct Window *win;
    APTR vi;
    struct Gadget *gadgets;
    struct Gadget *last;
    struct Gadget *g_theme;
    struct Gadget *g_side;
    struct Gadget *g_position;
    struct Gadget *g_font;
    struct Gadget *g_labels;
    struct Gadget *g_size;
    struct Gadget *g_shape;
    struct Gadget *g_width;
    struct Gadget *g_height;
    struct Gadget *g_gap;
    struct Gadget *g_running;
    struct NDPrefsLayout layout;
    char theme_root[256];
    struct AppearanceTheme *themes;
    STRPTR *theme_labels;
    int theme_count;
    int theme_capacity;
    LONG pens[A_COLOR_COUNT];
    int done;
    int status;
};

static STRPTR appearance_sides[] = {"Bottom", "Top", "Left", "Right", NULL};
static STRPTR appearance_shapes[] = {"Rounded", "Square", "Chamfer", "Minimal", NULL};
/* Running applications only affect launchers that the user explicitly put
 * in the dock. Unlisted applications belong in the optional Running Apps
 * plugin, not as transient dock tiles. */
static STRPTR appearance_running[] = {"Off", "Mark", NULL};
#define RUNNING_STATE(cfg) ((cfg)->running_apps ? 1 : 0)
static STRPTR appearance_positions[] = {"Start", "Center", "End", NULL};
static STRPTR appearance_fonts[] = {"Workbench", "Topaz 8", "Bold", NULL};
static STRPTR appearance_labels[] = {"Always", "Hover", "Hidden", NULL};
static STRPTR appearance_item_sizes[] = {"Normal", "Tiny", NULL};

static int text_equal(const char *a,const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca=*a++,cb=*b++;
        if (ca>='a' && ca<='z') ca=(char)(ca-'a'+'A');
        if (cb>='a' && cb<='z') cb=(char)(cb-'a'+'A');
        if (ca!=cb) return 0;
    }
    return *a==0 && *b==0;
}

static int join_path(char *out,int out_size,const char *left,const char *right)
{
    char temp[512];
    int len;
    if (!out || out_size<2 || !left || !right) return 0;
    if ((int)strlen(left)+(int)strlen(right)+2>(int)sizeof(temp)) return 0;
    strcpy(temp,left);len=(int)strlen(temp);
    if (len && temp[len-1]!=':' && temp[len-1]!='/') strcat(temp,"/");
    strcat(temp,right);
    if ((int)strlen(temp)>=out_size) return 0;
    strcpy(out,temp);return 1;
}

static int catalogue_index(struct AppearancePrefs *p,const char *id)
{
    int i;
    for (i=0;i<p->theme_count;++i)
        if (text_equal(p->themes[i].id,id)) return i;
    return 0;
}

static int reserve_theme_capacity(struct AppearancePrefs *p,int needed)
{
    struct AppearanceTheme *themes;
    int capacity;
    if (needed<=p->theme_capacity) return 1;
    capacity=p->theme_capacity ? p->theme_capacity : 16;
    while (capacity<needed) {
        if (capacity>INT_MAX/2) {
            capacity=needed;
            break;
        }
        capacity*=2;
    }
    themes=(struct AppearanceTheme *)AllocVec(
        (ULONG)(sizeof(*themes)*capacity),MEMF_PUBLIC|MEMF_CLEAR);
    if (!themes) return 0;
    if (p->themes) {
        CopyMem(p->themes,themes,
                (ULONG)(sizeof(*themes)*p->theme_count));
        FreeVec(p->themes);
    }
    p->themes=themes;
    p->theme_capacity=capacity;
    return 1;
}

static int add_catalogue_theme(struct AppearancePrefs *p,const char *id,
                               const char *label)
{
    int i;
    if (!nd_theme_id_valid(id)) return 0;
    for (i=0;i<p->theme_count;++i)
        if (text_equal(p->themes[i].id,id)) return 0;
    if (!reserve_theme_capacity(p,p->theme_count+1)) return 0;
    strncpy(p->themes[p->theme_count].id,id,ND_THEME_ID_LEN-1);
    p->themes[p->theme_count].id[ND_THEME_ID_LEN-1]=0;
    strncpy(p->themes[p->theme_count].label,label ? label : id,
            sizeof(p->themes[p->theme_count].label)-1);
    p->themes[p->theme_count].label[sizeof(p->themes[p->theme_count].label)-1]=0;
    ++p->theme_count;return 1;
}

static void sort_custom_themes(struct AppearancePrefs *p,int first)
{
    int i,j;
    struct AppearanceTheme temp;
    for (i=first;i<p->theme_count-1;++i)
        for (j=i+1;j<p->theme_count;++j)
            if (strcmp(p->themes[i].label,p->themes[j].label)>0) {
                temp=p->themes[i];p->themes[i]=p->themes[j];p->themes[j]=temp;
            }
}

static int build_theme_catalogue(struct AppearancePrefs *p)
{
    struct FileInfoBlock *fib;
    struct NDThemeSpec spec;
    char themes_dir[512];
    BPTR lock;
    int i,builtins;
    p->theme_count=0;
    if (p->theme_labels) {
        FreeVec(p->theme_labels);
        p->theme_labels=NULL;
    }
    for (i=0;i<ND_THEME_COUNT;++i) {
        const char *id=nd_theme_directory(i);
        if (nd_theme_load_id(p->theme_root,id,&spec))
            add_catalogue_theme(p,id,spec.name);
        else add_catalogue_theme(p,id,nd_theme_name(i));
    }
    builtins=p->theme_count;
    if (join_path(themes_dir,sizeof(themes_dir),p->theme_root,"Themes")) {
        lock=Lock((STRPTR)themes_dir,ACCESS_READ);
        fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
        if (lock && fib && Examine(lock,fib)) {
            while (ExNext(lock,fib)) {
                if (fib->fib_DirEntryType>0 && nd_theme_id_valid(fib->fib_FileName) &&
                    nd_theme_load_id(p->theme_root,fib->fib_FileName,&spec))
                    add_catalogue_theme(p,fib->fib_FileName,spec.name);
            }
        }
        if (fib) FreeDosObject(DOS_FIB,fib);
        if (lock) UnLock(lock);
    }
    sort_custom_themes(p,builtins);
    p->theme_labels=(STRPTR *)AllocVec(
        (ULONG)(sizeof(*p->theme_labels)*(p->theme_count+1)),
        MEMF_PUBLIC|MEMF_CLEAR);
    if (!p->theme_labels) return 0;
    for (i=0;i<p->theme_count;++i) p->theme_labels[i]=p->themes[i].label;
    p->theme_labels[p->theme_count]=NULL;
    return 1;
}

static void free_theme_catalogue(struct AppearancePrefs *p)
{
    if (p->theme_labels) FreeVec(p->theme_labels);
    if (p->themes) FreeVec(p->themes);
    p->theme_labels=NULL;
    p->themes=NULL;
    p->theme_count=0;
    p->theme_capacity=0;
}
static void setup_gadget(struct AppearancePrefs *p, struct NewGadget *ng,
                         int x, int y, int w, int h, UWORD id, STRPTR name)
{
    memset(ng,0,sizeof(*ng));
    ng->ng_LeftEdge=nd_prefs_layout_x(&p->layout,x);
    ng->ng_TopEdge=nd_prefs_layout_y(&p->layout,y);
    ng->ng_Width=nd_prefs_layout_w(&p->layout,w);
    ng->ng_Height=nd_prefs_layout_h(&p->layout,h);
    ng->ng_GadgetText=name; ng->ng_TextAttr=p->screen->Font;
    ng->ng_GadgetID=id; ng->ng_Flags=PLACETEXT_LEFT;
    ng->ng_VisualInfo=p->vi;
}

static struct Gadget *create_button(struct AppearancePrefs *p, int x, int y,
                                    int w, UWORD id, STRPTR title)
{
    struct NewGadget ng;
    setup_gadget(p,&ng,x,y,w,16,id,title);
    ng.ng_Width=nd_prefs_layout_button_width(&p->layout,w,title);
    ng.ng_Flags=PLACETEXT_IN;
    p->last=CreateGadget(BUTTON_KIND,p->last,&ng,
                         GA_Disabled,(ULONG)(title[0]=='>'),TAG_DONE);
    return p->last;
}

static void theme_rgb(struct AppearancePrefs *p,const char *theme_id,
                      UBYTE rgb[A_COLOR_COUNT][3])
{
    struct NDThemeSpec spec;
    if (!nd_theme_load_id(p->theme_root,theme_id,&spec))
        nd_theme_load_id("SYS:NovaDock",theme_id,&spec);
    memcpy(rgb,spec.rgb,A_COLOR_COUNT*3);
}

static void free_pens(struct AppearancePrefs *p)
{
    int i;
    if (!p->screen) return;
    for (i=0;i<A_COLOR_COUNT;++i) {
        if (p->pens[i]>=0)
            ReleasePen(p->screen->ViewPort.ColorMap,p->pens[i]);
        p->pens[i]=-1;
    }
}

static void obtain_pens(struct AppearancePrefs *p)
{
    UBYTE rgb[A_COLOR_COUNT][3];
    int i;
    free_pens(p);
    theme_rgb(p,nd_config_theme_id(p->cfg),rgb);
    for (i=0;i<A_COLOR_COUNT;++i)
        p->pens[i]=ObtainBestPenA(p->screen->ViewPort.ColorMap,
                      RGBBYTE(rgb[i][0]),RGBBYTE(rgb[i][1]),RGBBYTE(rgb[i][2]),NULL);
}

static ULONG preview_pen(struct AppearancePrefs *p, int index)
{
    return p->pens[index]>=0 ? (ULONG)p->pens[index] :
           (ULONG)(index==3 ? 1 : 0);
}

static void preview_text(struct AppearancePrefs *p, int x, int y,
                         int color, const char *text)
{
    SetAPen(p->win->RPort,preview_pen(p,color));
    Move(p->win->RPort,x,y);
    Text(p->win->RPort,(STRPTR)text,strlen(text));
}

static void preview_centered_text(struct AppearancePrefs *p,int x,int y,
                                  int w,int color,const char *text)
{
    int width=(int)TextLength(p->win->RPort,(STRPTR)text,strlen(text));
    preview_text(p,x+(w-width)/2,y,color,text);
}

static void preview_meter(struct AppearancePrefs *p,int x,int y,int w,
                          int color,int percent)
{
    struct RastPort *rp=p->win->RPort;
    if (w<4) return;
    SetAPen(rp,preview_pen(p,6));
    RectFill(rp,x,y,x+w-1,y+2);
    if (percent>0) {
        SetAPen(rp,preview_pen(p,color));
        RectFill(rp,x,y,x+(w*percent)/100-1,y+2);
    }
}

static void preview_tiny_running(struct AppearancePrefs *p,int x,int y,int w)
{
    struct RastPort *rp=p->win->RPort;
    SetAPen(rp,preview_pen(p,7));
    RectFill(rp,x+w-9,y+4,x+w-6,y+7);
}

static void preview_tiny_icon(struct AppearancePrefs *p,int x,int y,
                              int w,int h,int artwork,int running)
{
    struct RastPort *rp=p->win->RPort;
    int size=h-8,ix,iy,cx,cy;
    if (size>w-8) size=w-8;
    if (size<8) size=8;
    ix=x+(w-size)/2;iy=y+(h-size)/2;
    cx=ix+size/2;cy=iy+size/2;
    if (artwork==0) {
        /* System volume: Workbench disk with the familiar round badge. */
        SetAPen(rp,preview_pen(p,2));
        RectFill(rp,ix+1,iy+size/2,ix+size-2,iy+size-2);
        SetAPen(rp,preview_pen(p,4));
        DrawEllipse(rp,cx,iy+size/3,size/4,size/4);
        SetAPen(rp,preview_pen(p,6));
        Move(rp,ix+1,iy+size/2);Draw(rp,ix+size-2,iy+size/2);
        Move(rp,ix+size/3,iy+size-5);Draw(rp,ix+(size*2)/3,iy+size-5);
    } else if (artwork==1) {
        /* DOpus: a compact two-pane file-manager badge. */
        SetAPen(rp,preview_pen(p,2));
        RectFill(rp,ix,iy+2,ix+size-1,iy+size-2);
        SetAPen(rp,preview_pen(p,4));
        Move(rp,cx,iy+3);Draw(rp,cx,iy+size-3);
        Move(rp,ix+3,iy+size/3);Draw(rp,cx-3,iy+size/3);
        Move(rp,cx+3,iy+(size*2)/3);Draw(rp,ix+size-4,iy+(size*2)/3);
        SetAPen(rp,preview_pen(p,6));
        Move(rp,ix,iy+2);Draw(rp,ix+size-1,iy+2);
        Draw(rp,ix+size-1,iy+size-2);Draw(rp,ix,iy+size-2);Draw(rp,ix,iy+2);
    } else if (artwork==2) {
        /* Shell: small console window with a prompt. */
        SetAPen(rp,preview_pen(p,2));
        RectFill(rp,ix,iy+2,ix+size-1,iy+size-2);
        SetAPen(rp,preview_pen(p,6));
        Move(rp,ix,iy+2);Draw(rp,ix+size-1,iy+2);
        Draw(rp,ix+size-1,iy+size-2);Draw(rp,ix,iy+size-2);Draw(rp,ix,iy+2);
        SetAPen(rp,preview_pen(p,3));
        Move(rp,ix+4,cy-1);Draw(rp,ix+7,cy+1);Draw(rp,ix+4,cy+3);
        Move(rp,ix+9,cy+3);Draw(rp,ix+size-4,cy+3);
    } else {
        /* Music: head with over-ear headphones, matching the idle tile. */
        SetAPen(rp,preview_pen(p,3));
        DrawEllipse(rp,cx,cy+1,size/5,size/4);
        Move(rp,cx-size/3,cy);Draw(rp,cx-size/3,iy+size/3);
        Draw(rp,cx-size/5,iy+size/5);Draw(rp,cx+size/5,iy+size/5);
        Draw(rp,cx+size/3,iy+size/3);Draw(rp,cx+size/3,cy);
        SetAPen(rp,preview_pen(p,5));
        RectFill(rp,cx-size/3-1,cy-2,cx-size/3+2,cy+4);
        RectFill(rp,cx+size/3-2,cy-2,cx+size/3+1,cy+4);
    }
    if (running) preview_tiny_running(p,x,y,w);
}

static void preview_tiny_clock(struct AppearancePrefs *p,int x,int y,int w,int h)
{
    struct RastPort *rp=p->win->RPort;
    int cx=x+w/2,cy=y+h/2,rad=(h-8)/2;
    if (rad>(w-8)/2) rad=(w-8)/2;
    if (rad<6) return;
    SetAPen(rp,preview_pen(p,4));
    DrawEllipse(rp,cx,cy,rad,rad);
    SetAPen(rp,preview_pen(p,3));
    Move(rp,cx,cy);Draw(rp,cx-3,cy-5);
    SetAPen(rp,preview_pen(p,5));
    Move(rp,cx,cy);Draw(rp,cx+rad-3,cy-2);
}

static void preview_tiny_sample(struct AppearancePrefs *p,int kind,
                                int x,int y,int w,int h)
{
    int meter_x=x+5,meter_w=w-10;
    switch (kind) {
    case 0:
        preview_tiny_icon(p,x,y,w,h,0,0);
        break;
    case 1:
        preview_tiny_icon(p,x,y,w,h,1,p->cfg->running_apps);
        break;
    case 2:
        preview_tiny_icon(p,x,y,w,h,2,p->cfg->running_apps);
        break;
    case 3:
        preview_tiny_icon(p,x,y,w,h,3,0);
        break;
    case 4:
        preview_centered_text(p,x,y+h/2+3,w,3,"EXTR");
        break;
    case 5:
        preview_centered_text(p,x,y+9,w,3,"RAM");
        preview_meter(p,meter_x,y+14,meter_w,5,28);
        preview_meter(p,meter_x,y+h-6,meter_w,7,61);
        break;
    case 6:
        preview_centered_text(p,x,y+10,w,3,"CPU");
        preview_meter(p,meter_x,y+h-6,meter_w,5,37);
        break;
    default:
        if (!strcmp(nd_option_get(p->cfg,"CLOCK","FORMAT"),"ANALOG"))
            preview_tiny_clock(p,x,y,w,h);
        else
            preview_centered_text(p,x,y+h/2+3,w,3,"12:34");
        break;
    }
}

static void preview_normal_sample(struct AppearancePrefs *p,int kind,
                                  int x,int y,int w,int h)
{
    int meter_x=x+6,meter_w=w-12;
    switch (kind) {
    case 0:
        /* A real launcher-like Shell sample: artwork, label and optional
         * running-app mark instead of a generic SHELL text rectangle. */
        preview_tiny_icon(p,x,y+2,w,h-18,2,p->cfg->running_apps);
        preview_centered_text(p,x,y+h-5,w,3,"SHELL");
        break;
    case 1:
        preview_centered_text(p,x,y+10,w,3,"CLOCK");
        preview_tiny_clock(p,x,y+10,w,h-12);
        break;
    case 2:
        preview_centered_text(p,x,y+10,w,3,"CPU");
        preview_centered_text(p,x,y+h/2+3,w,3,"37%");
        preview_meter(p,meter_x,y+h-7,meter_w,5,37);
        break;
    default:
        preview_centered_text(p,x,y+10,w,3,"RAM");
        preview_meter(p,meter_x,y+h/2,meter_w,5,28);
        preview_meter(p,meter_x,y+h-7,meter_w,7,61);
        break;
    }
}

static void preview_tile(struct AppearancePrefs *p,int x,int y,int w,int h,int color)
{
    struct RastPort *rp=p->win->RPort;
    SetAPen(rp,preview_pen(p,color));
    if (p->cfg->shape==ND_SHAPE_MINIMAL) {
        /* Minimal has no decorative progress-like baseline. */
    } else if (p->cfg->shape==ND_SHAPE_CHAMFER) {
        RectFill(rp,x+5,y,x+w-5,y+h);
        RectFill(rp,x+2,y+3,x+w-2,y+h-3);
        RectFill(rp,x,y+6,x+w,y+h-6);
    } else if (p->cfg->shape==ND_SHAPE_ROUNDED) {
        RectFill(rp,x+2,y,x+w-2,y+h);
        RectFill(rp,x,y+2,x+w,y+h-2);
    } else RectFill(rp,x,y,x+w,y+h);
    if (p->cfg->shape==ND_SHAPE_MINIMAL || w<8 || h<8) return;
    SetAPen(rp,preview_pen(p,color==2 ? 5 : 2));
    if (p->cfg->shape==ND_SHAPE_CHAMFER) {
        Move(rp,x+5,y);Draw(rp,x+w-5,y);
        Move(rp,x+5,y);Draw(rp,x,y+5);Draw(rp,x,y+h-5);
        SetAPen(rp,preview_pen(p,6));
        Move(rp,x+w-5,y);Draw(rp,x+w,y+5);Draw(rp,x+w,y+h-5);
        Draw(rp,x+w-5,y+h);Draw(rp,x+5,y+h);Draw(rp,x,y+h-5);
    } else if (p->cfg->shape==ND_SHAPE_ROUNDED) {
        Move(rp,x+2,y);Draw(rp,x+w-2,y);
        Move(rp,x+1,y+1);Draw(rp,x,y+2);Draw(rp,x,y+h-2);
        SetAPen(rp,preview_pen(p,6));
        Move(rp,x+w-1,y+1);Draw(rp,x+w,y+2);Draw(rp,x+w,y+h-2);
        Draw(rp,x+w-2,y+h);Draw(rp,x+2,y+h);
    } else {
        Move(rp,x,y+h-1);Draw(rp,x,y);Draw(rp,x+w-1,y);
        SetAPen(rp,preview_pen(p,6));
        Move(rp,x+w,y);Draw(rp,x+w,y+h);Draw(rp,x,y+h);
    }
}

static void preview_background(struct AppearancePrefs *p,int x,int y,int w,int h)
{
    struct RastPort *rp=p->win->RPort;
    SetAPen(rp,preview_pen(p,0));
    RectFill(rp,x,y,x+w,y+h);
}

static void draw_preview(struct AppearancePrefs *p)
{
    struct RastPort *rp;
    static const int tiny_kinds[4]={2,7,6,5};
    int x,y,w,h,gap=3,i,column,columns=4,samples=4,start_x;
    int sx,sy,sw,sh;
    if (!p->win) return;
    obtain_pens(p);
    rp=p->win->RPort;
    SetDrMd(rp,JAM1);
    preview_background(p,nd_prefs_layout_x(&p->layout,140),
        nd_prefs_layout_y(&p->layout,46),
        nd_prefs_layout_w(&p->layout,468),
        nd_prefs_layout_y(&p->layout,68));
    SetAPen(rp,preview_pen(p,4));
    Move(rp,nd_prefs_layout_x(&p->layout,140),nd_prefs_layout_y(&p->layout,46));
    Draw(rp,nd_prefs_layout_x(&p->layout,608),nd_prefs_layout_y(&p->layout,46));
    Draw(rp,nd_prefs_layout_x(&p->layout,608),nd_prefs_layout_y(&p->layout,114));
    Draw(rp,nd_prefs_layout_x(&p->layout,140),nd_prefs_layout_y(&p->layout,114));
    Draw(rp,nd_prefs_layout_x(&p->layout,140),nd_prefs_layout_y(&p->layout,46));
    /* Both sizes compare the same four representative items.  Tiny keeps
     * their real compact proportions and centres the shorter row instead of
     * substituting an unrelated eight-item sample. */
    if (p->cfg->item_size==ND_ITEM_SIZE_TINY) {
        w=70;h=48;
        start_x=147+(454-(columns*w+(columns-1)*gap))/2;
        y=56;
    } else {
        w=(454-(columns-1)*gap)/columns;h=58;
        start_x=147;y=51;
    }
    for (i=0;i<samples;++i) {
        column=i%columns;x=start_x+column*(w+gap);
        sx=nd_prefs_layout_x(&p->layout,x);
        sy=nd_prefs_layout_y(&p->layout,y);
        sw=nd_prefs_layout_w(&p->layout,w);
        sh=nd_prefs_layout_y(&p->layout,h);
        preview_tile(p,sx,sy,sw,sh,i==1 ? 2 : 1);
        if (p->cfg->item_size==ND_ITEM_SIZE_TINY) {
            preview_tiny_sample(p,tiny_kinds[i],sx,sy,sw,sh);
        } else {
            preview_normal_sample(p,i,sx,sy,sw,sh);
        }
    }
}

static void set_gadgets(struct AppearancePrefs *p)
{
    char width_text[8],height_text[8],gap_text[8];
    int tiny=p->cfg->item_size==ND_ITEM_SIZE_TINY;
    if (tiny) p->cfg->label_mode=ND_LABEL_HIDDEN;
    sprintf(width_text,"%d",p->cfg->tile_width);
    sprintf(height_text,"%d",p->cfg->tile_height);
    sprintf(gap_text,"%d",p->cfg->gap);
    GT_SetGadgetAttrs(p->g_theme,p->win,NULL,
                      GTCY_Active,catalogue_index(p,nd_config_theme_id(p->cfg)),
                      TAG_DONE);
    GT_SetGadgetAttrs(p->g_side,p->win,NULL,GTCY_Active,p->cfg->side,TAG_DONE);
    GT_SetGadgetAttrs(p->g_position,p->win,NULL,GTCY_Active,p->cfg->position,TAG_DONE);
    GT_SetGadgetAttrs(p->g_font,p->win,NULL,GTCY_Active,p->cfg->font_mode,TAG_DONE);
    GT_SetGadgetAttrs(p->g_labels,p->win,NULL,
                      GTCY_Active,p->cfg->label_mode,
                      GA_Disabled,(ULONG)tiny,TAG_DONE);
    GT_SetGadgetAttrs(p->g_size,p->win,NULL,GTCY_Active,p->cfg->item_size,TAG_DONE);
    GT_SetGadgetAttrs(p->g_shape,p->win,NULL,GTCY_Active,p->cfg->shape,TAG_DONE);
    GT_SetGadgetAttrs(p->g_width,p->win,NULL,GTST_String,(ULONG)width_text,TAG_DONE);
    GT_SetGadgetAttrs(p->g_height,p->win,NULL,GTST_String,(ULONG)height_text,TAG_DONE);
    GT_SetGadgetAttrs(p->g_gap,p->win,NULL,GTST_String,(ULONG)gap_text,TAG_DONE);
    GT_SetGadgetAttrs(p->g_running,p->win,NULL,
                      GTCY_Active,RUNNING_STATE(p->cfg),TAG_DONE);
    draw_preview(p);
}

static int collect_numbers(struct AppearancePrefs *p, int report)
{
    LONG w,h,gap;
    if (!nd_numeric_gadget_read(p->g_width,p->win,&w) ||
        !nd_numeric_gadget_read(p->g_height,p->win,&h) ||
        !nd_numeric_gadget_read(p->g_gap,p->win,&gap)) {
        if (report) {
            SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - enter numeric dimensions",(STRPTR)-1);
            DisplayBeep(p->screen);
        }
        return 0;
    }
    if (w<55 || w>140 || h<46 || h>100 || gap<0 || gap>16) {
        if (report) {
            SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - invalid dimensions",(STRPTR)-1);
            DisplayBeep(p->screen);
        }
        return 0;
    }
    p->cfg->tile_width=(int)w;p->cfg->tile_height=(int)h;p->cfg->gap=(int)gap;
    return 1;
}

static void create_settings_dir(const char *path)
{
    BPTR lock=CreateDir((STRPTR)path);
    if (lock) UnLock(lock);
}

static int file_exists(const char *path)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    if (!lock) return 0;
    UnLock(lock);return 1;
}

static int copy_file(const char *source_name,const char *dest_name)
{
    FILE *source,*destination;
    char buffer[1024];
    size_t got;
    int ok=1;
    source=fopen(source_name,"rb");
    if (!source) return 0;
    destination=fopen(dest_name,"wb");
    if (!destination) {fclose(source);return 0;}
    while ((got=fread(buffer,1,sizeof(buffer),source))!=0)
        if (fwrite(buffer,1,got,destination)!=got) {ok=0;break;}
    if (ferror(source)) ok=0;
    if (fclose(source)!=0) ok=0;
    if (fclose(destination)!=0) ok=0;
    return ok;
}

static int drawer_theme_id(const char *drawer,char *id,int id_size)
{
    const char *slash,*colon,*start;
    int len;
    if (!drawer || !id || id_size<2) return 0;
    len=(int)strlen(drawer);
    while (len>0 && drawer[len-1]=='/') --len;
    slash=NULL;colon=NULL;
    {
        int i;
        for (i=0;i<len;++i) {
            if (drawer[i]=='/') slash=drawer+i;
            else if (drawer[i]==':') colon=drawer+i;
        }
    }
    start=slash ? slash+1 : (colon ? colon+1 : drawer);
    len-=(int)(start-drawer);
    if (len<=0 || len>=id_size || len>=ND_THEME_ID_LEN) return 0;
    memcpy(id,start,len);id[len]=0;
    return nd_theme_id_valid(id);
}

/* Imported themes can be removed again: select one and Import... offers
 * removal.  Never a bundled theme, nor one named by the live (ENV:) or saved
 * (ENVARC:) configuration.  Only the files an import creates are deleted;
 * the drawer goes only if that leaves it empty. */
static int same_theme(const char *a,const char *b)
{
    while (*a && *b) {
        char x=*a++,y=*b++;
        if (x>='A' && x<='Z') x=(char)(x-'A'+'a');
        if (y>='A' && y<='Z') y=(char)(y-'A'+'a');
        if (x!=y) return 0;
    }
    return *a==*b;
}
/* THEME= of a configuration file, without loading the whole file. */
static int file_theme(const char *path,char *out,int size)
{
    FILE *f;
    char line[160],*v;
    int found=0;
    out[0]=0;
    f=fopen(path,"r");
    if (!f) return 0;
    while (!found && fgets(line,sizeof(line),f))
        if (!strncmp(line,"THEME=",6)) {
            v=line+6;v[strcspn(v,"\r\n \t")]=0;
            strncpy(out,v,size-1);out[size-1]=0;found=out[0]!=0;
        }
    fclose(f);
    return found;
}
static const char *live_theme(struct AppearancePrefs *p)
{
    static char theme[ND_THEME_ID_LEN];
    if (file_theme("ENV:NovaDock/novadock.cfg",theme,sizeof(theme))) return theme;
    return nd_config_theme_id(p->baseline);
}
static int theme_removable(struct AppearancePrefs *p,const char *id)
{
    char themes_dir[512],theme_dir[512],path[512],saved[ND_THEME_ID_LEN];
    if (!id || nd_theme_index_from_id(id)>=0) return 0;   /* bundled */
    if (same_theme(id,live_theme(p)) ||
        (file_theme("ENVARC:NovaDock/novadock.cfg",saved,sizeof(saved)) &&
         same_theme(id,saved))) return 0;                 /* in use */
    return join_path(themes_dir,sizeof(themes_dir),p->theme_root,"Themes") &&
           join_path(theme_dir,sizeof(theme_dir),themes_dir,id) &&
           join_path(path,sizeof(path),theme_dir,"theme.cfg") &&
           file_exists(path);
}

static int remove_selected_theme(struct AppearancePrefs *p,const char *id)
{
    struct EasyStruct es;
    struct NDThemeSpec spec;
    char themes_dir[512],theme_dir[512],path[512],extra[520];
    static const char *const leftovers[]={".bak",".tmp",NULL};
    ULONG args[2];
    int i;
    BPTR lock;
    if (!join_path(themes_dir,sizeof(themes_dir),p->theme_root,"Themes") ||
        !join_path(theme_dir,sizeof(theme_dir),themes_dir,id) ||
        !join_path(path,sizeof(path),theme_dir,"theme.cfg")) return 0;
    memset(&es,0,sizeof(es));
    es.es_StructSize=sizeof(es);
    es.es_Title=(UBYTE *)"NovaDock Appearance";
    es.es_TextFormat=(UBYTE *)"Remove the imported theme %s?\n\nIts files are deleted from\n%s";
    es.es_GadgetFormat=(UBYTE *)"Remove|Cancel";
    args[0]=(ULONG)id;args[1]=(ULONG)theme_dir;
    if (EasyRequestArgs(p->win,&es,NULL,args)!=1) return 0;
    if (!DeleteFile((STRPTR)path)) {
        SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - theme could not be removed",(STRPTR)-1);
        DisplayBeep(p->screen);return 0;
    }
    for (i=0;leftovers[i];++i) {
        sprintf(extra,"%s%s",path,leftovers[i]);
        DeleteFile((STRPTR)extra);
    }
    /* Succeeds only for an empty drawer: other files are never touched. */
    lock=Lock((STRPTR)theme_dir,ACCESS_READ);
    if (lock) {UnLock(lock);DeleteFile((STRPTR)theme_dir);}
    /* Back to the theme the dock is using. */
    nd_config_set_theme(p->cfg,live_theme(p));
    if (!nd_theme_load_id(p->theme_root,nd_config_theme_id(p->cfg),&spec))
        nd_theme_load_id("SYS:NovaDock",nd_config_theme_id(p->cfg),&spec);
    nd_theme_apply_preferences(p->cfg,&spec);
    nd_theme_apply_spacers(p->cfg,&spec);
    ND_PREFS_MARK_DIRTY();
    return 1;
}

static int install_selected_theme(struct AppearancePrefs *p)
{
    struct FileRequester *fr;
    struct NDThemeSpec spec;
    char source[512],themes_dir[512],theme_dir[512];
    char destination[512],temporary[512],backup[512],id[ND_THEME_ID_LEN];
    int had_destination,ok=0;
    BPTR lock;
    if (!AslBase) {
        SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - file requester unavailable",(STRPTR)-1);
        DisplayBeep(p->screen);return 0;
    }
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)"Select a theme.cfg to import",
        ASLFR_Window,(ULONG)p->win,
        ASLFR_SleepWindow,TRUE,
        ASLFR_InitialFile,(ULONG)"theme.cfg",TAG_DONE);
    if (!fr) return 0;
    if (!AslRequest(fr,NULL)) {FreeAslRequest(fr);return 0;}
    if (!text_equal(fr->fr_File,"theme.cfg") ||
        !drawer_theme_id(fr->fr_Drawer,id,sizeof(id))) {
        FreeAslRequest(fr);
        SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - select theme.cfg inside a named theme drawer",(STRPTR)-1);
        DisplayBeep(p->screen);return 0;
    }
    strncpy(source,fr->fr_Drawer,sizeof(source)-1);source[sizeof(source)-1]=0;
    if (!AddPart(source,fr->fr_File,sizeof(source))) source[0]=0;
    FreeAslRequest(fr);
    if (!source[0] || !join_path(themes_dir,sizeof(themes_dir),p->theme_root,"Themes") ||
        !join_path(theme_dir,sizeof(theme_dir),themes_dir,id) ||
        !join_path(destination,sizeof(destination),theme_dir,"theme.cfg")) goto failed;
    create_settings_dir(themes_dir);
    lock=CreateDir((STRPTR)theme_dir);if (lock) UnLock(lock);
    if ((int)strlen(destination)+5>=(int)sizeof(temporary)) goto failed;
    strcpy(temporary,destination);strcat(temporary,".tmp");
    strcpy(backup,destination);strcat(backup,".bak");
    DeleteFile((STRPTR)temporary);DeleteFile((STRPTR)backup);
    if (!copy_file(source,temporary)) goto failed;
    had_destination=file_exists(destination);
    if (had_destination && !Rename((STRPTR)destination,(STRPTR)backup)) goto failed_temp;
    if (!Rename((STRPTR)temporary,(STRPTR)destination)) {
        if (had_destination) Rename((STRPTR)backup,(STRPTR)destination);
        goto failed_temp;
    }
    if (!nd_theme_load_id(p->theme_root,id,&spec)) {
        DeleteFile((STRPTR)destination);
        if (had_destination) Rename((STRPTR)backup,(STRPTR)destination);
        goto failed;
    }
    DeleteFile((STRPTR)backup);
    nd_config_set_theme(p->cfg,id);
    nd_theme_apply_preferences(p->cfg,&spec);
    nd_theme_apply_spacers(p->cfg,&spec);
    ND_PREFS_MARK_DIRTY();ok=1;
    SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - theme imported",(STRPTR)-1);
    return ok;
failed_temp:
    DeleteFile((STRPTR)temporary);
failed:
    SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - theme import failed",(STRPTR)-1);
    DisplayBeep(p->screen);return 0;
}

static int apply_appearance(struct AppearancePrefs *p, int permanent,int finish)
{
    struct NDLayout layout;
    LONG rc;
    int fits;
    if (!collect_numbers(p,1)) return 0;
    fits=nd_layout_fit_enabled(p->cfg,p->screen->Width,p->screen->Height,
                               p->screen->BarHeight,&layout);
    if (!fits) {
        SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - dock does not fit screen",(STRPTR)-1);
        DisplayBeep(p->screen);return 0;
    }
    create_settings_dir("ENV:NovaDock");
    if (permanent) {
        create_settings_dir("ENVARC:NovaDock");
        if (!nd_control_write_config(p->cfg,"ENVARC:NovaDock/novadock.cfg")) return 0;
    }
    if (!nd_control_write_config(p->cfg,"ENV:NovaDock/novadock.cfg")) return 0;
    rc=nd_control_reload();
    if (finish) {
        /* The requested configuration is already safely in ENV: (and in
         * ENVARC: for Save).  A missing, old, or temporarily busy dock must
         * not trap Use/Save in Preferences; it will read the file on its next
         * successful reload or start. */
        ND_PREFS_MARK_CLEAN();ND_PREFS_CLEAR_PREVIEW();
        p->done=1;p->status=0;
    } else {
        ND_PREFS_MARK_PREVIEW();
        if (rc==ND_CONTROL_OK || rc==ND_CONTROL_NOHOST)
            SetWindowTitles(p->win,(STRPTR)"NovaDock Preferences - Appearance (test active)",(STRPTR)-1);
        else if (rc==ND_CONTROL_PENDING)
            /* Accepted; the dock is still rebuilding its artwork. */
            SetWindowTitles(p->win,(STRPTR)"NovaDock Preferences - Appearance (test applying...)",(STRPTR)-1);
        else {
            SetWindowTitles(p->win,(STRPTR)(rc==ND_CONTROL_TIMEOUT ?
                "NovaDock Appearance - test written; dock reload timed out" :
                "NovaDock Appearance - test written; dock rejected reload"),(STRPTR)-1);
            DisplayBeep(p->screen);
        }
    }
    return 1;
}

static int open_editor(struct AppearancePrefs *p)
{
    struct NewGadget ng;
    char width_text[8],height_text[8],gap_text[8];
    int left,top,width,height,font_x=8,font_y=8;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    nd_prefs_layout_init(&p->layout,font_x,font_y,
        p->screen->Width,p->screen->Height,p->screen->BarHeight,620,267);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    left=nd_prefs_layout_left(&p->layout);top=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfo(p->screen,TAG_DONE);
    if (!p->vi || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;
    sprintf(width_text,"%d",p->cfg->tile_width);
    sprintf(height_text,"%d",p->cfg->tile_height);
    sprintf(gap_text,"%d",p->cfg->gap);
    if (!create_button(p,10,21,118,A_GENERAL,"General") ||
        !create_button(p,10,45,118,A_APPEARANCE,"> Appearance") ||
        !create_button(p,10,69,118,A_ITEMS,"Organize") ||
        !create_button(p,10,93,118,A_PLUGIN_MANAGER,"Plugins") ||
        !create_button(p,10,117,118,A_DIAGNOSTICS,"Diagnostics") ||
        !create_button(p,10,141,118,A_ABOUT,"About")) return 0;
    setup_gadget(p,&ng,190,21,130,16,A_THEME,"Theme:");
    p->g_theme=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)p->theme_labels,
        GTCY_Active,catalogue_index(p,nd_config_theme_id(p->cfg)),TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,380,21,100,16,A_SHAPE,"Shape:");
    p->g_shape=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_shapes,GTCY_Active,p->cfg->shape,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,535,21,70,16,A_RUNNING,"Apps:");
    p->g_running=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_running,
        GTCY_Active,RUNNING_STATE(p->cfg),TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,255,121,115,16,A_SIDE,"Dock side:");
    p->g_side=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_sides,GTCY_Active,p->cfg->side,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,500,121,105,16,A_POSITION,"Position:");
    p->g_position=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_positions,GTCY_Active,p->cfg->position,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,240,149,140,16,A_FONT,"Font:");
    p->g_font=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_fonts,GTCY_Active,p->cfg->font_mode,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,505,149,100,16,A_SIZE,"Size:");
    p->g_size=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_item_sizes,
        GTCY_Active,p->cfg->item_size,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,255,177,115,16,A_LABELS,"Labels:");
    p->g_labels=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_labels,GTCY_Active,p->cfg->label_mode,
        GA_Disabled,(ULONG)(p->cfg->item_size==ND_ITEM_SIZE_TINY),TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,230,205,50,16,A_WIDTH,"Width:");
    p->g_width=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)width_text,GTST_MaxChars,3,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,365,205,50,16,A_HEIGHT,"Height:");
    p->g_height=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)height_text,GTST_MaxChars,3,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,500,205,45,16,A_GAP,"Gap:");
    p->g_gap=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)gap_text,GTST_MaxChars,2,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    if (!create_button(p,132,235,82,A_IMPORT_THEME,"Import...") ||
        !create_button(p,222,235,82,A_REVERT,"Revert") ||
        !create_button(p,312,235,82,A_TEST,"Test") ||
        !create_button(p,402,235,82,A_USE,"Use") ||
        !create_button(p,492,235,82,A_SAVE,"Save")) return 0;
    p->win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,
        WA_Left,left,WA_Top,top,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)(ND_PREFS_IS_DIRTY() ?
            "NovaDock Preferences - Appearance *" :
            "NovaDock Preferences - Appearance"),
        WA_Gadgets,(ULONG)p->gadgets,WA_DragBar,TRUE,WA_CloseGadget,TRUE,
        WA_Activate,TRUE,WA_DepthGadget,TRUE,
        WA_IDCMP,IDCMP_GADGETUP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,TAG_DONE);
    if (!p->win) return 0;
    GT_RefreshWindow(p->win,NULL);draw_preview(p);
    return 1;
}

static void on_gadget(struct AppearancePrefs *p,UWORD id,UWORD code)
{
    switch (id) {
    case A_THEME:
        if (code<p->theme_count) {
            struct NDThemeSpec spec;
            if (!nd_config_set_theme(p->cfg,p->themes[code].id)) break;
            if (!nd_theme_load_id(p->theme_root,p->themes[code].id,&spec))
                nd_theme_load_id("SYS:NovaDock",p->themes[code].id,&spec);
            nd_theme_apply_preferences(p->cfg,&spec);
            nd_theme_apply_spacers(p->cfg,&spec);
            ND_PREFS_MARK_DIRTY();set_gadgets(p);
            /* Browsing presets changes only the working copy and sample.
             * Test, Use, and Save are the only live-application actions. */
        } break;
    case A_SIDE:
        if (code<=ND_SIDE_RIGHT) {p->cfg->side=code;ND_PREFS_MARK_DIRTY();draw_preview(p);} break;
    case A_POSITION:
        if (code<ND_POSITION_COUNT) {p->cfg->position=code;ND_PREFS_MARK_DIRTY();draw_preview(p);} break;
    case A_FONT:
        if (code<ND_FONT_COUNT) {p->cfg->font_mode=code;ND_PREFS_MARK_DIRTY();draw_preview(p);} break;
    case A_LABELS:
        if (p->cfg->item_size!=ND_ITEM_SIZE_TINY && code<ND_LABEL_COUNT) {
            p->cfg->label_mode=code;ND_PREFS_MARK_DIRTY();draw_preview(p);
        } break;
    case A_SIZE:
        if (code<ND_ITEM_SIZE_COUNT) {
            p->cfg->item_size=code;
            if (code==ND_ITEM_SIZE_TINY)
                p->cfg->label_mode=ND_LABEL_HIDDEN;
            ND_PREFS_MARK_DIRTY();set_gadgets(p);
        } break;
    case A_SHAPE:
        if (code<ND_SHAPE_COUNT) {p->cfg->shape=code;ND_PREFS_MARK_DIRTY();draw_preview(p);} break;
    case A_RUNNING:
        if (code<=1) {
            p->cfg->running_apps=code==1;
            p->cfg->running_section=0;
            ND_PREFS_MARK_DIRTY();draw_preview(p);
        } break;
    case A_WIDTH: case A_HEIGHT: case A_GAP:
        if (collect_numbers(p,0)) {ND_PREFS_MARK_DIRTY();draw_preview(p);}break;
    case A_REVERT:
        { int i;
        nd_config_set_theme(p->cfg,nd_config_theme_id(p->baseline));
        p->cfg->shape=p->baseline->shape;
        p->cfg->side=p->baseline->side;p->cfg->position=p->baseline->position;
        p->cfg->font_mode=p->baseline->font_mode;
        p->cfg->label_mode=p->baseline->label_mode;
        p->cfg->item_size=p->baseline->item_size;
        p->cfg->tile_width=p->baseline->tile_width;
        p->cfg->tile_height=p->baseline->tile_height;p->cfg->gap=p->baseline->gap;
        p->cfg->running_apps=p->baseline->running_apps;
        p->cfg->running_section=0;
        for (i=0;i<p->cfg->count && i<p->baseline->count;++i)
            if (p->cfg->items[i].kind==ND_ITEM_SPACER &&
                p->baseline->items[i].kind==ND_ITEM_SPACER) {
                p->cfg->items[i].spacer_width=p->baseline->items[i].spacer_width;
                p->cfg->items[i].spacer_line=p->baseline->items[i].spacer_line;
                p->cfg->items[i].spacer_line_width=
                    p->baseline->items[i].spacer_line_width;
                p->cfg->items[i].spacer_line_color=
                    p->baseline->items[i].spacer_line_color;
            }
        ND_PREFS_MARK_DIRTY();set_gadgets(p);}break;
    case A_IMPORT_THEME: {
        const char *selected=nd_config_theme_id(p->cfg);
        LONG choice=1;
        if (theme_removable(p,selected)) {
            struct EasyStruct es;
            ULONG args[1];
            memset(&es,0,sizeof(es));
            es.es_StructSize=sizeof(es);
            es.es_Title=(UBYTE *)"NovaDock Appearance";
            es.es_TextFormat=(UBYTE *)"%s is an imported theme.\n\nImport another theme, or remove this one?";
            es.es_GadgetFormat=(UBYTE *)"Import...|Remove...|Cancel";
            args[0]=(ULONG)selected;
            choice=EasyRequestArgs(p->win,&es,NULL,args);
        }
        /* Either way, rebuild this page so GadTools gets the new list. */
        if ((choice==1 && install_selected_theme(p)) ||
            (choice==2 && remove_selected_theme(p,selected))) {
            p->done=1;p->status=ND_PREF_NAV_APPEARANCE;
        }
        break;
    }
    case A_APPEARANCE: break;
    case A_ITEMS:
        if (collect_numbers(p,1)) {ND_PREFS_MARK_DIRTY();p->done=1;p->status=ND_PREF_NAV_ORGANIZE;}break;
    case A_TEST: apply_appearance(p,0,0);break;
    case A_USE: apply_appearance(p,0,1);break;
    case A_SAVE: apply_appearance(p,1,1);break;
    case A_GENERAL:
        if (collect_numbers(p,1)) {ND_PREFS_MARK_DIRTY();p->done=1;p->status=ND_PREF_NAV_GENERAL;}break;
    case A_PLUGIN_MANAGER:
        if (collect_numbers(p,1)) {ND_PREFS_MARK_DIRTY();p->done=1;p->status=ND_PREF_NAV_PLUGIN_MANAGER;}break;
    case A_DIAGNOSTICS:
        if (collect_numbers(p,1)) {ND_PREFS_MARK_DIRTY();p->done=1;p->status=ND_PREF_NAV_DIAGNOSTICS;}break;
    case A_ABOUT:
        if (collect_numbers(p,1)) {ND_PREFS_MARK_DIRTY();p->done=1;p->status=ND_PREF_NAV_ABOUT;}break;
    default: break;
    }
}

static void editor_loop(struct AppearancePrefs *p)
{
    struct IntuiMessage *msg;
    ULONG cls;UWORD code,id;
    while (!p->done) {
        Wait(1UL<<p->win->UserPort->mp_SigBit);
        while ((msg=GT_GetIMsg(p->win->UserPort))!=NULL) {
            cls=msg->Class;code=msg->Code;
            id=msg->IAddress ? ((struct Gadget *)msg->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(msg);
            if (cls==IDCMP_CLOSEWINDOW) {p->done=1;p->status=0;}
            else if (cls==IDCMP_RAWKEY && code==ND_HELP_KEY) nd_prefs_help(p->screen);
            else if (cls==IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(p->win);draw_preview(p);GT_EndRefresh(p->win,TRUE);
            } else if (cls==IDCMP_GADGETUP) on_gadget(p,id,code);
        }
    }
}

int nd_appearance_prefs_run(int argc,char **argv)
{
    struct AppearancePrefs *p;
    int parsed,result=20,i;
    (void)argc;(void)argv;
    p=(struct AppearancePrefs *)AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p) return 20;
    for (i=0;i<A_COLOR_COUNT;++i) p->pens[i]=-1;
    {
        /* The NovaDock drawer whose Themes these are: beside Preferences
         * (development), an installation elsewhere (NovaDock:), then the
         * standard places.  Only a candidate that has a Themes drawer
         * counts, and missing volumes or assigns fail without a requester. */
        static const char *candidates[]={
            "PROGDIR:","NovaDock:","SYS:NovaDock","SYS:Utilities/NovaDock","Bin:NovaDock",NULL
        };
        struct Process *self=(struct Process *)FindTask(NULL);
        APTR requesters=self->pr_WindowPtr;
        char themes[300];
        BPTR lock;
        self->pr_WindowPtr=(APTR)-1L;
        for (i=0;candidates[i] && !p->theme_root[0];++i) {
            if (i==0 && !GetProgramDir()) continue;
            strcpy(themes,candidates[i]);
            if (!AddPart((STRPTR)themes,(STRPTR)"Themes",sizeof(themes))) continue;
            lock=Lock((STRPTR)themes,ACCESS_READ);
            if (!lock) continue;
            UnLock(lock);
            lock=Lock((STRPTR)candidates[i],ACCESS_READ);
            if (lock) {
                if (!NameFromLock(lock,p->theme_root,sizeof(p->theme_root)))
                    p->theme_root[0]='\0';
                UnLock(lock);
            }
        }
        self->pr_WindowPtr=requesters;
    }
    if (!p->theme_root[0]) strcpy(p->theme_root,"SYS:NovaDock");
    p->cfg=ND_PREFS_SHARED_CONFIG;
    if (!p->cfg)
        p->cfg=(struct NDConfig *)AllocVec(sizeof(*p->cfg),MEMF_PUBLIC|MEMF_CLEAR);
    p->baseline=(struct NDConfig *)AllocVec(sizeof(*p->baseline),MEMF_PUBLIC|MEMF_CLEAR);
    if (!p->cfg || !p->baseline) goto done;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    AslBase=OpenLibrary("asl.library",37);
    if (!IntuitionBase || !GadToolsBase || !GfxBase) goto done;
    parsed=0;
    if (!ND_PREFS_SHARED_CONFIG) parsed=nd_load_effective(p->cfg);
    if (parsed>0) goto done;
    *p->baseline=*p->cfg;
    if (!build_theme_catalogue(p) || !p->theme_count) goto done;
    p->screen=LockPubScreen("Workbench");
    if (!p->screen || !open_editor(p)) goto done;
    editor_loop(p);result=p->status;
done:
    free_pens(p);
    if (p->win) CloseWindow(p->win);
    if (p->gadgets) FreeGadgets(p->gadgets);
    if (p->vi) FreeVisualInfo(p->vi);
    if (p->screen) UnlockPubScreen(NULL,p->screen);
    if (AslBase) CloseLibrary(AslBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (p->baseline) FreeVec(p->baseline);
    if (p->cfg && !ND_PREFS_SHARED_CONFIG) FreeVec(p->cfg);
    free_theme_catalogue(p);
    FreeVec(p);
    AslBase=NULL;GfxBase=NULL;GadToolsBase=NULL;IntuitionBase=NULL;
    return result;
}
