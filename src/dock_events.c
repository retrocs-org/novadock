/* NovaDock: Drops, clicks, keyboard control and the dock window event loop. */
#include "dock_internal.h"
#include "nd_trace.h"

/* Defined below, private to this file. */
static int drop_opens_launcher(struct NDDock *dock,int index,int mx,int my);
static int drop_copies_into_drawer(struct NDDock *dock,int index,int mx,int my);
static int drop_extracts_archive(struct NDDock *dock,int index);
static int toggle_monitor_view(struct NDDock *dock,int index);
static int open_dropped_objects(struct NDDock *dock,int index, const struct AppMessage *message,int *failed);
static void click_tile(struct NDDock *dock,int index,int mx,int my, UWORD qualifier);
static int keyboard_focusable(struct NDDock *dock,int index);
static void keyboard_focus(struct NDDock *dock,int index);
static void keyboard_move(struct NDDock *dock,int step);
static void keyboard_end(struct NDDock *dock);
static void keyboard_point(struct NDDock *dock,int index,int *mx,int *my);
static int keyboard_key(struct NDDock *dock,ULONG cls,UWORD code,UWORD qualifier);

static int drop_opens_launcher(struct NDDock *dock,int index,int mx,int my)
{
    struct NDItem *item;
    int x,y,w,h,position,extent;
    if (!dock || index<0 || index>=dock->conf.count) return 0;
    item=&dock->conf.items[index];
    if (item->transient) return 0;
    if (item->kind!=ND_ITEM_LAUNCH || !item->command[0] ||
        item->launch_mode==ND_LAUNCH_AREXX ||
        !strcmp(item->command,"@quit")) return 0;
    tile_coords(dock,index,&x,&y);
    tile_dimensions(dock,index,&w,&h);
    if(dock->hover_headroom && dock->hover_plan_ready){
        const struct NDRect *box=&dock->hover_rects[index];
        x=box->left;y=box->top;w=box->width;h=box->height;
    }
    if (vertical(dock)) { position=my-y;extent=h; }
    else { position=mx-x;extent=w; }
    return nd_drop_opens_item(position,extent);
}
/* Centre of a drawer or volume tile: copy the dropped objects into it.  The
 * outer fifths stay insertion zones in Edit mode, as for applications. */
static int drop_copies_into_drawer(struct NDDock *dock,int index,int mx,int my)
{
    int x,y,w,h;
    if (!dock || index<0 || index>=dock->conf.count ||
        dock->conf.items[index].kind!=ND_ITEM_DRAWER) return 0;
    tile_coords(dock,index,&x,&y);
    tile_dimensions(dock,index,&w,&h);
    if(dock->hover_headroom && dock->hover_plan_ready){
        const struct NDRect *box=&dock->hover_rects[index];
        x=box->left;y=box->top;w=box->width;h=box->height;
    }
    return vertical(dock) ? nd_drop_opens_item(my-y,h) :
                            nd_drop_opens_item(mx-x,w);
}
static int drop_extracts_archive(struct NDDock *dock,int index)
{
    return dock && index>=0 && index<dock->conf.count &&
           dock->conf.items[index].kind==ND_ITEM_PLUGIN &&
           !strcmp(dock->conf.items[index].id,"ARCHIVE");
}
/* CPU and RAM are visual instruments, so their primary click changes the
 * view directly instead of sending an otherwise meaningless CLICK event to
 * the sampler process. Persist the choice without reloading the dock: both
 * views occupy the same tile and graph history is already kept current. */
void persist_folder_choice(struct NDDock *dock)
{
    struct NDConfig *full=AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_REVERSE);
    if(!full)return;
    if(load_edit_configuration(dock,full)<=0){
        full->folder_view=dock->folder_view;full->folder_sort=dock->folder_sort;
        if(queue_settings_save(dock,&full)){
            dock->conf.folder_view=dock->folder_view;
            dock->conf.folder_sort=dock->folder_sort;
        }
    }
    if (full) FreeVec(full);
}
static int toggle_monitor_view(struct NDDock *dock,int index)
{
    struct NDConfig *full;int row,next,ok=0;
    if(!dock || index<0 || index>=dock->conf.count)return 0;
    if(strcmp(dock->conf.items[index].id,"CPU") && strcmp(dock->conf.items[index].id,"RAM") && strcmp(dock->conf.items[index].id,"NET") && strcmp(dock->conf.items[index].id,"ROADSHOW"))return 0;
    full=AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_REVERSE);if(!full)return 0;
    row=load_edit_configuration(dock,full)<=0 ? nd_visible_to_full(full,index) : -1;
    next=!dock->conf.items[index].graph;
    if(row>=0 && full->items[row].kind==ND_ITEM_PLUGIN){
        int is_net=!strcmp(full->items[row].id,"NET");
        full->items[row].graph=next;
        if(!is_net || nd_option_set(full,"NET","VIEW",next ? "GRAPH":"METER"))
            ok=queue_settings_save(dock,&full); /* owns FULL on success */
        if(ok){
            dock->conf.items[index].graph=next;
            if(is_net){
                (void)nd_option_set(&dock->conf,"NET","VIEW",next ? "GRAPH":"METER");
                invalidate_settings_cache(dock);
            }
            draw_tile(dock,index);
        }
    }
    if (full) FreeVec(full);return ok;
}
static int open_dropped_objects(struct NDDock *dock,int index,
                                const struct AppMessage *message,int *failed)
{
    const struct NDItem *item=&dock->conf.items[index];
    char path[ND_COMMAND_LEN];
    char command[ND_COMMAND_LEN*3];
    int i,kind,opened=0;
    *failed=0;
    for (i=0;i<message->am_NumArgs;++i) {
        if (!build_wb_path(&message->am_ArgList[i],path,sizeof(path),&kind) ||
            !nd_build_openwith_action(command,sizeof(command),item->command,
                                      ITEM_TEXT(item,ND_TEXT_ARGS),path) ||
            !launch_command_at(command,ITEM_TEXT(item,ND_TEXT_WORKDIR),
                               icon_stack(item->command))) {
            ++*failed;
        } else ++opened;
    }
    return opened;
}
/* Edit mode always adds objects. Locked mode routes drops to the tile action,
 * so arranging the dock cannot accidentally launch, copy or trash a file. */
