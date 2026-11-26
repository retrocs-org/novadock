#include "nd_aminotis.h"
/* NovaDock: Timer-driven work: auto-hide, hover, tooltips, feedback, media and task scans. */
#include "nd_trace.h"
#include "dock_internal.h"
#include "nd_render_policy.h"
#include "nd_volume.h"
#include "nd_whdload_status.h"
#include "nd_weather_codes.h"
#include "nd_cpu_info.h"

/* Defined below, private to this file. */
static void arm_timer(struct NDDock *dock);
static void auto_hide_tick(struct NDDock *dock,int quantum);
static void eyes_timer_tick(struct NDDock *dock,int quantum);
static const struct NDMediaVolume *media_missing( const struct NDMediaVolume *list,int count, const struct NDMediaVolume *other,int other_count);
static void media_rescan(struct NDDock *dock);
static void show_tooltip(struct NDDock *dock, int index);
static void show_tooltip_window(struct NDDock *dock, int index);
static void copy_bstr(char *out,int size,BSTR bstr);
static int dock_covered(const struct NDDock *dock);

/* True only when another screen certainly hides the whole dock: it is the
 * front screen, has not been dragged down, shares the dock's monitor (an
 * RTG screen and a native one may be on two displays) and reaches past the
 * dock's bottom edge in the same coordinates.  Any doubt counts as visible,
 * so a dock that can be seen is never left unpainted. */
static int dock_covered(const struct NDDock *dock)
{
    struct Screen *front;
    ULONG a,b;
    if (!dock->screen || !dock->window || dock->screen_closed) return 0;
    front=IntuitionBase->FirstScreen;
    if (!front || front==dock->screen || front->TopEdge>0) return 0;
    a=GetVPModeID(&front->ViewPort);
    b=GetVPModeID(&dock->screen->ViewPort);
    if (a==INVALID_ID || b==INVALID_ID ||
        (a&MONITOR_ID_MASK)!=(b&MONITOR_ID_MASK)) return 0;
    return front->TopEdge+front->Height >=
           dock->screen->TopEdge+dock_top(dock)+dock_height(dock);
}

/* Reload from ENV:/ENVARC:/PROGDIR: transactionally. The running dock
 * preserves plugin state/history by ID, and launches only newly added IDs.
 * A removed plugin receives QUIT at its next update, never a blind kill.
 * Inability to open the new window rolls back the previous configuration.
 */
/* The tooltip is an unactivated separate Intuition window: zero bitmap
 * allocation and no timer.device dependency. It appears only after the
 * pointer has stayed on one tile for several IntuiTicks. */
/* An independent timer.device request is necessary because IntuiTicks
 * stop arriving when another Workbench window is active. It is armed only
 * once at a time, and shutdown aborts and drains an outstanding request. */
static void arm_timer(struct NDDock *dock)
{
    ULONG interval;
    int fast=0,index;
    if (!dock->timer_open || dock->timer_pending) return;
    /* Five checks per second are useful only while the pointer can expose,
     * hide or interact with the dock, or while Eyes is animating.  A fully
     * idle visible dock otherwise needs just one housekeeping wake per
     * second.  The timer consumer advances counters by timer_quanta, so
     * watchdog, media, task and persistence deadlines retain their original
     * wall-clock intervals. */
    if (dock->window && dock->screen && !dock->screen_closed) {
        int mx=dock->screen->MouseX;
        int my=dock->screen->MouseY;
        int pointer_inside=mx>=dock->full_box.left &&
            mx<dock->full_box.left+dock->full_box.width &&
            my>=dock->full_box.top &&
            my<dock->full_box.top+dock->full_box.height;
        /* Auto-hide counts idle time in quanta, so 1 Hz is enough until the
         * last second before the dock hides. */
        fast=pointer_inside || dock->hidden ||
             nd_autohide_needs_fast_timer(dock->conf.auto_hide,dock->idle_ticks,
                                          dock->conf.hide_delay,(int)ND_TIMER_IDLE_QUANTA) ||
             dock->box_pending || dock->activate_popup_pending ||
             dock->press_item>=0 || dock->grip_drag || dock->is_drag || dock->popup ||
             dock->calendar || dock->folder_popup || dock->tooltip ||
             dock->icons_pending || dock->rexx_apply_pending;
        index=eyes_index(dock);
        if (index>=0 && !dock->hidden && !dock->covered &&
            dock->conf.items[index].online)
            fast=1;
    }
    dock->timer_quanta=fast ? 1 : ND_TIMER_IDLE_QUANTA;
    interval=ND_TIMER_MICROSECONDS*(ULONG)dock->timer_quanta;
    /* Keep animation CPU cost bounded; fast systems retain a 40 ms wait.
     * process_timer() carries actual elapsed time for housekeeping. */
    dock->timer_anim=dock->hover_animating && dock->hover_headroom && dock->window &&
                     !dock->hidden && !dock->box_pending;
    if (dock->timer_anim) {
        dock->timer_quanta=1;dock->timer_frame_ms=nd_frame_interval(dock->frame_average_us);
        interval=(ULONG)dock->timer_frame_ms*1000UL;
    }
    dock->timer_req->tr_node.io_Command = TR_ADDREQUEST;
    dock->timer_req->tr_time.tv_secs = interval/1000000UL;
    dock->timer_req->tr_time.tv_micro = interval%1000000UL;
    SendIO((struct IORequest *)dock->timer_req);
    dock->timer_pending = 1;
}
/* A magnification transition has started: switch the timer to animation
 * frames now instead of after the pending (up to one second) wait.  The
 * interrupted wait's housekeeping simply happens a little earlier. */
void kick_animation_timer(struct NDDock *dock)
{
    if (!dock->timer_open || dock->timer_anim) return;
    if (dock->timer_pending) {
        AbortIO((struct IORequest *)dock->timer_req);
        WaitIO((struct IORequest *)dock->timer_req);
        dock->timer_pending=0;
    }
    arm_timer(dock);
}
void stop_timer(struct NDDock *dock)
{
    dock->timer_clock.valid=0;dock->timer_clock.carry=0;
    if (dock->timer_pending) {
        AbortIO((struct IORequest *)dock->timer_req);
        WaitIO((struct IORequest *)dock->timer_req);
        dock->timer_pending = 0;
    }
    if (dock->timer_open) {
        CloseDevice((struct IORequest *)dock->timer_req);
        dock->timer_open = 0;
    }
    if (dock->timer_req) {
        DeleteIORequest((struct IORequest *)dock->timer_req);
        dock->timer_req = NULL;
    }
    if (dock->timer_port) {
        DeleteMsgPort(dock->timer_port);
        dock->timer_port = NULL;
    }
}

