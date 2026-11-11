/* NovaDock Running Apps plugin.
 *
 * The main dock never creates tiles for unrelated applications. This
 * optional plugin scans public screens only while its Launchpad-style popup
 * is open and lets the user bring an application to the front. The popup
 * reads the active NovaDock theme so it remains visually part of the dock.
 */
#include <exec/types.h>
#include <dos/dosextens.h>
#include <exec/tasks.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"

const char nd_version[]=ND_VER_STRING("RunningApps");
long __stack=16384;

struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;

#define APP_POPUP_PORT "NovaDock.AppsPopup"
#define APP_MAX 48
#define APP_LABEL 48
#define APP_GAP 5
#define APP_PADDING 10
#define APP_HEADER 24
#define APP_TILE_W 92
#define APP_TILE_H 64

#define PT_BG 0
#define PT_TILE 1
#define PT_HOVER 2
#define PT_TEXT 3
#define PT_SUBTLE 4
#define PT_ACCENT 5
#define PT_BAR 6
#define PT_ACCENT2 7
#define PT_COUNT 8

#define PS_SQUARE 0
#define PS_CHAMFER 1
#define PS_ROUNDED 2
#define PS_MINIMAL 3


#include "nd_appscan.h"
#include "nd_search.h"

struct PopupTheme {
    UBYTE rgb[PT_COUNT][3];
    LONG pens[PT_COUNT];
    UBYTE owned[PT_COUNT];
    int shape;
};

struct AppPanel {
    struct Window *window;
    struct AppEntry apps[APP_MAX];
    struct PopupTheme theme;
    int count;
    int columns;
    int rows;
    int tile_width;
    int tile_height;
    int hover;
    char query[32];
};

static void trim_line(char *line)
{
    size_t length;
    char *start=line;
    if (!line) return;
    while (*start==' ' || *start=='\t') ++start;
    if (start!=line) memmove(line,start,strlen(start)+1);
    length=strlen(line);
    while (length && (line[length-1]=='\r' || line[length-1]=='\n' ||
                      line[length-1]==' ' || line[length-1]=='\t'))
        line[--length]='\0';
}

static void theme_defaults(struct PopupTheme *theme)
{
    static const UBYTE defaults[PT_COUNT][3]={
        {24,24,37},{49,50,68},{69,71,90},{205,214,244},
        {166,173,200},{137,180,250},{35,36,50},{166,227,161}
    };
    int i;
    memset(theme,0,sizeof(*theme));
    for (i=0;i<PT_COUNT;++i) {
        theme->rgb[i][0]=defaults[i][0];
        theme->rgb[i][1]=defaults[i][1];
        theme->rgb[i][2]=defaults[i][2];
        theme->pens[i]=-1;
    }
    theme->shape=PS_CHAMFER;
}

static int theme_colour_index(const char *key)
{
    static const char *keys[PT_COUNT]={
        "BG","TILE","HOVER","TEXT","SUBTLE","ACCENT","BAR","ACCENT2"
    };
    int i;
    for (i=0;i<PT_COUNT;++i) if (!strcmp(key,keys[i])) return i;
    return -1;
}

static void read_theme_file(struct PopupTheme *theme,const char *path)
{
    FILE *file;
    char line[160],*equals;
    int index,r,g,b;
    file=fopen(path,"r");
    if (!file) return;
    while (fgets(line,sizeof(line),file)) {
        trim_line(line);
        if (!line[0] || line[0]=='#') continue;
        equals=strchr(line,'=');
        if (!equals) continue;
        *equals++='\0';trim_line(line);trim_line(equals);
        index=theme_colour_index(line);
        if (index>=0 && sscanf(equals,"%d,%d,%d",&r,&g,&b)==3) {
            if (r<0) r=0;if (r>255) r=255;
            if (g<0) g=0;if (g>255) g=255;
            if (b<0) b=0;if (b>255) b=255;
            theme->rgb[index][0]=(UBYTE)r;
            theme->rgb[index][1]=(UBYTE)g;
            theme->rgb[index][2]=(UBYTE)b;
        } else if (!strcmp(line,"SHAPE")) {
            if (!strcmp(equals,"SQUARE")) theme->shape=PS_SQUARE;
            else if (!strcmp(equals,"ROUNDED")) theme->shape=PS_ROUNDED;
            else if (!strcmp(equals,"MINIMAL")) theme->shape=PS_MINIMAL;
            else theme->shape=PS_CHAMFER;
        }
    }
    fclose(file);
}

