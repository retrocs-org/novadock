/* NovaDock: The dock window: size, opening, hit-testing and scrolling. */
#include "dock_internal.h"

/* Defined below, private to this file. */
static int calculate_size(struct NDDock *dock);

/* The portable layout helper keeps all tiles inside small Workbench screens. */
static int calculate_size(struct NDDock *dock)
{
    struct NDLayout layout;
    if (!nd_layout_fit(&dock->conf, dock->screen->Width,
                       dock->screen->Height, dock->screen->BarHeight,
                       &layout)) return 0;
    dock->tile_width = layout.tile_width;
    dock->tile_height = layout.tile_height;
    dock->gap = layout.gap;
    dock->width = layout.width;
    dock->height = layout.height;
    dock->visible_count=layout.visible_count;
    dock->layout_slots=layout.slots;
    dock->layout_rows=layout.rows;
    dock->layout_columns=layout.columns;
    dock->overflow=layout.overflow;
    if (!dock->overflow) dock->view_start=0;
    else {
        int maximum=dock->conf.count-dock->visible_count;
        if (dock->view_start<0) dock->view_start=0;
        if (dock->view_start>maximum) dock->view_start=maximum;
    }
    return 1;
}
int open_window(struct NDDock *dock)
{
    struct NDLayout layout;
    struct Screen *locked_screen;
    dock->screen = LockPubScreen("Workbench");
    if (!dock->screen) return 0;
    locked_screen=dock->screen;
    if (!calculate_size(dock)) goto failed;
    layout.tile_width = dock->tile_width;
    layout.tile_height = dock->tile_height;
    layout.gap = dock->gap;
    layout.width = dock->width;
    layout.height = dock->height;
    if (!nd_dock_rect(&dock->conf, &layout, dock->screen->Width,
                      dock->screen->Height, dock->screen->BarHeight,
                      &dock->full_box) ||
        !nd_hidden_rect(&dock->conf, dock->screen->Width,
                        dock->screen->Height, dock->screen->BarHeight,
                        &dock->tab_box)) goto failed;
    alloc_colors(dock);
    dock->window = OpenWindowTags(NULL,
        WA_PubScreen, (ULONG)dock->screen,
        WA_Left, dock->full_box.left, WA_Top, dock->full_box.top,
        WA_Width, dock->width, WA_Height, dock->height,
        WA_MinWidth, 8, WA_MinHeight, 8,
        WA_MaxWidth, dock->screen->Width,
        WA_MaxHeight, dock->screen->Height,
        WA_Borderless, TRUE,
        /* A desktop dock must not become the active Workbench window merely
         * because it was started.  Besides stealing focus, DOpus 4 uses the
         * active Workbench window while constructing its clone screen; a
         * borderless custom-drawn dock is not a valid template for that. */
        WA_Activate, FALSE,
        WA_RMBTrap, TRUE,
        /* timer.device performs the low-rate housekeeping after startup.
         * IntuiTicks are retained only as the fallback when that optional
         * device cannot be opened, avoiding two periodic wake-up streams. */
        WA_IDCMP, ND_DOCK_IDCMP_BASE |
                  (dock->timer_open ? 0 : IDCMP_INTUITICKS),
        /* Low memory remains the safe default: a full-width SMART_REFRESH
         * window keeps a backing bitmap that may be expensive on planar and
         * RTG screens. Smooth is an explicit opt-in for systems with enough
         * display memory; reload reopens the window with the selected mode. */
        WA_Flags, (dock->conf.refresh_mode==ND_REFRESH_SMOOTH ?
                   WFLG_SMART_REFRESH : WFLG_SIMPLE_REFRESH) |
                  WFLG_REPORTMOUSE,
        TAG_DONE);
    if (!dock->window) goto failed;
    /* The open window itself keeps Workbench alive. Retaining the public-
     * screen lock for NovaDock's whole lifetime needlessly interferes with
     * other programs opening or cloning screens. */
    UnlockPubScreen(NULL,locked_screen);
    dock->screen=dock->window->WScreen;
    apply_dock_font(dock);
    dock->hidden = 0;
    dock->box_pending = 0;
    dock->idle_ticks = 0;
    dock->hover = -1;
    dock->hover_ticks = 0;
    dock->tip_item = -1;
    dock->press_item = -1;
    build_scaled_icon_bitmaps(dock);
    build_brush_bitmaps(dock);
    refresh_running_items(dock);
    draw_all(dock);
    return 1;
failed:
    UnlockPubScreen(NULL,locked_screen);
    dock->screen=NULL;
    return 0;
}
/* Keep the Intuition window and its AppWindow registration alive. Geometry
 * changes are acknowledged asynchronously, just like auto-hide transitions. */