/* Reading the live mouse qualifier lets an inactive dock distinguish the
 * right button that activated it. Intuition may consume that first click,
 * so IDCMP alone is not sufficient. */
int start_input_qualifier(struct NDDock *dock)
{
    dock->input_port=CreateMsgPort();
    if (!dock->input_port) return 0;
    dock->input_req=(struct IOStdReq *)CreateIORequest(
        dock->input_port,sizeof(struct IOStdReq));
    if (!dock->input_req) return 0;
    if (OpenDevice("input.device",0,(struct IORequest *)dock->input_req,0)!=0)
        return 0;
    dock->input_open=1;
    InputBase=(struct Device *)dock->input_req->io_Device;
    return 1;
}
void stop_input_qualifier(struct NDDock *dock)
{
    if (dock->input_open && dock->input_req) {
        CloseDevice((struct IORequest *)dock->input_req);
        dock->input_open=0;
    }
    InputBase=NULL;
    if (dock->input_req) {
        DeleteIORequest((struct IORequest *)dock->input_req);
        dock->input_req=NULL;
    }
    if (dock->input_port) {
        DeleteMsgPort(dock->input_port);
        dock->input_port=NULL;
    }
}
int start_timer(struct NDDock *dock)
{
    dock->timer_port = CreateMsgPort();
    if (!dock->timer_port) return 0;
    dock->timer_req = (struct timerequest *)CreateIORequest(
        dock->timer_port, sizeof(struct timerequest));
    if (!dock->timer_req) { stop_timer(dock); return 0; }
    if (OpenDevice(TIMERNAME, UNIT_VBLANK,
                   (struct IORequest *)dock->timer_req, 0) != 0) {
        stop_timer(dock); return 0;
    }
    dock->timer_open = 1;
    arm_timer(dock);
    /* The window initially asks for IntuiTicks so it remains usable if the
     * timer cannot be opened.  Once the independent timer is live, receiving
     * both sources only wakes NovaDock needlessly while the dock is idle. */
    if (dock->window) ModifyIDCMP(dock->window,ND_DOCK_IDCMP_BASE);
    return 1;
}
int inside_rect(const struct NDRect *r, int x, int y)
{
    return x >= r->left && x < r->left + r->width &&
           y >= r->top && y < r->top + r->height;
}
void set_hidden(struct NDDock *dock, int hide)
{
    const struct NDRect *r;struct NDRect target;
    if (!dock->window || dock->box_pending || hide == dock->hidden) return;
    dock_window_box(dock,hide,&target);r=&target;
    dock->window_box_target=target;dock->viewport_pending=0;
    /* ChangeWindowBox is asynchronous: don't paint in the new geometry
     * until the IDCMP_CHANGEWINDOW acknowledgement has arrived. */
    dock->drop_preview=0;
    dock->box_pending = 1;dock->box_pending_ticks=0;
    dock->box_target_hidden = hide;
    dock->idle_ticks = 0;
    dock->box_origin_x=dock->window->LeftEdge;dock->box_origin_y=dock->window->TopEdge;
    ChangeWindowBox(dock->window, r->left, r->top, r->width, r->height);
}
void show_dock(struct NDDock *dock)
{
    if (!dock->window) return;
    set_hidden(dock,0);
    if(dock->conf.always_on_top)WindowToFront(dock->window);
    if(!dock->box_pending)draw_all(dock);
}
void hide_dock(struct NDDock *dock)
{
    if (!dock->window) return;
    close_popup(dock);
    close_calendar(dock);
    close_folder_popup(dock);
    close_tooltip(dock);
    set_hidden(dock,1);
}
static void auto_hide_tick(struct NDDock *dock,int quantum)
{
    int mx, my, pointer_inside, blocked;
    if (!dock->window || !dock->screen || dock->box_pending ||
        (!dock->conf.auto_hide && !dock->hidden)) return;
    mx = dock->screen->MouseX;
    my = dock->screen->MouseY;
    if (dock->hidden) {
        if (inside_rect(&dock->tab_box, mx, my)) set_hidden(dock, 0);
        return;
    }
    pointer_inside = inside_rect(&dock->full_box, mx, my);
    blocked = dock->popup || dock->calendar || dock->folder_popup ||
              dock->tooltip || dock->press_item >= 0 || dock->grip_drag || dock->is_drag ||
              dock->keyboard;
    if (pointer_inside || blocked) dock->idle_ticks = 0;
    else if (dock->idle_ticks < 300) {
        dock->idle_ticks+=quantum;
        if (dock->idle_ticks>300) dock->idle_ticks=300;
    }
    if (nd_should_hide(dock->conf.auto_hide, dock->hidden, blocked,
                       pointer_inside, dock->idle_ticks,
                       dock->conf.hide_delay)) set_hidden(dock, 1);
}
static void eyes_timer_tick(struct NDDock *dock,int quantum)
{
    int index=eyes_index(dock),interval,xp[2],yp[2],rx,ry,pr,blink;
    int blink_changed;
    if (index<0 || dock->hidden || dock->box_pending || dock->covered ||
        !dock->window) return;
    interval=DOCK_OPTS(dock)->eyes_poll_ticks;
    if (interval<20) interval=20;
    dock->eyes_timer_ticks+=quantum;
    if (dock->eyes_timer_ticks*10<interval) return;
    dock->eyes_timer_ticks=0;
    ++dock->eyes_blink_counter;
    blink=dock->opts.eyes_blink &&
          (dock->eyes_blink_counter%40==0);
    blink_changed=blink!=dock->eyes_blink;
    dock->eyes_blink=blink;
    for (index=0;index<dock->conf.count;++index) {
        struct NDItem *item=&dock->conf.items[index];
        if (item->kind!=ND_ITEM_PLUGIN || strcmp(item->id,"EYES") ||
            !item->online || !item_visible(dock,index)) continue;
        eyes_positions(dock,index,xp,yp,&rx,&ry,&pr);
        if (dock->eyes_last_valid[index] && !blink_changed &&
            !nd_eyes_moved(dock->eyes_last_x[index][0],
                           dock->eyes_last_y[index][0],xp[0],yp[0]) &&
            !nd_eyes_moved(dock->eyes_last_x[index][1],
                           dock->eyes_last_y[index][1],xp[1],yp[1]))
            continue;
        draw_tile(dock,index);
    }
}
/* A press whose release never arrived (its window was replaced, or the
 * message was lost) must not keep the dock "pressed": that blocks auto-hide
 * and snapshot refreshes and pins the fast timer.  Cancel it, without any
 * action, once the button has been up for three ticks with no queued input. */
