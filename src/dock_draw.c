/* NovaDock: Drawing: colours, fonts, tile geometry and every tile, badge and graph. */
#include "dock_internal.h"
#include "nd_indicator.h"

/* Defined below, private to this file. */
static void color_pair(struct NDDock *dock, int index, int r, int g, int b);
void remap_brush_colors(struct NDDock *dock);
static void unmap_brush_colors(struct NDDock *dock);
static void fill(struct NDDock *dock, int color, int x, int y, int w, int h);
static void paint_background(struct NDDock *dock,int x,int y,int w,int h);
static void rounded_box(struct NDDock *dock, int color, int x, int y, int w, int h);
static void tile_bevel(struct NDDock *dock,int fill_color, int x,int y,int w,int h);
static void tile_box(struct NDDock *dock,int color,int x,int y,int w,int h);
static void centered_text(struct NDDock *dock, int color, const char *value, int x, int baseline, int w);
static void centered_text_tight(struct NDDock *dock,int color, const char *value,int x,int baseline,int w);
static int launcher_label_visible(const struct NDDock *dock,int index);
static int launcher_task_name(const struct NDItem *item,char *name,int size);
static struct Task *find_cli_command(const char *wanted);
static void draw_running_indicator(struct NDDock *dock,int index, int x,int y,int w);
static int row_offset_pixels(struct NDDock *dock,int row);
static int item_axis_size(struct NDDock *dock,int index);
static int item_slot_span(struct NDDock *dock,int index);
static void draw_graph(struct NDDock *dock, int index, int x, int y, int tw, int th);
static int tile_interior(struct NDDock *dock,int index);
static int draw_brush(struct NDDock *dock, int index, int x, int y, int tw, int th);
static void draw_analog_clock(struct NDDock *dock,int x,int y,int w,int h);
static int eyes_color(struct NDDock *dock,const char *key,int white_pen);
static void eyes_filled_ellipse(struct NDDock *dock, int cx,int cy,int rx,int ry,int color);
static void eyes_geometry(struct NDDock *dock,int index, int centers_x[2],int centers_y[2], int *rx,int *ry,int *pr);
static void draw_eyes(struct NDDock *dock,int index,int x,int y,int tw,int th);
static void draw_volume_meter(struct NDDock *dock, const struct NDItem *item, int x, int y, int tw, int th);
static int parse_ram_pair(const char *value,unsigned int *chip, unsigned int *fast);
static void draw_ram_tile(struct NDDock *dock,const struct NDItem *item, int x,int y,int tw,int th);
static void draw_ram_graph_tile(struct NDDock *dock,int index, int x,int y,int tw,int th);
static void music_button_box(struct NDDock *dock,int x,int y,int w,int h, int enabled);
static void music_transport_icon(struct NDDock *dock,int kind, int x,int y,int w,int h,int enabled,int playing);
static void draw_music_tile(struct NDDock *dock,const struct NDItem *item, int x,int y,int tw,int th);
static unsigned int music_control_hit(struct NDDock *dock,int index, int mx,int my,unsigned int *volume);
static void draw_tiny_plugin(struct NDDock *dock,const struct NDItem *item, int x,int y,int tw,int th);
static int dopus_application_running(const struct NDItem *item);
static void draw_plugin_launcher_artwork(struct NDDock *dock,int index, const char *label, int x,int y,int tw,int th);
static void draw_dopus_tile(struct NDDock *dock,int index, const struct NDItem *item, int x,int y,int tw,int th);
static void draw_music_idle_tile(struct NDDock *dock,int index, int x,int y,int tw,int th);
static void draw_running_apps_tile(struct NDDock *dock,int index, int x,int y,int tw,int th);
static int draw_plugin_badge(struct NDDock *dock,const struct NDItem *item, int x,int y,int tw);
static void draw_plugin_running_indicator(struct NDDock *dock, const struct NDItem *item, int x,int y,int tw);
static void draw_scroll_control(struct NDDock *dock, int direction);
static void draw_insertion_marker(struct NDDock *dock);
static void draw_tab(struct NDDock *dock);




static void color_pair(struct NDDock *dock, int index, int r, int g, int b)
{
    int max_pen;
    if (!dock->screen) { dock->colors[index] = -1; return; }
    max_pen = dock->screen->RastPort.BitMap->Depth >= 8 ? 255 :
              (1 << dock->screen->RastPort.BitMap->Depth) - 1;
    /* Theme colours need real palette entries.  FindColor alone collapses
     * several dark shades to an unrelated Workbench pen on an 8-bit public
     * screen, producing the bright-blue frames seen after the DOpus memory
     * investigation.  Eight shared pens are a small, bounded reservation;
     * release them when the window is rebuilt or NovaDock exits. */
    dock->colors[index] = ObtainBestPenA(dock->screen->ViewPort.ColorMap,
                                        RGBBYTE(r),RGBBYTE(g),RGBBYTE(b),NULL);
    dock->color_owned[index] = dock->colors[index] >= 0 ? 1 : 0;
    if (dock->colors[index] < 0)
        dock->colors[index] = FindColor(dock->screen->ViewPort.ColorMap,
                                        RGBBYTE(r),RGBBYTE(g),RGBBYTE(b),max_pen);
}
void remap_item_brush_colors(struct NDDock *dock,int i)
{
    int j, max_pen;
    if (!dock->screen || i<0 || i>=ND_MAX_ITEMS) return;
    max_pen = dock->screen->RastPort.BitMap->Depth >= 8 ? 255 :
              (1 << dock->screen->RastPort.BitMap->Depth) - 1;
    for (j = 0; j < ND_ICON_MAX_COLORS; ++j) {
        LONG mapped = -1;
        const struct NDILBM *b = dock->brushes[i];
        if (b && j < b->colors && j != b->transparent)
            mapped = FindColor(dock->screen->ViewPort.ColorMap,
                RGBBYTE(b->palette[j][0]), RGBBYTE(b->palette[j][1]),
                RGBBYTE(b->palette[j][2]), max_pen);
        dock->brush_pens[i][j] = mapped;
    }
}
void remap_brush_colors(struct NDDock *dock)
{
    int i;
    for (i = 0; i < ND_MAX_ITEMS; ++i) remap_item_brush_colors(dock,i);
}
static void unmap_brush_colors(struct NDDock *dock)
{
    int i, j;
    if (!dock->screen) return;
    for (i = 0; i < ND_MAX_ITEMS; ++i)
        for (j = 0; j < ND_ICON_MAX_COLORS; ++j)
            dock->brush_pens[i][j] = -1;
}
void alloc_colors(struct NDDock *dock)
{
    int i;
    for (i = 0; i < ND_COLOR_COUNT; ++i) {
        dock->colors[i] = -1;
        dock->color_owned[i] = 0;
    }
    for (i=0;i<ND_COLOR_COUNT;++i)
        color_pair(dock,i,dock->theme_spec.rgb[i][0],
                          dock->theme_spec.rgb[i][1],
                          dock->theme_spec.rgb[i][2]);
    remap_brush_colors(dock);
}
void free_colors(struct NDDock *dock)
{
    int i;
    if (!dock->screen) return;
    unmap_brush_colors(dock);
    for (i = 0; i < ND_COLOR_COUNT; ++i) {
        if (dock->color_owned[i] && dock->colors[i] >= 0)
            ReleasePen(dock->screen->ViewPort.ColorMap,dock->colors[i]);
        dock->colors[i] = -1;
        dock->color_owned[i] = 0;
    }
}
void release_dock_font(struct NDDock *dock)
{
    if (dock->dock_font_owned && dock->dock_font) CloseFont(dock->dock_font);
    dock->dock_font=NULL;
    dock->dock_font_owned=0;
}
void apply_dock_font(struct NDDock *dock)
{
    static struct TextAttr topaz_attr={"topaz.font",8,FS_NORMAL,FPF_ROMFONT};
    struct TextFont *font=NULL;
    if (!dock || !dock->window || !dock->screen) return;
    release_dock_font(dock);
    if (dock->conf.font_mode==ND_FONT_TOPAZ) {
        font=OpenFont(&topaz_attr);
        if (font) dock->dock_font_owned=1;
    }
    if (!font) font=dock->screen->RastPort.Font;
    dock->dock_font=font;
    SetFont(dock->window->RPort,font);
    SetSoftStyle(dock->window->RPort,
                 dock->conf.font_mode==ND_FONT_BOLD ? FSF_BOLD : FS_NORMAL,
                 0xff);
}
ULONG pen(struct NDDock *dock, int color)
{
    if (dock->colors[color] >= 0) return (ULONG)dock->colors[color];
    return color == ND_BG || color == ND_BAR_BG ? 0UL : 1UL;
}
static void fill(struct NDDock *dock, int color, int x, int y, int w, int h)
{
    struct RastPort *rp = dock->window->RPort;
    if (w < 1 || h < 1) return;
    SetAPen(rp, pen(dock, color));
    RectFill(rp, x, y, x + w - 1, y + h - 1);
}
static void paint_background(struct NDDock *dock,int x,int y,int w,int h)
{
    fill(dock,ND_BG,x,y,w,h);
}
static void rounded_box(struct NDDock *dock, int color,
                        int x, int y, int w, int h)
{
    if (w < 6 || h < 6) return;
    fill(dock, color, x + 2, y, w - 4, h);
    fill(dock, color, x, y + 2, w, h - 4);
}
static void tile_bevel(struct NDDock *dock,int fill_color,
                       int x,int y,int w,int h)
{
    struct RastPort *rp=dock->window->RPort;
    int light=fill_color==ND_HOVER ? ND_ACCENT : ND_HOVER;
    if (w<8 || h<8 || dock->conf.shape==ND_SHAPE_MINIMAL) return;
    SetAPen(rp,pen(dock,light));
    if (dock->conf.shape==ND_SHAPE_CHAMFER) {
        Move(rp,x+5,y);Draw(rp,x+w-6,y);
        Move(rp,x+5,y);Draw(rp,x,y+5);Draw(rp,x,y+h-6);
        SetAPen(rp,pen(dock,ND_BAR_BG));
        Move(rp,x+w-6,y);Draw(rp,x+w-1,y+5);
        Draw(rp,x+w-1,y+h-6);Draw(rp,x+w-6,y+h-1);
        Draw(rp,x+5,y+h-1);Draw(rp,x,y+h-6);
    } else if (dock->conf.shape==ND_SHAPE_ROUNDED) {
        Move(rp,x+2,y);Draw(rp,x+w-3,y);
        Move(rp,x+1,y+1);Draw(rp,x,y+2);Draw(rp,x,y+h-3);
        SetAPen(rp,pen(dock,ND_BAR_BG));
        Move(rp,x+w-2,y+1);Draw(rp,x+w-1,y+2);
        Draw(rp,x+w-1,y+h-3);Draw(rp,x+w-2,y+h-2);
        Draw(rp,x+w-3,y+h-1);Draw(rp,x+2,y+h-1);
    } else {
        Move(rp,x,y+h-2);Draw(rp,x,y);Draw(rp,x+w-2,y);
        SetAPen(rp,pen(dock,ND_BAR_BG));
        Move(rp,x+w-1,y);Draw(rp,x+w-1,y+h-1);Draw(rp,x,y+h-1);
    }
}
static void tile_box(struct NDDock *dock,int color,int x,int y,int w,int h)
{
    paint_background(dock,x,y,w,h);
    if (dock->conf.shape==ND_SHAPE_SQUARE) {
        fill(dock,color,x,y,w,h);
    } else if (dock->conf.shape==ND_SHAPE_CHAMFER) {
        fill(dock,color,x+5,y,w-10,h);
        fill(dock,color,x+2,y+3,w-4,h-6);
        fill(dock,color,x,y+6,w,h-12);
    } else if (dock->conf.shape==ND_SHAPE_MINIMAL) {
        if (color==ND_HOVER) fill(dock,color,x+1,y+1,w-2,h-2);
        /* Minimal means content directly on the dock backdrop.  The former
         * five-pixel baseline looked like an unexplained progress bar and
         * collided visually with real disk/plugin meters. */
    } else rounded_box(dock,color,x,y,w,h);
    tile_bevel(dock,color,x,y,w,h);
}
static void centered_text(struct NDDock *dock, int color,
                          const char *value, int x, int baseline, int w)
{
    struct RastPort *rp = dock->window->RPort;
    char printable[ND_TEXT_LEN];
    WORD extent;
    int len;
    if (w <= 4) return;
    strncpy(printable, value, sizeof(printable) - 1);
    printable[sizeof(printable) - 1] = '\0';
    len = (int)strlen(printable);
    SetAPen(rp, pen(dock, color));
    SetBPen(rp, pen(dock, ND_TILE));
    SetDrMd(rp, JAM1);
    while (len && TextLength(rp, printable, len) > w - 6) --len;
    extent = TextLength(rp, printable, len);
    Move(rp, x + (w - extent) / 2, baseline);
    Text(rp, printable, len);
}
static void centered_text_tight(struct NDDock *dock,int color,
                                const char *value,int x,int baseline,int w)
{
    struct RastPort *rp=dock->window->RPort;
    int len=(int)strlen(value),extent;
    SetAPen(rp,pen(dock,color));
    SetDrMd(rp,JAM1);
    while (len && TextLength(rp,(STRPTR)value,len)>w-1) --len;
    extent=TextLength(rp,(STRPTR)value,len);
    Move(rp,x+(w-extent)/2,baseline);
    Text(rp,(STRPTR)value,len);
}
int vertical(const struct NDDock *dock)
{
    return dock->conf.side == ND_SIDE_LEFT || dock->conf.side == ND_SIDE_RIGHT;
}
int item_visible(const struct NDDock *dock, int index)
{
    if (index < 0 || index >= dock->conf.count) return 0;
    if (!dock->overflow) return 1;
    return index >= dock->view_start &&
           index < dock->view_start + dock->visible_count;
}
static int launcher_label_visible(const struct NDDock *dock,int index)
{
    /* Tiny is deliberately icon-first.  The item's full name remains
     * available through the ordinary hover tooltip. */
    if (dock->conf.item_size==ND_ITEM_SIZE_TINY) return 0;
    if (dock->conf.label_mode==ND_LABEL_HIDDEN) return 0;
    if (dock->conf.label_mode==ND_LABEL_HOVER)
        return index==dock->hover || index==dock->press_item;
    return 1;
}
static int launcher_task_name(const struct NDItem *item,char *name,int size)
{
    const char *p,*last,*end;
    int n;
    if (!item || item->kind!=ND_ITEM_LAUNCH || !item->command[0] || size<2 ||
        item->launch_mode==ND_LAUNCH_AREXX || item->command[0]=='@') return 0;
    p=item->command;
    while (*p==' ' || *p=='\t' || *p=='\"') ++p;
    end=p;
    while (*end && *end!=' ' && *end!='\t' && *end!='\"') ++end;
    last=p;
    while (p<end) {
        if (*p=='/' || *p==':') last=p+1;
        ++p;
    }
    n=(int)(end-last);
    if (n<=0) return 0;
    if (n>=size) n=size-1;
    memcpy(name,last,(size_t)n);name[n]='\0';
    return 1;
}
/* Programs started from a shell (which includes everything NovaDock itself
 * launches) run inside a CLI process whose Exec task name is the shell's,
 * not the program's.  The loaded command is in cli_CommandName. */