void dropped_icons(struct NDDock *dock)
{
    struct AppMessage *m;
    struct NDConfig *full;
    int i, kind, rc, pos, added,opened,failed;
    char path[ND_COMMAND_LEN], icon[ND_COMMAND_LEN], name[ND_LABEL_LEN];
    char added_name[ND_LABEL_LEN],message[200];
    const char *failure;
    if (dock->drop_preview) {dock->drop_preview=0;draw_all(dock);}
    while (dock->drop_port &&
           (m=(struct AppMessage *)GetMsg(dock->drop_port)) != NULL) {
        if (m->am_Type==AMTYPE_APPMENUITEM) {
            int action=(int)m->am_UserData;
            ReplyMsg((struct Message *)m);
            if (action>=0 && action<dock->action_count &&
                !launch_action(dock,&dock->action_items[action]))
                show_feedback(dock,"Tools Launcher Action failed",1);
            continue;
        }
        pos=hovered_tile(dock,(m->am_MouseX+dock_view_x(dock)),(m->am_MouseY+dock_view_y(dock)));
        if (!dock->edit_mode && is_trash_tile(dock,pos)) {
            /* One helper per object; each move is a rename on its own
             * volume, and the helper reports anything it cannot move. */
            opened=failed=0;
            for (i=0;i<m->am_NumArgs;++i) {
                if (!build_wb_path(&m->am_ArgList[i],path,sizeof(path),&kind) ||
                    !launch_plugin_helper(dock,pos,"PUT",path)) ++failed;
                else ++opened;
            }
            ReplyMsg((struct Message *)m);
            if (opened && !failed) {
                if (opened==1) strcpy(message,nd_tr("Moving to the trash"));
                else sprintf(message,nd_trf("Moving %d items to the trash"),opened);
                show_feedback(dock,message,0);
            } else if (failed)
                show_feedback(dock,nd_tr("Some items could not be moved to the trash"),1);
            continue;
        }
        if(!dock->edit_mode && pos>=0 && pos<dock->conf.count && dock->conf.items[pos].kind==ND_ITEM_PLUGIN &&
           !strcmp(dock->conf.items[pos].id,"BITSKIM")){
            failed=m->am_NumArgs!=1;
            if(!failed)failed=!build_wb_path(&m->am_ArgList[0],path,sizeof(path),&kind) || kind==ND_ITEM_DRAWER;
            if(!failed)failed=!launch_plugin_helper(dock,pos,"SEND",path);
            ReplyMsg((struct Message *)m);
            if(failed)show_feedback(dock,"BitSkim accepts one regular file at a time",1);
            continue;
        }
        if(!dock->edit_mode && pos>=0 && pos<dock->conf.count && dock->conf.items[pos].kind==ND_ITEM_PLUGIN && nd_shelf_plugin_path(dock->conf.items[pos].command)){
            opened=launch_archive_drop(dock,pos,m);ReplyMsg((struct Message *)m);show_feedback(dock,opened ? "Adding shelf shortcuts":"Could not add shelf shortcuts",!opened);continue;
        }
        if(!dock->edit_mode && pos>=0 && pos<dock->conf.count && dock->conf.items[pos].kind==ND_ITEM_PLUGIN && !strcmp(dock->conf.items[pos].id,"TOOLTYPES")){
            opened=m->am_NumArgs==1 && build_wb_path(&m->am_ArgList[0],path,sizeof(path),&kind) && launch_plugin_helper(dock,pos,"EDIT",path);
            ReplyMsg((struct Message *)m);show_feedback(dock,opened ? "Opening icon ToolTypes":"Drop one Workbench icon at a time",!opened);continue;
        }
        if (!dock->edit_mode && drop_extracts_archive(dock,pos)) {
            opened=launch_archive_drop(dock,pos,m) ? m->am_NumArgs:0;
            failed=opened ? 0:m->am_NumArgs;
            ReplyMsg((struct Message *)m);
            if (opened && !failed) {
                if (opened==1) strcpy(message,nd_tr("Archiver opened for dropped item"));
                else sprintf(message,nd_trf("Archiver opened for %d items"),opened);
                show_feedback(dock,message,0);
            } else if (opened) {
                /* Separate catalog strings: plurals differ per language. */
                if (opened==1)
                    sprintf(message,nd_trf("Started 1 archive; %d rejected"),failed);
                else sprintf(message,nd_trf("Started %d archives; %d rejected"),
                             opened,failed);
                show_feedback(dock,message,1);
            } else {
                DisplayBeep(dock->screen);
                show_feedback(dock,"Could not open item in Archiver",1);
            }
            continue;
        }
        if (!dock->edit_mode && drop_opens_launcher(dock,pos,(m->am_MouseX+dock_view_x(dock)),(m->am_MouseY+dock_view_y(dock)))) {
            const char *label=dock->conf.items[pos].label;
            opened=open_dropped_objects(dock,pos,m,&failed);
            ReplyMsg((struct Message *)m);
            if (opened && !failed) {
                if (opened==1) sprintf(message,nd_trf("Opened with %.31s"),label);
                else sprintf(message,nd_trf("Opened %d items with %.31s"),opened,label);
                show_feedback(dock,message,0);
            } else if (opened) {
                if (opened==1)
                    sprintf(message,nd_trf("Opened 1 item; %d failed"),failed);
                else sprintf(message,nd_trf("Opened %d items; %d failed"),
                             opened,failed);
                show_feedback(dock,message,1);
            } else {
                DisplayBeep(dock->screen);
                show_feedback(dock,"Open With failed",1);
            }
            continue;
        }
        if (!dock->edit_mode && drop_copies_into_drawer(dock,pos,(m->am_MouseX+dock_view_x(dock)),(m->am_MouseY+dock_view_y(dock)))) {
            const struct NDItem *target=&dock->conf.items[pos];
            char copy[ND_COMMAND_LEN*5];
            opened=failed=0;
            for (i=0;i<m->am_NumArgs;++i) {
                if (!build_wb_path(&m->am_ArgList[i],path,sizeof(path),&kind) ||
                    !nd_build_copy_command(copy,sizeof(copy),path,
                                           kind==ND_ITEM_DRAWER,target->command) ||
                    !launch_command_console(copy)) ++failed;
                else ++opened;
            }
            ReplyMsg((struct Message *)m);
            if (opened && !failed) {
                if (opened==1)
                    sprintf(message,nd_trf("Copying 1 item to %.28s"),target->label);
                else sprintf(message,nd_trf("Copying %d items to %.28s"),
                             opened,target->label);
                show_feedback(dock,message,0);
            } else if (opened) {
                sprintf(message,nd_trf("Copying %d; %d rejected"),opened,failed);
                show_feedback(dock,message,1);
            } else {
                DisplayBeep(dock->screen);
                show_feedback(dock,"Copy rejected - invalid object",1);
            }
            continue;
        }
        if (!dock->edit_mode) {
            ReplyMsg((struct Message *)m);
            show_feedback(dock,"Dock locked - right-click and choose Unlock dock",1);
            continue;
        }
        if (pos>=0)
            pos=nd_drop_insert_position(pos,
                    drop_after_tile(dock,pos,(m->am_MouseX+dock_view_x(dock)),(m->am_MouseY+dock_view_y(dock))),
                    nd_media_persistent_count(&dock->conf));
        if (pos<0 && dock->overflow) {
            int scroll=hovered_scroll(dock,(m->am_MouseX+dock_view_x(dock)),(m->am_MouseY+dock_view_y(dock)));
            if (scroll<0) pos=dock->view_start;
            else if (scroll>0) {
                pos=dock->view_start+dock->visible_count;
                if (pos>dock->conf.count) pos=dock->conf.count;
            }
        }
        if(dock->write_owner){
            ReplyMsg((struct Message *)m);
            show_feedback(dock,"Preferences are saving - try the drop again",1);
            continue;
        }
        failure="Drop rejected - invalid object";
        full=(struct NDConfig *)AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_CLEAR|MEMF_REVERSE);
        added=0;
        added_name[0]='\0';
        if (full) {
            rc=load_edit_configuration(dock,full);
            if(rc>0)failure="Drop rejected - settings could not be read";
            if (rc <= 0) {
                /* New launchers go before any removable-media tiles. */
                if (pos < 0 || pos > nd_media_persistent_count(&dock->conf))
                    pos=nd_media_persistent_count(&dock->conf);
                for (i=0; i<m->am_NumArgs; ++i) {
                    if (!build_wb_path(&m->am_ArgList[i],path,sizeof(path),&kind)) continue;
                    nd_label_from_path(path,name,sizeof(name));
                    if (!*name) continue;
                    dropped_icon_path(path,kind,icon,sizeof(icon));
                    if (nd_insert_launcher_visible(full,pos,kind,name,path,icon)) {
                        if (!added) {
                            strncpy(added_name,name,sizeof(added_name)-1);
                            added_name[sizeof(added_name)-1]='\0';
                        }
                        ++added; ++pos;
                    } else {
                        failure=full->count>=ND_MAX_ITEMS ?
                            "Dock catalogue is full - remove an unused item":
                            "Drop rejected - item settings are too large";
                        break;
                    }
                }
            }
        }
        /* Workbench owns all WBArg strings; release them before window reload. */
        ReplyMsg((struct Message *)m);
        if (full) {
            if (added && !save_and_reload(dock,&full)) {
                added=0;failure="Could not apply dropped items - see Diagnostics";
            }
            if (!added) {
                DisplayBeep(dock->screen);
                show_feedback(dock,failure,1);
            } else {
                if (added==1) sprintf(message,nd_trf("Added %s"),added_name);
                else sprintf(message,nd_trf("Added %d items"),added);
                show_feedback(dock,message,0);
            }
            if (full) FreeVec(full);
        } else {
            DisplayBeep(dock->screen);
            show_feedback(dock,"Drop rejected - not enough memory",1);
        }
    }
}
/* A click on tile INDEX at window position MX,MY (mouse release on the tile
 * it was pressed on, or Return in keyboard control). */