/* Intuition normally acknowledges a ChangeWindowBox() within a frame or
 * two.  If the exact target is never reported (the box was clamped, or a
 * screen change superseded it) accept the geometry the window really has
 * after about two seconds instead of ignoring input and paints forever. */
#define ND_BOX_PENDING_TIMEOUT_TICKS 10
static void box_pending_tick(struct NDDock *dock)
{
    if(!dock->window || !dock->box_pending){dock->box_pending_ticks=0;return;}
    if(++dock->box_pending_ticks<ND_BOX_PENDING_TIMEOUT_TICKS)return;
    ND_TRACE_NOTE("box change timed out",dock->window->Width,dock->window->Height,
                  dock->window_box_target.width,dock->window_box_target.height);
    /* A slow (not lost) acknowledgement may still follow: window_events()
     * then installs the geometry the window really ended up with. */
    if(!complete_window_box(dock))dock->box_late_ack=1;
}
static void orphan_press_tick(struct NDDock *dock);
/* Without timer.device, Intuition ticks (about 10 Hz while the dock is
 * active) drive the same recovery at the timer's 5 Hz rate. */
void fallback_recovery_tick(struct NDDock *dock)
{
    if(dock->timer_open || (dock->fallback_tick_phase^=1))return;
    box_pending_tick(dock);
    orphan_press_tick(dock);
}
static void orphan_press_tick(struct NDDock *dock)
{
    int held,waiting;
    if(!dock->window || dock->box_pending || dock->deferred_buttons.count){dock->orphan_press_ticks=0;return;}
    held=!InputBase || (PeekQualifier()&IEQUALIFIER_LEFTBUTTON)!=0;
    Forbid();
    waiting=dock->window->UserPort->mp_MsgList.lh_Head->ln_Succ!=NULL;
    Permit();
    if(nd_press_orphaned(dock->press_item>=0 || dock->grip_drag || dock->is_drag || dock->music_press_action!=0,
                         held,waiting,&dock->orphan_press_ticks,3)){
        cancel_dock_grip(dock);
        dock->press_item=-1;dock->drag_to=-1;dock->drag_after=0;
        dock->is_drag=0;dock->grip_drag=0;dock->music_press_action=0;
        draw_all(dock);
    }
}
/* True when a layer in front of the dock, other than the dock's own
 * popups, intersects it.  Layer links change only under the layer lock in
 * other tasks, so a short Forbid() walk is consistent. */
static int foreign_layer_overlaps(struct NDDock *dock)
{
    struct Layer *own=dock->window->WLayer,*l;
    int overlap=0;
    /* Walk the chain under the screen's layer lock: Forbid() does not stop
     * Intuition's input task from relinking layers on another CPU-less but
     * preemptive path, and a window closed mid-walk leaves a dangling
     * front pointer.  Forbid() remains the fallback without layers.library. */
    if(LayersBase)LockLayerInfo(&dock->screen->LayerInfo);else Forbid();
    for(l=own->front;l && !overlap;l=l->front){
        struct Window *w=(struct Window *)l->Window;
        if(w && (w==dock->caption_popup || w==dock->tooltip || w==dock->popup ||
                 w==dock->calendar || w==dock->folder_popup || w==dock->about ||
                 w==dock->feedback))continue;
        overlap=nd_rect_overlap(own->bounds.MinX,own->bounds.MinY,own->bounds.MaxX,own->bounds.MaxY,
                                l->bounds.MinX,l->bounds.MinY,l->bounds.MaxX,l->bounds.MaxY);
    }
    if(LayersBase)UnlockLayerInfo(&dock->screen->LayerInfo);else Permit();
    return overlap;
}
void always_on_top_tick(struct NDDock *dock,int quantum)
{
    if (!dock->conf.always_on_top) {
        dock->top_timer_ticks=0;
        return;
    }
    dock->top_timer_ticks+=quantum;
    if (dock->top_timer_ticks<5) return;
    dock->top_timer_ticks=0;
    /* Classic Intuition has no persistent topmost-window flag. Moving the
     * layer forward is the compatible Workbench 3.x approximation and does
     * not activate the dock or steal keyboard focus. Keep dock-owned popup
     * windows unobscured while they are in use. */
    if (dock->window && !dock->hidden && !dock->box_pending &&
        !dock->popup && !dock->calendar && !dock->folder_popup &&
        !dock->tooltip && !dock->feedback &&
        dock->window->WLayer && foreign_layer_overlaps(dock))
        /* Skip the layer operation when nothing in front overlaps the dock:
         * WLayer->front alone is set by any window, anywhere on screen, and
         * a pointless raise every second can ping-pong with an application
         * that fronts itself, repainting both each time. */
        WindowToFront(dock->window);
}
/* Workbench owns the mouse during an incoming icon drag, so ordinary dock
 * mouse messages cannot drive its insertion feedback. Use the existing
 * housekeeping tick and live qualifier; no additional timer is needed. */