static struct Task *find_cli_command(const char *wanted)
{
    struct List *lists[2];
    struct Node *node;
    struct Task *found=NULL;
    char name[64];
    int list,length;
    if (!wanted || !*wanted) return NULL;
    lists[0]=&SysBase->TaskReady;
    lists[1]=&SysBase->TaskWait;
    /* Interrupts move tasks between these lists, so walk them disabled and
     * keep the critical section short (no library calls). */
    Disable();
    for (list=0;list<2 && !found;++list)
        for (node=lists[list]->lh_Head;node->ln_Succ;node=node->ln_Succ) {
            struct Process *process=(struct Process *)node;
            struct CommandLineInterface *cli;
            UBYTE *command;
            if (node->ln_Type!=NT_PROCESS || !process->pr_CLI) continue;
            cli=(struct CommandLineInterface *)BADDR(process->pr_CLI);
            if (!cli || !cli->cli_Module || !cli->cli_CommandName) continue;
            command=(UBYTE *)BADDR(cli->cli_CommandName);
            if (!command || !command[0]) continue;
            length=command[0];
            if (length>(int)sizeof(name)-1) length=(int)sizeof(name)-1;
            memcpy(name,command+1,(size_t)length);name[length]='\0';
            if (nd_command_name_matches(name,wanted)) {
                found=(struct Task *)process;break;
            }
        }
    Enable();
    return found;
}
/* CLI command names of all running processes, taken in one pass with
 * interrupts off, so that checking many launchers does not walk the task
 * lists (and disable interrupts) once per launcher. */
#define ND_CLI_SNAPSHOT 24
struct NDCliSnapshot {
    int count;
    int complete;              /* 0: more processes than slots */
    struct Task *task[ND_CLI_SNAPSHOT];
    char name[ND_CLI_SNAPSHOT][64];
};

static void snapshot_cli_commands(struct NDCliSnapshot *snap)
{
    struct List *lists[2];
    struct Node *node;
    int list,length;
    snap->count=0;
    snap->complete=1;
    lists[0]=&SysBase->TaskReady;
    lists[1]=&SysBase->TaskWait;
    Disable();
    for (list=0;list<2;++list)
        for (node=lists[list]->lh_Head;node->ln_Succ;node=node->ln_Succ) {
            struct Process *process=(struct Process *)node;
            struct CommandLineInterface *cli;
            UBYTE *command;
            if (node->ln_Type!=NT_PROCESS || !process->pr_CLI) continue;
            cli=(struct CommandLineInterface *)BADDR(process->pr_CLI);
            if (!cli || !cli->cli_Module || !cli->cli_CommandName) continue;
            command=(UBYTE *)BADDR(cli->cli_CommandName);
            if (!command || !command[0]) continue;
            if (snap->count>=ND_CLI_SNAPSHOT) {snap->complete=0;continue;}
            length=command[0];
            if (length>63) length=63;
            memcpy(snap->name[snap->count],command+1,(size_t)length);
            snap->name[snap->count][length]='\0';
            snap->task[snap->count++]=(struct Task *)process;
        }
    Enable();
}

static struct Task *snapshot_cli_find(const struct NDCliSnapshot *snap,
                                      const char *wanted)
{
    int i;
    if (!wanted || !*wanted) return NULL;
    for (i=0;i<snap->count;++i)
        if (nd_command_name_matches(snap->name[i],wanted)) return snap->task[i];
    return snap->complete ? NULL : find_cli_command(wanted);
}

/* launcher_task(), optionally matching CLI commands against SNAP. */
static struct Task *launcher_task_using(const struct NDItem *item,
                                        const struct NDCliSnapshot *snap)
{
    char name[64];
    struct Task *task;
    if (!item) return NULL;
    if (ITEM_TEXT(item,ND_TEXT_MATCH_TASK)[0]) {
        strncpy(name,ITEM_TEXT(item,ND_TEXT_MATCH_TASK),sizeof(name)-1);
        name[sizeof(name)-1]='\0';
    } else {
        /* Any explicit identity switches this launcher out of automatic
         * basename matching. This prevents a stale/unrelated task with the
         * executable's name from overriding a deliberate window/port rule. */
        if (ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW)[0] || ITEM_TEXT(item,ND_TEXT_MATCH_PORT)[0]) return NULL;
        if (!launcher_task_name(item,name,sizeof(name))) return NULL;
    }
    Forbid();task=FindTask((STRPTR)name);Permit();
    if (!task) task=snap ? snapshot_cli_find(snap,name) : find_cli_command(name);
    return task;
}

struct Task *launcher_task(const struct NDItem *item)
{
    return launcher_task_using(item,NULL);
}
struct Task *launcher_port_task(const struct NDItem *item,int *present)
{
    struct MsgPort *port=NULL;
    struct Task *task=NULL;
    if (present) *present=0;
    if (!item || !ITEM_TEXT(item,ND_TEXT_MATCH_PORT)[0]) return NULL;
    Forbid();
    port=FindPort((STRPTR)ITEM_TEXT(item,ND_TEXT_MATCH_PORT));
    if (port) {
        if (present) *present=1;
        task=port->mp_SigTask;
    }
    Permit();
    return task;
}
struct Window *launcher_window(struct NDDock *dock,
                                      const struct NDItem *item,
                                      struct Task *task,
                                      struct Task *port_task)
{
    struct Screen *screen;
    struct Window *window,*found=NULL;
    ULONG lock;
    int shell;
    if (!dock || !item) return NULL;
    shell=!ITEM_TEXT(item,ND_TEXT_MATCH_TASK)[0] && !ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW)[0] &&
          !ITEM_TEXT(item,ND_TEXT_MATCH_PORT)[0] &&
          nd_launcher_matches_shell_window(item->command,"AmigaShell");
    if (!task && !port_task && !shell && !ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW)[0]) return NULL;
    lock=LockIBase(0);
    for (screen=IntuitionBase->FirstScreen;screen && !found;
         screen=screen->NextScreen)
        for (window=screen->FirstWindow;window;window=window->NextWindow) {
            if (window==dock->window) continue;
            if ((task && window->UserPort &&
                 window->UserPort->mp_SigTask==task) ||
                (port_task && window->UserPort &&
                 window->UserPort->mp_SigTask==port_task) ||
                (ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW)[0] && window->Title &&
                 nd_text_matches_pattern(ITEM_TEXT(item,ND_TEXT_MATCH_WINDOW),window->Title)) ||
                (shell && window->Title &&
                 nd_launcher_matches_shell_window(item->command,
                                                  window->Title))) {
                found=window;break;
            }
        }
    UnlockIBase(lock);
    return found;
}
static int launcher_running_using(struct NDDock *dock,const struct NDItem *item,
                                  const struct NDCliSnapshot *snap)
{
    struct Task *task=launcher_task_using(item,snap);
    struct Task *port_task;
    int port_present=0;
    port_task=launcher_port_task(item,&port_present);
    return task!=NULL || port_present ||
           launcher_window(dock,item,task,port_task)!=NULL;
}

