#include "nd_trace.h"
/* NovaDock: The dock window: size, opening, hit-testing and scrolling. */
#include "dock_internal.h"
#include "nd_underlay.h"

/* Defined below, private to this file. */
static int calculate_size(struct NDDock *dock);
static int desktop_repaint_pending(struct Screen *screen,const struct NDRect *box);

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
    dock->hover_headroom=0;dock->hover_pad_x=dock->hover_pad_y=0;
    dock->hover_axis_padding=0;dock->hover_plan_ready=0;
    if(dock->conf.hover_magnification && !layout.overflow && !dock->grip_drag){
        int cross=vertical(dock) ? dock->screen->Width-layout.width-10:
            dock->screen->Height-dock->screen->BarHeight-layout.height-10;
        int along=vertical(dock) ? dock->screen->Height-dock->screen->BarHeight-layout.height-10:
            dock->screen->Width-layout.width-10;
        if(cross>=32){
            dock->hover_headroom=cross<96 ? cross:96;
            dock->hover_axis_padding=along>0 ? along/2:0;
            if(dock->hover_axis_padding>192)dock->hover_axis_padding=192;
            if(vertical(dock)){
                dock->width+=dock->hover_headroom;
                dock->height+=2*dock->hover_axis_padding;
                dock->hover_pad_x=dock->conf.side==ND_SIDE_RIGHT ? dock->hover_headroom:0;
                dock->hover_pad_y=dock->conf.position==ND_POSITION_START ? 0:
                    dock->conf.position==ND_POSITION_END ? 2*dock->hover_axis_padding:dock->hover_axis_padding;
            }else{
                dock->height+=dock->hover_headroom;
                dock->width+=2*dock->hover_axis_padding;
                dock->hover_pad_y=dock->conf.side==ND_SIDE_BOTTOM ? dock->hover_headroom:0;
                dock->hover_pad_x=dock->conf.position==ND_POSITION_START ? 0:
                    dock->conf.position==ND_POSITION_END ? 2*dock->hover_axis_padding:dock->hover_axis_padding;
            }
        }
    }
    dock->canvas_view=dock->hover_headroom && dock->conf.always_on_top && LayersBase;
    dock->viewport_pending=0;
    dock->viewport.left=dock->hover_pad_x;dock->viewport.top=dock->hover_pad_y;
    dock->viewport.width=dock->width-(vertical(dock) ? dock->hover_headroom:2*dock->hover_axis_padding);
    dock->viewport.height=dock->height-(vertical(dock) ? 2*dock->hover_axis_padding:dock->hover_headroom);
    /* A live resize must never leave a snapshot of the old size behind. */
    if(dock->hover_desktop && nd_snapshot_needs_alloc(1,dock->hover_desktop_width,
       dock->hover_desktop_height,dock->width,dock->height)){
        release_desktop_snapshot(dock);
        if(dock->window){dock->desktop_refresh_pending=2;dock->desktop_ready_ms=dock->desktop_probe_ms=0;}
    }
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
/* CUSTOM offsets describe the visible shelf, not its wider hover canvas. */
static int position_dock(struct NDDock *dock,const struct NDLayout *layout)
{
    int offset;
    if(!nd_dock_rect_padded(&dock->conf,layout,dock->screen->Width,
        dock->screen->Height,dock->screen->BarHeight,dock->hover_axis_padding,
        &dock->full_box,&offset))return 0;
    if(vertical(dock))dock->hover_pad_y=offset;else dock->hover_pad_x=offset;
    dock->viewport.left=dock->hover_pad_x;dock->viewport.top=dock->hover_pad_y;
    return nd_hidden_rect(&dock->conf,dock->screen->Width,dock->screen->Height,
        dock->screen->BarHeight,&dock->tab_box);
}
void dock_window_box(struct NDDock *dock,int hidden,struct NDRect *box)
{
    *box=hidden ? dock->tab_box:dock->full_box;
    if(dock->canvas_view && !hidden){box->left+=dock->viewport.left;box->top+=dock->viewport.top;box->width=dock->viewport.width;box->height=dock->viewport.height;}
}
void apply_dock_viewport(struct NDDock *dock)
{
    struct Layer *layer;
    int dx,dy;
    if(!LayersBase || !dock->window || !dock->canvas_view)return;
    layer=dock->window->WLayer;
    dx=-layer->Scroll_X;
    dy=-layer->Scroll_Y;
    if(!dx && !dy)return;
    ScrollLayer(0,layer,dx,dy);
    /* Intuition must resize an unscrolled layer. Logical canvas offsets
     * belong to frame presentation and hit testing, not its backing store. */
    note_pointer_epoch_now(dock);
}
/* A window can open/move under the always-on-top dock after the full
 * desktop snapshot was taken. Sample only the newly covered strips BEFORE
 * asking Intuition to enlarge the window. Shrink exposes the real lower
 * layer; sampling the old overlap would instead bake dock pixels into it. */