void update_drop_preview(struct NDDock *dock)
{
    int active=0,index=-1,after=0,pos,count,mx,my,scroll;
    if (!dock->drop_zones && dock->edit_mode && dock->window && dock->screen && InputBase &&
        !dock->hidden && !dock->covered && !dock->box_pending &&
        !dock->popup && !dock->calendar && !dock->folder_popup &&
        dock->press_item<0 && !dock->is_drag &&
        (PeekQualifier() & IEQUALIFIER_LEFTBUTTON)) {
        mx=dock->screen->MouseX-dock_left(dock);
        my=dock->screen->MouseY-dock_top(dock);
        if (mx>=0 && my>=0 && mx<dock_width(dock) &&
            my<dock_height(dock)) {
            count=nd_media_persistent_count(&dock->conf);
            index=hovered_tile(dock,mx,my);
            if (index>=0) {
                after=drop_after_tile(dock,index,mx,my);
                pos=nd_drop_insert_position(index,after,count);
            } else {
                pos=count;
                scroll=hovered_scroll(dock,mx,my);
                if (scroll<0) pos=dock->view_start;
                else if (scroll>0) pos=dock->view_start+dock->visible_count;
            }
            if (pos>count) pos=count;
            /* Keep the hovered edge at row boundaries. Transient media and
             * background drops instead point at the persistent append edge. */
            if (index<0 || index>=count) {
                index=pos<count ? pos : count-1;
                after=pos>=count;
            }
            active=index>=0 && item_visible(dock,index);
        }
    }
    if (active==dock->drop_preview && (!active ||
        (index==dock->drop_preview_item && after==dock->drop_preview_after)))
        return;
    dock->drop_preview=active;
    dock->drop_preview_item=index;
    dock->drop_preview_after=after;
    if (active) close_tooltip(dock);
    /* Repaint only when the insertion edge changes, never every idle tick. */
    draw_all(dock);
}
/* A backdrop dock must not react through an application window. Rectangles
 * suffice here: every ordinary window is above the backdrop group. */
static int dock_pointer_visible(struct NDDock *dock)
{
    struct Window *w;ULONG lock;int visible=1,x=dock->screen->MouseX,y=dock->screen->MouseY;
    /* Moving a foreign window or dragging a Workbench icon must not expand
     * the topmost canvas under the drag. Dock-owned tile drags remain live. */
    if(InputBase && (PeekQualifier()&IEQUALIFIER_LEFTBUTTON) &&
       dock->press_item<0 && IntuitionBase->ActiveWindow!=dock->window)return 0;
    if(dock->canvas_view && (x<dock->window->LeftEdge || y<dock->window->TopEdge ||
       x>=dock->window->LeftEdge+dock->window->Width || y>=dock->window->TopEdge+dock->window->Height))return 0;
    if(!(dock->window->Flags&WFLG_BACKDROP))return 1;
    lock=LockIBase(0);
    for(w=dock->screen->FirstWindow;w;w=w->NextWindow){
        if(w==dock->window || w==dock->tooltip || w==dock->caption_popup ||
           (w->Flags&WFLG_BACKDROP))continue;
        if(x>=w->LeftEdge && x<w->LeftEdge+w->Width &&
           y>=w->TopEdge && y<w->TopEdge+w->Height){visible=0;break;}
    }
    UnlockIBase(lock);return visible;
}
void update_hover_pointer(struct NDDock *dock)
{
    int x,y,index,axis,previous,focus_changed;
    if(!dock->window || !dock->screen || dock->hidden || dock->screen_closed ||
       dock->covered || dock->box_pending || dock->popup || dock->calendar ||
       dock->folder_popup || dock->grip_drag || dock->is_drag || dock->music_press_action || !dock->conf.hover_magnification)return;
    if(dock->keyboard){
        if(dock->screen->MouseX==dock->keyboard_mouse_x &&
           dock->screen->MouseY==dock->keyboard_mouse_y)return;
        dock->keyboard=0;
    }
    x=dock->screen->MouseX-dock_left(dock);
    y=dock->screen->MouseY-dock_top(dock);
    index=dock_pointer_visible(dock) ? pointer_hover_target(dock,x,y):-1;axis=nd_hover_axis(vertical(dock) ? y:x,dock->hover_pointer);
    previous=dock->hover;
    focus_changed=update_music_control_hover(dock,index,x,y);
    if(index!=previous){
        close_tooltip(dock);dock->hover=index;dock->hover_ticks=0;
    }
    if(focus_changed || index!=previous || (index>=0 && axis!=dock->hover_pointer)){
        dock->hover_pointer=axis;
        if(dock->hover_headroom)draw_hover(dock);
        else if(focus_changed || index!=previous)redraw_changed_tiles(dock,previous,index,-1);
    }
}