int launcher_running(struct NDDock *dock,const struct NDItem *item)
{
    return launcher_running_using(dock,item,NULL);
}
static void draw_running_indicator(struct NDDock *dock,int index,
                                   int x,int y,int w)
{
    if (dock->conf.running_apps && index>=0 && index<ND_MAX_ITEMS &&
        dock->running_items[index])
        fill(dock,ND_ACCENT2,x+w-9,y+5,4,4);
}
void refresh_running_items(struct NDDock *dock)
{
    struct NDCliSnapshot snap;
    int i,running,taken=0;
    if (!dock || dock->hidden || dock->covered) return;
    for (i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i) {
        const struct NDItem *item=&dock->conf.items[i];
        int check=dock->conf.running_apps && item->kind==ND_ITEM_LAUNCH;
        /* Tiles on another page are refreshed when they come into view. */
        if (check && dock->window && !item_visible(dock,i)) continue;
        if (check && !taken) {snapshot_cli_commands(&snap);taken=1;}
        running=item->transient==ND_TRANSIENT_TASK ||
                (check && launcher_running_using(dock,item,&snap));
        if (dock->running_items[i]!=(unsigned char)running) {
            dock->running_items[i]=(unsigned char)running;
            if (dock->window && item_visible(dock,i)) draw_tile(dock,i);
        }
    }
    for (;i<ND_MAX_ITEMS;++i) dock->running_items[i]=0;
}
static int row_offset_pixels(struct NDDock *dock,int row)
{
    int used;
    if (!dock || vertical(dock) || dock->overflow ||
        dock->layout_rows<=1 || dock->layout_columns<1) return 0;
    used=nd_layout_row_used_cells(&dock->conf,row,dock->layout_columns);
    if (used<1 || used>=dock->layout_columns) return 0;
    return (dock->layout_columns-used)*(dock->tile_width+dock->gap)/2;
}
void slot_coords(struct NDDock *dock, int slot, int *x, int *y)
{
    int row=0,column=slot,row_offset=0;
    if (!vertical(dock) && dock->layout_rows>1 && dock->layout_columns>0) {
        row=slot/dock->layout_columns;
        column=slot%dock->layout_columns;
        /* Pack items predictably from the left, then center each completed
         * row inside the common dock frame.  In particular, a short final
         * row no longer leaves all of its unused space on the right. */
        row_offset=row_offset_pixels(dock,row);
    }
    *x = ND_BORDER + (vertical(dock) ? 0 :
                     column * (dock->tile_width + dock->gap)+row_offset);
    *y = ND_BORDER + (vertical(dock) ?
                     slot * (dock->tile_height + dock->gap) :
                     row * (dock->tile_height + dock->gap));
}
static int item_axis_size(struct NDDock *dock,int index)
{
    int normal=vertical(dock) ? dock->tile_height : dock->tile_width;
    int custom;
    if (index<0 || index>=dock->conf.count) return normal;
    /* The music transport needs room for metadata and three controls.  It is
     * one logical item, but spans exactly two ordinary horizontal slots. */
    if (!vertical(dock) && dock->conf.item_size!=ND_ITEM_SIZE_TINY &&
        dock->conf.items[index].kind==ND_ITEM_PLUGIN &&
        !strcmp(dock->conf.items[index].id,"MUSIC"))
        return normal*2+dock->gap;
    if (dock->overflow || dock->conf.items[index].kind!=ND_ITEM_SPACER)
        return normal;
    custom=dock->conf.items[index].spacer_width;
    if (custom && dock->conf.item_size==ND_ITEM_SIZE_TINY)
        custom=(custom+1)/2;
    return custom ? custom : (normal+1)/2;
}
static int item_slot_span(struct NDDock *dock,int index)
{
    if (!vertical(dock) && dock->conf.item_size!=ND_ITEM_SIZE_TINY &&
        index>=0 && index<dock->conf.count &&
        dock->conf.items[index].kind==ND_ITEM_PLUGIN &&
        !strcmp(dock->conf.items[index].id,"MUSIC")) return 2;
    return 1;
}
void tile_dimensions(struct NDDock *dock,int index,int *w,int *h)
{
    *w=dock->tile_width;*h=dock->tile_height;
    if (vertical(dock)) *h=item_axis_size(dock,index);
    else *w=item_axis_size(dock,index);
}
void tile_coords(struct NDDock *dock, int index, int *x, int *y)
{
    int slot,i,axis=ND_BORDER;
    if (!item_visible(dock,index)) { *x=-10000; *y=-10000; return; }
    if (dock->overflow || (!vertical(dock) && dock->layout_rows>1)) {
        if (!dock->overflow && dock->conf.row_layout) {
            int row,column,span;
            if (!nd_layout_item_cell(&dock->conf,index,dock->layout_columns,
                                     &row,&column,&span)) {
                *x=-10000;*y=-10000;return;
            }
            slot=row*dock->layout_columns+column;
        } else {
            slot=dock->overflow ? 1 : 0;
            for (i=dock->overflow ? dock->view_start : 0;i<=index;++i) {
                int span=item_slot_span(dock,i);
                int column=dock->layout_columns>0 ?
                           slot%dock->layout_columns : 0;
                if (dock->layout_rows>1 && column+span>dock->layout_columns)
                    slot+=dock->layout_columns-column;
                if (i==index) break;
                slot+=span;
            }
        }
        slot_coords(dock,slot,x,y);return;
    }
    for (i=0;i<index;++i) axis+=item_axis_size(dock,i)+dock->gap;
    *x=vertical(dock) ? ND_BORDER : axis;
    *y=vertical(dock) ? axis : ND_BORDER;
}
/* Render at most one dot per stored sample. This is a sparkline, not
 * an oscilloscope: every incoming plugin update adds one sample.
 * Normalization is supplied by each plugin and documented per metric.
 */
/* The graph area of tile INDEX drawn at X,Y (TW x TH). */
static void graph_area(struct NDDock *dock,int index,int x,int y,int tw,int th,
                       int *gx,int *gy,int *gw,int *gh)
{
    *gx=x+6;*gw=tw-12;
    if (index>=0 && index<dock->conf.count &&
        (!strcmp(dock->conf.items[index].id,"RAM") ||
         !strcmp(dock->conf.items[index].id,"NET") ||
         !strcmp(dock->conf.items[index].id,"ROADSHOW"))) {
        /* RAM's two colours identify Chip and Fast memory; its
         * detailed figures live in the tooltip, leaving the graph uncluttered.
         * Reserve the common plugin heading row above the graph. */
        *gy=y+(th>=60 ? 31 : 22);*gh=th-(th>=60 ? 36 : 27);
    } else if (th >= 60) { *gy = y + 34; *gh = th - 39; }
    else { *gy = y + 27; *gh = th - 32; }
}

/* The traces from column FIRST_COL to the right edge (all of them when it
 * is GX).  Drawing rules that let a scrolled graph (draw_graph_step) match a
 * full redraw pixel for pixel (checked by simulation):
 *  - the two series are drawn interleaved, segment by segment, so redrawing
 *    the segments entering a zone restores the same stacking order;
 *  - every segment restarts its dash pattern (SetDrPt + Move), so its pixels
 *    depend only on its end points;
 *  - a full draw also draws the segment entering the leftmost column from
 *    the sample just outside, then clears that outer column;
 *  - a partial draw never draws at the point just left of FIRST_COL: it is
 *    already correct and only starts the first segment into the zone;
 *  - the newest-point marker appears only when the newest sample exists. */
static void graph_series(struct NDDock *dock,int index,int gx,int gy,int gw,
                         int gh,int first_col)
{
    static const UWORD patterns[2]={0xFFFFU,0xCCCCU}; /* TX dashed */
    static const int colors[2]={ND_ACCENT,ND_ACCENT2};
    struct NDGraphHistory *hist = &dock->history[index];
    struct RastPort *rp = dock->window->RPort;
    int count, extra, age, series, px, py;
    int old_x[2], old_y[2], have[2];
    unsigned int sample[2];
    count = (int)hist->count;
    if (count > gw) count = gw;
    extra = first_col==gx && (int)hist->count > count;
    have[0]=have[1]=0;
    old_x[0]=old_x[1]=old_y[0]=old_y[1]=0;
    for (age = count - 1 + extra; age >= 0; --age) {
        if (!nd_graph_get(hist, (unsigned int)age, &sample[0], &sample[1])) {
            have[0]=have[1]=0;
            continue;
        }
        px = gx + gw - 1 - age;
        for (series = 0; series < 2; ++series) {
            if (sample[series] > 100U || px < first_col - 1) {
                have[series] = 0;
                continue;
            }
            py = gy + gh - 1 - (int)(sample[series] * (unsigned int)(gh - 1) / 100U);
            if (px < first_col) {
                old_x[series]=px;old_y[series]=py;have[series]=1;
                continue;
            }
            SetAPen(rp, pen(dock, colors[series]));
            SetDrPt(rp, patterns[series]);
            Move(rp, have[series] ? old_x[series] : px,
                     have[series] ? old_y[series] : py);
            Draw(rp, px, py);
            old_x[series]=px;old_y[series]=py;have[series]=1;
        }
    }
    SetDrPt(rp, 0xFFFFU);
    if (extra) fill(dock, tile_interior(dock,index), gx-1, gy, 1, gh);
    for (series = 0; series < 2; ++series)
        if (have[series] && old_x[series] == gx + gw - 1) {
            SetAPen(rp, pen(dock, colors[series]));
            RectFill(rp,old_x[series]>gx ? old_x[series]-1 : old_x[series],
                     old_y[series],old_x[series],
                     old_y[series]<gy+gh-1 ? old_y[series]+1 : old_y[series]);
        }
}

/* The interior colour a tile is painted on (see tile_box). */
static int tile_interior(struct NDDock *dock,int index)
{
    if (index==dock->hover) return ND_HOVER;
    return dock->conf.shape==ND_SHAPE_MINIMAL ? ND_BG : ND_TILE;
}

