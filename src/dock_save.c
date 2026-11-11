#include "dock_internal.h"
int init_settings_writer(struct NDDock *dock)
{
    dock->save_port=CreateMsgPort();if(!dock->save_port)return 0;
    sprintf(dock->save_port_name,"NovaDock.Save.%08lx",(ULONG)dock);
    dock->save_port->mp_Node.ln_Name=dock->save_port_name;
    AddPort(dock->save_port);return 1;
}
int load_edit_configuration(struct NDDock *dock,struct NDConfig *full)
{
    if(dock->edit_config){memcpy(full,dock->edit_config,sizeof(*full));return 0;}
    return nd_load_effective(full);
}
/* A configuration buffer that is neither the newest edit nor the snapshot
 * being written is no longer needed. */
static void release_config(struct NDDock *dock,struct NDConfig *cfg)
{
    if(cfg && cfg!=dock->save_snapshot && cfg!=dock->edit_config)FreeVec(cfg);
}
/* Take ownership of COPY as the newest unsaved configuration.  The edit it
 * replaces is freed unless SaveConfig is still writing it. */
void accept_settings_save(struct NDDock *dock,struct NDConfig *copy)
{
    struct NDConfig *old=dock->edit_config;
    dock->edit_config=copy;
    release_config(dock,old);
    nd_save_changed(&dock->save_state);
}
int queue_settings_save(struct NDDock *dock,struct NDConfig **full)
{
    if(!full || !*full || dock->write_owner || !dock->save_port)return 0;
    accept_settings_save(dock,*full);*full=NULL;return 1;
}
/* The snapshot is finished with (written, failed or reclaimed). */
static void drop_snapshot(struct NDDock *dock)
{
    struct NDConfig *snapshot=dock->save_snapshot;
    dock->save_snapshot=NULL;dock->save_lent=0;
    release_config(dock,snapshot);
}
static void start_save(struct NDDock *dock)
{
    char path[512],command[1100];BPTR tool;APTR requesters;
    if(!dock->save_port || !dock->edit_config || dock->save_state.active)return;
    /* No copy: the edit buffer itself becomes the snapshot.  A later edit
     * gets a new buffer (accept_settings_save), so the snapshot never
     * changes while SaveConfig reads it. */
    dock->save_snapshot=dock->edit_config;dock->save_lent=0;
    nd_save_begin(&dock->save_state);dock->save_claimed=0;
    dock->save_started_at=diagnostic_ticks();
    strcpy(path,dock->directory);
    if(!AddPart(path,"Tools/SaveConfig",sizeof(path)))goto failed;
    requesters=quiet_requesters();
    tool=Lock((STRPTR)path,ACCESS_READ);
    if(!tool) {
        strcpy(path,dock->directory);
        if(AddPart(path,"SaveConfig",sizeof(path))) tool=Lock((STRPTR)path,ACCESS_READ);
    }
    restore_requesters(requesters);
    if(!tool)goto failed;UnLock(tool);
    if(!nd_quote_path(command,sizeof(command)-80,path))goto failed;
    strcat(command," ");strcat(command,dock->save_port_name);
    if(launch_command(command))return;
failed:
    drop_snapshot(dock);
    nd_save_complete(&dock->save_state,0);
    dock->save_env_ok=dock->save_envarc_ok=0;
    dock->save_last_ticks=dock->save_env_ticks=dock->save_envarc_ticks=0;
    show_feedback(dock,"Settings save failed - see Diagnostics",1);
    fire_hook(dock,"SAVE_FAILED","");
}
void process_settings_writer(struct NDDock *dock)
{
    struct NDSaveMessage *m;
    if(!dock->save_port)return;
    while((m=(struct NDSaveMessage *)GetMsg(dock->save_port))!=NULL){
        if(m->msg.mn_Length!=sizeof(*m) || m->magic!=ND_SAVE_MAGIC){
            if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);
            continue;
        }
        if(m->kind==ND_SAVE_REQUEST || m->kind==ND_SAVE_BORROW){
            m->config=NULL;
            if(dock->save_state.active && !dock->save_claimed && dock->save_snapshot &&
               m->config_size==sizeof(struct NDConfig)){
                if(m->kind==ND_SAVE_BORROW){
                    /* This SaveConfig only reads the snapshot and returns
                     * it with COMPLETE; the dock still owns it. */
                    m->config=dock->save_snapshot;dock->save_lent=1;
                }else{
                    /* An older SaveConfig frees what it receives. */
                    struct NDConfig *copy=AllocVec(sizeof(*copy),MEMF_PUBLIC|MEMF_REVERSE);
                    if(copy){memcpy(copy,dock->save_snapshot,sizeof(*copy));m->config=copy;}
                    dock->save_lent=0;
                }
                if(m->config){
                    dock->save_claimed=1;
                    m->revision=dock->save_state.active_revision;
                }
            }
            ReplyMsg((struct Message *)m);
        }else if(m->kind==ND_SAVE_COMPLETE){
            if(dock->save_state.active && dock->save_claimed &&
               m->revision==dock->save_state.active_revision){
                dock->save_last_ticks=m->elapsed;dock->save_total_ticks+=m->elapsed;
                dock->save_env_ticks=m->env_ticks;dock->save_envarc_ticks=m->envarc_ticks;
                dock->save_env_ok=m->env_ok;dock->save_envarc_ok=m->envarc_ok;
                nd_save_complete(&dock->save_state,m->env_ok && m->envarc_ok);
                dock->save_claimed=0;
                drop_snapshot(dock);
                if(dock->save_state.revision==dock->save_state.saved){
                    dock->save_quit_refused=0;
                    FreeVec(dock->edit_config);dock->edit_config=NULL;
                }else if(!m->env_ok || !m->envarc_ok){
                    show_feedback(dock,"Settings not saved - will retry",1);
                    fire_hook(dock,"SAVE_FAILED","");
                }
            }
            FreeVec(m);
        }
    }
}
void settings_save_tick(struct NDDock *dock,int quantum,int force)
{
    int busy;
    process_settings_writer(dock);
    if(dock->write_owner && diagnostic_ticks()-dock->write_deadline<0x80000000UL)
        dock->write_owner=NULL;
    /* A failed executable lookup may still have returned a successful shell
     * launch. Unclaimed snapshots can be reclaimed; claimed ones belong to
     * the independent writer until its COMPLETE message arrives. */
    if(dock->save_state.active && !dock->save_claimed &&
       diagnostic_ticks()-dock->save_started_at>1500UL){
        drop_snapshot(dock);
        nd_save_complete(&dock->save_state,0);
        dock->save_env_ok=dock->save_envarc_ok=0;
        dock->save_last_ticks=dock->save_env_ticks=dock->save_envarc_ticks=0;
    }
    busy=!force && (dock->press_item>=0 || dock->is_drag || dock->popup ||
                    dock->calendar || dock->folder_popup);
    if(force && !(dock->save_state.last_failed && dock->save_state.quiet>ND_SAVE_IDLE_TICKS))
        dock->save_state.quiet=0;
    if(nd_save_tick(&dock->save_state,quantum,busy))start_save(dock);
}
/* While the last save finishes at quit, keep the dock painted: answer
 * refresh requests and ignore other input. */