static void load_active_theme(struct PopupTheme *theme)
{
    FILE *file;
    char line[160],name[48]="Mocha",path[160];
    char *equals;
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR requesters=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1L;
    theme_defaults(theme);
    file=fopen("ENV:NovaDock/novadock.cfg","r");
    if (!file) file=fopen("ENVARC:NovaDock/novadock.cfg","r");
    if (!file) file=fopen("SYS:NovaDock/novadock.cfg.default","r");
    /* PROGDIR: only exists with a home directory; without one, touching it
     * raises a "Please insert volume PROGDIR" requester. */
    if (!file && GetProgramDir()) file=fopen("PROGDIR:/novadock.cfg","r");
    if (file) {
        while (fgets(line,sizeof(line),file)) {
            trim_line(line);
            equals=strchr(line,'=');
            if (!equals) continue;
            *equals++='\0';trim_line(line);trim_line(equals);
            if (!strcmp(line,"THEME") && equals[0]) {
                strncpy(name,equals,sizeof(name)-1);
                name[sizeof(name)-1]='\0';
                break;
            }
        }
        fclose(file);
    }
    if (strchr(name,'/') || strchr(name,':')) strcpy(name,"Mocha");
    sprintf(path,"SYS:NovaDock/Themes/%s/theme.cfg",name);
    read_theme_file(theme,path);
    sprintf(path,"NovaDock:Themes/%s/theme.cfg",name);
    read_theme_file(theme,path);
    if (GetProgramDir()) {
        sprintf(path,"PROGDIR:/Themes/%s/theme.cfg",name);
        read_theme_file(theme,path);
    }
    self->pr_WindowPtr=requesters;
}

static void obtain_theme_pens(struct PopupTheme *theme,struct Screen *screen)
{
    int i;
    LONG colour;
    for (i=0;i<PT_COUNT;++i) {
        colour=ObtainBestPenA(screen->ViewPort.ColorMap,
            (ULONG)theme->rgb[i][0]*0x01010101UL,
            (ULONG)theme->rgb[i][1]*0x01010101UL,
            (ULONG)theme->rgb[i][2]*0x01010101UL,NULL);
        if (colour>=0) {
            theme->pens[i]=colour;theme->owned[i]=1;
        } else theme->pens[i]=(i==PT_TEXT || i==PT_ACCENT) ? 1 : 0;
    }
}

static void release_theme_pens(struct PopupTheme *theme,struct Screen *screen)
{
    int i;
    if (!theme || !screen) return;
    for (i=0;i<PT_COUNT;++i) if (theme->owned[i]) {
        ReleasePen(screen->ViewPort.ColorMap,theme->pens[i]);
        theme->owned[i]=0;
    }
}


static void fill_rect(struct AppPanel *panel,int colour,
                      int x,int y,int width,int height)
{
    if (width<1 || height<1) return;
    SetAPen(panel->window->RPort,panel->theme.pens[colour]);
    RectFill(panel->window->RPort,x,y,x+width-1,y+height-1);
}

static void fill_tile(struct AppPanel *panel,int colour,
                      int x,int y,int width,int height)
{
    int cut=panel->theme.shape==PS_ROUNDED ? 5 : 3;
    if (panel->theme.shape==PS_MINIMAL && colour==PT_TILE) {
        fill_rect(panel,PT_BG,x,y,width,height);
        return;
    }
    if (panel->theme.shape==PS_SQUARE) {
        fill_rect(panel,colour,x,y,width,height);return;
    }
    fill_rect(panel,colour,x+cut,y,width-cut*2,height);
    fill_rect(panel,colour,x,y+cut,width,height-cut*2);
    if (panel->theme.shape==PS_ROUNDED) {
        fill_rect(panel,colour,x+2,y+2,width-4,height-4);
        fill_rect(panel,colour,x+1,y+4,width-2,height-8);
    }
}

static void centered_text(struct AppPanel *panel,const char *text,
                          int y,int left,int width)
{
    struct RastPort *rp=panel->window->RPort;
    int length=(int)strlen(text),pixels;
    while (length>1 && TextLength(rp,(STRPTR)text,length)>width-6) --length;
    pixels=TextLength(rp,(STRPTR)text,length);
    Move(rp,left+(width-pixels)/2,y);
    Text(rp,(STRPTR)text,(ULONG)length);
}

