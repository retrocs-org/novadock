#include "nd_metric_art.h"
#include "nd_icon_colors.h"
/* Appearance page for the unified NovaDock Preferences application.
 * Native GadTools/Intuition, OS 3.0+, baseline 68000.  Editing updates only
 * the local sample; Test is the explicit reversible live-preview action.
 */
#define A_UPDATES 950
#define IntuitionBase NDAppearanceIntuitionBase
#define GadToolsBase NDAppearanceGadToolsBase
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
#include "nd_backdrop.h"
#include "nd_ilbm.h"
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
#define A_BACKGROUND 128
#define A_HOVER_MAGNIFICATION 129
#define A_TOOLTIPS 130
#define A_TRANSPARENCY 131
#define A_COLOURFUL 132
#define A_ALWAYS_ON_TOP 133
#define A_AUTO_HIDE 134
#define A_HIDE_DELAY 135
#define A_HOVER_ANIMATION 136
#define A_DRAG_BAR 137
#define A_COLOR_COUNT 33
#define A_METRIC_COLORS 13
#define RGBBYTE(n) ((ULONG)(n) * 0x01010101UL)

struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
/* Preferences pages are active one at a time. The shared artwork renderer
 * uses the same graphics.library base as the active Appearance page. */
extern struct GfxBase *GfxBase;
struct Library *AslBase = NULL;

struct AppearanceTheme {
    char id[ND_THEME_ID_LEN];
    char label[64];
};