static void click_tile(struct NDDock *dock,int index,int mx,int my,
                       UWORD qualifier)
{
    struct NDItem *it = &dock->conf.items[index];
    if (it->kind == ND_ITEM_PLUGIN) {
        if(!strcmp(it->id,"PREFERENCES")){(void)launch_companion(dock,"NovaDockPrefs","");return;}
        if((!strcmp(it->id,"NET") || !strcmp(it->id,"ROADSHOW")) && strcmp(it->badge,"*"))return;
        if (queue_music_control(dock,index,mx,my)) {
            /* Delivered with the plugin's next normal update;
             * no extra polling or synchronous ARexx in dock. */
        } else if (!strcmp(it->id,"MUSIC")) {
            /* An online player uses the in-tile controls and
             * keeps the rest of the tile inert.  Offline, the
             * user may nominate a preferred player executable
             * in Music plugin settings and launch it here. */
            if (!it->media_capabilities)
                (void)launch_default_music_application(dock);
        } else if (!strcmp(it->id,"ARCHIVE")) {
            /* Archiver is a drop target. Ordinary
             * clicks must never queue a plugin action. */
        } else if (!strcmp(it->id,"TRASH")) {
            open_trash(dock);
        } else if (!strcmp(it->id,"CLOCK")) {
            open_calendar(dock,index);
        } else if (!strcmp(it->id,"DOPUS4")) {
            request_dopus_action(dock,index);
        } else if (!strcmp(it->id,"RUNNINGAPPS")) {
            if(!launch_plugin_helper(dock,index,"OPEN",NULL))
                show_feedback(dock,"Could not open Running Apps",1);
        } else if (!strcmp(it->id,"SCREENSHOT") || !strcmp(it->id,"SCREENMODE") ||
                   !strcmp(it->id,"SCREENSWITCHER") || !strcmp(it->id,"WEATHER") || !strcmp(it->id,"AMIROCKET") || !strcmp(it->id,"ALERTS") ||
                   !strcmp(it->id,"CHRISTMAS") || !strcmp(it->id,"AQUARIUM") || !strcmp(it->id,"PET") ||
                   !strcmp(it->id,"DOWNLOAD") || !strcmp(it->id,"SYSTEM") || !strcmp(it->id,"PLACES") || !strcmp(it->id,"SAVER") || !strcmp(it->id,"AMINET") || nd_independent_plugin_path(it->command)) {
            /* These tiles perform actions, even if a Shift qualifier is
             * present. A stale graph flag must not turn them into monitors. */
            if(it->graph){it->graph=0;draw_tile(dock,index);}
            if(it->clicks<3)++it->clicks;
        } else if (!strcmp(it->id,"CPU") ||
                   !strcmp(it->id,"RAM") || !strcmp(it->id,"NET") ||
                   !strcmp(it->id,"ROADSHOW")) {
            if (!toggle_monitor_view(dock,index) && dock->screen)
                DisplayBeep(dock->screen);
        } else if ((qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) &&
            strcmp(it->id,"EYES") && strcmp(it->id,"ARCHIVE") &&
            strcmp(it->id,"DOPUS4") && strcmp(it->id,"BITSKIM")) {
            it->graph = !it->graph;
            draw_tile(dock, index);
        } else if (it->clicks < 3) ++it->clicks;
    } else if (dock->conf.left_click == ND_CLICK_MENU)
        open_popup(dock, index, mx, my);
    else launch_item(dock, index);
}