static void quit_wait_refresh(struct NDDock *dock)
{
    struct IntuiMessage *m;
    if (!dock->window) return;
    while ((m=(struct IntuiMessage *)GetMsg(dock->window->UserPort))!=NULL) {
        ULONG cls=m->Class;
        ReplyMsg((struct Message *)m);
        if (cls==IDCMP_REFRESHWINDOW) {
            BeginRefresh(dock->window);draw_all(dock);
            EndRefresh(dock->window,TRUE);
        }
    }
}
int finish_settings_saves(struct NDDock *dock)
{
    int told=0,step;
    ULONG start=diagnostic_ticks();
    dock->write_owner=NULL;
    while(dock->save_state.revision!=dock->save_state.saved){
        settings_save_tick(dock,1,1);
        /* Policy in save_policy.c (host-tested).  A snapshot SaveConfig has
         * claimed is its own to finish or free when the dock leaves. */
        step=nd_save_quit_step(&dock->save_state,diagnostic_ticks()-start,
                               dock->save_quit_refused);
        if(step==ND_SAVE_QUIT_LEAVE)return 1;
        if(step==ND_SAVE_QUIT_REFUSE){
            dock->save_quit_refused=1;
            dock->running=1;
            show_feedback(dock,"Settings not saved - quit again to discard changes",1);
            return 0;
        }
        if (!told && diagnostic_ticks()-start>25UL) {
            show_feedback(dock,"Saving settings...",0);told=1;
        }
        quit_wait_refresh(dock);
        process_ipc(dock,0);process_control(dock,1);Delay(1);
    }
    return 1;
}
void close_settings_writer(struct NDDock *dock)
{
    struct NDConfig *busy;
    if(dock->save_port){
        RemPort(dock->save_port);process_settings_writer(dock);
        DeleteMsgPort(dock->save_port);dock->save_port=NULL;
    }
    /* A snapshot lent to a SaveConfig that is still writing belongs to that
     * write until it ends; leave it rather than free memory being read. */
    busy=dock->save_claimed && dock->save_lent ? dock->save_snapshot : NULL;
    if(dock->save_snapshot && dock->save_snapshot!=busy &&
       dock->save_snapshot!=dock->edit_config)FreeVec(dock->save_snapshot);
    if(dock->edit_config && dock->edit_config!=busy)FreeVec(dock->edit_config);
    dock->save_snapshot=dock->edit_config=NULL;dock->save_lent=0;
}