struct AppearancePrefs {
    int compact;
    int notify_reload;
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
    struct Gadget *g_drag_bar;
    struct Gadget *g_tooltips,*g_transparency,*g_colourful,*g_top,*g_auto_hide,*g_delay,*g_hover_animation;
    struct Gadget *g_background;
    struct Gadget *g_hover_magnification;
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
    UBYTE pen_owned[A_COLOR_COUNT];
    struct NDBackdrop background;
    char preview_theme[ND_THEME_ID_LEN];
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
static STRPTR appearance_positions[] = {"Start", "Center", "End", "Custom", NULL};
static STRPTR appearance_fonts[] = {"Workbench", "Topaz 8", "Bold", NULL};
static STRPTR appearance_labels[] = {"Always", "Hover", "Hidden", NULL};
static STRPTR appearance_backgrounds[] = {"Solid", "Artwork", "Wallpaper", NULL};
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
static ULONG background_choice(const struct NDConfig *cfg)
{
    return cfg->theme_artwork ? 1UL : cfg->wallpaper_match ? 2UL : 0UL;
}
static void setup_gadget(struct AppearancePrefs *p, struct NewGadget *ng,
                         int x, int y, int w, int h, UWORD id, STRPTR name)
{
    if(p->compact){
        switch(id){
        case A_THEME:x=190;y=21;w=130;break;
        case A_SHAPE:x=380;y=21;w=100;break;
        case A_RUNNING:x=535;y=21;w=70;break;
        case A_SIDE:x=255;y=93;w=115;break;
        case A_POSITION:x=500;y=93;w=105;break;
        case A_FONT:x=240;y=111;w=140;break;
        case A_SIZE:x=505;y=111;w=100;break;
        case A_LABELS:x=255;y=129;w=115;break;
        case A_BACKGROUND:x=505;y=129;w=100;break;
        case A_WIDTH:x=198;y=147;w=44;break;
        case A_HEIGHT:x=310;y=147;w=44;break;
        case A_GAP:x=398;y=147;w=38;break;
        case A_HOVER_MAGNIFICATION:x=552;y=147;w=44;break;
        case A_COLOURFUL:x=160;y=165;w=18;break;
        case A_TOOLTIPS:x=430;y=165;w=18;break;
        case A_HOVER_ANIMATION:x=522;y=165;w=18;break;
        case A_DRAG_BAR:x=430;y=183;w=18;break;
        case A_TRANSPARENCY:x=285;y=183;w=50;break;
        case A_ALWAYS_ON_TOP:x=160;y=201;w=18;break;
        case A_AUTO_HIDE:x=350;y=201;w=18;break;
        case A_HIDE_DELAY:x=552;y=201;w=44;break;
        case A_IMPORT_THEME:x=132;y=219;w=82;break;
        case A_REVERT:x=222;y=219;w=82;break;
        case A_TEST:x=312;y=219;w=82;break;
        case A_USE:x=402;y=219;w=82;break;
        case A_SAVE:x=492;y=219;w=82;break;
        default:break;
        }
    }
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
    int k;
    if (!nd_theme_load_id(p->theme_root,theme_id,&spec))
        nd_theme_load_id("SYS:NovaDock",theme_id,&spec);
    memcpy(rgb,spec.rgb,ND_THEME_COLOR_COUNT*3);
    nd_icon_palette(spec.rgb,p->cfg->colourful_icons,rgb+ND_THEME_COLOR_COUNT);
    for(k=1;k<=4;++k)nd_metric_palette(spec.rgb,p->cfg->colourful_icons,k,rgb+A_METRIC_COLORS+(k-1)*5);
}

static void free_pens(struct AppearancePrefs *p)
{
    int i;
    if (!p->screen) return;
    for (i=0;i<A_COLOR_COUNT;++i) {
        if (p->pens[i]>=0 && p->pen_owned[i])
            ReleasePen(p->screen->ViewPort.ColorMap,p->pens[i]);
        p->pens[i]=-1;p->pen_owned[i]=0;
    }
}

static void obtain_pens(struct AppearancePrefs *p)
{
    UBYTE rgb[A_COLOR_COUNT][3];
    int i,max_pen;
    free_pens(p);
    max_pen=p->screen->RastPort.BitMap->Depth>=8 ? 255:
            (1<<p->screen->RastPort.BitMap->Depth)-1;
    theme_rgb(p,nd_config_theme_id(p->cfg),rgb);
    for (i=0;i<A_COLOR_COUNT;++i) {
        p->pens[i]=ObtainBestPenA(p->screen->ViewPort.ColorMap,
                      RGBBYTE(rgb[i][0]),RGBBYTE(rgb[i][1]),RGBBYTE(rgb[i][2]),NULL);
        if(p->pens[i]>max_pen){
            ReleasePen(p->screen->ViewPort.ColorMap,p->pens[i]);
            p->pens[i]=-1;
        }
        p->pen_owned[i]=p->pens[i]>=0;
        if(p->pens[i]<0)p->pens[i]=FindColor(p->screen->ViewPort.ColorMap,
                      RGBBYTE(rgb[i][0]),RGBBYTE(rgb[i][1]),RGBBYTE(rgb[i][2]),max_pen);
    }
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
    RectFill(rp,x+w/2-1,y,x+w/2+1,y+1);
}

static void preview_tiny_icon(struct AppearancePrefs *p,int x,int y,
                              int w,int h,int artwork,int running)
{
    struct RastPort *rp=p->win->RPort;
    int margin=artwork==2 ? 8-4*p->cfg->hover_magnification/100:8;
    int size=h-margin,ix,iy,cx,cy;
    if (size>w-margin) size=w-margin;
    if(!p->compact && p->cfg->item_size!=ND_ITEM_SIZE_TINY && !p->cfg->hover_magnification && size>48)size=48;
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
    if (running) preview_tiny_running(p,ix,iy+size+2,size);
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

static void preview_metric(struct AppearancePrefs *p,int kind,int x,int y,int w,int h);

static void preview_tiny_sample(struct AppearancePrefs *p,int kind,
                                int x,int y,int w,int h)
{
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
        preview_metric(p,2,x,y,w,h);
        break;
    case 6:
        preview_metric(p,1,x,y,w,h);
        break;
    case 8:
        preview_metric(p,4,x,y,w,h);
        break;
    default:
        if (!strcmp(nd_option_get(p->cfg,"CLOCK","FORMAT"),"ANALOG"))
            preview_tiny_clock(p,x,y,w,h);
        else
            preview_metric(p,3,x,y,w,h);
        break;
    }
}

struct PreviewMetric {struct AppearancePrefs *p;int x,y,size,kind;};
static void preview_metric_box(void *ctx,int ink,int x,int y,int w,int h)
{
    struct PreviewMetric *p=(struct PreviewMetric *)ctx;
    int color=A_METRIC_COLORS+(p->kind-1)*5+ink;
    struct RastPort *rp=p->p->win->RPort;
    int left=x*p->size/48,top=y*p->size/48;
    int right=(x+w)*p->size/48,bottom=(y+h)*p->size/48;
    if(right<=left || bottom<=top)return;
    SetAPen(rp,preview_pen(p->p,color));
    RectFill(rp,p->x+left,p->y+top,p->x+right-1,p->y+bottom-1);
}
static void preview_metric_text_box(void *ctx,int ink,int x,int y,int w,int h)
{
    struct PreviewMetric *m=(struct PreviewMetric *)ctx;
    struct RastPort *rp=m->p->win->RPort;
    SetAPen(rp,preview_pen(m->p,ink));
    RectFill(rp,x,y,x+w-1,y+h-1);
}
static void preview_metric(struct AppearancePrefs *p,int kind,int x,int y,int w,int h)
{
    struct PreviewMetric m;int baseline;
    int tiny=p->compact || p->cfg->item_size==ND_ITEM_SIZE_TINY;
    m.p=p;m.kind=kind;m.size=h-(tiny ? 4:24);
    if(m.size>w-(tiny ? 4:12))m.size=w-(tiny ? 4:12);
    if(m.size>48)m.size=48;
    if(m.size<16)return;
    m.x=x+(w-m.size)/2;m.y=y+(h-(tiny ? 0:16)-m.size)/2;
    nd_metric_art(&m,preview_metric_box,kind);
    baseline=m.y+25*m.size/48+p->win->RPort->TxBaseline/2;
    if(kind==2){
        preview_metric_box(&m,3,7,16,34*28/100,5);
        preview_metric_box(&m,4,7,27,34*61/100,5);
    }else if(kind==4){
        if(tiny){
            nd_metric_tiny_text(&m,preview_metric_text_box,A_METRIC_COLORS+17,"42",m.x+12*m.size/48,m.y+13*m.size/48,28*m.size/48);
            nd_metric_tiny_text(&m,preview_metric_text_box,A_METRIC_COLORS+19,"12",m.x+12*m.size/48,m.y+24*m.size/48,28*m.size/48);
        }else{
            preview_centered_text(p,m.x+12*m.size/48,m.y+19*m.size/48,28*m.size/48,A_METRIC_COLORS+17,"42");
            preview_centered_text(p,m.x+12*m.size/48,m.y+29*m.size/48,28*m.size/48,A_METRIC_COLORS+19,"12");
        }
    }else{
        int ink=A_METRIC_COLORS+(kind-1)*5+2;
        const char *value=kind==1 ? "37%":"12:34";
        if(tiny)nd_metric_tiny_text(&m,preview_metric_text_box,ink,value,m.x+4*m.size/48,m.y+25*m.size/48-2,40*m.size/48);
        else preview_centered_text(p,m.x+4*m.size/48,baseline,40*m.size/48,ink,value);
        if(kind==1)preview_metric_box(&m,3,11,33,26*37/100,2);
    }
}

static void preview_normal_sample(struct AppearancePrefs *p,int kind,
                                  int x,int y,int w,int h)
{
    int focused=kind==(p->cfg->hover_magnification ? 0:1);
    int label=nd_label_visible(p->cfg->label_mode,0,focused);
    if(p->cfg->label_mode==ND_LABEL_HOVER && p->cfg->hover_magnification)label=0;
    switch (kind) {
    case 0:
        /* A real launcher-like Shell sample: artwork, label and optional
         * running-app mark instead of a generic SHELL text rectangle. */
        preview_tiny_icon(p,x,y+2,w,h-18,2,p->cfg->running_apps);
        if(label)preview_centered_text(p,x,y+h-5,w,3,"SHELL");
        break;
    case 1:
        if(label)preview_centered_text(p,x,y+h-5,w,3,"CLOCK");
        if(!strcmp(nd_option_get(p->cfg,"CLOCK","FORMAT"),"ANALOG"))
            preview_tiny_clock(p,x,y+2,w,h-18);
        else preview_metric(p,3,x,y,w,h);
        break;
    case 2:
        if(label)preview_centered_text(p,x,y+h-5,w,3,"CPU");
        preview_metric(p,1,x,y,w,h);
        break;
    case 3:
        if(label)preview_centered_text(p,x,y+h-5,w,3,"RAM");
        preview_metric(p,2,x,y,w,h);
        break;
    default:
        if(label)preview_centered_text(p,x,y+h-5,w,3,"NET");
        preview_metric(p,4,x,y,w,h);
        break;
    }
}

static void preview_tile(struct AppearancePrefs *p,int x,int y,int w,int h,int color)
{
    struct RastPort *rp=p->win->RPort;
    if (color==2) {
        nd_tile_highlight(rp,x,y,w,h,p->cfg->shape,preview_pen(p,2),preview_pen(p,5));
        return;
    }
    if (p->background.parts[0].bitmap) return;
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
    SetAPen(rp,preview_pen(p,2));
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

struct PreviewGeometry {
    int left,top,width,height;
    int row_x,row_y,tile_width,tile_height;
};

/* Keep the form's preview area fixed, but fit the shelf to the sample row.
 * Coordinates remain in baseline font units until they are painted. */
static void preview_geometry(struct AppearancePrefs *p,struct PreviewGeometry *g)
{
    int grip=p->cfg->drag_bar ? 12:0;
    int available=p->layout.base_width-12-140+1;
    int area_height=(p->compact ? 86:138)-46+1;
    int sample_width=(p->layout.base_width>620 ? 534:454)-grip;
    g->left=140;g->top=46;g->width=available;g->height=area_height;
    g->tile_width=(sample_width-12)/5;
    g->tile_height=p->compact ? 30:78;
    g->row_x=147+grip;g->row_y=p->compact ? 51:55;
    if (p->cfg->item_size==ND_ITEM_SIZE_TINY) {
        g->tile_width=70;g->tile_height=p->compact ? 24:48;
        g->width=5*g->tile_width+12+14+grip;
        g->height=g->tile_height+(p->compact ? 8:14);
        g->left+=(available-g->width)/2;
        g->top+=(area_height-g->height)/2;
        g->row_x=g->left+7+grip;
        g->row_y=g->top+(g->height-g->tile_height)/2;
    }
}

static int preview_thickness(struct AppearancePrefs *p)
{
    struct PreviewGeometry g;
    preview_geometry(p,&g);
    return nd_prefs_layout_y(&p->layout,g.top+g.height-1)-
           nd_prefs_layout_y(&p->layout,g.top)+1;
}

/* Prepare only when a setting changes. Refresh messages just draw the cache. */
static void prepare_preview(struct AppearancePrefs *p)
{
    const char *theme=nd_config_theme_id(p->cfg);
    if (strcmp(p->preview_theme,theme)) {
        nd_backdrop_free(&p->background,p->screen);
        obtain_pens(p);
        strcpy(p->preview_theme,theme);
    }
    if (!p->cfg->theme_artwork) {
        nd_backdrop_free(&p->background,p->screen);
    } else if (p->background.thickness!=preview_thickness(p)) {
        struct DrawInfo *info=GetScreenDrawInfo(p->screen);
        ULONG matte=info ? info->dri_Pens[BACKGROUNDPEN] : 0;
        if (info) FreeScreenDrawInfo(p->screen,info);
        nd_backdrop_load(&p->background,p->screen,p->theme_root,theme,
                         preview_thickness(p),0,matte,NULL);
    }
}

static void appearance_sections(struct AppearancePrefs *p)
{
    static const char *names[]={"Layout","Icons and labels","Background and effects","Behaviour"};
    static const int rows[]={150,244,314,384};
    struct DrawInfo *info;struct RastPort *rp;int i,x,y;
    if(p->compact)return;
    rp=p->win->RPort;info=GetScreenDrawInfo(p->screen);if(!info)return;
    for(i=0;i<4;++i){
        x=nd_prefs_layout_x(&p->layout,148);y=nd_prefs_layout_y(&p->layout,rows[i]);
        SetAPen(rp,info->dri_Pens[TEXTPEN]);Move(rp,x,y);Text(rp,(STRPTR)names[i],strlen(names[i]));
        x+=TextLength(rp,(STRPTR)names[i],strlen(names[i]))+10;
        SetAPen(rp,info->dri_Pens[SHADOWPEN]);Move(rp,x,y-3);Draw(rp,nd_prefs_layout_x(&p->layout,685),y-3);
    }
    FreeScreenDrawInfo(p->screen,info);
}

static void draw_preview(struct AppearancePrefs *p)
{
    struct RastPort *rp;
    struct DrawInfo *info;
    struct PreviewGeometry geometry;
    static const int tiny_kinds[5]={2,7,6,5,8};
    int x,y,w,h,gap=3,i,column,columns=5,samples=5,start_x;
    int sx,sy,sw,sh,preview_x,preview_y,preview_w,preview_h;
    if (!p->win) return;
    rp=p->win->RPort;
    SetDrMd(rp,JAM1);
    appearance_sections(p);
    /* Clear the whole reserved area, including the old larger shelf and
     * transparent corners, before painting the new footprint. */
    info=GetScreenDrawInfo(p->screen);
    SetAPen(rp,info ? info->dri_Pens[BACKGROUNDPEN]:0);
    RectFill(rp,nd_prefs_layout_x(&p->layout,140),
        nd_prefs_layout_y(&p->layout,46),
        nd_prefs_layout_x(&p->layout,p->layout.base_width-12),
        nd_prefs_layout_y(&p->layout,p->compact ? 86:138));
    if (info) FreeScreenDrawInfo(p->screen,info);
    preview_geometry(p,&geometry);
    preview_x=nd_prefs_layout_x(&p->layout,geometry.left);
    preview_y=nd_prefs_layout_y(&p->layout,geometry.top);
    preview_w=nd_prefs_layout_x(&p->layout,geometry.left+geometry.width-1)-preview_x+1;
    preview_h=preview_thickness(p);
    if (!nd_backdrop_paint(&p->background,rp,preview_x,preview_y,preview_w,
                           0,0,preview_w,preview_h)) {
        SetAPen(rp,preview_pen(p,0));
        RectFill(rp,preview_x,preview_y,preview_x+preview_w-1,preview_y+preview_h-1);
        SetAPen(rp,preview_pen(p,4));
        Move(rp,preview_x,preview_y); Draw(rp,preview_x+preview_w-1,preview_y);
        Draw(rp,preview_x+preview_w-1,preview_y+preview_h-1);
        Draw(rp,preview_x,preview_y+preview_h-1); Draw(rp,preview_x,preview_y);
    }
    /* Both sizes compare the same five representative items. */
    w=geometry.tile_width;h=geometry.tile_height;
    start_x=geometry.row_x;y=geometry.row_y;
    if(p->cfg->drag_bar){
        int gx=preview_x+8,gy=preview_y+preview_h/2,j;
        for(j=-6;j<=6;j+=4){
            SetAPen(rp,preview_pen(p,6));RectFill(rp,gx,gy+j,gx+5,gy+j+1);
            SetAPen(rp,preview_pen(p,3));RectFill(rp,gx,gy+j,gx+4,gy+j);
        }
    }
    for (i=0;i<samples;++i) {
        column=i%columns;x=start_x+column*(w+gap);
        sx=nd_prefs_layout_x(&p->layout,x);
        sy=nd_prefs_layout_y(&p->layout,y);
        sw=nd_prefs_layout_w(&p->layout,w);
        sh=nd_prefs_layout_y(&p->layout,h);
        preview_tile(p,sx,sy,sw,sh,(!p->cfg->hover_magnification && i==1) ? 2 : 1);
        if (p->compact || p->cfg->item_size==ND_ITEM_SIZE_TINY) {
            preview_tiny_sample(p,tiny_kinds[i],sx,sy,sw,sh);
        } else {
            preview_normal_sample(p,i,sx,sy,sw,sh);
        }
    }    if(!p->compact && p->cfg->item_size!=ND_ITEM_SIZE_TINY && p->cfg->label_mode==ND_LABEL_HOVER && p->cfg->hover_magnification){
        int bx=nd_prefs_layout_x(&p->layout,147)+nd_prefs_layout_w(&p->layout,w)/2-26;
        int by=preview_y+2,bw=52,bh=rp->TxHeight+6;
        SetAPen(rp,preview_pen(p,0));RectFill(rp,bx,by,bx+bw-1,by+bh-1);
        SetAPen(rp,preview_pen(p,5));Move(rp,bx,by);Draw(rp,bx+bw-1,by);Draw(rp,bx+bw-1,by+bh-1);Draw(rp,bx,by+bh-1);Draw(rp,bx,by);
        preview_centered_text(p,bx,by+3+rp->TxBaseline,bw,3,"Shell");
    }

}

static void set_gadgets(struct AppearancePrefs *p)
{
    char width_text[8],height_text[8],gap_text[8],hover_text[8],transparency_text[8],delay_text[8];
    int tiny=p->cfg->item_size==ND_ITEM_SIZE_TINY;
    if (tiny) p->cfg->label_mode=ND_LABEL_HIDDEN;
    sprintf(width_text,"%d",p->cfg->tile_width);
    sprintf(height_text,"%d",p->cfg->tile_height);
    sprintf(gap_text,"%d",p->cfg->gap);
    sprintf(hover_text,"%d",p->cfg->hover_magnification);
    sprintf(transparency_text,"%d",p->cfg->background_transparency);
    sprintf(delay_text,"%d",p->cfg->hide_delay);
    GT_SetGadgetAttrs(p->g_theme,p->win,NULL,
                      GTCY_Active,catalogue_index(p,nd_config_theme_id(p->cfg)),
                      TAG_DONE);
    GT_SetGadgetAttrs(p->g_side,p->win,NULL,GTCY_Active,p->cfg->side,TAG_DONE);
    GT_SetGadgetAttrs(p->g_position,p->win,NULL,GTCY_Active,p->cfg->position,TAG_DONE);
    GT_SetGadgetAttrs(p->g_font,p->win,NULL,GTCY_Active,p->cfg->font_mode,TAG_DONE);
    GT_SetGadgetAttrs(p->g_colourful,p->win,NULL,GTCB_Checked,(ULONG)p->cfg->colourful_icons,TAG_DONE);
    GT_SetGadgetAttrs(p->g_top,p->win,NULL,GTCB_Checked,(ULONG)p->cfg->always_on_top,TAG_DONE);
    GT_SetGadgetAttrs(p->g_auto_hide,p->win,NULL,GTCB_Checked,(ULONG)p->cfg->auto_hide,TAG_DONE);
    GT_SetGadgetAttrs(p->g_delay,p->win,NULL,GTST_String,(ULONG)delay_text,GA_Disabled,(ULONG)!p->cfg->auto_hide,TAG_DONE);
    GT_SetGadgetAttrs(p->g_tooltips,p->win,NULL,GTCB_Checked,(ULONG)p->cfg->tooltips,TAG_DONE);
    GT_SetGadgetAttrs(p->g_drag_bar,p->win,NULL,GTCB_Checked,(ULONG)p->cfg->drag_bar,TAG_DONE);
    GT_SetGadgetAttrs(p->g_hover_animation,p->win,NULL,GTCB_Checked,(ULONG)p->cfg->hover_animation,TAG_DONE);
    GT_SetGadgetAttrs(p->g_labels,p->win,NULL,
                      GTCY_Active,p->cfg->label_mode,
                      GA_Disabled,(ULONG)tiny,TAG_DONE);
    GT_SetGadgetAttrs(p->g_background,p->win,NULL,GTCY_Active,background_choice(p->cfg),TAG_DONE);
    GT_SetGadgetAttrs(p->g_hover_magnification,p->win,NULL,GTST_String,
                      (ULONG)hover_text,TAG_DONE);
    GT_SetGadgetAttrs(p->g_transparency,p->win,NULL,GTST_String,(ULONG)transparency_text,TAG_DONE);
    GT_SetGadgetAttrs(p->g_size,p->win,NULL,GTCY_Active,p->cfg->item_size,TAG_DONE);
    GT_SetGadgetAttrs(p->g_shape,p->win,NULL,GTCY_Active,p->cfg->shape,TAG_DONE);
    GT_SetGadgetAttrs(p->g_width,p->win,NULL,GTST_String,(ULONG)width_text,TAG_DONE);
    GT_SetGadgetAttrs(p->g_height,p->win,NULL,GTST_String,(ULONG)height_text,TAG_DONE);
    GT_SetGadgetAttrs(p->g_gap,p->win,NULL,GTST_String,(ULONG)gap_text,TAG_DONE);
    GT_SetGadgetAttrs(p->g_running,p->win,NULL,
                      GTCY_Active,RUNNING_STATE(p->cfg),TAG_DONE);
    prepare_preview(p);
    draw_preview(p);
}

static int collect_numbers(struct AppearancePrefs *p, int report)
{
    LONG w,h,gap,hover,transparency,delay=p->cfg->hide_delay;
    if (!nd_numeric_gadget_read(p->g_width,p->win,&w) ||
        !nd_numeric_gadget_read(p->g_height,p->win,&h) ||
        !nd_numeric_gadget_read(p->g_gap,p->win,&gap) ||
        !nd_numeric_gadget_read(p->g_hover_magnification,p->win,&hover) ||
        !nd_numeric_gadget_read(p->g_transparency,p->win,&transparency) ||
        (p->cfg->auto_hide && !nd_numeric_gadget_read(p->g_delay,p->win,&delay))) {
        if (report) {
            SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - enter whole numbers",(STRPTR)-1);
            DisplayBeep(p->screen);
        }
        return 0;
    }
    if (w<55 || w>140 || h<46 || h>100 || gap<0 || gap>16 || hover<0 || hover>100 || transparency<0 || transparency>100 || delay<1 || delay>60) {
        if (report) {
            SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - check sizes, percentages and hide delay (1-60)",(STRPTR)-1);
            DisplayBeep(p->screen);
        }
        return 0;
    }
    p->cfg->tile_width=(int)w;p->cfg->tile_height=(int)h;p->cfg->gap=(int)gap;
    p->cfg->hover_magnification=(int)hover;p->cfg->background_transparency=(int)transparency;p->cfg->hide_delay=(int)delay;
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

/* ---- Theme artwork: imported with a pack, or picked as a backdrop ---- */
static const char *const theme_images[]={"Backdrop.iff","BackdropLeft.iff","BackdropRight.iff",NULL};
/* Indexed ILBM the dock can use as backdrop artwork: at most 96 x 96 pixels
 * and 16 colours, which nd_ilbm_load() enforces. */
static int backdrop_image_valid(const char *path)
{
    struct NDILBM *art=(struct NDILBM *)AllocVec(sizeof(*art),MEMF_ANY);
    int ok;
    if (!art) return 0;
    ok=nd_ilbm_load(path,art)!=0;
    FreeVec(art);
    return ok;
}
/* Copy SOURCE to DEST through a temporary file, so an interrupted copy
 * never replaces a working image with a partial one. */
static int install_file(const char *source,const char *dest)
{
    char temporary[600];
    if ((int)strlen(dest)+5>=(int)sizeof(temporary)) return 0;
    strcpy(temporary,dest);strcat(temporary,".tmp");
    DeleteFile((STRPTR)temporary);
    if (!copy_file(source,temporary)) {DeleteFile((STRPTR)temporary);return 0;}
    DeleteFile((STRPTR)dest);
    if (!Rename((STRPTR)temporary,(STRPTR)dest)) {DeleteFile((STRPTR)temporary);return 0;}
    return 1;
}
/* A pack's Images/Backdrop*.iff go beside its imported theme.cfg.  Returns
 * how many were installed, or -1 when one was present but not usable. */
static int import_theme_images(const char *source_drawer,const char *theme_dir)
{
    char source_images[512],images[512],source[560],dest[560];
    int i,installed=0,bad=0;
    if (!join_path(source_images,sizeof(source_images),source_drawer,"Images") ||
        !join_path(images,sizeof(images),theme_dir,"Images")) return 0;
    for (i=0;theme_images[i];++i) {
        if (!join_path(source,sizeof(source),source_images,theme_images[i]) ||
            !file_exists(source)) continue;
        if (!backdrop_image_valid(source)) {bad=1;continue;}
        create_settings_dir(images);
        if (join_path(dest,sizeof(dest),images,theme_images[i]) && install_file(source,dest)) ++installed;
        else bad=1;
    }
    return bad ? -1 : installed;
}
/* Delete the artwork an import or the picker installed, and Images if empty. */
static void remove_theme_images(const char *theme_dir)
{
    char images[512],path[560],extra[570];
    int i;
    BPTR lock;
    if (!join_path(images,sizeof(images),theme_dir,"Images")) return;
    for (i=0;theme_images[i];++i)
        if (join_path(path,sizeof(path),images,theme_images[i])) {
            DeleteFile((STRPTR)path);
            sprintf(extra,"%s.tmp",path);DeleteFile((STRPTR)extra);
        }
    lock=Lock((STRPTR)images,ACCESS_READ);
    if (lock) {UnLock(lock);DeleteFile((STRPTR)images);}
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
    remove_theme_images(theme_dir);
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
    char destination[512],temporary[512],backup[512],id[ND_THEME_ID_LEN],source_drawer[512];
    int had_destination,ok=0,images;
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
    strcpy(source_drawer,source);
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
    p->preview_theme[0]=0;
    nd_theme_apply_preferences(p->cfg,&spec);
    nd_theme_apply_spacers(p->cfg,&spec);
    ND_PREFS_MARK_DIRTY();ok=1;
    /* The pack's backdrop artwork comes along with its theme.cfg. */
    images=import_theme_images(source_drawer,theme_dir);
    SetWindowTitles(p->win,(STRPTR)(images<0 ?
        "NovaDock Appearance - theme imported; some artwork was not usable" : images ?
        "NovaDock Appearance - theme and artwork imported" :
        "NovaDock Appearance - theme imported"),(STRPTR)-1);
    return ok;
failed_temp:
    DeleteFile((STRPTR)temporary);
failed:
    SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - theme import failed",(STRPTR)-1);
    DisplayBeep(p->screen);return 0;
}

/* Backdrop image...: pick an ILBM as the dock's backdrop artwork.  An
 * imported theme takes it directly; for a bundled theme a "Custom" theme is
 * created from the current one (its colours, shape and placement), so
 * bundled themes are never modified.  The new centre replaces any end caps,
 * which would not match it. */
static int pick_backdrop_image(struct AppearancePrefs *p)
{
    struct FileRequester *fr;
    struct NDThemeSpec spec;
    char image[512],themes_dir[512],theme_dir[512],images[512],path[560],source_cfg[512];
    char target[ND_THEME_ID_LEN],current[ND_THEME_ID_LEN];
    int i,user_theme;
    if (!AslBase) {DisplayBeep(p->screen);return 0;}
    fr=(struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText,(ULONG)"Select a backdrop image (ILBM, up to 96x96, 16 colours)",
        ASLFR_Window,(ULONG)p->win,ASLFR_SleepWindow,TRUE,
        ASLFR_DoPatterns,TRUE,ASLFR_InitialPattern,(ULONG)"#?.(iff|ilbm)",TAG_DONE);
    if (!fr) return 0;
    if (!AslRequest(fr,NULL)) {FreeAslRequest(fr);return 0;}
    strncpy(image,fr->fr_Drawer,sizeof(image)-1);image[sizeof(image)-1]=0;
    if (!AddPart(image,fr->fr_File,sizeof(image))) image[0]=0;
    FreeAslRequest(fr);
    if (!image[0] || !backdrop_image_valid(image)) {
        SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - needs an ILBM of up to 96x96 pixels and 16 colours",(STRPTR)-1);
        DisplayBeep(p->screen);return 0;
    }
    strncpy(current,nd_config_theme_id(p->cfg),sizeof(current)-1);current[sizeof(current)-1]=0;
    if (!join_path(themes_dir,sizeof(themes_dir),p->theme_root,"Themes")) return 0;
    user_theme=nd_theme_index_from_id(current)<0 &&
               join_path(theme_dir,sizeof(theme_dir),themes_dir,current) &&
               join_path(path,sizeof(path),theme_dir,"theme.cfg") && file_exists(path);
    strcpy(target,user_theme ? current : "Custom");
    if (!join_path(theme_dir,sizeof(theme_dir),themes_dir,target) ||
        !join_path(images,sizeof(images),theme_dir,"Images") ||
        !join_path(path,sizeof(path),theme_dir,"theme.cfg")) goto failed;
    create_settings_dir(themes_dir);create_settings_dir(theme_dir);
    if (!user_theme) {
        /* Custom starts as a copy of the current theme, renamed. */
        FILE *in,*out;
        char line[200];
        int ok=1;
        if (!join_path(source_cfg,sizeof(source_cfg),themes_dir,current)) goto failed;
        strcat(source_cfg,"/theme.cfg");
        in=fopen(source_cfg,"r");
        if (!in) {sprintf(source_cfg,"SYS:NovaDock/Themes/%s/theme.cfg",current);in=fopen(source_cfg,"r");}
        out=fopen(path,"w");
        if (!out) {if (in) fclose(in);goto failed;}
        if (fputs("NAME=Custom\n",out)<0) ok=0;
        while (in && ok && fgets(line,sizeof(line),in))
            if (strncmp(line,"NAME=",5) && fputs(line,out)<0) ok=0;
        if (in) fclose(in);
        if (fclose(out)!=0) ok=0;
        if (!ok) {DeleteFile((STRPTR)path);goto failed;}
    }
    create_settings_dir(images);
    for (i=1;theme_images[i];++i)
        if (join_path(source_cfg,sizeof(source_cfg),images,theme_images[i])) DeleteFile((STRPTR)source_cfg);
    if (!join_path(source_cfg,sizeof(source_cfg),images,theme_images[0]) ||
        !install_file(image,source_cfg) ||
        !nd_theme_load_id(p->theme_root,target,&spec)) goto failed;
    nd_config_set_theme(p->cfg,target);
    nd_theme_apply_preferences(p->cfg,&spec);
    nd_theme_apply_spacers(p->cfg,&spec);
    p->cfg->theme_artwork=1;p->cfg->wallpaper_match=0;
    p->preview_theme[0]=0;
    ND_PREFS_MARK_DIRTY();
    SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - backdrop set; Test or Save to apply",(STRPTR)-1);
    return 1;
failed:
    SetWindowTitles(p->win,(STRPTR)"NovaDock Appearance - backdrop image could not be installed",(STRPTR)-1);
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
    rc=ND_CONTROL_OK;
    if (finish) {
        /* Close this window before the dock samples its desktop background. */
        p->notify_reload=1;
        /* The requested configuration is already safely in ENV: (and in
         * ENVARC: for Save).  A missing, old, or temporarily busy dock must
         * not trap Use/Save in Preferences; it will read the file on its next
         * successful reload or start. */
        ND_PREFS_MARK_CLEAN();ND_PREFS_CLEAR_PREVIEW();
        p->done=1;p->status=0;
    } else {
        rc=nd_control_reload();
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
    char width_text[8],height_text[8],gap_text[8],hover_text[8],transparency_text[8],delay_text[8];
    int left,top,width,height,font_x=8,font_y=8;
    if (p->screen->RastPort.Font) {
        font_x=p->screen->RastPort.Font->tf_XSize;
        font_y=p->screen->RastPort.Font->tf_YSize;
    }
    p->compact=(p->screen->Height-p->screen->BarHeight)*8/font_y<480 || p->screen->Width*8/font_x<720;
    nd_prefs_layout_init(&p->layout,font_x,font_y,
        p->screen->Width,p->screen->Height,p->screen->BarHeight,(p->compact ? 620:700),p->compact ? 239:468);
    if (!nd_prefs_layout_fits(&p->layout,10,12)) return 0;
    width=p->layout.window_width;height=p->layout.window_height;
    left=nd_prefs_layout_left(&p->layout);top=nd_prefs_layout_top(&p->layout);
    p->vi=GetVisualInfo(p->screen,TAG_DONE);
    if (!p->vi || !CreateContext(&p->gadgets)) return 0;
    p->last=p->gadgets;
    sprintf(width_text,"%d",p->cfg->tile_width);
    sprintf(height_text,"%d",p->cfg->tile_height);
    sprintf(gap_text,"%d",p->cfg->gap);
    sprintf(hover_text,"%d",p->cfg->hover_magnification);
    sprintf(transparency_text,"%d",p->cfg->background_transparency);
    sprintf(delay_text,"%d",p->cfg->hide_delay);
    if (!create_button(p,10,21,118,A_GENERAL,"General") ||
        !create_button(p,10,45,118,A_APPEARANCE,"> Appearance") ||
        !create_button(p,10,69,118,A_ITEMS,"Organize") ||
        !create_button(p,10,93,118,A_PLUGIN_MANAGER,"Plugins") ||
        !create_button(p,10,117,118,A_DIAGNOSTICS,"Support") ||
        !create_button(p,10,141,118,A_UPDATES,"Update") ||
        !create_button(p,10,165,118,A_ABOUT,"About")) return 0;
    setup_gadget(p,&ng,160,288,18,12,A_COLOURFUL,"Colourful icons");
    ng.ng_Flags=PLACETEXT_RIGHT;
    p->g_colourful=p->last=CreateGadget(CHECKBOX_KIND,p->last,&ng,GTCB_Checked,(ULONG)p->cfg->colourful_icons,TAG_DONE);
    if(!p->last)return 0;
    setup_gadget(p,&ng,160,404,18,12,A_ALWAYS_ON_TOP,"Always on top");
    ng.ng_Flags=PLACETEXT_RIGHT;
    p->g_top=p->last=CreateGadget(CHECKBOX_KIND,p->last,&ng,GTCB_Checked,(ULONG)p->cfg->always_on_top,TAG_DONE);
    if(!p->last)return 0;
    setup_gadget(p,&ng,350,404,18,12,A_AUTO_HIDE,"Auto hide");
    ng.ng_Flags=PLACETEXT_RIGHT;
    p->g_auto_hide=p->last=CreateGadget(CHECKBOX_KIND,p->last,&ng,GTCB_Checked,(ULONG)p->cfg->auto_hide,TAG_DONE);
    if(!p->last)return 0;
    setup_gadget(p,&ng,620,404,44,16,A_HIDE_DELAY,"Hide delay:");
    p->g_delay=p->last=CreateGadget(STRING_KIND,p->last,&ng,GTST_String,(ULONG)delay_text,GTST_MaxChars,2,GA_TabCycle,TRUE,GA_Disabled,(ULONG)!p->cfg->auto_hide,TAG_DONE);
    if(!p->last)return 0;
    setup_gadget(p,&ng,450,288,18,12,A_TOOLTIPS,"Tooltips");
    ng.ng_Flags=PLACETEXT_RIGHT;
    p->g_tooltips=p->last=CreateGadget(CHECKBOX_KIND,p->last,&ng,
        GTCB_Checked,(ULONG)p->cfg->tooltips,TAG_DONE);
    if(!p->last)return 0;
    setup_gadget(p,&ng,200,21,180,16,A_THEME,"Theme:");
    p->g_theme=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)p->theme_labels,
        GTCY_Active,catalogue_index(p,nd_config_theme_id(p->cfg)),TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,535,21,150,16,A_SHAPE,"Shape:");
    p->g_shape=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_shapes,GTCY_Active,p->cfg->shape,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,560,218,125,16,A_RUNNING,"Apps:");
    p->g_running=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_running,
        GTCY_Active,RUNNING_STATE(p->cfg),TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,250,170,115,16,A_SIDE,"Dock side:");
    p->g_side=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_sides,GTCY_Active,p->cfg->side,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,560,170,125,16,A_POSITION,"Position:");
    p->g_position=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_positions,GTCY_Active,p->cfg->position,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,250,264,140,16,A_FONT,"Font:");
    p->g_font=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_fonts,GTCY_Active,p->cfg->font_mode,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,250,194,115,16,A_SIZE,"Size:");
    p->g_size=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_item_sizes,
        GTCY_Active,p->cfg->item_size,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,560,264,125,16,A_LABELS,"Labels:");
    p->g_labels=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_labels,GTCY_Active,p->cfg->label_mode,
        GA_Disabled,(ULONG)(p->cfg->item_size==ND_ITEM_SIZE_TINY),TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,250,334,140,16,A_BACKGROUND,"Background:");
    p->g_background=p->last=CreateGadget(CYCLE_KIND,p->last,&ng,
        GTCY_Labels,(ULONG)appearance_backgrounds,
        GTCY_Active,background_choice(p->cfg),TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,450,194,44,16,A_WIDTH,"Width:");
    p->g_width=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)width_text,GTST_MaxChars,3,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,620,194,44,16,A_HEIGHT,"Height:");
    p->g_height=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)height_text,GTST_MaxChars,3,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,250,218,44,16,A_GAP,"Gap:");
    p->g_gap=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)gap_text,GTST_MaxChars,2,
        GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,250,358,44,16,A_HOVER_MAGNIFICATION,"Magnify %:");
    p->g_hover_magnification=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)hover_text,GTST_MaxChars,3,GA_TabCycle,TRUE,TAG_DONE);
    if (!p->last) return 0;
    /* Magnified tiles ease in and out instead of snapping. */
    setup_gadget(p,&ng,450,360,18,12,A_DRAG_BAR,"Show drag bar");
    ng.ng_Flags=PLACETEXT_RIGHT;
    p->g_drag_bar=p->last=CreateGadget(CHECKBOX_KIND,p->last,&ng,
        GTCB_Checked,(ULONG)p->cfg->drag_bar,TAG_DONE);
    if(!p->last)return 0;
    setup_gadget(p,&ng,320,360,18,12,A_HOVER_ANIMATION,"Animate");
    ng.ng_Flags=PLACETEXT_RIGHT;
    p->g_hover_animation=p->last=CreateGadget(CHECKBOX_KIND,p->last,&ng,
        GTCB_Checked,(ULONG)p->cfg->hover_animation,TAG_DONE);
    if (!p->last) return 0;
    setup_gadget(p,&ng,620,334,44,16,A_TRANSPARENCY,"Transparency %:");
    p->g_transparency=p->last=CreateGadget(STRING_KIND,p->last,&ng,
        GTST_String,(ULONG)transparency_text,GTST_MaxChars,3,GA_TabCycle,TRUE,TAG_DONE);
    if(!p->last)return 0;
    if (!create_button(p,162,436,90,A_IMPORT_THEME,"Import...") ||
        !create_button(p,262,436,90,A_REVERT,"Revert") ||
        !create_button(p,362,436,90,A_TEST,"Test") ||
        !create_button(p,462,436,90,A_USE,"Use") ||
        !create_button(p,562,436,90,A_SAVE,"Save")) return 0;
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
    prepare_preview(p);
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
    case A_COLOURFUL:
        p->cfg->colourful_icons=code!=0;ND_PREFS_MARK_DIRTY();obtain_pens(p);draw_preview(p);break;
    case A_ALWAYS_ON_TOP:
        p->cfg->always_on_top=code!=0;ND_PREFS_MARK_DIRTY();break;
    case A_AUTO_HIDE:
        p->cfg->auto_hide=code!=0;ND_PREFS_MARK_DIRTY();
        GT_SetGadgetAttrs(p->g_delay,p->win,NULL,GA_Disabled,(ULONG)!code,TAG_DONE);break;
    case A_TOOLTIPS:
        p->cfg->tooltips=code!=0;ND_PREFS_MARK_DIRTY();break;
    case A_DRAG_BAR:
        p->cfg->drag_bar=code!=0;ND_PREFS_MARK_DIRTY();draw_preview(p);break;
    case A_HOVER_ANIMATION:
        p->cfg->hover_animation=code!=0;ND_PREFS_MARK_DIRTY();break;
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
    case A_BACKGROUND:
        if (code<=2) {
            /* Solid, Artwork, or Wallpaper: solid colours derived from the
             * desktop behind the dock when it opens (palette screens). */
            p->cfg->theme_artwork=code==1;
            p->cfg->wallpaper_match=code==2;
            ND_PREFS_MARK_DIRTY();
            prepare_preview(p);
            draw_preview(p);
            SetWindowTitles(p->win,(STRPTR)(code==1 && !p->background.parts[0].bitmap ?
                "NovaDock Appearance - theme has no usable artwork" :
                "NovaDock Preferences - Appearance *"),(STRPTR)-1);
        }
        break;
    case A_SHAPE:
        if (code<ND_SHAPE_COUNT) {p->cfg->shape=code;ND_PREFS_MARK_DIRTY();draw_preview(p);} break;
    case A_RUNNING:
        if (code<=1) {
            p->cfg->running_apps=code==1;
            p->cfg->running_section=0;
            ND_PREFS_MARK_DIRTY();draw_preview(p);
        } break;
    case A_WIDTH: case A_HEIGHT: case A_GAP: case A_HOVER_MAGNIFICATION: case A_TRANSPARENCY: case A_HIDE_DELAY:
        if (collect_numbers(p,0)) {ND_PREFS_MARK_DIRTY();draw_preview(p);}break;
    case A_REVERT:
        { int i;
        nd_config_set_theme(p->cfg,nd_config_theme_id(p->baseline));
        p->cfg->shape=p->baseline->shape;
        p->cfg->theme_artwork=p->baseline->theme_artwork;
        p->cfg->wallpaper_match=p->baseline->wallpaper_match;
        p->cfg->background_transparency=p->baseline->background_transparency;
        p->cfg->hover_magnification=p->baseline->hover_magnification;
        p->cfg->side=p->baseline->side;p->cfg->position=p->baseline->position;
        p->cfg->position_offset=p->baseline->position_offset;
        p->cfg->font_mode=p->baseline->font_mode;
        p->cfg->label_mode=p->baseline->label_mode;
        p->cfg->tooltips=p->baseline->tooltips;
        p->cfg->hover_animation=p->baseline->hover_animation;
        p->cfg->drag_bar=p->baseline->drag_bar;
        p->cfg->colourful_icons=p->baseline->colourful_icons;
        p->cfg->always_on_top=p->baseline->always_on_top;
        p->cfg->auto_hide=p->baseline->auto_hide;p->cfg->hide_delay=p->baseline->hide_delay;
        obtain_pens(p);
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
        int removable=theme_removable(p,selected);
        struct EasyStruct es;
        ULONG args[1];
        LONG choice;
        memset(&es,0,sizeof(es));
        es.es_StructSize=sizeof(es);
        es.es_Title=(UBYTE *)"NovaDock Appearance";
        es.es_TextFormat=(UBYTE *)(removable ?
            "%s is an imported theme.\n\nImport a theme pack, pick a backdrop image,\nor remove this theme?" :
            "Import a theme pack (theme.cfg with its Images),\nor pick a backdrop image for %s?");
        es.es_GadgetFormat=(UBYTE *)(removable ? "Import...|Backdrop...|Remove...|Cancel" :
                                                 "Import...|Backdrop...|Cancel");
        args[0]=(ULONG)selected;
        choice=EasyRequestArgs(p->win,&es,NULL,args);
        /* Any change rebuilds this page so GadTools gets the new list. */
        if ((choice==1 && install_selected_theme(p)) ||
            (choice==2 && pick_backdrop_image(p)) ||
            (choice==3 && removable && remove_selected_theme(p,selected))) {
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
    case A_UPDATES:
        if (collect_numbers(p,1)) {ND_PREFS_MARK_DIRTY();p->done=1;p->status=ND_PREF_NAV_UPDATES;}break;
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
            if (cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45)) {p->done=1;p->status=0;}
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
    if (p->screen) nd_backdrop_free(&p->background,p->screen);
    free_pens(p);
    if (p->win) CloseWindow(p->win);
    if (p->gadgets) FreeGadgets(p->gadgets);
    if (p->vi) FreeVisualInfo(p->vi);
    if (p->screen) UnlockPubScreen(NULL,p->screen);
    if (p->notify_reload) (void)nd_control_reload();
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