/* ---- Keyboard control ----------------------------------------------------
 * DOCK_HOTKEY (or a cursor key while the dock window is active) highlights
 * a tile; cursor keys move between visible tiles, Return or Space acts as a
 * click, M opens the tile's menu, Esc ends it.  Moving the pointer, or
 * another window becoming active, hands control back to the mouse. */
static int keyboard_focusable(struct NDDock *dock,int index)
{
    return index>=0 && index<dock->conf.count && item_visible(dock,index) &&
           dock->conf.items[index].kind!=ND_ITEM_SPACER;
}

static void keyboard_focus(struct NDDock *dock,int index)
{
    int old=dock->hover;
    close_tooltip(dock);
    dock->hover=index;
    dock->hover_ticks=0;
    if (old!=index) redraw_changed_tiles(dock,old,index,-1);
}

static void keyboard_move(struct NDDock *dock,int step)
{
    int i=dock->hover,n;
    for (n=0;n<dock->conf.count;++n) {
        i=(i+step+dock->conf.count)%dock->conf.count;
        if (keyboard_focusable(dock,i)) {keyboard_focus(dock,i);return;}
    }
}

static void keyboard_end(struct NDDock *dock)
{
    if (!dock->keyboard) return;
    dock->keyboard=0;
    keyboard_focus(dock,-1);
}