static void draw_app_glyph(struct AppPanel *panel,int x,int y,int width)
{
    struct RastPort *rp=panel->window->RPort;
    int glyph_w=width>78 ? 38 : 30;
    int glyph_h=25;
    int gx=x+(width-glyph_w)/2;
    int gy=y+7;
    fill_rect(panel,PT_SUBTLE,gx,gy,glyph_w,glyph_h);
    fill_rect(panel,PT_TEXT,gx+2,gy+2,glyph_w-4,glyph_h-4);
    fill_rect(panel,PT_ACCENT,gx+3,gy+3,glyph_w-6,4);
    fill_rect(panel,PT_TILE,gx+4,gy+9,glyph_w/2-5,glyph_h-13);
    fill_rect(panel,PT_HOVER,gx+glyph_w/2+1,gy+9,glyph_w/2-5,glyph_h-13);
    SetAPen(rp,panel->theme.pens[PT_ACCENT2]);
    RectFill(rp,gx+glyph_w-8,gy+4,gx+glyph_w-5,gy+5);
}

static void tile_origin(const struct AppPanel *panel,int index,int *x,int *y)
{
    *x=APP_PADDING+(index%panel->columns)*(panel->tile_width+APP_GAP);
    *y=APP_HEADER+(index/panel->columns)*(panel->tile_height+APP_GAP);
}

static int panel_hit(const struct AppPanel *panel,int mouse_x,int mouse_y)
{
    int col,row,index,x,y;
    if (mouse_x<APP_PADDING || mouse_y<APP_HEADER) return -1;
    col=(mouse_x-APP_PADDING)/(panel->tile_width+APP_GAP);
    row=(mouse_y-APP_HEADER)/(panel->tile_height+APP_GAP);
    if (col<0 || col>=panel->columns || row<0 || row>=panel->rows) return -1;
    index=row*panel->columns+col;
    if (index>=panel->count) return -1;
    tile_origin(panel,index,&x,&y);
    if (mouse_x>=x+panel->tile_width || mouse_y>=y+panel->tile_height)
        return -1;
    return index;
}

static void draw_panel(struct AppPanel *panel)
{
    struct RastPort *rp;
    int i,x,y;
    if (!panel || !panel->window) return;
    rp=panel->window->RPort;
    SetDrMd(rp,JAM2);
    fill_rect(panel,PT_BG,0,0,panel->window->Width,panel->window->Height);
    SetAPen(rp,panel->theme.pens[PT_ACCENT]);
    Move(rp,0,0);Draw(rp,panel->window->Width-1,0);
    Draw(rp,panel->window->Width-1,panel->window->Height-1);
    Draw(rp,0,panel->window->Height-1);Draw(rp,0,0);
    SetAPen(rp,panel->theme.pens[PT_TEXT]);
    SetBPen(rp,panel->theme.pens[PT_BG]);
    centered_text(panel,panel->query[0] ? panel->query : "RUNNING APPS - type to search",rp->TxBaseline+5,0,panel->window->Width);
    if (!panel->count) {
        centered_text(panel,"No running applications",APP_HEADER+18,
                      0,panel->window->Width);
        return;
    }
    for (i=0;i<panel->count;++i) {
        tile_origin(panel,i,&x,&y);
        fill_tile(panel,i==panel->hover ? PT_HOVER : PT_TILE,
                  x,y,panel->tile_width,panel->tile_height);
        draw_app_glyph(panel,x,y,panel->tile_width);
        SetAPen(rp,panel->theme.pens[PT_TEXT]);
        SetBPen(rp,panel->theme.pens[i==panel->hover ? PT_HOVER : PT_TILE]);
        centered_text(panel,panel->apps[i].label,
                      y+panel->tile_height-7,x,panel->tile_width);
    }
}

static void close_panel(struct AppPanel *panel)
{
    struct Screen *screen;
    if (panel && panel->window) {
        screen=panel->window->WScreen;
        release_theme_pens(&panel->theme,screen);
        CloseWindow(panel->window);panel->window=NULL;panel->count=0;
        panel->hover=-1;panel->query[0]=0;
    }
}