static void draw_graph(struct NDDock *dock, int index,
                       int x, int y, int tw, int th)
{
    struct RastPort *rp = dock->window->RPort;
    struct NDGraphView *view = &dock->graph_view[index];
    int gx, gy, gw, gh, s;
    UWORD pattern;
    graph_area(dock,index,x,y,tw,th,&gx,&gy,&gw,&gh);
    if (gw < 2 || gh < 3) return;
    /* One dotted midpoint guide provides scale without looking like a third
     * data series or an unexplained grey progress bar.  Its dots sit where
     * (x-gx+phase) is a multiple of four, so they move with scrolled graphs. */
    s=(4-(view->phase&3))&3;
    pattern=(UWORD)(((0x8888U>>s)|(0x8888U<<(16-s)))&0xFFFFU);
    SetAPen(rp,pen(dock,ND_BAR_BG));
    SetDrPt(rp,pattern);
    Move(rp,gx,gy+gh/2);Draw(rp,gx+gw-1,gy+gh/2);
    SetDrPt(rp,0xFFFFU);
    graph_series(dock,index,gx,gy,gw,gh,gx);
    view->x=(short)x;view->y=(short)y;view->tw=(short)tw;view->th=(short)th;
    view->gx=(short)gx;view->gy=(short)gy;view->gw=(short)gw;view->gh=(short)gh;
    view->bg=(UBYTE)tile_interior(dock,index);
    view->painted=dock->graph_samples[index];
}
/* Palette-based ILBM drawing; no PNG datatype or chunky RTG surface.
 * Coalesce neighbouring pixels of the same colour into horizontal runs. */
static int draw_brush(struct NDDock *dock, int index, int x, int y,
                      int tw, int th)
{
    const struct NDILBM *b = dock->brushes[index];
    struct RastPort *rp = dock->window->RPort;
    int row, col, start, pal, xx, yy,iw,ih,max_width,max_height,tiny;
    if (!b) return 0;
    tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    nd_icon_art_box(tw,th,tiny,&max_width,&max_height);
    iw=dock->scaled_icon_width[index] ? dock->scaled_icon_width[index] : b->width;
    ih=dock->scaled_icon_height[index] ? dock->scaled_icon_height[index] : b->height;
    if (iw>max_width || ih>max_height) return 0;
    xx = x + (tw - iw) / 2;
    yy=nd_icon_art_top(y,th,ih,tiny);
    /* The ILBM is converted once into a screen-friendly bitmap and mask.
     * Repainting then becomes one blit instead of dozens of RectFill calls. */
    if (dock->brush_bitmaps[index] && dock->brush_masks[index]) {
        BltMaskBitMapRastPort(dock->brush_bitmaps[index], 0, 0, rp, xx, yy,
                              iw, ih, 0xE0,
                              dock->brush_masks[index]->Planes[0]);
        return 1;
    }
    if (iw!=b->width || ih!=b->height) return 0;
    for (row = 0; row < b->height; ++row) {
        col = 0;
        while (col < b->width) {
            pal = b->pixels[row * ND_ICON_MAX_DIM + col];
            start = col++;
            while (col < b->width &&
                   b->pixels[row * ND_ICON_MAX_DIM + col] == pal) ++col;
            if (pal != b->transparent && dock->brush_pens[index][pal] >= 0) {
                SetAPen(rp, (ULONG)dock->brush_pens[index][pal]);
                RectFill(rp, xx + start, yy + row, xx + col - 1, yy + row);
            }
        }
    }
    return 1;
}
/* Analog Clock is drawn by the host, not by an unsafe plugin RastPort.
 * Fixed-point lookup eliminates floating point/libm and supports 68000. */
static const signed char nd_clock_x[60] = {
0, 10, 21, 31, 41, 50, 59, 67, 74, 81, 87, 91, 95, 98, 99, 100, 99, 98, 95, 91, 87, 81, 74, 67, 59, 50, 41, 31, 21, 10, 0, -10, -21, -31, -41, -50, -59, -67, -74, -81, -87, -91, -95, -98, -99, -100, -99, -98, -95, -91, -87, -81, -74, -67, -59, -50, -41, -31, -21, -10
};
static const signed char nd_clock_y[60] = {
-100, -99, -98, -95, -91, -87, -81, -74, -67, -59, -50, -41, -31, -21, -10, 0, 10, 21, 31, 41, 50, 59, 67, 74, 81, 87, 91, 95, 98, 99, 100, 99, 98, 95, 91, 87, 81, 74, 67, 59, 50, 41, 31, 21, 10, 0, -10, -21, -31, -41, -50, -59, -67, -74, -81, -87, -91, -95, -98, -99
};
static void draw_analog_clock(struct NDDock *dock,int x,int y,int w,int h)
{
    struct RastPort *rp=dock->window->RPort;
    struct DateStamp stamp;
    int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    int cx=x+w/2,cy=tiny ? y+h/2 : y+(h-13)/2;
    int rad=tiny ? (h-8)/2 : (h-23)/2;
    int i,m,hour,hr_index,second,hr_len,min_len,sec_len,tick_len;
    int max_rad=tiny ? (w-8)/2 : (w-20)/2;
    if (rad>max_rad) rad=max_rad;
    if (rad<(tiny ? 6 : 8)) return;
    tick_len=tiny ? 2 : 3;
    hr_len=tiny ? (rad*55)/100 : rad-8;
    min_len=tiny ? (rad*75)/100 : rad-4;
    sec_len=tiny ? (rad*80)/100 : rad-3;
    DateStamp(&stamp);
    hour=(int)((stamp.ds_Minute/60L)%24L);
    m=(int)(stamp.ds_Minute%60L);
    second=(int)(stamp.ds_Tick/50L);
    SetAPen(rp,pen(dock,ND_SUBTLE));
    for (i=0;i<60;i+=5) {
        int ax=cx+(nd_clock_x[i]*rad)/100;
        int ay=cy+(nd_clock_y[i]*rad)/100;
        Move(rp,ax,ay);
        Draw(rp,cx+nd_clock_x[i]*(rad-tick_len)/100,
                cy+nd_clock_y[i]*(rad-tick_len)/100);
    }
    SetAPen(rp,pen(dock,ND_TEXT));
    hr_index=((hour%12)*5+m/12)%60;
    Move(rp,cx,cy);
    Draw(rp,cx+nd_clock_x[hr_index]*hr_len/100,
            cy+nd_clock_y[hr_index]*hr_len/100);
    SetAPen(rp,pen(dock,ND_ACCENT));
    Move(rp,cx,cy);
    Draw(rp,cx+nd_clock_x[m]*min_len/100,
            cy+nd_clock_y[m]*min_len/100);
    if (!strcmp(nd_option_get(&dock->conf,"CLOCK","SECONDS"),"YES")) {
        SetAPen(rp,pen(dock,ND_ACCENT2));
        Move(rp,cx,cy);
        Draw(rp,cx+nd_clock_x[second%60]*sec_len/100,
                cy+nd_clock_y[second%60]*sec_len/100);
    }
    if (!tiny) centered_text(dock,ND_SUBTLE,"CLOCK",x,y+h-7,w);
}
/* The Eyes plugin is an ordinary Exec-message process; screen-pointer
 * rendering stays inside the host because RastPorts are not IPC-safe.
 * Scan-line ellipse fill avoids floating point and extra graphics surfaces.
 */
int eyes_index(const struct NDDock *dock)
{
    int i;
    for (i=0;i<dock->conf.count;++i)
        if (dock->conf.items[i].kind==ND_ITEM_PLUGIN &&
            !strcmp(dock->conf.items[i].id,"EYES") &&
            dock->conf.items[i].online) return i;
    return -1;
}
int eyes_option_num(struct NDDock *dock,const char *key,int fallback)
{
    const char *v=nd_option_get(&dock->conf,"EYES",key);
    return v ? atoi(v) : fallback;
}
int eyes_option_yes(struct NDDock *dock,const char *key)
{
    const char *v=nd_option_get(&dock->conf,"EYES",key);
    return v && !strcmp(v,"YES");
}
static int eyes_color(struct NDDock *dock,const char *key,int white_pen)
{
    const char *v=nd_option_get(&dock->conf,"EYES",key);
    if (v && !strcmp(v,"BLUE")) return ND_ACCENT;
    if (v && !strcmp(v,"GREEN")) return ND_ACCENT2;
    return white_pen;
}
static void eyes_filled_ellipse(struct NDDock *dock,
                                int cx,int cy,int rx,int ry,int color)
{
    struct RastPort *rp=dock->window->RPort;
    int y,x;
    LONG rx2,ry2,bound;
    if (rx<1 || ry<1) return;
    rx2=(LONG)rx*rx; ry2=(LONG)ry*ry; bound=rx2*ry2;
    SetAPen(rp,pen(dock,color));
    for (y=-ry;y<=ry;++y) {
        for (x=rx;x>0;--x)
            if ((LONG)x*x*ry2+(LONG)y*y*rx2 <= bound) break;
        RectFill(rp,cx-x,cy+y,cx+x,cy+y);
    }
}
static void eyes_geometry(struct NDDock *dock,int index,
                          int centers_x[2],int centers_y[2],
                          int *rx,int *ry,int *pr)
{
    int x,y,w=dock->tile_width,h=dock->tile_height;
    const char *size=nd_option_get(&dock->conf,"EYES","SIZE");
    int r = size && !strcmp(size,"SMALL") ? 9 :
            size && !strcmp(size,"LARGE") ? 15 : 12;
    int usable_h=h-(eyes_option_yes(dock,"SHOW_LABEL") ? 16 : 6);
    tile_coords(dock,index,&x,&y);
    if (r>(w-20)/4) r=(w-20)/4;
    if (r>(usable_h-8)/2) r=(usable_h-8)/2;
    if (r<3) r=3;
    *rx=r; *ry=r+2; if (*ry>(usable_h-6)/2) *ry=(usable_h-6)/2;
    if (*ry<3) *ry=3;
    *pr=r>=13 ? 4 : r>=9 ? 3 : 2;
    centers_x[0]=dock->window->LeftEdge+x+w/2-r-3;
    centers_x[1]=dock->window->LeftEdge+x+w/2+r+3;
    centers_y[0]=centers_y[1]=dock->window->TopEdge+y+usable_h/2;
}
void eyes_positions(struct NDDock *dock,int index,
                           int xp[2],int yp[2],int *rx,int *ry,int *pr)
{
    int ex[2],ey[2],i,sensitivity;
    eyes_geometry(dock,index,ex,ey,rx,ry,pr);
    sensitivity=eyes_option_num(dock,"SENSITIVITY",100);
    for (i=0;i<2;++i)
        nd_eyes_pupil(ex[i],ey[i],dock->screen->MouseX,
                      dock->screen->MouseY,*rx-*pr-2,*ry-*pr-2,
                      sensitivity,&xp[i],&yp[i]);
}
static void draw_eyes(struct NDDock *dock,int index,int x,int y,int tw,int th)
{
    int xp[2],yp[2],rx,ry,pr,ex[2],ey[2],i;
    int sclera=eyes_color(dock,"EYE_COLOR",ND_TEXT);
    int pupil=eyes_color(dock,"PUPIL_COLOR",ND_BG);
    eyes_positions(dock,index,xp,yp,&rx,&ry,&pr);
    eyes_geometry(dock,index,ex,ey,&rx,&ry,&pr);
    for (i=0;i<2;++i) {
        int xc=ex[i]-dock->window->LeftEdge;
        int yc=ey[i]-dock->window->TopEdge;
        if (dock->eyes_blink) {
            fill(dock,ND_SUBTLE,xc-rx,yc,rx*2+1,2);
        } else {
            eyes_filled_ellipse(dock,xc,yc,rx,ry,sclera);
            SetAPen(dock->window->RPort,pen(dock,ND_ACCENT));
            DrawEllipse(dock->window->RPort,xc,yc,rx,ry);
            eyes_filled_ellipse(dock,xp[i]-dock->window->LeftEdge,
                yp[i]-dock->window->TopEdge,pr,pr,pupil);
        }
        dock->eyes_last_x[index][i]=xp[i];
        dock->eyes_last_y[index][i]=yp[i];
    }
    dock->eyes_last_valid[index]=1;
    if (eyes_option_yes(dock,"SHOW_LABEL"))
        centered_text(dock,ND_SUBTLE,"EYES",x,y+th-7,tw);
}