void keyboard_begin(struct NDDock *dock)
{
    int i;
    if (!dock->window || !dock->screen || dock->box_pending) return;
    if (dock->hidden) set_hidden(dock,0);
    ActivateWindow(dock->window);
    dock->keyboard=1;
    dock->keyboard_mouse_x=dock->screen->MouseX;
    dock->keyboard_mouse_y=dock->screen->MouseY;
    if (keyboard_focusable(dock,dock->hover)) {
        keyboard_focus(dock,dock->hover);
        return;
    }
    for (i=0;i<dock->conf.count;++i)
        if (keyboard_focusable(dock,i)) {keyboard_focus(dock,i);return;}
    keyboard_end(dock);
}

/* The centre of tile INDEX, in window coordinates. */
static void keyboard_point(struct NDDock *dock,int index,int *mx,int *my)
{
    int x,y,w,h;
    tile_coords(dock,index,&x,&y);
    tile_dimensions(dock,index,&w,&h);
    *mx=x+w/2;*my=y+h/2;
}

/* Keys while the dock window is active; 1 when the key was used. */
static int keyboard_key(struct NDDock *dock,ULONG cls,UWORD code,UWORD qualifier)
{
    int mx,my,index=dock->hover;
    if (cls==IDCMP_RAWKEY) {
        if (code&IECODE_UP_PREFIX) return 0;
        if (code!=0x4C && code!=0x4D && code!=0x4E && code!=0x4F) return 0;
        if (!dock->keyboard) {keyboard_begin(dock);return 1;}
        keyboard_move(dock,code==0x4C || code==0x4F ? -1 : 1);
        return 1;
    }
    if (cls!=IDCMP_VANILLAKEY || !dock->keyboard) return 0;
    if (code==27) {keyboard_end(dock);return 1;}
    if (!keyboard_focusable(dock,index)) return 0;
    keyboard_point(dock,index,&mx,&my);
    if (code==13 || code==' ') {click_tile(dock,index,mx,my,qualifier);return 1;}
    if (code=='m' || code=='M') {close_tooltip(dock);open_popup(dock,index,mx,my);return 1;}
    return 0;
}


/* SELECTDOWN at canvas coordinates MX,MY (see window_events). */
static void select_down_at(struct NDDock *dock,int mx,int my)
{
    int index;
    close_tooltip(dock);
    dock->hover_ticks=0;
    if(hovered_grip(dock,mx,my)){
        dock->grip_drag=1;dock->press_item=-1;dock->is_drag=0;
        dock->grip_start=vertical(dock) ? dock->screen->MouseY:dock->screen->MouseX;
        dock->grip_position=dock->conf.position;dock->grip_offset=dock->conf.position_offset;
        dock->grip_moved=0;
        dock->grip_origin=vertical(dock) ? dock->full_box.top+dock->hover_pad_y-dock->screen->BarHeight:
            dock->full_box.left+dock->hover_pad_x;
        dock->grip_length=(vertical(dock) ? dock->full_box.height:dock->full_box.width)-2*dock->hover_axis_padding;
        return;
    }
    { int scroll=hovered_scroll(dock,mx,my);
      if (scroll) { scroll_view(dock,scroll); return; } }
    index = hovered_tile(dock, mx, my);
    dock->press_item = -1;
    dock->is_drag = 0;
    dock->music_press_action=0;
    dock->orphan_press_ticks=0;
    if (index >= 0) {
        /* Every tile can be picked up. Plugin clicks are deferred to
         * SELECTUP so a drag never triggers the plugin action. */
        dock->press_item = index;
        dock->press_x = mx; dock->press_y = my;
        dock->drag_to = index;
        dock->drag_after = drop_after_tile(dock,index,mx,my);
        if(!begin_music_control(dock,index,mx,my)){
            /* The live music panel has independent controls; its
             * empty space is not a launcher or a tile press. */
            if(!dock->edit_mode && !strcmp(dock->conf.items[index].id,"MUSIC") &&
               dock->conf.items[index].media_capabilities){
                dock->press_item=-1;dock->drag_to=-1;dock->drag_after=0;
            }
            draw_tile(dock,index);
        }
    }
}
/* SELECTUP at canvas coordinates. Returns 1 when the configuration was
 * reloaded, so the caller must stop using the (possibly new) window. */