static void choose_grid(struct AppPanel *panel,struct Screen *screen,
                        int *width,int *height)
{
    int count=panel->count ? panel->count : 1;
    int max_columns=(screen->Width-APP_PADDING*2)/(APP_TILE_W+APP_GAP);
    int max_rows=(screen->Height-screen->BarHeight-APP_HEADER-
                  APP_PADDING)/(APP_TILE_H+APP_GAP);
    if (max_columns<1) max_columns=1;
    if (max_rows<1) max_rows=1;
    panel->columns=count<max_columns ? count : max_columns;
    panel->rows=(count+panel->columns-1)/panel->columns;
    panel->tile_width=APP_TILE_W;
    panel->tile_height=APP_TILE_H;
    if (panel->rows>max_rows) {
        panel->rows=max_rows;
        panel->columns=(count+panel->rows-1)/panel->rows;
        panel->tile_width=(screen->Width-APP_PADDING*2-
            (panel->columns-1)*APP_GAP)/panel->columns;
        if (panel->tile_width<54) panel->tile_width=54;
        panel->rows=(count+panel->columns-1)/panel->columns;
    }
    *width=APP_PADDING*2+panel->columns*panel->tile_width+
           (panel->columns-1)*APP_GAP;
    *height=APP_HEADER+panel->rows*panel->tile_height+
            (panel->rows-1)*APP_GAP+APP_PADDING;
    if(!panel->count){
        int empty_width=28*screen->RastPort.Font->tf_XSize+APP_PADDING*2;
        if(*width<empty_width)*width=empty_width;
        if(*width>screen->Width)*width=screen->Width;
    }
}

static void open_panel(struct AppPanel *panel)
{
    struct Screen *screen;
    int width,height,left,top;
    if (!panel || panel->window) return;
    panel->count=collect_apps(panel->apps);
    panel->hover=panel->count ? 0 : -1;
    screen=LockPubScreen((STRPTR)"Workbench");
    if (!screen) return;
    load_active_theme(&panel->theme);
    obtain_theme_pens(&panel->theme,screen);
    choose_grid(panel,screen,&width,&height);
    /* Keep the app switcher visually stable.  It belongs to the Workbench
     * screen, not to the pointer or to the dock edge that opened it. */
    left=(screen->Width-width)/2;
    top=screen->BarHeight+
        (screen->Height-screen->BarHeight-height)/2;
    if (left<0) left=0;
    if (top<screen->BarHeight) top=screen->BarHeight;
    if (left+width>screen->Width) left=screen->Width-width;
    if (top+height>screen->Height) top=screen->Height-height;
    panel->window=OpenWindowTags(NULL,
        WA_PubScreen,(ULONG)screen,
        WA_Left,left,WA_Top,top,WA_Width,width,WA_Height,height,
        WA_Title,(ULONG)"NovaDock Apps",
        WA_Backdrop,FALSE,
        WA_Borderless,TRUE,
        WA_Activate,TRUE,
        WA_RMBTrap,TRUE,
        WA_SimpleRefresh,TRUE,
        WA_ReportMouse,TRUE,
        WA_IDCMP,IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_REFRESHWINDOW|
                 IDCMP_INACTIVEWINDOW|IDCMP_RAWKEY|IDCMP_VANILLAKEY,
        TAG_DONE);
    if (!panel->window) release_theme_pens(&panel->theme,screen);
    UnlockPubScreen((STRPTR)"Workbench",screen);
    draw_panel(panel);
}