static void capture_view_underlay(struct NDDock *dock,const struct NDRect *view)
{
    struct NDRect old,strips[4];int n,i;
    if(!dock->hover_desktop||!dock->screen)return;
    old.left=dock->window->LeftEdge-dock->full_box.left;
    old.top=dock->window->TopEdge-dock->full_box.top;
    old.width=dock->window->Width;old.height=dock->window->Height;
    n=nd_underlay_added(view,&old,strips);if(!n)return;
    /* These transient windows belong to us, not to the application behind
     * the dock. Remove them before reading any newly covered screen area. */
    close_caption_popup(dock);close_tooltip(dock);WaitBlit();
    /* Shrinking exposes SIMPLE_REFRESH strips which can still contain layer
     * backfill on the next animation tick. Do not replace a clean underlay
     * with those pixels, and never wait inside the animation loop. Repair
     * the full snapshot once the pointer leaves and the repaint completes. */
    for(i=0;i<n;++i){
        struct NDRect physical=strips[i];
        physical.left+=dock->full_box.left;physical.top+=dock->full_box.top;
        if(desktop_repaint_pending(dock->screen,&physical)){
            dock->desktop_refresh_pending=2;
            dock->desktop_ready_ms=dock->desktop_probe_ms=0;
            return;
        }
    }
    LockLayerInfo(&dock->screen->LayerInfo);
    for(i=0;i<n;++i){
        struct NDRect *s=&strips[i];
        BltBitMap(dock->screen->RastPort.BitMap,dock->full_box.left+s->left,dock->full_box.top+s->top,
            dock->hover_desktop,s->left,s->top,s->width,s->height,0xc0,0xff,NULL);
    }
    WaitBlit();UnlockLayerInfo(&dock->screen->LayerInfo);
    /* Keep the optional RTG tint in sync without reallocating its bitmap. */
    for(i=0;i<n;++i)if(!nd_transparency_refresh(&dock->transparency,dock->hover_desktop,strips[i].left,strips[i].top,strips[i].width,strips[i].height)){prepare_transparency(dock);break;}
    dock->hover_prev_valid=0;
}
int sync_dock_viewport(struct NDDock *dock)
{
    struct NDRect box,view;int i,right,bottom;
    if(!dock->canvas_view || dock->hidden || !dock->hover_plan_ready)return 1;
    /* Never resize under a held button: the asynchronous box change would
     * delay its release and move the press target.  The frame is clipped to
     * the current viewport until the button is released. */
    if(dock->press_item>=0 || dock->grip_drag || dock->is_drag || dock->music_press_action ||
       dock->deferred_buttons.count ||
       (InputBase && (PeekQualifier()&IEQUALIFIER_LEFTBUTTON)))return 1;
    view=dock->hover_shelf_rect;right=view.left+view.width;bottom=view.top+view.height;
    for(i=0;i<dock->conf.count;++i)if(item_visible(dock,i)){
        struct NDRect *b=&dock->hover_rects[i];int extra=dock->hover_layout[i].kind==ND_HOVER_ART ? dock->window->RPort->TxHeight+5:0;
        if(b->left<view.left)view.left=b->left;if(b->top<view.top)view.top=b->top;
        if(b->left+b->width>right)right=b->left+b->width;
        if(b->top+b->height+extra>bottom)bottom=b->top+b->height+extra;
    }
    if(view.left<0)view.left=0;if(view.top<0)view.top=0;
    if(right>dock->width)right=dock->width;if(bottom>dock->height)bottom=dock->height;
    view.width=right-view.left;view.height=bottom-view.top;
    capture_view_underlay(dock,&view);
    dock->viewport=view;dock_window_box(dock,0,&box);
    if(dock->window->LeftEdge==box.left && dock->window->TopEdge==box.top &&
       dock->window->Width==box.width && dock->window->Height==box.height){apply_dock_viewport(dock);return 1;}
    dock->window_box_target=box;dock->viewport_pending=1;dock->box_pending=1;dock->box_pending_ticks=0;dock->box_target_hidden=0;
    dock->box_origin_x=dock->window->LeftEdge;dock->box_origin_y=dock->window->TopEdge;
    ChangeWindowBox(dock->window,box.left,box.top,box.width,box.height);return 0;
}
static struct Window *create_dock_window(struct NDDock *dock,const struct NDRect *box)
{
    struct Window *window;struct NDRect target=*box;
    if(box==&dock->full_box)dock_window_box(dock,0,&target);
    dock->window_box_target=target;box=&target;
    window=OpenWindowTags(NULL,
        WA_PubScreen, (ULONG)dock->screen,
        WA_Left, box->left, WA_Top, box->top,
        WA_Width, box->width, WA_Height, box->height,
        WA_MinWidth, 8, WA_MinHeight, 8,
        WA_MaxWidth, dock->screen->Width,
        WA_MaxHeight, dock->screen->Height,
        WA_Borderless, TRUE,
        /* The reserved magnification canvas contains a desktop snapshot,
         * not live transparency. Keep the normal dock in the backdrop group
         * so application windows remain above that whole canvas, including
         * after Test/reopen and explicit Show requests. */
        WA_Backdrop, !dock->conf.always_on_top,
        /* The complete buffered frame paints new magnification headroom.
         * Intuition's default backfill would flash grey during resize. */
        WA_BackFill, (ULONG)(dock->hover_headroom ? LAYERS_NOBACKFILL:LAYERS_BACKFILL),
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
    /* A new window starts a new pointer geometry; anything queued for the
     * previous window is gone with it. */
    nd_deferred_clear(&dock->deferred_buttons);dock->deferred_menu=0;
    dock->orphan_press_ticks=0;
    if(window)note_pointer_epoch_now(dock);
    return window;
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
    if (!position_dock(dock,&layout)) goto failed;
    /* Wallpaper colours sample the desktop: let it repaint first. */
    if(dock->conf.wallpaper_match && !dock->conf.theme_artwork){Delay(3);WaitBlit();}
    alloc_colors(dock);
    /* Closing the previous layer exposes refresh work owned by Workbench
     * and other visitors. Let them repaint before sampling the desktop. */
    if(dock->conf.theme_artwork || dock->hover_headroom || dock->conf.background_transparency) {Delay(3);WaitBlit();}
    prepare_dock_background(dock);
    dock->hidden=0;
    dock->window = create_dock_window(dock,&dock->full_box);
    if (!dock->window) {
        free_colors(dock);
        goto failed;
    }
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
    dock->music_press_action=dock->music_hover_action=0;
    dock->press_item = -1;
    dock->drop_preview=0;
    build_scaled_icon_bitmaps(dock);
    build_brush_bitmaps(dock);
    refresh_running_items(dock);
    draw_all(dock);
    dock->desktop_refresh_pending=dock->hover_headroom || dock->conf.theme_artwork || dock->conf.background_transparency;
    dock->desktop_ready_ms=dock->desktop_probe_ms=0;
    dock->desktop_scene_valid=dock->desktop_scene_dirty=dock->desktop_scene_age=0;
    /* Remember the windows present in this snapshot now. A quick Test/Save
     * can close Preferences before the first housekeeping tick. */
    desktop_scene_tick(dock,1);
    return 1;
failed:
    UnlockPubScreen(NULL,locked_screen);
    dock->screen=NULL;
    return 0;
}
/* Snapshot refresh must expose Workbench synchronously. HideWindow and
 * ShowWindow only queue Intuition requests: painting a SIMPLE_REFRESH
 * window before ShowWindow finishes loses that paint. Recreate just the
 * window, retaining plugin processes, artwork, fonts and public ports. */
/* SIMPLE_REFRESH owners repaint exposed desktop pixels asynchronously.
 * Copying before EndRefresh would cache the dock's own former icons. */
static int desktop_repaint_pending(struct Screen *screen,const struct NDRect *box)
{
    struct Window *w;ULONG key=LockIBase(0);int pending=0;
    for(w=screen->FirstWindow;w;w=w->NextWindow){
        if(!w->WLayer || !(w->WLayer->Flags&(LAYERREFRESH|LAYERUPDATING)))continue;
        if(w->LeftEdge<box->left+box->width && w->LeftEdge+w->Width>box->left &&
           w->TopEdge<box->top+box->height && w->TopEdge+w->Height>box->top){pending=1;break;}
    }
    UnlockIBase(key);return pending;
}
static int recapture_desktop(struct NDDock *dock)
{
    struct Window *window=dock->window;
    struct Screen *locked;
    struct NDRect box;
    int registered=dock->app_window!=NULL,result=0,ready,i;
    ND_TRACE_START(capture_start);
    if(!window || !dock->screen)return 0;
    locked=LockPubScreen("Workbench");
    if(!locked)return 0;
    if(locked!=dock->screen){UnlockPubScreen(NULL,locked);return 0;}
    if(!ensure_desktop_snapshot(dock,locked->RastPort.BitMap)){UnlockPubScreen(NULL,locked);return 0;}
    box.left=window->LeftEdge;box.top=window->TopEdge;
    box.width=window->Width;box.height=window->Height;
    close_caption_popup(dock);
    close_tooltip(dock);
    detach_appwindow(dock);CloseWindow(window);dock->window=NULL;
    /* Wait for actual refresh completion, rather than assuming a desktop
     * picture can repaint in three ticks. Keep the last clean snapshot on
     * timeout and let housekeeping retry; never bake old dock pixels in. */
    Delay(3);
    for(i=0;i<25 && desktop_repaint_pending(locked,&dock->full_box);++i)Delay(1);
    ready=!desktop_repaint_pending(locked,&dock->full_box);
    if(ready){
        WaitBlit();
        BltBitMap(locked->RastPort.BitMap,dock->full_box.left,dock->full_box.top,
                  dock->hover_desktop,0,0,dock->width,dock->height,0xc0,0xff,NULL);
        WaitBlit();
    }
    dock->window=create_dock_window(dock,&box);
    if(dock->window){
        apply_dock_font(dock);
        if(registered)attach_appwindow(dock);
        result=ready;
    }else{
        /* The regular screen-reopen path retries after allocation fails. */
        release_icons(dock);release_dock_font(dock);free_colors(dock);
        dock->screen=NULL;dock->screen_closed=1;
    }
    UnlockPubScreen(NULL,locked);
    if(dock->window){prepare_transparency(dock);dock->hover_shelf_valid=0;draw_all(dock);}
    ND_TRACE_END(NDT_SNAPSHOT,capture_start);
    return result;
}
int refresh_dock_desktop(struct NDDock *dock)
{
    return recapture_desktop(dock);
}
static int desktop_region_clear(struct NDDock *dock)
{
    struct Window *window;const struct NDRect *box=&dock->full_box;
    ULONG lock=LockIBase(0);int clear=1;
    for(window=dock->screen->FirstWindow;window;window=window->NextWindow){
        if(window==dock->window || (window->Flags&WFLG_BACKDROP))continue;
        if(window->LeftEdge<box->left+box->width &&
           window->LeftEdge+window->Width>box->left &&
           window->TopEdge<box->top+box->height &&
           window->TopEdge+window->Height>box->top){clear=0;break;}
    }
    UnlockIBase(lock);return clear;
}
/* Desktop snapshots must follow window lifetime changes, not just startup.
 * Hash nearby foreign windows, including backdrop peers: the temporary
 * startup console can disappear after the first settling capture. Ignoring
 * its backdrop flag would leave its grey pixels cached indefinitely.
 * Stable scenes do no graphics work. */
void desktop_scene_tick(struct NDDock *dock,int milliseconds)
{
    struct Window *w;ULONG stamp=2166136261UL,lock;const struct NDRect *b=&dock->full_box;
    if(!dock->window || !dock->screen || !dock->hover_desktop || milliseconds<=0 || dock->screen_closed)return;
    if(dock->desktop_scene_age<3000)dock->desktop_scene_age+=milliseconds;
    lock=LockIBase(0);
    for(w=dock->screen->FirstWindow;w;w=w->NextWindow){
        if(w==dock->window || w==dock->tooltip || w==dock->caption_popup ||
           w==dock->popup || w==dock->calendar || w==dock->folder_popup || w==dock->about || w==dock->feedback)continue;
        if(w->LeftEdge>=b->left+b->width || w->LeftEdge+w->Width<=b->left ||
           w->TopEdge>=b->top+b->height || w->TopEdge+w->Height<=b->top)continue;
        stamp=(stamp^(ULONG)w)*16777619UL;
        stamp=(stamp^(ULONG)w->LeftEdge)*16777619UL;stamp=(stamp^(ULONG)w->TopEdge)*16777619UL;
        stamp=(stamp^(ULONG)w->Width)*16777619UL;stamp=(stamp^(ULONG)w->Height)*16777619UL;
    }
    UnlockIBase(lock);
    if(dock->desktop_scene_valid && stamp!=dock->desktop_scene_stamp)dock->desktop_scene_dirty=1;
    dock->desktop_scene_valid=1;dock->desktop_scene_stamp=stamp;
    if(dock->desktop_scene_dirty && dock->desktop_scene_age>=3000 &&
       !(InputBase && (PeekQualifier()&IEQUALIFIER_LEFTBUTTON))){
        if(dock->desktop_refresh_pending!=2)dock->desktop_probe_ms=dock->desktop_ready_ms=0;
        dock->desktop_refresh_pending=2;
    }
}
/* Leave an active foreign string gadget undisturbed while repairing a
 * snapshot. A window merely subscribing to keys is not an active text edit. */
static int desktop_input_busy(struct NDDock *dock)
{
    struct Window *active;
    struct Gadget *g;
    ULONG key=LockIBase(0);
    int busy=0;
    active=IntuitionBase->ActiveWindow;
    if(active && active!=dock->window)
        for(g=active->FirstGadget;g;g=g->NextGadget)
            if((g->GadgetType&GTYP_GTYPEMASK)==GTYP_STRGADGET &&
               (g->Flags&GFLG_SELECTED)){busy=1;break;}
    UnlockIBase(key);
    return busy;
}
void desktop_startup_tick(struct NDDock *dock,int milliseconds)
{
    int ready,elapsed,runtime;
    if(!dock->desktop_refresh_pending || milliseconds<=0)return;
    if(!dock->window || !dock->screen || dock->screen_closed)return;
    if(!dock->hover_headroom && !dock->conf.theme_artwork && !dock->conf.background_transparency){
        dock->desktop_refresh_pending=0;return;
    }
    runtime=dock->desktop_refresh_pending==2;
    dock->desktop_probe_ms+=milliseconds;
    if(dock->desktop_probe_ms<(dock->desktop_refresh_pending==2 ? 200:1000))return;
    elapsed=dock->desktop_probe_ms;dock->desktop_probe_ms=0;
    Forbid();ready=FindPort((STRPTR)"WORKBENCH")!=NULL;Permit();
    if(!ready || (InputBase && (PeekQualifier()&IEQUALIFIER_LEFTBUTTON)) || dock->covered || dock->box_pending || dock->press_item>=0 ||
       dock->is_drag || dock->drop_preview || dock->popup || dock->calendar || dock->folder_popup ||
       dock->about || dock->feedback || dock->icons_pending ||
       (runtime && dock->keyboard) || desktop_input_busy(dock) ||
       (!runtime && (dock->tooltip || IntuitionBase->ActiveWindow==dock->window)) ||
       /* Recapturing closes the dock window for several frames.  With the
        * pointer over it, a click in that gap would reach the Workbench icon
        * or window underneath, so a changed scene waits until it leaves. */
       (dock->screen->MouseX>=dock->window->LeftEdge &&
        dock->screen->MouseX<dock->window->LeftEdge+dock->window->Width &&
        dock->screen->MouseY>=dock->window->TopEdge &&
        dock->screen->MouseY<dock->window->TopEdge+dock->window->Height) ||
       (dock->desktop_refresh_pending!=2 && !dock->desktop_scene_valid && !desktop_region_clear(dock))){
        dock->desktop_ready_ms=0;return;
    }
    dock->desktop_ready_ms+=elapsed;
    if(dock->desktop_ready_ms<(dock->desktop_refresh_pending==2 ? 200:3000))return;
    /* A changed scene refreshes as soon as the pointer leaves the dock
     * window (a tooltip no longer blocks a runtime refresh), never while the
     * pointer is over it.  Modal/drag/keyboard guards above still apply. */
    /* Retry a transient allocation/hide failure at a low rate. */
    dock->desktop_ready_ms=0;
    if(refresh_dock_desktop(dock)){dock->desktop_refresh_pending=0;dock->desktop_scene_dirty=0;}
}
/* Keep the Intuition window and its AppWindow registration alive. Geometry
 * changes are acknowledged asynchronously, just like auto-hide transitions. */
static void resize_live_window(struct NDDock *dock,int dragging)
{
    struct NDLayout layout;
    const struct NDRect *target;struct NDRect target_box;
    if (!calculate_size(dock)) return; /* preflight already fitted this config */
    layout.width=dock->width;layout.height=dock->height;
    layout.tile_width=dock->tile_width;layout.tile_height=dock->tile_height;
    layout.gap=dock->gap;
    if(!position_dock(dock,&layout))return;
    dock->hover=-1;dock->hover_ticks=0;dock->tip_item=-1;
    dock->music_press_action=dock->music_hover_action=0;
    dock->press_item=-1;dock->drag_to=-1;dock->is_drag=0;
    if(!dragging){dock->grip_drag=0;dock->grip_moved=0;
        nd_deferred_clear(&dock->deferred_buttons);dock->deferred_menu=0;}
    dock->drop_preview=0;
    dock->idle_ticks=0;
    prepare_dock_background(dock);
    build_scaled_icon_bitmaps(dock);
    remap_brush_colors(dock);
    build_brush_bitmaps(dock);
    refresh_running_items(dock);
    dock_window_box(dock,dock->hidden,&target_box);target=&target_box;
    if (dock->window->LeftEdge!=target->left ||
        dock->window->TopEdge!=target->top ||
        dock->window->Width!=target->width || dock->window->Height!=target->height) {
        dock->window_box_target=*target;dock->viewport_pending=0;
        dock->box_pending=1;dock->box_pending_ticks=0;dock->box_target_hidden=dock->hidden;
        dock->box_origin_x=dock->window->LeftEdge;dock->box_origin_y=dock->window->TopEdge;
        ChangeWindowBox(dock->window,target->left,target->top,
                        target->width,target->height);
    } else {dock->box_pending=0;refresh_drop_zones(dock);draw_all(dock);}
}
void update_live_window(struct NDDock *dock)
{
    dock->grip_drag=0;dock->grip_moved=0;
    resize_live_window(dock,0);
}
void update_grip_window(struct NDDock *dock)
{
    resize_live_window(dock,1);
}
/* Only the first motion rebuilds the small idle frame. Subsequent motions
 * move its existing Intuition layer; no config reload or allocation per pixel. */
void move_dock_grip(struct NDDock *dock,int coordinate)
{
    struct NDLayout layout;struct NDRect box;
    int length=vertical(dock) ? dock->screen->Height-dock->screen->BarHeight:dock->screen->Width;
    int delta=coordinate-dock->grip_start;
    int offset=nd_drag_offset(dock->grip_origin,delta,length,dock->grip_length);
    if(!dock->grip_moved && !delta)return;
    if(dock->grip_moved && dock->conf.position_offset==offset)return;
    dock->conf.position=ND_POSITION_CUSTOM;dock->conf.position_offset=offset;
    if(!dock->grip_moved){
        dock->grip_moved=1;close_caption_popup(dock);update_grip_window(dock);return;
    }
    layout.width=dock->width;layout.height=dock->height;
    if(!position_dock(dock,&layout))return;
    dock_window_box(dock,0,&box);
    if(dock->window->LeftEdge==box.left && dock->window->TopEdge==box.top)return;
    dock->window_box_target=box;dock->viewport_pending=0;
    dock->box_pending=1;dock->box_pending_ticks=0;dock->box_target_hidden=0;
    dock->box_origin_x=dock->window->LeftEdge;dock->box_origin_y=dock->window->TopEdge;
    ChangeWindowBox(dock->window,box.left,box.top,box.width,box.height);
}
void cancel_dock_grip(struct NDDock *dock)
{
    int moved=dock->grip_moved;
    if(!dock->grip_drag && !moved)return;
    dock->grip_drag=0;dock->grip_moved=0;
    dock->conf.position=dock->grip_position;dock->conf.position_offset=dock->grip_offset;
    if(moved)update_live_window(dock);
}
int hovered_tile(struct NDDock *dock, int mx, int my)
{
    int i, x, y,w,h,first,last;
    /* The idle pointer normally sits outside the window. Multi-row tile
     * coordinates walk the layout, so reject that common case once. */
    if (!dock || !dock->window || mx<0 || my<0 ||
        mx>=dock_width(dock) || my>=dock_height(dock)) return -1;
    if(hovered_grip(dock,mx,my))return -1;
    if(dock->hover_headroom && dock->hover_plan_ready && dock->hover>=0){
        int pass;
        for(pass=0;pass<2;++pass)for(i=0;i<dock->conf.count;++i){
            const struct NDRect *b=&dock->hover_rects[i];
            int extra=dock->hover_layout[i].kind==ND_HOVER_ART && dock->conf.item_size!=ND_ITEM_SIZE_TINY ? dock->window->RPort->TxHeight+3:0;
            if((i==dock->hover)!=(pass==0))continue;
            if(mx>=b->left && mx<b->left+b->width && my>=b->top && my<b->top+b->height+extra)return i;
        }
    }
    if(dock->hover_headroom){
        if(vertical(dock)){
            if(mx<dock->hover_pad_x+ND_BORDER)mx=dock->hover_pad_x+ND_BORDER;
            if(mx>=dock->hover_pad_x+dock_width(dock)-dock->hover_headroom-ND_BORDER)
                mx=dock->hover_pad_x+dock_width(dock)-dock->hover_headroom-ND_BORDER-1;
        }else{
            if(my<dock->hover_pad_y+ND_BORDER)my=dock->hover_pad_y+ND_BORDER;
            if(my>=dock->hover_pad_y+dock_height(dock)-dock->hover_headroom-ND_BORDER)
                my=dock->hover_pad_y+dock_height(dock)-dock->hover_headroom-ND_BORDER-1;
        }
    }
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
/* Pointer hover bridges the whole space between adjacent tiles. Click/drop
 * hit testing stays exact; explicit spacers remain empty.
 * Reflow is never allowed to change the target of a stationary pointer. */
int pointer_hover_target(struct NDDock *dock,int mx,int my)
{
    int i,best=-1,distance=32767,margin=dock->gap/2+3;
    int ax=mx+dock_left(dock),ay=my+dock_top(dock);
    if(hovered_grip(dock,mx,my))return -1;
    if(!dock->hover_headroom || !dock->hover_plan_ready || dock->is_drag)
        return hovered_tile(dock,mx,my);
    if(mx<dock_view_x(dock) || my<dock_view_y(dock) ||
       mx>=dock_view_x(dock)+dock->window->Width || my>=dock_view_y(dock)+dock->window->Height){
        dock->hover_pick_valid=0;return -1;
    }
    if(dock->hover>=0 && dock->hover_pick_valid && dock->hover==dock->hover_pick_index &&
       ax==dock->hover_pick_x && ay==dock->hover_pick_y)return dock->hover;
    if(margin>8)margin=8;
    for(i=0;i<dock->conf.count;++i){
        const struct NDRect *b=&dock->hover_rects[i];int dx=0,dy=0,extra=0;
        if(!item_visible(dock,i) || dock->conf.items[i].kind==ND_ITEM_SPACER)continue;
        if(dock->hover_layout[i].kind==ND_HOVER_ART && dock->conf.item_size!=ND_ITEM_SIZE_TINY)
            extra=24;
        if(mx<b->left)dx=b->left-mx;else if(mx>=b->left+b->width)dx=mx-(b->left+b->width-1);
        if(my<b->top)dy=b->top-my;else if(my>=b->top+b->height+extra)dy=my-(b->top+b->height+extra-1);
        if(i==dock->hover && dx<=3 && dy<=3){best=i;break;}
        if(dx<=margin && dy<=margin && dx+dy<distance){distance=dx+dy;best=i;}
    }
    /* Artwork can be much narrower than its tile (and MUSIC can span two
     * cells). A fixed pixel margin leaves a dead strip between those icons.
     * Join adjacent artwork centres inside their shared row envelope, so the
     * wave continues following the pointer throughout that space. */
    if(best<0){
        int j,v=vertical(dock);
        for(i=0;i<dock->conf.count-1;++i){
            const struct NDRect *a,*b;
            int start,end,low,high,position,cross,mid,candidate;
            if(!item_visible(dock,i) || dock->conf.items[i].kind==ND_ITEM_SPACER)continue;
            j=i+1;while(j<dock->conf.count && !item_visible(dock,j))++j;
            if(j>=dock->conf.count || dock->conf.items[j].kind==ND_ITEM_SPACER ||
               (!v && dock->hover_layout[i].row!=dock->hover_layout[j].row))continue;
            a=&dock->hover_rects[i];b=&dock->hover_rects[j];
            if(v){
                start=a->top+a->height/2;end=b->top+b->height/2;
                low=a->left<b->left ? a->left:b->left;
                high=a->left+a->width>b->left+b->width ? a->left+a->width:b->left+b->width;
                position=my;cross=mx;
            }else{
                start=a->left+a->width/2;end=b->left+b->width/2;
                low=a->top<b->top ? a->top:b->top;
                high=a->top+a->height>b->top+b->height ? a->top+a->height:b->top+b->height;
                if(dock->conf.item_size!=ND_ITEM_SIZE_TINY)high+=24;
                position=mx;cross=my;
            }
            if(position<start || position>end || cross<low-margin || cross>=high+margin)continue;
            mid=start+(end-start)/2;
            candidate=position<=mid ? i:j;
            if((dock->hover==i || dock->hover==j) && position>=mid-2 && position<=mid+2)candidate=dock->hover;
            best=candidate;break;
        }
    }
    if(dock->hover<0 && best<0)best=hovered_tile(dock,mx,my);
    dock->hover_pick_valid=1;dock->hover_pick_x=ax;dock->hover_pick_y=ay;dock->hover_pick_index=best;
    return best;
}
int hovered_dock_row(struct NDDock *dock,int mx,int my)
{
    int relative,row,step;
    if (!dock || !dock->window || vertical(dock) || dock->overflow ||
        dock->layout_rows<1 || mx<0 || my<0 ||
        mx>=dock_width(dock) || my>=dock_height(dock)) return -1;
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
    refresh_drop_zones(dock);
    /* Off-page meters and indicators are refreshed as they enter view. */
    refresh_volume_meters(dock);
    refresh_running_items(dock);
    draw_all(dock);
}