void update_live_window(struct NDDock *dock)
{
    struct NDLayout layout;
    const struct NDRect *target;
    if (!calculate_size(dock)) return; /* preflight already fitted this config */
    layout.width=dock->width;layout.height=dock->height;
    layout.tile_width=dock->tile_width;layout.tile_height=dock->tile_height;
    layout.gap=dock->gap;
    (void)nd_dock_rect(&dock->conf,&layout,dock->screen->Width,
                      dock->screen->Height,dock->screen->BarHeight,&dock->full_box);
    (void)nd_hidden_rect(&dock->conf,dock->screen->Width,
                        dock->screen->Height,dock->screen->BarHeight,&dock->tab_box);
    dock->hover=-1;dock->hover_ticks=0;dock->tip_item=-1;
    dock->press_item=-1;dock->drag_to=-1;dock->is_drag=0;
    dock->idle_ticks=0;
    build_scaled_icon_bitmaps(dock);
    remap_brush_colors(dock);
    build_brush_bitmaps(dock);
    refresh_running_items(dock);
    target=dock->hidden ? &dock->tab_box : &dock->full_box;
    if (dock->window->LeftEdge!=target->left ||
        dock->window->TopEdge!=target->top ||
        dock->window->Width!=target->width || dock->window->Height!=target->height) {
        dock->box_pending=1;dock->box_target_hidden=dock->hidden;
        ChangeWindowBox(dock->window,target->left,target->top,
                        target->width,target->height);
    } else {dock->box_pending=0;draw_all(dock);}
}
int hovered_tile(struct NDDock *dock, int mx, int my)
{
    int i, x, y,w,h,first,last;
    first=dock->overflow ? dock->view_start : 0;
    last=dock->overflow ? first+dock->visible_count : dock->conf.count;
    if (last>dock->conf.count) last=dock->conf.count;
    for (i=first;i<last;++i) {
        tile_coords(dock, i, &x, &y);
        tile_dimensions(dock,i,&w,&h);
        if (mx >= x && my >= y && mx < x + w && my < y + h) return i;
    }
    return -1;
}
int hovered_dock_row(struct NDDock *dock,int mx,int my)
{
    int relative,row,step;
    if (!dock || !dock->window || vertical(dock) || dock->overflow ||
        dock->layout_rows<1 || mx<0 || my<0 ||
        mx>=dock->window->Width || my>=dock->window->Height) return -1;
    relative=my-ND_BORDER;
    step=dock->tile_height+dock->gap;
    if (relative<0 || step<1) return -1;
    row=relative/step;
    if (row<0 || row>=dock->layout_rows ||
        relative-row*step>=dock->tile_height) return -1;
    return row;
}
int hovered_scroll(struct NDDock *dock,int mx,int my)
{
    int x,y;
    if (!dock->overflow) return 0;
    slot_coords(dock,0,&x,&y);
    if (mx>=x && my>=y && mx<x+dock->tile_width &&
        my<y+dock->tile_height) return -1;
    slot_coords(dock,dock->layout_slots-1,&x,&y);
    if (mx>=x && my>=y && mx<x+dock->tile_width &&
        my<y+dock->tile_height) return 1;
    return 0;
}
void scroll_view(struct NDDock *dock,int direction)
{
    int maximum,step;
    if (!dock->overflow || !direction) return;
    maximum=dock->conf.count-dock->visible_count;
    step=dock->visible_count>1 ? dock->visible_count-1 : 1;
    dock->view_start+=direction*step;
    if (dock->view_start<0) dock->view_start=0;
    if (dock->view_start>maximum) dock->view_start=maximum;
    dock->hover=-1;dock->hover_ticks=0;
    /* Running indicators are only kept current for visible launchers. */
    refresh_running_items(dock);
    draw_all(dock);
}