static void draw_volume_meter(struct NDDock *dock, const struct NDItem *item,
                              int x, int y, int tw, int th)
{
    int meter_w;
    if (!nd_item_supports_space_meter(item) || !item->show_volume_meter ||
        item->meter > 100U) return;
    meter_w = tw - 16;
    if (meter_w < 4) return;
    /* Leave visible dock backdrop below the meter, especially in Minimal. */
    fill(dock, ND_BAR_BG, x + 8, y + th - 6, meter_w, 2);
    if (item->meter > 0)
        fill(dock, ND_ACCENT, x + 8, y + th - 6,
             (int)(item->meter * (ULONG)meter_w / 100UL), 2);
}

static int parse_ram_pair(const char *value,unsigned int *chip,
                          unsigned int *fast)
{
    unsigned long cu,cd,ct,ctd,fu,fd,ft,ftd;
    unsigned long used,total;
    if (!value || sscanf(value,"C:%lu.%lu/%lu.%lu F:%lu.%lu/%lu.%lu",
        &cu,&cd,&ct,&ctd,&fu,&fd,&ft,&ftd)!=8) return 0;
    used=cu*10UL+cd;total=ct*10UL+ctd;
    *chip=total ? (unsigned int)(used*100UL/total) : 0U;
    used=fu*10UL+fd;total=ft*10UL+ftd;
    *fast=total ? (unsigned int)(used*100UL/total) : 0U;
    if (*chip>100U) *chip=100U;
    if (*fast>100U) *fast=100U;
    return 1;
}

int format_ram_tooltip(const char *value,char *chip_text,
                              unsigned int chip_size,char *fast_text,
                              unsigned int fast_size)
{
    unsigned long gu,gd,gt,gtd,fu,fd,ft,ftd;
    unsigned long used,total,free_tenths;
    if (!value || !chip_text || !chip_size || !fast_text || !fast_size ||
        sscanf(value,"C:%lu.%lu/%lu.%lu F:%lu.%lu/%lu.%lu",
               &gu,&gd,&gt,&gtd,&fu,&fd,&ft,&ftd)!=8) return 0;
    used=gu*10UL+gd;total=gt*10UL+gtd;
    free_tenths=total>used ? total-used : 0;
    sprintf(chip_text,"CHIP: %lu.%luM used, %lu.%luM free, %lu.%luM total",
            used/10UL,used%10UL,free_tenths/10UL,free_tenths%10UL,
            total/10UL,total%10UL);
    chip_text[chip_size-1]='\0';
    used=fu*10UL+fd;total=ft*10UL+ftd;
    free_tenths=total>used ? total-used : 0;
    sprintf(fast_text,"Fast: %lu.%luM used, %lu.%luM free, %lu.%luM total",
            used/10UL,used%10UL,free_tenths/10UL,free_tenths%10UL,
            total/10UL,total%10UL);
    fast_text[fast_size-1]='\0';
    return 1;
}

static void draw_ram_tile(struct NDDock *dock,const struct NDItem *item,
                          int x,int y,int tw,int th)
{
    unsigned int chip=0,fast=0;
    int meter_w=tw-20,bar_h=th>=60 ? 5 : 3;
    int chip_y=y+th-29,fast_y=y+th-13;
    (void)parse_ram_pair(item->value,&chip,&fast);
    centered_text(dock,ND_SUBTLE,"RAM",x,y+(th>=60 ? 24 : 17),tw);
    fill(dock,ND_BAR_BG,x+10,chip_y,meter_w,bar_h);
    if (item->online && chip) fill(dock,ND_ACCENT,x+10,chip_y,
                    (int)(chip*(ULONG)meter_w/100UL),bar_h);
    fill(dock,ND_BAR_BG,x+10,fast_y,meter_w,bar_h);
    if (item->online && fast) fill(dock,ND_ACCENT2,x+10,fast_y,
                    (int)(fast*(ULONG)meter_w/100UL),bar_h);
}

/* Both traffic channels share the configured PPS scale.  State messages
 * replace rates during control operations; never leave stale traffic bars. */
static void draw_network_meters(struct NDDock *dock,int index,
                                 int x,int y,int tw,int th)
{
    struct NDItem *item=&dock->conf.items[index];
    unsigned int incoming=0,outgoing=0,rx=0,tx=0;
    int valid,channel,bar_h=th>=60 ? 5 : 3;
    int label_w=TextLength(dock->window->RPort,(STRPTR)"OUT",3)+6;
    int bar_x=x+8+label_w,bar_w=tw-16-label_w;
    int row[2];
    valid=sscanf(item->value,"%u/%u p/s",&rx,&tx)==2;
    if(valid) (void)nd_graph_get(&dock->history[index],0,&incoming,&outgoing);
    row[0]=y+th-(th>=60 ? 27 : 20);
    row[1]=y+th-10;
    centered_text(dock,ND_SUBTLE,"NETWORK",x,y+(th>=60 ? 24 : 17),tw);
    for(channel=0;channel<2;++channel){
        unsigned int level=channel ? outgoing:incoming;
        int color=channel ? ND_ACCENT2:ND_ACCENT;
        centered_text_tight(dock,color,channel ? "OUT":"IN",
                            x+8,row[channel]+bar_h,label_w-4);
        if(bar_w>0){
            fill(dock,ND_BAR_BG,bar_x,row[channel],bar_w,bar_h);
            if(valid && item->online && level<=100U && level)
                fill(dock,color,bar_x,row[channel],
                     (int)(level*(ULONG)bar_w/100UL),bar_h);
        }
    }
}

static void draw_ram_graph_tile(struct NDDock *dock,int index,
                                int x,int y,int tw,int th)
{
    centered_text(dock,ND_SUBTLE,"RAM",x,y+(th>=60 ? 24 : 17),tw);
    draw_graph(dock,index,x,y,tw,th);
}

static void music_button_box(struct NDDock *dock,int x,int y,int w,int h,
                             int enabled)
{
    struct RastPort *rp=dock->window->RPort;
    int color=enabled ? ND_SUBTLE : ND_BAR_BG;
    fill(dock,ND_BG,x,y,w,h);
    SetAPen(rp,pen(dock,color));
    Move(rp,x,y+h-1);Draw(rp,x,y);Draw(rp,x+w-1,y);
    Draw(rp,x+w-1,y+h-1);Draw(rp,x,y+h-1);
}
static void music_transport_icon(struct NDDock *dock,int kind,
                                 int x,int y,int w,int h,int enabled,int playing)
{
    struct RastPort *rp=dock->window->RPort;
    int cx=x+w/2,cy=y+h/2,color=enabled ? ND_TEXT : ND_SUBTLE;
    music_button_box(dock,x,y,w,h,enabled);
    SetAPen(rp,pen(dock,color));
    if (kind<0) {
        Move(rp,cx-4,cy-4);Draw(rp,cx-4,cy+4);
        Move(rp,cx+4,cy-4);Draw(rp,cx-3,cy);Draw(rp,cx+4,cy+4);
    } else if (kind>0) {
        Move(rp,cx+4,cy-4);Draw(rp,cx+4,cy+4);
        Move(rp,cx-4,cy-4);Draw(rp,cx+3,cy);Draw(rp,cx-4,cy+4);
    } else if (playing) {
        Move(rp,cx-3,cy-4);Draw(rp,cx-3,cy+4);
        Move(rp,cx+3,cy-4);Draw(rp,cx+3,cy+4);
    } else {
        Move(rp,cx-3,cy-4);Draw(rp,cx+4,cy);Draw(rp,cx-3,cy+4);
        Draw(rp,cx-3,cy-4);
    }
}
static void draw_music_tile(struct NDDock *dock,const struct NDItem *item,
                            int x,int y,int tw,int th)
{
    int top=th>=60 ? y+31 : y+22;
    int bh=th>=60 ? 14 : 12;
    int gap=2,margin=6,bw=(tw-margin*2-gap*2)/3;
    int track_x=x+8,track_w=tw-16,track_y=y+th-8;
    int volume=item->media_volume<=100U ? item->media_volume : 50;
    int knob;
    unsigned int caps=item->media_capabilities;
    centered_text(dock,ND_SUBTLE,"MUSIC",x,y+(th>=60 ? 11 : 9),tw);
    centered_text_tight(dock,item->online ? ND_TEXT : ND_SUBTLE,
                        item->value,x,y+(th>=60 ? 25 : 18),tw);
    music_transport_icon(dock,-1,x+margin,top,bw,bh,
                         (caps & ND_MEDIA_CAP_PREVIOUS)!=0,0);
    music_transport_icon(dock,0,x+margin+bw+gap,top,bw,bh,
                         (caps & ND_MEDIA_CAP_PLAY_PAUSE)!=0,
                         (item->media_state & ND_MEDIA_STATE_PLAYING)!=0);
    music_transport_icon(dock,1,x+margin+(bw+gap)*2,top,bw,bh,
                         (caps & ND_MEDIA_CAP_NEXT)!=0,0);
    fill(dock,ND_BAR_BG,track_x,track_y,track_w,2);
    if (caps & ND_MEDIA_CAP_VOLUME) {
        if (volume>0) fill(dock,ND_ACCENT2,track_x,track_y,
            (int)(volume*(ULONG)track_w/100UL),2);
        knob=track_x+(int)(volume*(ULONG)(track_w-1)/100UL);
        fill(dock,ND_TEXT,knob-1,track_y-2,3,6);
    }
}
/* Returns one ND_EVENT_MEDIA_* action. Volume uses the release position so
 * click-and-drag on the track acts like a compact absolute slider. */