static void panel_events(struct AppPanel *panel)
{
    struct IntuiMessage *message;
    ULONG cls;
    UWORD code;
    WORD mouse_x,mouse_y;
    int hit,next,i;size_t length;
    struct AppEntry selected;
    while (panel && panel->window &&
           (message=(struct IntuiMessage *)GetMsg(panel->window->UserPort))) {
        cls=message->Class;code=message->Code;
        mouse_x=message->MouseX;mouse_y=message->MouseY;
        ReplyMsg((struct Message *)message);
        if ((cls==IDCMP_INACTIVEWINDOW &&
             !(panel->window->Flags&WFLG_WINDOWACTIVE)) ||
            (cls==IDCMP_RAWKEY && code==0x45)) close_panel(panel);
        else if (cls==IDCMP_REFRESHWINDOW) {
            BeginRefresh(panel->window);draw_panel(panel);
            EndRefresh(panel->window,TRUE);
        } else if (cls==IDCMP_MOUSEMOVE) {
            hit=panel_hit(panel,mouse_x,mouse_y);
            if (hit!=panel->hover) {panel->hover=hit;draw_panel(panel);}
        } else if (cls==IDCMP_VANILLAKEY) {
            length=strlen(panel->query);
            if(code==8 && length)panel->query[--length]=0;
            else if(code>=32 && code<127 && length+1<sizeof(panel->query)) {
                panel->query[length++]=(char)code;panel->query[length]=0;
            } else continue;
            for(i=0;i<panel->count;++i)if(nd_search_contains(panel->apps[i].label,panel->query))break;
            panel->hover=i<panel->count ? i : -1;
            draw_panel(panel);
        } else if (cls==IDCMP_RAWKEY && panel->count>0) {
            next=panel->hover<0 ? 0 : panel->hover;
            if (code==0x4f && next>0) next--;
            else if (code==0x4e && next+1<panel->count) next++;
            else if (code==0x4c && next-panel->columns>=0)
                next-=panel->columns;
            else if (code==0x4d && next+panel->columns<panel->count)
                next+=panel->columns;
            else if (code==0x44) {
                if(panel->query[0] && panel->hover<0)continue;
                selected=panel->apps[next];
                close_panel(panel);(void)raise_app(&selected);
                continue;
            } else continue;
            if (next!=panel->hover) {panel->hover=next;draw_panel(panel);}
        } else if (cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN) {
            hit=panel_hit(panel,mouse_x,mouse_y);
            if (hit>=0) {
                selected=panel->apps[hit];
                close_panel(panel);(void)raise_app(&selected);
            } else close_panel(panel);
        }
    }
}

/* The tile launches this UI helper directly: a stalled badge/status poll
 * cannot swallow the user's open action. One public port prevents duplicate
 * popups; another click raises the existing window. */
static void popup_helper(struct AppPanel *panel)
{
    struct MsgPort *port=CreateMsgPort(),*existing;
    ULONG signals;
    int owner=0;
    if(!port)return;
    Forbid();existing=FindPort((STRPTR)APP_POPUP_PORT);
    if(existing){
        if(existing->mp_SigTask)Signal(existing->mp_SigTask,ND_WAKE_SIGNAL);
    }else{
        port->mp_Node.ln_Name=(STRPTR)APP_POPUP_PORT;
        AddPort(port);owner=1;
    }
    Permit();
    if(owner){
        open_panel(panel);
        while(panel->window){
            panel_events(panel);
            if(!panel->window)break;
            signals=Wait((1UL<<panel->window->UserPort->mp_SigBit)|
                         ND_WAKE_SIGNAL|SIGBREAKF_CTRL_C);
            if(signals&SIGBREAKF_CTRL_C)close_panel(panel);
            else if((signals&ND_WAKE_SIGNAL) && panel->window){
                WindowToFront(panel->window);ActivateWindow(panel->window);
            }
        }
        Forbid();RemPort(port);Permit();
    }
    DeleteMsgPort(port);
}

int main(int argc,char **argv)
{
    struct NDClient client;
    struct AppPanel panel;
    unsigned int events,elapsed=0;
    memset(&panel,0,sizeof(panel));panel.hover=-1;
    if (argc<2) return 20;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",37);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",37);
    if (!IntuitionBase || !GfxBase) {
        if (GfxBase) CloseLibrary((struct Library *)GfxBase);
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    if(argc>=3 && !strcmp(argv[2],"OPEN")){
        popup_helper(&panel);
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);return 0;
    }
    if (!nd_client_init(&client,argv[1])) {
        if (GfxBase) CloseLibrary((struct Library *)GfxBase);
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    for (;;) {
        panel_events(&panel);
        events=nd_client_update_quiet(&client,"APPS","",ND_NO_METER,
                                      elapsed,500U);
        if (!(events&ND_EVENT_QUIT) && !panel.window) {
            /* Badge: how many applications are running (none: no badge);
             * refreshed at the idle cadence, not while the panel polls. */
            char badge[4];
            int running=count_apps();
            if (running<=0) badge[0]='\0';
            else sprintf(badge,"%d",running>99 ? 99 : running);
            events|=nd_client_set_badge(&client,badge);
        }
        if (events&ND_EVENT_QUIT) break;
        if (events&ND_EVENT_CLICK) {
            /* A second click may already be queued when the first opens
             * the panel. Keep it open instead of undoing the first click. */
            if (panel.window) {
                WindowToFront(panel.window);ActivateWindow(panel.window);
            }else open_panel(&panel);
        }
        if (panel.window) {Delay(5);elapsed=5;}
        else elapsed=nd_client_wait(&client,250);
    }
    close_panel(&panel);nd_client_close(&client);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