void hover_tick(struct NDDock *dock,int tip_delay)
{
    int ax,ay,index;
    update_drop_preview(dock);
    if(!dock->conf.tooltips && dock->tooltip)close_tooltip(dock);
    if (!dock->window || !dock->screen || dock->hidden || dock->box_pending)
        return;
    if (dock->drop_preview || dock->grip_drag) return;
    update_hover_pointer(dock); /* low-rate fallback if Commodities is unavailable */
    if (dock->keyboard) {
        /* The keys own the highlight until the pointer moves. */
        if (dock->screen->MouseX==dock->keyboard_mouse_x &&
            dock->screen->MouseY==dock->keyboard_mouse_y) {
            if (dock->hover>=0 && !dock->popup) {
                if (dock->hover_ticks < tip_delay) ++dock->hover_ticks;
                if (dock->hover_ticks == tip_delay) show_tooltip(dock,dock->hover);
            }
            return;
        }
        dock->keyboard=0;
    }
    ax=dock->screen->MouseX-dock_left(dock);
    ay=dock->screen->MouseY-dock_top(dock);
    index=dock_pointer_visible(dock) ? pointer_hover_target(dock,ax,ay):-1;
    if (index != dock->hover) {
        int old_hover=dock->hover;
        close_tooltip(dock);dock->hover=index;
        dock->hover_ticks=0;
        redraw_changed_tiles(dock,old_hover,index,-1);
    } else if (index >= 0 && !dock->popup && dock->press_item < 0) {
        if (dock->hover_ticks < tip_delay) ++dock->hover_ticks;
        if (dock->hover_ticks == tip_delay) show_tooltip(dock,index);
    }
}
static const struct NDMediaVolume *media_missing(
    const struct NDMediaVolume *list,int count,
    const struct NDMediaVolume *other,int other_count)
{
    int i,j;
    for (i=0;i<count;++i) {
        for (j=0;j<other_count;++j)
            if (!strcmp(list[i].volume,other[j].volume) &&
                !strcmp(list[i].drive,other[j].drive)) break;
        if (j==other_count) return &list[i];
    }
    return NULL;
}
static void media_rescan(struct NDDock *dock)
{
    struct NDMediaVolume found[ND_MEDIA_MAX];
    const struct NDMediaVolume *inserted,*removed;
    char message[200],name[ND_MEDIA_NAME_LEN];
    int count;
    /* Rebuilding the dock would close an open menu, calendar or drawer
     * popup; the periodic scan simply tries again two seconds later. */
    if (dock->popup || dock->folder_popup || dock->calendar) return;
    count=scan_media(dock,found);
    if (nd_media_same(found,count,dock->media,dock->media_count)) return;
    inserted=media_missing(found,count,dock->media,dock->media_count);
    removed=media_missing(dock->media,dock->media_count,found,count);
    name[0]='\0';
    if (inserted) strcpy(name,inserted->volume);
    else if (removed) strcpy(name,removed->volume);
    memcpy(dock->media,found,sizeof(found));
    dock->media_count=count;
    if (inserted) fire_hook(dock,"DISK_IN",name);
    else if (removed) fire_hook(dock,"DISK_OUT",name);
    if (!dock->window || dock->screen_closed) return; /* applied on reopen */
    if (reload_configuration(dock)!=ND_CONTROL_OK) return;
    if (inserted) {
        sprintf(message,nd_trf("Disk inserted: %.28s"),name);
        show_feedback(dock,message,0);
    } else if (removed) {
        sprintf(message,nd_trf("Disk removed: %.28s"),name);
        show_feedback(dock,message,0);
    }
}
void process_timer(struct NDDock *dock)
{
    if (!dock->timer_port) return;
    while (GetMsg(dock->timer_port) != NULL) {
        int ax,ay,index,quantum;ULONG seconds,micros,fallback;
        fallback=dock->timer_anim ? (ULONG)dock->timer_frame_ms*1000UL:
            ND_TIMER_MICROSECONDS*(ULONG)(dock->timer_quanta>0?dock->timer_quanta:1);
        CurrentTime(&seconds,&micros);
        quantum=(int)nd_timer_quanta(&dock->timer_clock,seconds,micros,fallback);
        dock->timer_pending = 0;
        if (dock->timer_anim && dock->hover_animating && dock->window && !dock->hidden && !dock->box_pending)
            draw_hover(dock);
        if(quantum<1)continue;
        nd_aminotis_tick(dock->conf.send_alerts_aminotis,(unsigned int)quantum*200U);
        nd_update_tick(&dock->updates,dock->conf.check_updates,dock->conf.check_updates_running,(unsigned int)dock->conf.update_interval_hours,dock->directory,(unsigned long)quantum*10UL);
        dock->alerts_update_tick+=(ULONG)quantum;
        if(dock->alerts_update_tick>=25UL){struct NDUpdateStatus status;int active;dock->alerts_update_tick=0;
            Forbid();active=FindPort("NovaDock.UpdateCheck")!=NULL;Permit();
            if(active)dock->alerts_update_waiting=1;
            if(dock->conf.check_updates || dock->conf.check_updates_running || dock->alerts_update_waiting){nd_update_status(&status);
                if(!active)dock->alerts_update_waiting=0;
                if(status.checked && status.checked!=dock->alerts_update_checked && !status.checking){dock->alerts_update_checked=status.checked;if(status.result==2 && nd_update_compare(status.version,ND_VERSION)==1)dock_alert(dock,"UPDATES","New NovaDock version",status.version,ND_ALERT_INFO);}}}
        if (dock->suspended) {
            dock->suspend_ticks-=quantum;
            if (dock->suspend_ticks<=0) dock->suspended=0;
        }
        if (dock->screen_closed && !dock->window && !dock->suspended &&
            (dock->reopen_timer_ticks+=quantum)>=5) {
            /* Workbench reopened but the dock could not: retry each second. */
            dock->reopen_timer_ticks=0;
            Forbid();
            ax=FindPort((STRPTR)"WORKBENCH")!=NULL;
            Permit();
            if (ax) (void)reopen_dock_ui(dock);
        }
        /* AddAppWindowA can fail transiently while LoadWB is still bringing
         * Workbench online.  A failed first attempt must not permanently
         * disable drag-and-drop for the whole NovaDock session. */
        /* User-Startup can run while Workbench is still initialising.
         * AddAppWindowA may report success at that point even though the
         * Workbench desktop later replaces its internal AppObject list.
         * Register only after the desktop has had about three seconds to
         * settle.  Runtime window reloads attach immediately elsewhere. */
        if (!dock->app_window) {
            if (dock->drop_attach_ticks < 15) {
                dock->drop_attach_ticks+=quantum;
                if (dock->drop_attach_ticks>15) dock->drop_attach_ticks=15;
            }
            else attach_appwindow(dock);
        }
        if (dock->activate_popup_pending && dock->screen && dock->window &&
            !dock->hidden && !dock->box_pending && !dock->popup) {
            /* Relative mouse coordinates can update after IDCMP_ACTIVEWINDOW.
             * The independent timer sees the settled Intuition coordinates. */
            ax=dock->screen->MouseX-dock_left(dock);
            ay=dock->screen->MouseY-dock_top(dock);
            dock->activate_popup_pending=0;
            index=dock_pointer_visible(dock) ? pointer_hover_target(dock,ax,ay):-1;
            open_popup(dock,index>=0 ? index : -2,ax,ay);
        }
        {int was_covered=dock->covered;dock->covered=dock_covered(dock);
         if(was_covered && !dock->covered) {
             refresh_volume_meters(dock);
             refresh_running_items(dock);
         }}
        /* Startup artwork: a few tiles per tick, so the dock opens at once
         * and stays responsive while icons arrive. */
        if (dock->icons_pending) (void)load_pending_icons(dock,4);
        folder_icons_tick(dock);
        box_pending_tick(dock);
        orphan_press_tick(dock);
        auto_hide_tick(dock,quantum);
        always_on_top_tick(dock,quantum);
        eyes_timer_tick(dock,quantum);
        plugin_watchdog_tick(dock,quantum);
        hover_tick(dock,ND_TIP_TIMER_TICKS);
        if ((dock->volume_timer_ticks+=quantum) >= 25) {
            dock->volume_timer_ticks = 0;
            refresh_volume_meters(dock);
        }
        if ((dock->running_timer_ticks+=quantum) >= 5) {
            dock->running_timer_ticks=0;
            refresh_running_items(dock);
        }
        if (dock->feedback_ticks>0) {
            dock->feedback_ticks-=quantum;
            if (dock->feedback_ticks<=0) close_feedback(dock);
        }
        settings_save_tick(dock,quantum,0);
        if (dock->rexx_apply_pending &&
            (dock->rexx_apply_ticks-=quantum)<=0) flush_rexx_settings(dock);
        dock->media_scan_ticks-=quantum;
        if (dock->media_scan_ticks<=0) {
            dock->media_scan_ticks=ND_MEDIA_SCAN_TICKS;
            if (dock->conf.removable_media || dock->media_count)
                media_rescan(dock);
        }
        flush_dirty_tiles(dock);
        desktop_scene_tick(dock,quantum*200);
        desktop_startup_tick(dock,quantum*200);
    }
    arm_timer(dock);
}
void close_tooltip(struct NDDock *dock)
{
    if (dock->tooltip) { CloseWindow(dock->tooltip); dock->tooltip=NULL; }
    dock->tip_item=-1;
}
int tile_icon_art(const struct NDDock *dock,int index,struct BitMap **bitmap,
                  struct BitMap **mask,int *width,int *height)
{
    if (!dock->scaled_icon_bitmaps[index] || !dock->scaled_icon_masks[index])
        return 0;
    *bitmap=dock->scaled_icon_bitmaps[index];*mask=dock->scaled_icon_masks[index];
    *width=dock->scaled_icon_width[index];*height=dock->scaled_icon_height[index];
    return 1;
}
void close_feedback(struct NDDock *dock)
{
    if (dock->feedback) { CloseWindow(dock->feedback); dock->feedback=NULL; }
    dock->feedback_ticks=0;
}
void show_feedback(struct NDDock *dock,const char *text,int error)
{
    struct RastPort *rp;
    int len,w,h=23,x,y;
    if (!dock->screen || !dock->window || !text || !*text) return;
    text=nd_tr(text);
    close_feedback(dock);
    len=(int)strlen(text); if (len>54) len=54;
    w=(int)TextLength(dock->window->RPort,(STRPTR)text,len)+20;
    if (w<120) w=120;
    if (w>dock->screen->Width) w=dock->screen->Width;
    x=dock_left(dock)+(dock_width(dock)-w)/2;
    y=dock_top(dock)-h-3;
    if (y<dock->screen->BarHeight)
        y=dock_top(dock)+dock_height(dock)+3;
    if (y+h>dock->screen->Height) y=dock->screen->Height-h;
    if (x<0) x=0;
    if (x+w>dock->screen->Width) x=dock->screen->Width-w;
    dock->feedback=OpenWindowTags(NULL,
        WA_PubScreen,(ULONG)dock->screen,WA_Left,x,WA_Top,y,
        WA_Width,w,WA_Height,h,WA_Borderless,TRUE,WA_Activate,FALSE,
        WA_IDCMP,0,WA_NoCareRefresh,TRUE,TAG_DONE);
    if (!dock->feedback) return;
    rp=dock->feedback->RPort;
    SetAPen(rp,pen(dock,ND_BG));RectFill(rp,0,0,w-1,h-1);
    SetAPen(rp,pen(dock,error ? ND_ACCENT2 : ND_ACCENT));
    Move(rp,0,0);Draw(rp,w-1,0);Draw(rp,w-1,h-1);
    Draw(rp,0,h-1);Draw(rp,0,0);
    SetDrMd(rp,JAM1);SetAPen(rp,pen(dock,ND_TEXT));
    Move(rp,(w-(int)TextLength(rp,(STRPTR)text,len))/2,15);
    Text(rp,(STRPTR)text,len);
    dock->feedback_ticks=12;
}
static void show_tooltip(struct NDDock *dock, int index)
{
    if (dock->edit_mode || dock->tooltip || !dock->window || dock->popup ||
        index < 0 || index >= dock->conf.count) return;
    if (dock->conf.tooltips) show_tooltip_window(dock,index);
}
static void show_tooltip_window(struct NDDock *dock, int index)
{
    struct RastPort *rp;
    const struct NDItem *it;
    const char *name,*second_line=NULL,*third_line=NULL;
    char volume_name[192],ram_chip[80],ram_fast[80],plugin_detail[80],heading[96];
    unsigned long incoming,outgoing;
    int x,y,w,h=21,tw,th,tx,ty,len,len2=0,len3=0,text_w,text_w2=0,text_w3=0,row_height;
    if (!dock->conf.tooltips || dock->edit_mode || dock->tooltip || !dock->window ||
        dock->popup || index < 0 || index >= dock->conf.count) return;
    it = &dock->conf.items[index];
    name = it->label;
    if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"CPU")){
        sprintf(volume_name,"CPU %s",nd_system_cpu(SysBase->AttnFlags));name=volume_name;
        if(it->online && it->meter<=100U)sprintf(plugin_detail,"%u%% CPU used",(unsigned int)it->meter);
        else strcpy(plugin_detail,"CPU usage unavailable");
        second_line=plugin_detail;h=35;
    } else if (!strcmp(it->id,"RAM")) {
        if (format_ram_tooltip(it->value,ram_chip,sizeof(ram_chip),
                               ram_fast,sizeof(ram_fast))) {
            name=ram_chip;
            second_line=ram_fast;
        } else {
            name="CHIP memory unavailable";
            second_line="Fast memory unavailable";
        }
        h=35;
    } else if(it->kind==ND_ITEM_PLUGIN &&
              (!strcmp(it->id,"NET") || !strcmp(it->id,"ROADSHOW"))){
        name="Network";
        if(sscanf(it->value,"%lu/%lu p/s",&incoming,&outgoing)==2){
            sprintf(plugin_detail,"IN: %lu packets/s   OUT: %lu packets/s",incoming,outgoing);
            second_line=plugin_detail;
        }else second_line=it->value;
        h=35;
    } else if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"ARCHIVE")){
        name="ARCHIVE";
        if(it->caption[0] && strcmp(it->caption,"ARCHIVE") && strcmp(it->caption,"ARCHIVER"))
            sprintf(plugin_detail,"%.31s: %.31s",it->value,it->caption);
        else {
            strncpy(plugin_detail,it->value,sizeof(plugin_detail)-1);
            plugin_detail[sizeof(plugin_detail)-1]=0;
        }
        second_line=plugin_detail;h=35;
    } else if(it->kind==ND_ITEM_PLUGIN && (!strcmp(it->id,"SCREENSHOT") || !strcmp(it->id,"SCREENMODE"))){
        name=!strcmp(it->id,"SCREENSHOT") ? "Screenshot":"Display mode";second_line=it->value;h=35;
    } else if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"WEATHER")){
        const char *city=strchr(it->caption,' ');int code=-1;
        if(!strncmp(it->caption,"WX:",3))code=atoi(it->caption+3);
        name=city && city[1] ? city+1:"Weather";
        sprintf(plugin_detail,"%.20s  %.20s%s",it->value,code>=0 ? nd_weather_condition(code):"",it->badge[0] ? " (cached)":"");
        second_line=plugin_detail;h=35;
    } else if(it->kind==ND_ITEM_PLUGIN && nd_collection_plugin_path(it->command)){
        const char *root=nd_option_get(&dock->conf,it->id,"DRAWER");
        int reason=(!root || !*root) ? ND_WHD_FOLDER_UNSET:nd_whd_folder_code(it->value);
        switch(reason){
        case ND_WHD_FOLDER_UNSET: second_line=nd_tr("No drawer selected. Open Edit settings.");break;
        case ND_WHD_FOLDER_MISSING: second_line=nd_tr("The selected drawer does not exist.");break;
        case ND_WHD_FOLDER_NOT_DRAWER: second_line=nd_tr("The selected path is a file, not a drawer.");break;
        case ND_WHD_FOLDER_UNREADABLE: second_line=nd_tr("Cannot read the selected drawer.");break;
        case ND_WHD_FOLDER_MEMORY: second_line=nd_tr("Not enough memory to check the drawer.");break;
        case ND_WHD_FOLDER_UNAVAILABLE: second_line=nd_tr("The selected drawer's volume is unavailable.");break;
        default: second_line=!strcmp(it->value,"INDEX INCOMPLETE") ? nd_tr("Index incomplete. Use Rescan to try again."):it->value;break;
        }
        h=35;
    } else if(it->kind==ND_ITEM_PLUGIN && nd_hass_plugin_path(it->command)){
        second_line=it->value;h=35;
    } else if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"NOTES")){
        name="Notes";second_line=it->value;h=35;
    } else if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"TIMER")){
        name="Timer";sprintf(plugin_detail,"%.15s: %.31s",it->caption,it->value);
        second_line=plugin_detail;h=35;
    } else if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"BITSKIM")){
        name="BitSkim";second_line=it->value;h=35;
    } else if (nd_item_supports_space_meter(it) && it->show_volume_meter) {
        struct NDVolumeStatus status;int known=nd_volume_meters_status(it->command,&status);
        if(it->meter<=100U){sprintf(volume_name,nd_trf("%s: %u%% used%s"),it->label,(unsigned int)it->meter,known && status.state==ND_VOLUME_STALE ? nd_tr(" (cached)"):"");name=volume_name;}
        else name=it->label;
        {static char capacity[100];struct NDVolumeSpace space;space.total=it->volume_total;space.used=it->volume_used;space.block_bytes=it->volume_block_bytes;nd_volume_summary(&space,capacity);second_line=capacity;}
        if(!known)strcpy(plugin_detail,nd_tr("Waiting for disk reading"));
        else if(status.state==ND_VOLUME_UNAVAILABLE)sprintf(plugin_detail,nd_trf("Disk unavailable (DOS error %lu)"),status.error);
        else if(status.error)sprintf(plugin_detail,nd_trf("Cached: %lus old; DOS error %lu"),status.age_seconds,status.error);
        else sprintf(plugin_detail,nd_trf("Last updated %lus ago%s"),status.age_seconds,status.state==ND_VOLUME_STALE ? nd_tr(" (stale)"):"");
        third_line=plugin_detail;

    }
    if(it->kind==ND_ITEM_PLUGIN){
        const char *schema=nd_option_schema_id(&dock->conf,it->id);
        int state=nd_tile_state(schema,it->online,it->value);
        if(state){
            sprintf(heading,"%.31s - %s",it->label,nd_tile_state_name(state));name=heading;h=35;
            if(!strcmp(it->value,"STARTING"))second_line="Waiting for the plugin's first reading.";
            else if(!strcmp(it->value,"MISSING PLUGIN"))second_line="Plugin executable is missing. Check Plugins or reinstall it.";
            else if(!strcmp(it->value,"FAILED TO START"))second_line="Could not start the plugin. Check Diagnostics and available memory.";
            else if(!strcmp(it->value,"NOT RESPONDING"))second_line="Plugin stopped responding. Open Diagnostics.";
            else if(!it->online)second_line="Plugin is not running. Check Plugins or Diagnostics.";
            else{struct NDPluginHealth health=nd_plugin_health(schema,1,0,it->value);
                second_line=strcmp(health.label,"Running") ? health.help:it->value;}
        }else if(strcmp(name,it->label)){
            third_line=second_line;second_line=name;name=it->label;
        }
    }
    if(dock->caption_popup && dock->caption_index==index && !second_line)return;
    len=(int)strlen(name);
    if (len > 78) len = 78;
    text_w=(int)TextLength(dock->window->RPort,(STRPTR)name,len);
    if (second_line) {
        len2=(int)strlen(second_line);if (len2>78) len2=78;
        text_w2=(int)TextLength(dock->window->RPort,(STRPTR)second_line,len2);
    }
    if(third_line){len3=strlen(third_line);if(len3>78)len3=78;
        text_w3=TextLength(dock->window->RPort,(STRPTR)third_line,len3);}
    w=text_w>text_w2 ? text_w:text_w2;if(text_w3>w)w=text_w3;w+=18;
    row_height=dock->window->RPort->TxHeight+5;
    h=(third_line ? 3:second_line ? 2:1)*row_height+8;
    if (w < 70) w=70;
    if (w > dock->screen->Width) {
        w=dock->screen->Width;
        while (len>0 && (int)TextLength(dock->window->RPort,
               (STRPTR)name,len)>w-18) --len;
        while (len2>0 && (int)TextLength(dock->window->RPort,
               (STRPTR)second_line,len2)>w-18) --len2;
        while(len3>0 && TextLength(dock->window->RPort,(STRPTR)third_line,len3)>w-18)--len3;
    }
    tile_coords(dock,index,&x,&y);
    tile_dimensions(dock,index,&tw,&th);
    /* Attach to the artwork's current bounds, not the reserved hover canvas. */
    {struct NDRect box;visible_tile_rect(dock,index,&box);
     x=box.left;y=box.top;tw=box.width;th=box.height;}
    tx=dock_left(dock)+x+(tw-w)/2;
    ty=dock_top(dock)+y-h-4;
    if(dock->caption_popup && dock->caption_index==index)ty=dock->caption_popup->TopEdge-h-4;
    if (ty < dock->screen->BarHeight)
        ty=dock_top(dock)+y+th+3;
    if (ty+h > dock->screen->Height) ty=dock->screen->Height-h;
    if (tx < 0) tx=0;
    if (tx+w > dock->screen->Width) tx=dock->screen->Width-w;
    dock->tooltip=OpenWindowTags(NULL,
        WA_PubScreen,(ULONG)dock->screen, WA_Left,tx, WA_Top,ty,
        WA_Width,w, WA_Height,h, WA_Borderless,TRUE,
        WA_Activate,FALSE, WA_IDCMP,0, WA_NoCareRefresh,TRUE,
        TAG_DONE);
    if (!dock->tooltip) return;
    dock->tip_item=index;
    rp=dock->tooltip->RPort;
    SetFont(rp,dock->window->RPort->Font);
    SetSoftStyle(rp,dock->conf.font_mode==ND_FONT_BOLD ? FSF_BOLD:FS_NORMAL,0xff);
    SetAPen(rp,pen(dock,ND_BG)); RectFill(rp,0,0,w-1,h-1);
    SetAPen(rp,pen(dock,ND_ACCENT));
    Move(rp,0,0); Draw(rp,w-1,0); Draw(rp,w-1,h-1);
    Draw(rp,0,h-1); Draw(rp,0,0);
    SetDrMd(rp,JAM1); SetAPen(rp,pen(dock,ND_TEXT));
    Move(rp,(w-(int)TextLength(rp,(STRPTR)name,len))/2,
         4+rp->TxBaseline);
    Text(rp,(STRPTR)name,len);
    if (second_line) {
        Move(rp,(w-(int)TextLength(rp,(STRPTR)second_line,len2))/2,4+rp->TxBaseline+row_height);
        Text(rp,(STRPTR)second_line,len2);
    }
    if(third_line){
        Move(rp,(w-(int)TextLength(rp,(STRPTR)third_line,len3))/2,4+rp->TxBaseline+2*row_height);
        Text(rp,(STRPTR)third_line,len3);
    }
}
/* Volume part of SYS: (without colon); its medium is never shown as a
 * removable tile even when the system boots from a CF card or USB stick. */