static unsigned int music_control_hit(struct NDDock *dock,int index,
                                      int mx,int my,unsigned int *volume)
{
    struct NDItem *item;
    int x,y,tw,th,top,bh,gap=2,margin=6,bw,which;
    if (volume) *volume=ND_MEDIA_VOLUME_UNKNOWN;
    if (dock->edit_mode) return 0;
    if (index<0 || index>=dock->conf.count) return 0;
    item=&dock->conf.items[index];
    if (item->kind!=ND_ITEM_PLUGIN || strcmp(item->id,"MUSIC") ||
        !item->online || !item->media_capabilities) return 0;
    tile_coords(dock,index,&x,&y);tile_dimensions(dock,index,&tw,&th);
    if (dock->conf.item_size==ND_ITEM_SIZE_TINY) {
        int button_w=15,button_h=12;
        int button_x=x+(tw-button_w)/2;
        if ((item->media_capabilities&ND_MEDIA_CAP_VOLUME) &&
            my>=y+th-12 && my<y+th && mx>=x+5 && mx<x+tw-5) {
            int start=x+5,width=tw-10,value;
            if (mx<=start) value=0;
            else if (mx>=start+width-1) value=100;
            else value=(mx-start)*100/(width-1);
            if (volume) *volume=(unsigned int)value;
            return ND_EVENT_MEDIA_VOLUME;
        }
        if ((item->media_capabilities&ND_MEDIA_CAP_PLAY_PAUSE) &&
            mx>=button_x && mx<button_x+button_w &&
            my>=y+3 && my<y+3+button_h)
            return ND_EVENT_MEDIA_PLAY_PAUSE;
        return 0;
    }
    if ((item->media_capabilities & ND_MEDIA_CAP_VOLUME) &&
        my>=y+th-13 && my<y+th && mx>=x+5 && mx<x+tw-5) {
        int start=x+8,width=tw-16,value;
        if (mx<=start) value=0;
        else if (mx>=start+width-1) value=100;
        else value=(mx-start)*100/(width-1);
        if (volume) *volume=(unsigned int)value;
        return ND_EVENT_MEDIA_VOLUME;
    }
    top=th>=60 ? y+31 : y+22;bh=th>=60 ? 14 : 12;
    bw=(tw-margin*2-gap*2)/3;
    if (my<top || my>=top+bh || mx<x+margin ||
        mx>=x+margin+bw*3+gap*2) return 0;
    which=(mx-(x+margin))/(bw+gap);
    if (which==0 && (item->media_capabilities & ND_MEDIA_CAP_PREVIOUS))
        return ND_EVENT_MEDIA_PREVIOUS;
    if (which==1 && (item->media_capabilities & ND_MEDIA_CAP_PLAY_PAUSE))
        return ND_EVENT_MEDIA_PLAY_PAUSE;
    if (which==2 && (item->media_capabilities & ND_MEDIA_CAP_NEXT))
        return ND_EVENT_MEDIA_NEXT;
    return 0;
}
int queue_music_control(struct NDDock *dock,int index,int mx,int my)
{
    struct NDItem *item;
    unsigned int action,volume;
    action=music_control_hit(dock,index,mx,my,&volume);
    if (!action) return 0;
    item=&dock->conf.items[index];
    item->media_event=(unsigned short)action;
    item->media_volume_request=ND_MEDIA_VOLUME_UNKNOWN;
    if (action==ND_EVENT_MEDIA_VOLUME && volume<=100U) {
        item->media_volume=(unsigned short)volume;
        item->media_volume_request=(unsigned short)volume;
    } else if (action==ND_EVENT_MEDIA_PLAY_PAUSE) {
        item->media_state ^= ND_MEDIA_STATE_PLAYING;
    }
    draw_tile(dock,index);
    return 1;
}

/* Half-size plugins retain the two pieces of information which matter at a
 * glance and omit transport buttons, legends and full graphs. */
static void draw_tiny_plugin(struct NDDock *dock,const struct NDItem *item,
                             int x,int y,int tw,int th)
{
    int meter_w=tw-10;
    int first=y+10,second=y+(th>=36 ? 23 : 20);
    const char *caption=item->caption[0] ? item->caption : item->label;
    if (!strcmp(item->id,"ARCHIVE")) {
        centered_text_tight(dock,ND_TEXT,"EXTR",x,y+th/2+3,tw);
        return;
    }
    if (!strcmp(item->id,"CLOCK")) {
        if (!strcmp(nd_option_get(&dock->conf,"CLOCK","FORMAT"),"ANALOG"))
            draw_analog_clock(dock,x,y,tw,th);
        else
            centered_text_tight(dock,item->online ? ND_TEXT : ND_SUBTLE,
                                item->value,x,y+th/2+3,tw);
        return;
    }
    if (!strcmp(item->id,"MUSIC")) {
        int caps=item->media_capabilities;
        int button_w=15,button_h=12;
        int button_x=x+(tw-button_w)/2;
        int track_x=x+5,track_w=tw-10,track_y=y+th-6;
        int volume=item->media_volume<=100U ? item->media_volume : 50;
        int knob;
        /* An idle player still needs an immediately recognisable tile.  Keep
         * it as compact as EXTR; controls replace the label when available. */
        if (!item->online || !caps) {
            centered_text_tight(dock,ND_TEXT,"MUSIC",x,y+th/2+3,tw);
            return;
        }
        music_transport_icon(dock,0,button_x,y+3,button_w,button_h,
            (caps&ND_MEDIA_CAP_PLAY_PAUSE)!=0,
            (item->media_state&ND_MEDIA_STATE_PLAYING)!=0);
        fill(dock,ND_BAR_BG,track_x,track_y,track_w,2);
        if (caps&ND_MEDIA_CAP_VOLUME) {
            if (volume>0) fill(dock,ND_ACCENT2,track_x,track_y,
                (int)(volume*(ULONG)track_w/100UL),2);
            knob=track_x+(int)(volume*(ULONG)(track_w-1)/100UL);
            fill(dock,ND_TEXT,knob-1,track_y-2,3,6);
        }
        return;
    }
    if (!strcmp(item->id,"RAM")) {
        unsigned int chip=0,fast=0;
        int bar_h=3,chip_y=y+18,fast_y=y+th-8;
        (void)parse_ram_pair(item->value,&chip,&fast);
        centered_text_tight(dock,ND_SUBTLE,"RAM",x,y+10,tw);
        fill(dock,ND_BAR_BG,x+5,chip_y,meter_w,bar_h);
        if (item->online && chip) fill(dock,ND_ACCENT,x+5,chip_y,
                    (int)(chip*(ULONG)meter_w/100UL),bar_h);
        fill(dock,ND_BAR_BG,x+5,fast_y,meter_w,bar_h);
        if (item->online && fast) fill(dock,ND_ACCENT2,x+5,fast_y,
                    (int)(fast*(ULONG)meter_w/100UL),bar_h);
        return;
    }
    centered_text_tight(dock,ND_SUBTLE,caption,x,first,tw);
    centered_text_tight(dock,item->online ? ND_TEXT : ND_SUBTLE,
                        item->value,x,second,tw);
    if (item->meter<=100U && meter_w>=4) {
        fill(dock,ND_BAR_BG,x+5,y+th-5,meter_w,2);
        if (item->meter)
            fill(dock,ND_ACCENT,x+5,y+th-5,
                 (int)(item->meter*(ULONG)meter_w/100UL),2);
    }
}

static int dopus_application_running(const struct NDItem *item)
{
    if (!item || strcmp(item->id,"DOPUS4")) return 0;
    /* Keep the protocol's more detailed state for click/restore handling,
     * but reduce the tile itself to the same visual language as launchers. */
    return !strcmp(item->value,"READY") ||
           !strcmp(item->value,"ICONIFIED") ||
           !strcmp(item->value,"FRONT FAILED");
}

static void draw_plugin_launcher_artwork(struct NDDock *dock,int index,
                                         const char *label,
                                         int x,int y,int tw,int th)
{
    int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    struct BitMap *art,*art_mask;
    int iw,ih;
    if (tile_icon_art(dock,index,&art,&art_mask,&iw,&ih)) {
        BltMaskBitMapRastPort(art,0,0,
            dock->window->RPort,x+(tw-iw)/2,
            nd_icon_art_top(y,th,ih,tiny),iw,ih,0xE0,art_mask->Planes[0]);
    } else if (dock->icons[index]) {
        struct Image *im=(struct Image *)
            dock->icons[index]->do_Gadget.GadgetRender;
        if (im && im->Depth>=1 && im->Depth<=4 && im->Width>0 &&
            im->Height>0 && im->Width<=tw-(tiny ? 2 : 14) &&
            im->Height<=th-(tiny ? 2 : 27))
            DrawImage(dock->window->RPort,im,
                      x+(tw-im->Width)/2,
                      nd_icon_art_top(y,th,im->Height,tiny));
    }
    if (!tiny)
        centered_text_tight(dock,ND_TEXT,label,x,y+th-9,tw);
}

static void draw_dopus_tile(struct NDDock *dock,int index,
                            const struct NDItem *item,
                            int x,int y,int tw,int th)
{
    draw_plugin_launcher_artwork(dock,index,"DOPUS",x,y,tw,th);
    if (dopus_application_running(item))
        fill(dock,ND_ACCENT2,x+tw-9,y+5,4,4);
}

static void draw_music_idle_tile(struct NDDock *dock,int index,
                                 int x,int y,int tw,int th)
{
    draw_plugin_launcher_artwork(dock,index,"MUSIC",x,y,tw,th);
}

static void draw_running_apps_tile(struct NDDock *dock,int index,
                                   int x,int y,int tw,int th)
{
    /* Running Apps is a launcher for a transient application picker, not an
     * application whose resident-plugin state belongs on the tile. */
    draw_plugin_launcher_artwork(dock,index,"APPS",x,y,tw,th);
}

/* A plugin's badge (ND_MSG_BADGE) on the tile's top-right corner: the text
 * on a secondary-accent label, or a dot for "*" and on tiny tiles.  It takes
 * the place of the running dot, which shares that corner. */
static int draw_plugin_badge(struct NDDock *dock,const struct NDItem *item,
                             int x,int y,int tw)
{
    struct RastPort *rp=dock->window->RPort;
    int length=(int)strlen(item->badge),width,height,bx,by;
    if (!length) return 0;
    if (!strcmp(item->badge,"*") || dock->conf.item_size==ND_ITEM_SIZE_TINY) {
        fill(dock,ND_ACCENT2,x+tw-11,y+4,6,6);
        return 1;
    }
    height=rp->TxHeight+2;
    while (length>0 &&
           (width=TextLength(rp,(STRPTR)item->badge,length)+6)>tw-6) --length;
    if (!length) {
        fill(dock,ND_ACCENT2,x+tw-11,y+4,6,6);
        return 1;
    }
    bx=x+tw-width-3;by=y+3;
    fill(dock,ND_ACCENT2,bx,by,width,height);
    SetAPen(rp,pen(dock,ND_BG));
    SetDrMd(rp,JAM1);
    Move(rp,bx+3,by+1+rp->TxBaseline);
    Text(rp,(STRPTR)item->badge,length);
    return 1;
}