static int select_up_at(struct NDDock *dock,int mx,int my,UWORD qualifier)
{
    int index;
    if(dock->grip_drag){
        struct NDConfig *full;int coordinate,offset;
        coordinate=vertical(dock) ? dock->screen->MouseY:dock->screen->MouseX;
        move_dock_grip(dock,coordinate);
        if(!dock->grip_moved){dock->grip_drag=0;return 0;}
        offset=dock->conf.position_offset;
        dock->grip_drag=0;
        /* Compare/apply against the saved configuration, not transient drag
         * coordinates. Keep the moved flag until reload chooses a fresh frame. */
        dock->conf.position=dock->grip_position;dock->conf.position_offset=dock->grip_offset;
        full=AllocVec(sizeof(*full),MEMF_PUBLIC|MEMF_REVERSE);
        if(full && load_edit_configuration(dock,full)<=0){
            full->position=ND_POSITION_CUSTOM;full->position_offset=offset;
            if(save_and_reload(dock,&full)){dock->grip_moved=0;return 1;}
        }
        FreeVec(full);dock->grip_moved=0;update_live_window(dock);
        show_feedback(dock,"Could not save dock position",1);return 0;
    }
    if(dock->music_press_action){
        int source=dock->press_item;
        finish_music_control(dock,mx,my);
        dock->press_item=-1;dock->drag_to=-1;dock->drag_after=0;dock->is_drag=0;
        if(dock->conf.hover_magnification)update_hover_pointer(dock);
        draw_tile(dock,source);return 0;
    }
    index = hovered_tile(dock, mx, my);
    if (dock->press_item >= 0) {
        int source = dock->press_item;
        int old_target = dock->drag_to;
        int after=index>=0 ? drop_after_tile(dock,index,mx,my) :
                             dock->drag_after;
        int was_drag = dock->is_drag;
        dock->press_item = -1;
        dock->is_drag = 0;
        dock->drag_to = -1;
        dock->drag_after = 0;
        if (was_drag) draw_all(dock);
        else redraw_changed_tiles(dock,source,old_target,index);
        if (was_drag && index >= 0 && index != source) {
            /* Any visible tile can be rearranged. Clicking happens
             * only when the pointer is released on the source tile. */
            do_move(dock, source, index, after);
            return 1; /* Reload may have replaced the Intuition window. */
        } else if (was_drag && hovered_scroll(dock,mx,my)) {
            scroll_view(dock,hovered_scroll(dock,mx,my));
            show_feedback(dock,"Page changed - drag the item to its destination",0);
        } else if (was_drag && index<0 &&
                   hovered_dock_row(dock,mx,my)>=0) {
            int target_row=hovered_dock_row(dock,mx,my);
            do_move_to_empty_space(dock,source,target_row);
            return 1;
        } else if (was_drag && index < 0 &&
                   (mx<0 || my<0 || mx>=dock_width(dock) ||
                    my>=dock_height(dock))) {
            /* Releasing outside disables the tile. Preferences keeps
             * its definition so it can be added again later. */
            remove_dragged_item(dock, source);
            return 1;
        } else if (was_drag && index<0) {
            show_feedback(dock,"No change - drop on a tile or outside to remove",1);
        } else if (!was_drag && index == source) {
            click_tile(dock,index,mx,my,qualifier);
        }
    }
    return 0;
}
/* Replay button events deferred while a window-box change was pending.
 * Returns 1 when a release reloaded the configuration. */
static int replay_deferred_buttons(struct NDDock *dock)
{
    struct NDDeferredButtons queue=dock->deferred_buttons;
    int i;
    nd_deferred_clear(&dock->deferred_buttons);
    for(i=0;i<queue.count;++i){
        const struct NDDeferredButton *e=&queue.event[i];
        int mx=e->x-dock_left(dock),my=e->y-dock_top(dock);
        if(!dock->window || dock->box_pending)return 0;
        if(dock->hidden){
            /* Auto-hide finished first: a click only reveals the dock. */
            if(e->code==ND_DEFERRED_DOWN)set_hidden(dock,0);
            continue;
        }
        if(e->code==ND_DEFERRED_DOWN)select_down_at(dock,mx,my);
        else if(select_up_at(dock,mx,my,e->qualifier))return 1;
    }
    return 0;
}
/* Install the geometry of a finished ChangeWindowBox() and replay the input
 * that arrived meanwhile.  Also used by the dock timer when Intuition never
 * acknowledges the exact target box (a clamped or superseded change), so the
 * dock cannot stay frozen with box_pending set.  1 = the dock was closed. */
int complete_window_box(struct NDDock *dock)
{
    int viewport_change=dock->viewport_pending;
    if(!dock->window || !dock->box_pending)return 0;
    dock->hidden = dock->box_target_hidden;
    dock->viewport_pending=0;apply_dock_viewport(dock);
    refresh_drop_zones(dock);
    dock->box_pending = 0;dock->box_pending_ticks=0;dock->box_late_ack=0;
    if(!dock->hidden && !viewport_change) {
        refresh_volume_meters(dock);
        refresh_running_items(dock);
    }
    if(!viewport_change){dock->hover = -1;dock->hover_ticks = 0;}
    dock->idle_ticks = 0;
    /* Paint the frame whose resize just finished. Advancing the ease here
     * requests another resize before painting and leaves old pixels moving
     * through every intermediate box. Only the next animation tick advances. */
    dock->hover_box_paint=viewport_change;
    if(viewport_change&&dock->hover_shelf_valid)draw_hover(dock);else draw_all(dock);
    dock->hover_box_paint=0;
    if (dock->deferred_buttons.count && replay_deferred_buttons(dock)) return 1;
    if (dock->deferred_menu && dock->window && !dock->box_pending) {
        int mx=dock->screen->MouseX-dock_left(dock),my=dock->screen->MouseY-dock_top(dock);
        dock->deferred_menu=0;
        if (dock->hidden) set_hidden(dock,0);
        else open_popup(dock,hovered_tile(dock,mx,my)>=0 ? hovered_tile(dock,mx,my):-2,mx,my);
    }
    return 0;
}
/* Advance the pointer epoch; it only moves backwards when the system clock
 * was set back, i.e. the current time is before the epoch
 * (nd_pointer_epoch_update). */
