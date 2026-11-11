/* Clipboard quick picker, using the same palette and shapes as Apps. */
#include <exec/types.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_clipboard_popup.h"
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
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

struct PopupTheme {
    UBYTE rgb[PT_COUNT][3];
    LONG pens[PT_COUNT];
    UBYTE owned[PT_COUNT];
    int shape;
};

struct NDClipQuick {
    struct Window *window;struct PopupTheme theme;struct NDClipHistory clips;
    int columns,rows,tile_width,tile_height,page,page_size,pages,hover;
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

static void fill_rect(struct NDClipQuick *panel,int colour,
                      int x,int y,int width,int height)
{
    if (width<1 || height<1) return;
    SetAPen(panel->window->RPort,panel->theme.pens[colour]);
    RectFill(panel->window->RPort,x,y,x+width-1,y+height-1);
}

static void fill_tile(struct NDClipQuick *panel,int colour,
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

static void centered_text(struct NDClipQuick *panel,const char *text,
                          int y,int left,int width)
{
    struct RastPort *rp=panel->window->RPort;
    int length=(int)strlen(text),pixels;
    while (length>1 && TextLength(rp,(STRPTR)text,length)>width-6) --length;
    pixels=TextLength(rp,(STRPTR)text,length);
    Move(rp,left+(width-pixels)/2,y);
    Text(rp,(STRPTR)text,(ULONG)length);
}

#define Q_PAD 10
#define Q_GAP 6
#define Q_HEAD 25
#define Q_FOOT 24
static void origin(const struct NDClipQuick *q,int slot,int *x,int *y)
{*x=Q_PAD+(slot%q->columns)*(q->tile_width+Q_GAP);*y=Q_HEAD+(slot/q->columns)*(q->tile_height+Q_GAP);}
static int hit(const struct NDClipQuick *q,int mx,int my)
{
    int col,row,slot,x,y,index;
    if(mx<Q_PAD || my<Q_HEAD)return -1;
    col=(mx-Q_PAD)/(q->tile_width+Q_GAP);row=(my-Q_HEAD)/(q->tile_height+Q_GAP);
    if(col>=q->columns || row>=q->rows)return -1;
    slot=row*q->columns+col;origin(q,slot,&x,&y);index=q->page*q->page_size+slot;
    if(mx>=x+q->tile_width || my>=y+q->tile_height || index>=(int)q->clips.count)return -1;
    return slot;
}
static void draw_quick(struct NDClipQuick *q)
{
    struct RastPort *rp=q->window->RPort;int slot,index,x,y,line,pos,n,cols;char text[80],label[48];
    SetDrMd(rp,JAM2);fill_rect(q,PT_BG,0,0,q->window->Width,q->window->Height);
    SetAPen(rp,q->theme.pens[PT_ACCENT]);Move(rp,0,0);Draw(rp,q->window->Width-1,0);
    Draw(rp,q->window->Width-1,q->window->Height-1);Draw(rp,0,q->window->Height-1);Draw(rp,0,0);
    SetAPen(rp,q->theme.pens[PT_TEXT]);SetBPen(rp,q->theme.pens[PT_BG]);
    centered_text(q,"CLIPPY",rp->TxBaseline+5,0,q->window->Width);
    if(!q->clips.count){centered_text(q,"Clipboard is empty",Q_HEAD+26,0,q->window->Width);}
    for(slot=0;slot<q->page_size;++slot){
        index=q->page*q->page_size+slot;if(index>=(int)q->clips.count)break;
        origin(q,slot,&x,&y);fill_tile(q,slot==q->hover ? PT_HOVER:PT_TILE,x,y,q->tile_width,q->tile_height);
        fill_rect(q,PT_SUBTLE,x+8,y+7,q->tile_width-16,40);
        fill_rect(q,PT_BG,x+10,y+9,q->tile_width-20,36);
        SetAPen(rp,q->theme.pens[PT_TEXT]);SetBPen(rp,q->theme.pens[PT_BG]);
        pos=0;cols=(q->tile_width-28)/(rp->TxWidth ? rp->TxWidth:8);if(cols>70)cols=70;if(cols<1)cols=1;
        for(line=0;line<3 && line<(36/(rp->TxHeight+1)) && q->clips.text[index][pos];++line){
            n=0;
            while(q->clips.text[index][pos] && q->clips.text[index][pos]!='\n' && n<cols){
                unsigned char ch=(unsigned char)q->clips.text[index][pos++];text[n++]=ch<32 || ch==127 ? ' ':(char)ch;
            }
            if(q->clips.text[index][pos]=='\n')++pos;text[n]=0;
            Move(rp,x+14,y+12+rp->TxBaseline+line*(rp->TxHeight+1));Text(rp,text,n);
        }
        sprintf(label,"Clip %d",index+1);SetBPen(rp,q->theme.pens[slot==q->hover ? PT_HOVER:PT_TILE]);
        centered_text(q,label,y+q->tile_height-8,x,q->tile_width);
    }
    SetAPen(rp,q->theme.pens[PT_SUBTLE]);SetBPen(rp,q->theme.pens[PT_BG]);
    if(q->pages>1){
        sprintf(label,"<   Page %d / %d   >",q->page+1,q->pages);
        centered_text(q,label,q->window->Height-8,0,q->window->Width);
    }else centered_text(q,q->clips.count ? "Click to copy - Esc to close":"Esc to close",q->window->Height-8,0,q->window->Width);
}
struct NDClipQuick *nd_clip_quick_create(void)
{return AllocVec(sizeof(struct NDClipQuick),MEMF_PUBLIC|MEMF_CLEAR);}
int nd_clip_quick_active(const struct NDClipQuick *q)
{return q && q->window!=NULL;}
void nd_clip_quick_close(struct NDClipQuick *q)
{
    if(!q)return;
    if(q->window){struct Screen *screen=q->window->WScreen;
        release_theme_pens(&q->theme,screen);CloseWindow(q->window);q->window=NULL;}
    nd_clip_history_clear(&q->clips);
}
void nd_clip_quick_free(struct NDClipQuick *q)
{if(q){nd_clip_quick_close(q);FreeVec(q);}}
int nd_clip_quick_open(struct NDClipQuick *q,const struct NDClipHistory *history)
{
    struct Screen *screen;int width,height,left,top,maxcols,maxrows,i,count;
    if(!q)return 0;nd_clip_quick_close(q);nd_clip_history_init(&q->clips,ND_CLIP_MAX);
    for(i=(int)history->count-1;i>=0;--i)if(!nd_clip_history_add(&q->clips,history->text[i]))goto fail;
    screen=LockPubScreen("Workbench");if(!screen)goto fail;
    load_active_theme(&q->theme);obtain_theme_pens(&q->theme,screen);
    count=q->clips.count ? (int)q->clips.count:1;
    q->tile_width=144;q->tile_height=78;
    maxcols=(screen->Width-2*Q_PAD)/(q->tile_width+Q_GAP);if(maxcols<1)maxcols=1;if(maxcols>4)maxcols=4;
    maxrows=(screen->Height-screen->BarHeight-Q_HEAD-Q_FOOT-Q_PAD)/(q->tile_height+Q_GAP);
    if(maxrows<1)maxrows=1;
    q->columns=count<maxcols ? count:maxcols;q->rows=(count+q->columns-1)/q->columns;
    if(q->rows>maxrows)q->rows=maxrows;q->page_size=q->rows*q->columns;
    q->pages=(count+q->page_size-1)/q->page_size;q->page=0;q->hover=q->clips.count ? 0:-1;
    width=2*Q_PAD+q->columns*q->tile_width+(q->columns-1)*Q_GAP;
    if(!q->clips.count)width=280;
    height=Q_HEAD+q->rows*(q->tile_height+Q_GAP)-Q_GAP+Q_FOOT;
    left=(screen->Width-width)/2;top=screen->BarHeight+(screen->Height-screen->BarHeight-height)/2;
    if(left<0)left=0;if(top<screen->BarHeight)top=screen->BarHeight;
    q->window=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Title,(ULONG)"NovaDock Clipboard Picker",
        WA_Left,left,WA_Top,top,WA_Width,width,WA_Height,height,WA_Borderless,TRUE,WA_Activate,TRUE,
        WA_RMBTrap,TRUE,WA_SimpleRefresh,TRUE,WA_ReportMouse,TRUE,
        WA_IDCMP,IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW|IDCMP_RAWKEY,TAG_DONE);
    if(!q->window)release_theme_pens(&q->theme,screen);
    UnlockPubScreen(NULL,screen);if(!q->window)goto fail;draw_quick(q);return 1;
fail:nd_clip_quick_close(q);return 0;
}
static void change_page(struct NDClipQuick *q,int direction)
{
    int page=q->page+direction;if(page<0 || page>=q->pages)return;
    q->page=page;q->hover=0;draw_quick(q);
}
const char *nd_clip_quick_events(struct NDClipQuick *q)
{
    struct IntuiMessage *m;ULONG cls;UWORD code;int mx,my,slot,next,index;
    while(q && q->window && (m=(struct IntuiMessage *)GetMsg(q->window->UserPort))!=NULL){
        cls=m->Class;code=m->Code;mx=m->MouseX;my=m->MouseY;ReplyMsg((struct Message *)m);
        if(cls==IDCMP_INACTIVEWINDOW || (cls==IDCMP_RAWKEY && code==0x45))nd_clip_quick_close(q);
        else if(cls==IDCMP_REFRESHWINDOW){BeginRefresh(q->window);draw_quick(q);EndRefresh(q->window,TRUE);}
        else if(cls==IDCMP_MOUSEMOVE){slot=hit(q,mx,my);if(slot!=q->hover){q->hover=slot;draw_quick(q);}}
        else if(cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN){
            if(q->pages>1 && my>=q->window->Height-Q_FOOT){change_page(q,mx<q->window->Width/2 ? -1:1);continue;}
            slot=hit(q,mx,my);if(slot<0){nd_clip_quick_close(q);continue;}
            return q->clips.text[q->page*q->page_size+slot];
        }else if(cls==IDCMP_RAWKEY && q->clips.count){
            next=q->hover<0 ? 0:q->hover;
            if(code==0x44)return q->clips.text[q->page*q->page_size+next];
            if(code==0x4f){if(next>0)--next;else {change_page(q,-1);continue;}}
            else if(code==0x4e){
                index=q->page*q->page_size+next+1;
                if(next+1<q->page_size && index<(int)q->clips.count)++next;
                else {change_page(q,1);continue;}
            }else if(code==0x4c && next>=q->columns)next-=q->columns;
            else if(code==0x4d && next+q->columns<q->page_size && q->page*q->page_size+next+q->columns<(int)q->clips.count)next+=q->columns;
            else continue;
            q->hover=next;draw_quick(q);
        }
    }
    return NULL;
}