static void draw_plugin_running_indicator(struct NDDock *dock,
                                          const struct NDItem *item,
                                          int x,int y,int tw)
{
    int mode=nd_indicator_mode(item->id,item->online,dock->conf.running_apps,
                                item->media_capabilities,item->badge);
    if(mode==ND_INDICATOR_BADGE)(void)draw_plugin_badge(dock,item,x,y,tw);
    else if(mode==ND_INDICATOR_DOT)fill(dock,ND_ACCENT2,x+tw-9,y+5,4,4);
}

/* Caption and value rows of the plain graph tile. */
static void graph_tile_text(struct NDDock *dock,const struct NDItem *item,
                            int x,int y,int tw,int th)
{
    const char *caption=!strcmp(item->id,"CPU") ? "CPU" : item->caption;
    if(!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW")){
        centered_text(dock,ND_SUBTLE,"NETWORK",x,y+(th>=60 ? 24 : 17),tw);
        return;
    }
    centered_text(dock, ND_SUBTLE, caption, x, y + (th >= 60 ? 13 : 10), tw);
    centered_text(dock, item->online ? ND_TEXT : ND_SUBTLE,
                  item->value, x, y + (th >= 60 ? 28 : 23), tw);
}

/* One new sample on a plain graph tile: scroll the graph left by a pixel and
 * redraw only the rightmost columns and the text rows.  Anything else (a
 * hover, a press, a moved tile, several samples at once, the RAM layout)
 * falls back to a full draw_tile(), so the result always matches one. */
void draw_graph_step(struct NDDock *dock,int index)
{
    struct NDItem *item=&dock->conf.items[index];
    struct NDGraphView *view=&dock->graph_view[index];
    struct RastPort *rp;
    int x,y,tw,th,bg,zone,col;
    ULONG behind;
    if (!dock->window || index<0 || index>=dock->conf.count) return;
    behind=dock->graph_samples[index]-view->painted;
    if (!behind) return;
    tile_coords(dock,index,&x,&y);
    tile_dimensions(dock,index,&tw,&th);
    bg=tile_interior(dock,index);
    if (behind!=1 || !view->valid || bg!=view->bg ||
        dock->press_item==index || dock->is_drag ||
        x!=view->x || y!=view->y || tw!=view->tw || th!=view->th ||
        view->gw<8 || view->gh<3 || view->gy<=y+3) {
        draw_tile(dock,index);
        return;
    }
    ++dock->diagnostic_tile_paints;
    rp=dock->window->RPort;
    /* Text rows and the badge or running dot above the graph. */
    fill(dock,bg,x+3,y+3,tw-6,view->gy-(y+3));
    graph_tile_text(dock,item,x,y,tw,th);
    draw_plugin_running_indicator(dock,item,x,y,tw);
    /* Graph: shift left, then redraw the four rightmost columns, which
     * also removes the previous newest-point marker. */
    ScrollRaster(rp,1,0,view->gx,view->gy,
                 view->gx+view->gw-1,view->gy+view->gh-1);
    ++view->phase;
    zone=view->gx+view->gw-4;
    fill(dock,bg,zone,view->gy,4,view->gh);
    SetAPen(rp,pen(dock,ND_BAR_BG));
    for (col=zone;col<view->gx+view->gw;++col)
        if (((col-view->gx+view->phase)&3)==0)
            WritePixel(rp,col,view->gy+view->gh/2);
    graph_series(dock,index,view->gx,view->gy,view->gw,view->gh,zone);
    view->painted=dock->graph_samples[index];
}