void find_system_volume(struct NDDock *dock)
{
    char path[128];
    char *colon;
    BPTR lock;
    APTR requesters=quiet_requesters();
    dock->system_volume[0]='\0';
    lock=Lock((STRPTR)"SYS:",ACCESS_READ);
    if (lock) {
        if (NameFromLock(lock,path,sizeof(path))) {
            colon=strchr(path,':');
            if (colon && colon-path<(int)sizeof(dock->system_volume)) {
                *colon='\0';
                strcpy(dock->system_volume,path);
            }
        }
        UnLock(lock);
    }
    restore_requesters(requesters);
}
static void copy_bstr(char *out,int size,BSTR bstr)
{
    UBYTE *text=(UBYTE *)BADDR(bstr);
    int length;
    out[0]='\0';
    if (!text) return;
    length=text[0];
    if (length>size-1) length=size-1;
    memcpy(out,text+1,(size_t)length);
    out[length]='\0';
}
/* Collect mounted volumes whose handler drives a removable-media exec
 * device.  Only the DOS list is read: no Lock() on any volume, so an empty
 * drive can never raise a requester or start a floppy motor. */
int scan_media(struct NDDock *dock,struct NDMediaVolume *out)
{
    struct DriveInfo {
        struct MsgPort *task;
        char drive[ND_MEDIA_DRIVE_LEN];
        char device[ND_MEDIA_DEVICE_LEN];
        ULONG unit;
    } drives[16];
    struct DosList *start,*entry;
    const char *override=nd_option_get(&dock->conf,"MEDIA","DEVICES");
    int drive_count=0,count=0,i;
    if (!dock->conf.removable_media) return 0;
    start=LockDosList(LDF_DEVICES|LDF_VOLUMES|LDF_READ);
    for (entry=start;(entry=NextDosEntry(entry,LDF_DEVICES))!=NULL;) {
        struct FileSysStartupMsg *startup;
        ULONG raw=(ULONG)entry->dol_misc.dol_handler.dol_Startup;
        if (!entry->dol_Task || raw<=64 || drive_count>=16) continue;
        startup=(struct FileSysStartupMsg *)BADDR(raw);
        if (!TypeOfMem(startup) || !startup->fssm_Device ||
            !TypeOfMem(BADDR(startup->fssm_Device))) continue;
        copy_bstr(drives[drive_count].device,ND_MEDIA_DEVICE_LEN,
                  startup->fssm_Device);
        if (!nd_media_device_removable(drives[drive_count].device,override))
            continue;
        copy_bstr(drives[drive_count].drive,ND_MEDIA_DRIVE_LEN-1,
                  entry->dol_Name);
        strcat(drives[drive_count].drive,":");
        drives[drive_count].task=entry->dol_Task;
        drives[drive_count].unit=startup->fssm_Unit;
        ++drive_count;
    }
    for (entry=start;(entry=NextDosEntry(entry,LDF_VOLUMES))!=NULL;) {
        if (!entry->dol_Task || count>=ND_MEDIA_MAX) continue;
        for (i=0;i<drive_count;++i)
            if (drives[i].task==entry->dol_Task) break;
        if (i==drive_count) continue;
        memset(&out[count],0,sizeof(out[count]));
        copy_bstr(out[count].volume,ND_MEDIA_NAME_LEN,entry->dol_Name);
        strcpy(out[count].drive,drives[i].drive);
        strcpy(out[count].device,drives[i].device);
        out[count].unit=drives[i].unit;
        if (out[count].volume[0]) ++count;
    }
    UnLockDosList(LDF_DEVICES|LDF_VOLUMES|LDF_READ);
    return nd_media_normalize(out,count);
}