void note_pointer_epoch(struct NDDock *dock,ULONG secs,ULONG micros)
{
    ULONG now_secs=0,now_micros=0;
    if(nd_pointer_event_stale(secs,micros,dock->pointer_epoch_secs,dock->pointer_epoch_micros))
        CurrentTime(&now_secs,&now_micros);   /* only needed to spot a clock reset */
    else now_secs=secs,now_micros=micros;
    if(nd_pointer_epoch_update(dock->pointer_epoch_secs,dock->pointer_epoch_micros,secs,micros,
                               now_secs,now_micros)){
        dock->pointer_epoch_secs=secs;dock->pointer_epoch_micros=micros;
    }
}
void note_pointer_epoch_now(struct NDDock *dock)
{
    ULONG secs,micros;
    CurrentTime(&secs,&micros);
    note_pointer_epoch(dock,secs,micros);
}
void window_events(struct NDDock *dock)
{
    struct IntuiMessage *msg;
    int desktop_elapsed=0;
    ULONG cls, code;
    UWORD qualifier;
    WORD mx, my, rx, ry;
    ULONG secs, micros;
    int index,hover_redraw=0;
    while ((msg = (struct IntuiMessage *)GetMsg(dock->window->UserPort)) != NULL) {
        cls = msg->Class;
        code = msg->Code;
        qualifier = msg->Qualifier;
        secs = msg->Seconds;
        micros = msg->Micros;
        rx = msg->MouseX;
        ry = msg->MouseY;
        mx = msg->MouseX+dock_view_x(dock);
        my = msg->MouseY+dock_view_y(dock);
        if(cls==IDCMP_MOUSEBUTTONS || cls==IDCMP_ACTIVEWINDOW || cls==IDCMP_INACTIVEWINDOW)
            ND_TRACE_NOTE("dock.pointer.event",(LONG)cls,code,mx,my);
        ReplyMsg((struct Message *)msg);
        /* Activation, a window-box change or a layer scroll can leave queued
         * pointer events whose window-relative coordinates no longer match
         * the window.  Only those older events are resolved at the live
         * pointer; a click processed late keeps the position where the user
         * actually clicked (nd_input_policy.h). */
        if(dock->screen && dock->window &&
           (cls==IDCMP_MOUSEBUTTONS || cls==IDCMP_MOUSEMOVE) &&
           nd_pointer_event_stale(secs,micros,dock->pointer_epoch_secs,
                                  dock->pointer_epoch_micros)){
            mx=dock->screen->MouseX-dock_left(dock);
            my=dock->screen->MouseY-dock_top(dock);
        }
        if (cls == IDCMP_CHANGEWINDOW) {
            /* Events generated before this change used the old geometry. */
            note_pointer_epoch(dock,secs,micros);
            if (!dock->box_pending && dock->box_late_ack) {
                /* The change the timeout gave up on has landed after all. */
                dock->box_late_ack=0;
                apply_dock_viewport(dock);refresh_drop_zones(dock);draw_all(dock);
            }
            if (dock->box_pending) {
                const struct NDRect *target=&dock->window_box_target;
                if (dock->window->LeftEdge == target->left &&
                    dock->window->TopEdge == target->top &&
                    dock->window->Width == target->width &&
                    dock->window->Height == target->height &&
                    complete_window_box(dock)) return;
            }
        } else if (cls == IDCMP_REFRESHWINDOW) {
            BeginRefresh(dock->window);
            dock->hover_prev_valid=0;   /* damaged: present the whole frame */
            if(dock->hover_headroom && dock->hover_shelf_valid)draw_hover(dock);
            else draw_all(dock);
            EndRefresh(dock->window, TRUE);
        } else if (cls == IDCMP_MOUSEMOVE) {
            int old_hover,after=0;
            if (dock->hidden || dock->box_pending) continue;
            if(dock->grip_drag){
                move_dock_grip(dock,vertical(dock) ? dock->screen->MouseY:dock->screen->MouseX);
                continue;
            }
            if(dock->music_press_action){drag_music_control(dock,mx);continue;}
            if (dock->keyboard && dock->screen &&
                (dock->screen->MouseX!=dock->keyboard_mouse_x ||
                 dock->screen->MouseY!=dock->keyboard_mouse_y))
                dock->keyboard=0;   /* the pointer takes over */
            if (dock->keyboard) continue;
            index = dock->edit_mode ? hovered_tile(dock,mx,my):pointer_hover_target(dock,mx,my);
            if(update_music_control_hover(dock,index,mx,my)){
                if(dock->hover_headroom)hover_redraw=1;
                else redraw_changed_tiles(dock,dock->hover,index,-1);
            }
            if (index>=0) after=drop_after_tile(dock,index,mx,my);
            if (dock->edit_mode && dock->press_item >= 0 &&
                !dock->conf.items[dock->press_item].transient &&
                (mx - dock->press_x > 4 || dock->press_x - mx > 4 ||
                 my - dock->press_y > 4 || dock->press_y - my > 4)) {
                if (index != dock->drag_to || after != dock->drag_after ||
                    !dock->is_drag) {
                    int starting=!dock->is_drag;
                    dock->is_drag = 1;
                    dock->drag_to = index;
                    dock->drag_after = after;
                    draw_all(dock);
                    if (starting)
                        show_feedback(dock,
                          "Edit: release on a tile to move; outside dock removes",0);
                }
            }
            if(dock->hover_headroom && index==dock->hover &&
               nd_hover_axis(vertical(dock) ? my:mx,dock->hover_pointer)!=dock->hover_pointer){
                dock->hover_pointer=nd_hover_axis(vertical(dock) ? my:mx,dock->hover_pointer);
                hover_redraw=1;
            }
            if (index != dock->hover) {
                close_tooltip(dock);
                dock->hover_ticks=0;
                old_hover = dock->hover;
                dock->hover = index;
                if(dock->hover_headroom)hover_redraw=1;
                else redraw_changed_tiles(dock, old_hover, index, -1);
            }
        } else if (cls == IDCMP_MOUSEBUTTONS &&
                   (code == SELECTDOWN || code == SELECTUP)) {
            if (code == SELECTDOWN && dock->hidden) { set_hidden(dock, 0); continue; }
            if (dock->box_pending) {
                /* The geometry is changing asynchronously.  Keep the event in
                 * screen coordinates and replay it on IDCMP_CHANGEWINDOW;
                 * dropping a release would leave the dock pressed.  Until
                 * that acknowledgement every queued event is relative to the
                 * window origin before the change (or stale: live pointer). */
                int sx,sy;
                if (nd_pointer_event_stale(secs,micros,dock->pointer_epoch_secs,
                                           dock->pointer_epoch_micros)) {
                    sx=dock->screen->MouseX;sy=dock->screen->MouseY;
                } else {
                    sx=rx+dock->box_origin_x;sy=ry+dock->box_origin_y;
                }
                (void)nd_deferred_add(&dock->deferred_buttons,
                    code==SELECTDOWN ? ND_DEFERRED_DOWN:ND_DEFERRED_UP,
                    sx,sy,qualifier);
                continue;
            }
            if (dock->hidden) continue;
            if (code == SELECTDOWN) select_down_at(dock,mx,my);
            else if (select_up_at(dock,mx,my,qualifier)) return;
        } else if (cls == IDCMP_MOUSEBUTTONS && code == MENUDOWN) {
            /* Always hit-test the live screen pointer. The message-relative
             * coordinates can still be from the formerly active window. */
            if (dock->screen && dock->window) {
                mx=dock->screen->MouseX-dock_left(dock);
                my=dock->screen->MouseY-dock_top(dock);
            }
            dock->activate_popup_pending=0;
            close_tooltip(dock);
            if (dock->hidden) { set_hidden(dock, 0); continue; }
            if (dock->box_pending) {
                /* Open the menu once the new geometry is in place. */
                dock->deferred_menu=1;
                continue;
            }
            dock->grip_drag=0;
            index = hovered_grip(dock,mx,my) ? -1:hovered_tile(dock, mx, my);
            if (index >= 0) open_popup(dock,index,mx,my);
            else open_popup(dock, -2, mx, my);
        } else if (cls == IDCMP_INTUITICKS) {
            if(!dock->timer_open)desktop_elapsed+=100;
            /* This is also the fallback when timer.device is unavailable. */
            if (!dock->app_window && !dock->timer_open) {
                if (dock->drop_attach_ticks < 30) ++dock->drop_attach_ticks;
                else attach_appwindow(dock);
            }
            if (!dock->timer_open && ++dock->volume_timer_ticks >= 50) {
                dock->volume_timer_ticks=0;
                refresh_volume_meters(dock);
                flush_dirty_tiles(dock);
            }
            if (!dock->timer_open && ++dock->running_timer_ticks>=10) {
                dock->running_timer_ticks=0;
                refresh_running_items(dock);
                flush_dirty_tiles(dock);
            }
            if (!dock->timer_open && dock->feedback_ticks>0 &&
                --dock->feedback_ticks==0) close_feedback(dock);
            if (!dock->timer_open) {
                always_on_top_tick(dock,1);
                plugin_watchdog_tick(dock,1);
                fallback_recovery_tick(dock);
            }
            hover_tick(dock,ND_TIP_INTUI_TICKS);
        } else if (cls == IDCMP_ACTIVEWINDOW) {
            UWORD live_qualifier=qualifier;
            note_pointer_epoch(dock,secs,micros);
            if (InputBase) live_qualifier|=PeekQualifier();
            /* Intuition consumes the first button press when it activates an
             * inactive window. Defer the popup until the relative-mouse
             * position has reached Intuition on the next tick. */
            if ((live_qualifier & IEQUALIFIER_RBUTTON) && !dock->hidden &&
                !dock->box_pending && !dock->popup)
                dock->activate_popup_pending=1;
        } else if (cls == IDCMP_INACTIVEWINDOW) {
            close_tooltip(dock); dock->hover_ticks=0;
            if (dock->popup) dock->activate_popup_pending=0;
            /* A menu or calendar opened from the keyboard keeps the focus
             * tile; anything else ends keyboard control. */
            if (!dock->popup && !dock->calendar && !dock->folder_popup)
                keyboard_end(dock);
        } else if ((cls == IDCMP_RAWKEY || cls == IDCMP_VANILLAKEY) &&
                   keyboard_key(dock,cls,code,qualifier)) {
            /* handled by keyboard control */
        } else if (cls == IDCMP_VANILLAKEY && (code == 27 || code == 'q')) {
            dock->running = 0;
        }
    }
    if(hover_redraw && dock->window && !dock->hidden && !dock->box_pending)draw_hover(dock);
    if(desktop_elapsed){desktop_scene_tick(dock,desktop_elapsed);desktop_startup_tick(dock,desktop_elapsed);}
}