void draw_tile(struct NDDock *dock, int index)
{
    struct NDItem *item = &dock->conf.items[index];
    int x, y, tw = dock->tile_width, th = dock->tile_height;
    int baseline1, baseline2, meter_w;
    char compact_value[32];
    const char *tile_value=item->value;
    dock->graph_view[index].valid=0;   /* set again by the plain graph layout */
    if (!dock->window || !item_visible(dock,index)) return;
    ++dock->diagnostic_tile_paints;
    tile_coords(dock, index, &x, &y);
    tile_dimensions(dock,index,&tw,&th);
    if (item->kind==ND_ITEM_SPACER) {
        int hovered=index==dock->hover;
        int engaged=dock->press_item==index ||
                    (dock->is_drag && dock->drag_to==index);
        int active=hovered || engaged;
        int color=ND_SUBTLE;
        int thickness=item->spacer_line_width;
        /* A spacer is negative space inside one continuous dock skin, not a
         * black tile.  Repaint its slice of the global backdrop first. */
        paint_background(dock,x,y,tw,th);
        if (active) {
            struct RastPort *rp=dock->window->RPort;
            /* Keep empty spacers as visual breathing room.  Hover uses a
             * quiet dotted outline; only an actual edit/drag uses Accent. */
            SetAPen(rp,pen(dock,engaged ? ND_ACCENT : ND_SUBTLE));
            SetDrPt(rp,engaged ? 0xCCCCU : 0x8888U);
            Move(rp,x+2,y+2);Draw(rp,x+tw-3,y+2);
            Draw(rp,x+tw-3,y+th-3);Draw(rp,x+2,y+th-3);
            Draw(rp,x+2,y+2);SetDrPt(rp,0xFFFFU);
        }
        if (item->spacer_line) {
            if (item->spacer_line_color==ND_SPACER_COLOR_ACCENT) color=ND_ACCENT;
            else if (item->spacer_line_color==ND_SPACER_COLOR_SECONDARY)
                color=ND_ACCENT2;
            else if (item->spacer_line_color==ND_SPACER_COLOR_TEXT) color=ND_TEXT;
            if (active) color=ND_ACCENT;
            if (thickness<1 || thickness>8) thickness=2;
            if (vertical(dock) && tw>20)
                fill(dock,color,x+10,y+(th-thickness)/2,tw-20,thickness);
            else if (!vertical(dock) && th>20)
                fill(dock,color,x+(tw-thickness)/2,y+10,thickness,th-20);
        }
        return;
    }
    tile_box(dock,index == dock->hover ? ND_HOVER : ND_TILE,x,y,tw,th);
    /* Explicit borders remain visible on two-/four-colour Workbench screens. */
    if (dock->press_item == index || (dock->is_drag && dock->drag_to == index)) {
        struct RastPort *rp = dock->window->RPort;
        SetAPen(rp, pen(dock, ND_ACCENT));
        SetDrPt(rp, dock->is_drag ? 0xCCCCU : 0xFFFFU);
        Move(rp, x+1, y+1); Draw(rp, x+tw-2, y+1);
        Draw(rp, x+tw-2, y+th-2); Draw(rp, x+1, y+th-2);
        Draw(rp, x+1, y+1); SetDrPt(rp, 0xFFFFU);
    }
    if ((item->kind == ND_ITEM_LAUNCH || item->kind == ND_ITEM_DRAWER ||
         item->kind == ND_ITEM_GROUP) &&
        dock->brushes[index] &&
        draw_brush(dock, index, x, y, tw, th)) {
        if (launcher_label_visible(dock,index))
            centered_text(dock, ND_TEXT, item->label, x, y + th - 9, tw);
        draw_volume_meter(dock, item, x, y, tw, th);
        draw_running_indicator(dock,index,x,y,tw);
        return;
    }
    if ((item->kind == ND_ITEM_LAUNCH || item->kind == ND_ITEM_DRAWER ||
         item->kind == ND_ITEM_GROUP) &&
        dock->scaled_icon_bitmaps[index] && dock->scaled_icon_masks[index]) {
        struct BitMap *art,*art_mask;
        int iw,ih,iy;
        (void)tile_icon_art(dock,index,&art,&art_mask,&iw,&ih);
        iy=nd_icon_art_top(y,th,ih,dock->conf.item_size==ND_ITEM_SIZE_TINY);
        BltMaskBitMapRastPort(art,0,0,
            dock->window->RPort,x+(tw-iw)/2,iy,iw,ih,0xE0,art_mask->Planes[0]);
        if (launcher_label_visible(dock,index))
            centered_text(dock,ND_TEXT,item->label,x,y+th-9,tw);
        draw_volume_meter(dock, item, x, y, tw, th);
        draw_running_indicator(dock,index,x,y,tw);
        return;
    }
    if ((item->kind == ND_ITEM_LAUNCH || item->kind == ND_ITEM_DRAWER ||
         item->kind == ND_ITEM_GROUP) &&
        dock->icons[index]) {
        struct Image *im = (struct Image *)dock->icons[index]->do_Gadget.GadgetRender;
        int tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
        int max_width=tw-(tiny ? 2 : 14);
        int max_height=th-(tiny ? 2 : 27);
        /* Never ask icon.library to render into the Workbench screen here.
         * Its native ColorIcon path may temporarily remap/allocate shared
         * pens on every repaint. ColorIcons are decoded into our cached
         * bitmap above; this path is only the pen-free planar fallback. */
        if (im && im->Depth >= 1 && im->Depth <= 4 &&
            im->Width > 0 && im->Height > 0 &&
            im->Width <= max_width && im->Height <= max_height) {
            int iy=nd_icon_art_top(y,th,im->Height,tiny);
            DrawImage(dock->window->RPort, im,
                      x + (tw - im->Width) / 2, iy);
            if (launcher_label_visible(dock,index))
                centered_text(dock, ND_TEXT, item->label, x, y + th - 9, tw);
            draw_volume_meter(dock, item, x, y, tw, th);
            draw_running_indicator(dock,index,x,y,tw);
            return;
        }
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"DOPUS4")) {
        draw_dopus_tile(dock,index,item,x,y,tw,th);
        return;
    }
    if(item->kind==ND_ITEM_PLUGIN && (!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW")) && strcmp(item->badge,"*")){
        draw_plugin_launcher_artwork(dock,index,"NETWORK",x,y,tw,th);return;
    }
    if(item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"BITSKIM") &&
       (!strcmp(item->value,"OFFLINE") || !strcmp(item->value,"RECEIVING OFF") || strstr(item->value," DEVICE"))){
        draw_plugin_launcher_artwork(dock,index,"BITSKIM",x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);return;
    }
    if(item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"CLIPBOARD")){
        draw_plugin_launcher_artwork(dock,index,"CLIPPY",x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);return;
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"RUNNINGAPPS")) {
        draw_running_apps_tile(dock,index,x,y,tw,th);
        return;
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"MUSIC") &&
        !item->media_capabilities) {
        draw_music_idle_tile(dock,index,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind==ND_ITEM_PLUGIN &&
        dock->conf.item_size==ND_ITEM_SIZE_TINY) {
        draw_tiny_plugin(dock,item,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind == ND_ITEM_PLUGIN && !strcmp(item->id,"EYES") &&
        item->online) {
        draw_eyes(dock,index,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind == ND_ITEM_PLUGIN && !strcmp(item->id,"CLOCK") &&
        !strcmp(nd_option_get(&dock->conf,"CLOCK","FORMAT"),"ANALOG")) {
        draw_analog_clock(dock,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"RAM")) {
        if (item->graph) draw_ram_graph_tile(dock,index,x,y,tw,th);
        else draw_ram_tile(dock,item,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"MUSIC")) {
        draw_music_tile(dock,item,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if(item->kind==ND_ITEM_PLUGIN && !item->graph &&
        (!strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW"))){
        draw_network_meters(dock,index,x,y,tw,th);
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if (item->kind == ND_ITEM_PLUGIN && item->graph &&
        strcmp(item->id,"DOPUS4") && strcmp(item->id,"ARCHIVE")) {
        graph_tile_text(dock,item,x,y,tw,th);
        draw_graph(dock, index, x, y, tw, th);
        dock->graph_view[index].valid=1;
        draw_plugin_running_indicator(dock,item,x,y,tw);
        return;
    }
    if(item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"BITSKIM") &&
       item->meter<=100U &&
       (!strncmp(item->value,"SEND ",5) || !strncmp(item->value,"RECV ",5))){
        sprintf(compact_value,"%s %u%%",item->value[0]=='S' ? "SEND":"RECV",
                (unsigned int)item->meter);
        tile_value=compact_value;
    }
    if (item->kind==ND_ITEM_PLUGIN &&
        (!strcmp(item->id,"CPU") || !strcmp(item->id,"BITSKIM"))) {
        /* CPU and RAM use the same heading row as the compact Clock, Music
         * and status tiles; the meter remains anchored at the bottom. */
        baseline1=y+(th>=60 ? 24 : 17);
        baseline2=y+(th>=60 ? 44 : 34);
    } else if (item->kind==ND_ITEM_PLUGIN && item->meter<=100U) {
        /* Meter tiles share RAM's heading/value baselines. */
        baseline1=y+(th>=60 ? 13 : 10);
        baseline2=y+(th>=60 ? 28 : 20);
    } else {
        baseline1 = y + (th >= 60 ? 24 : 17);
        baseline2 = y + (th >= 60 ? 44 : 34);
    }
    if ((item->kind!=ND_ITEM_LAUNCH && item->kind!=ND_ITEM_DRAWER &&
         item->kind!=ND_ITEM_GROUP) ||
        launcher_label_visible(dock,index)) {
        centered_text(dock, ND_SUBTLE, item->caption, x, baseline1, tw);
        centered_text(dock, item->online || item->kind == ND_ITEM_LAUNCH ||
                      item->kind == ND_ITEM_DRAWER || item->kind == ND_ITEM_GROUP ?
                      ND_TEXT : ND_SUBTLE, tile_value, x, baseline2, tw);
    }
    if (item->kind == ND_ITEM_PLUGIN && item->meter <= 100U) {
        meter_w = tw - 20;
        fill(dock, ND_BAR_BG, x + 10, y + th - 13, meter_w, 4);
        if (item->meter > 0)
            fill(dock, ND_ACCENT, x + 10, y + th - 13,
                 (int)(item->meter * (ULONG)meter_w / 100UL), 4);
    } else if (nd_item_supports_space_meter(item) && item->show_volume_meter &&
               item->meter <= 100U) {
        meter_w = tw - 20;
        fill(dock, ND_BAR_BG, x + 10, y + th - 13, meter_w, 4);
        if (item->meter > 0)
            fill(dock, ND_ACCENT, x + 10, y + th - 13,
                 (int)(item->meter * (ULONG)meter_w / 100UL), 4);
    } else if (item->kind == ND_ITEM_LAUNCH ||
               item->kind == ND_ITEM_DRAWER || item->kind == ND_ITEM_GROUP) {
        fill(dock, ND_ACCENT, x + tw / 2 - 6, y + th - 12, 12, 2);
    }
    draw_running_indicator(dock,index,x,y,tw);
    if (item->kind==ND_ITEM_PLUGIN)
        draw_plugin_running_indicator(dock,item,x,y,tw);
}
static void draw_scroll_control(struct NDDock *dock, int direction)
{
    int slot=direction<0 ? 0 : dock->layout_slots-1;
    int x,y,enabled;
    const char *arrow;
    if (!dock->overflow || slot<0) return;
    enabled=direction<0 ? dock->view_start>0 :
            dock->view_start+dock->visible_count<dock->conf.count;
    arrow=vertical(dock) ? (direction<0 ? "^^" : "vv") :
                           (direction<0 ? "<<" : ">>");
    slot_coords(dock,slot,&x,&y);
    rounded_box(dock,enabled ? ND_TILE : ND_BG,
                x,y,dock->tile_width,dock->tile_height);
    fill(dock,enabled ? ND_ACCENT : ND_SUBTLE,x+6,y+6,3,10);
    centered_text(dock,enabled ? ND_TEXT : ND_SUBTLE,arrow,x,
                  y+dock->tile_height/2+3,dock->tile_width);
    if (direction<0)
        centered_text(dock,ND_SUBTLE,"PREV",x,y+dock->tile_height-9,
                      dock->tile_width);
    else
        centered_text(dock,ND_SUBTLE,"NEXT",x,y+dock->tile_height-9,
                      dock->tile_width);
}
static void draw_insertion_marker(struct NDDock *dock)
{
    int x,y,w,h;
    if (!dock->edit_mode || !dock->is_drag ||
        !item_visible(dock,dock->drag_to)) return;
    tile_coords(dock,dock->drag_to,&x,&y);
    tile_dimensions(dock,dock->drag_to,&w,&h);
    if (vertical(dock))
        fill(dock,ND_ACCENT,x+8,dock->drag_after ? y+h : y-3,w-16,3);
    else
        fill(dock,ND_ACCENT,dock->drag_after ? x+w : x-3,y+8,3,h-16);
}
static void draw_tab(struct NDDock *dock)
{
    struct RastPort *rp = dock->window->RPort;
    int w = dock->window->Width, h = dock->window->Height;
    int cx = w / 2, cy = h / 2;
    if (w <= 1 || h <= 1) return;
    SetDrPt(rp, 0xFFFFU);
    paint_background(dock, 0, 0, w, h);
    SetAPen(rp, pen(dock, ND_ACCENT));
    Move(rp, 0, 0); Draw(rp, w-1, 0); Draw(rp, w-1, h-1);
    Draw(rp, 0, h-1); Draw(rp, 0, 0);
    if (w > h) fill(dock, ND_ACCENT, cx-11, cy-1, 22, 3);
    else fill(dock, ND_ACCENT, cx-1, cy-11, 3, 22);
}
void draw_all(struct NDDock *dock)
{
    int i,first,last;
    if (!dock->window || dock->box_pending) return;
    ++dock->diagnostic_full_paints;
    if (dock->hidden) { draw_tab(dock); return; }
    /* Intuition windows are rectangular and OS 3.x has no alpha window
     * shape.  Always paint the complete layer: leaving the short-row sides
     * unfilled exposes Intuition's white backfill rather than Workbench. */
    paint_background(dock,0,0,dock->width,dock->height);
    if (dock->theme_spec.bevel && dock->width>3 && dock->height>3) {
        struct RastPort *rp=dock->window->RPort;
        SetAPen(rp,pen(dock,ND_HOVER));
        Move(rp,0,dock->height-2);Draw(rp,0,0);Draw(rp,dock->width-2,0);
        SetAPen(rp,pen(dock,ND_SUBTLE));
        Move(rp,dock->width-1,1);Draw(rp,dock->width-1,dock->height-1);
        Draw(rp,1,dock->height-1);
    }
    if (dock->edit_mode) {
        struct RastPort *rp=dock->window->RPort;
        /* Editing must be discoverable without turning the entire dock into
         * a bright selection rectangle. */
        SetAPen(rp,pen(dock,ND_SUBTLE));
        SetDrPt(rp,0xCCCCU);
        Move(rp,0,0);Draw(rp,dock->width-1,0);
        Draw(rp,dock->width-1,dock->height-1);
        Draw(rp,0,dock->height-1);Draw(rp,0,0);
        SetDrPt(rp,0xFFFFU);
    }
    first=dock->overflow ? dock->view_start : 0;
    last=dock->overflow ? first+dock->visible_count : dock->conf.count;
    if (last>dock->conf.count) last=dock->conf.count;
    for (i=first;i<last;++i) draw_tile(dock,i);
    if (dock->overflow) {
        draw_scroll_control(dock,-1);
        draw_scroll_control(dock,1);
    }
    draw_insertion_marker(dock);
    memset(dock->dirty_tiles,0,sizeof(dock->dirty_tiles));
    memset(dock->graph_step,0,sizeof(dock->graph_step));
}
/* Hover and drag feedback normally changes only a few tiles. Repainting the
 * entire dock for each boundary crossing is conspicuous on baseline 68k
 * machines, especially when graph plugins and planar icons are visible. */
void redraw_changed_tiles(struct NDDock *dock, int a, int b, int c)
{
    if (item_visible(dock,a)) draw_tile(dock,a);
    if (item_visible(dock,b) && b != a) draw_tile(dock,b);
    if (item_visible(dock,c) && c != a && c != b)
        draw_tile(dock, c);
}
void mark_tile_dirty(struct NDDock *dock, int index)
{
    if (index >= 0 && index < ND_MAX_ITEMS)
        dock->dirty_tiles[index] = 1;
}
void refresh_volume_meters(struct NDDock *dock)
{
    int i;
    APTR requesters=quiet_requesters();
    for (i = 0; i < dock->conf.count; ++i) {
        struct NDItem *item = &dock->conf.items[i];
        UWORD next = ND_NO_METER;
        BPTR lock;
        struct InfoData info;
        if (!nd_item_supports_space_meter(item)) continue;
        if (!item->show_volume_meter) {
            if (item->meter != ND_NO_METER) {
                item->meter = ND_NO_METER;
                mark_tile_dirty(dock, i);
            }
            continue;
        }
        lock = Lock((STRPTR)item->command, ACCESS_READ);
        if (lock) {
            memset(&info, 0, sizeof(info));
            if (Info(lock, &info) && info.id_NumBlocks > 0 &&
                info.id_NumBlocksUsed >= 0)
                next = (UWORD)nd_used_percent(
                    (unsigned long)info.id_NumBlocksUsed,
                    (unsigned long)info.id_NumBlocks);
            UnLock(lock);
        }
        if (item->meter != next) {
            item->meter = next;
            mark_tile_dirty(dock, i);
        }
    }
    restore_requesters(requesters);
}
void flush_dirty_tiles(struct NDDock *dock)
{
    int i;
    /* Hidden or covered: keep the marks; the next visible flush paints. */
    if (!dock->window || dock->hidden || dock->box_pending || dock->covered)
        return;
    for (i = 0; i < dock->conf.count && i < ND_MAX_ITEMS; ++i) {
        int full=dock->dirty_tiles[i],step=dock->graph_step[i];
        dock->dirty_tiles[i]=0;
        dock->graph_step[i]=0;
        if (!item_visible(dock,i)) continue;
        if (full) draw_tile(dock,i);
        else if (step) draw_graph_step(dock,i);
    }
}
