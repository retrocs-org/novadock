/* Timed support recording with automatic plugin resource samples.
 * Runs independently of the dock so a dock crash does not lose the deadline. */
#define GadToolsBase NDCaptureGadToolsBase
#include "nd_build_id.h"
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <libraries/gadtools.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "nd_support.h"
#include "nd_control_client.h"
#include "nd_cpu_info.h"
#include "nd_version.h"
extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
struct Library *GadToolsBase;
static struct NDDiagnosticsSnapshot initial,final;
static ULONG list_count(struct List *list)
{struct Node *n;ULONG count=0;for(n=list->lh_Head;n->ln_Succ;n=n->ln_Succ)++count;return count;}
static void resources(ULONG *ports,ULONG *tasks)
{Forbid();*ports=list_count(&SysBase->PortList);*tasks=list_count(&SysBase->TaskReady)+list_count(&SysBase->TaskWait);Permit();}
static int owner_is(ULONG owner)
{FILE *f;unsigned long value;int ok;f=fopen(ND_SUPPORT_CAPTURE_OWNER,"r");if(!f)return 0;ok=fscanf(f,"%lu",&value)==1&&value==owner;fclose(f);return ok;}
static int save_measurement(const char *name,unsigned int seconds,int complete,
    ULONG chip,ULONG fast,ULONG ports,ULONG tasks,ULONG min_chip,ULONG min_fast,struct Screen *screen)
{
    FILE *f;char path[256];ULONG end_ports,end_tasks;struct Library *wb;int ok=1;
    sprintf(path,"%s/%s",ND_SUPPORT_DISK,name);f=fopen(path,"w");if(!f)return 0;
    if(fprintf(f,"NovaDock capture 1 completed=%d seconds=%u end=%lu\nNovaDock=" ND_VERSION " Build=" ND_BUILD_ID " CPU=%s Exec=%u.%u\n",
        complete,seconds,(ULONG)time(NULL),nd_system_cpu(SysBase->AttnFlags),SysBase->LibNode.lib_Version,SysBase->LibNode.lib_Revision)<0)ok=0;
    wb=OpenLibrary("workbench.library",0);if(wb){if(fprintf(f,"Workbench=%u.%u\n",wb->lib_Version,wb->lib_Revision)<0)ok=0;CloseLibrary(wb);}
    if(fprintf(f,"Screen=%ux%u Depth=%lu DisplayID=%08lx\nChip_start=%lu Chip_end=%lu Chip_min=%lu\nFast_start=%lu Fast_end=%lu Fast_min=%lu\n",
        (unsigned int)screen->Width,(unsigned int)screen->Height,GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH),GetVPModeID(&screen->ViewPort),
        chip,AvailMem(MEMF_CHIP),min_chip,fast,AvailMem(MEMF_FAST),min_fast)<0)ok=0;
    resources(&end_ports,&end_tasks);
    if(fprintf(f,"Ports_start=%lu Ports_end=%lu Tasks_start=%lu Tasks_end=%lu\nDock_start_telemetry=%lu Dock_end_telemetry=%lu\n",
        ports,end_ports,tasks,end_tasks,initial.settings_telemetry_version,final.settings_telemetry_version)<0)ok=0;
    if(initial.counter_epoch && initial.counter_epoch==final.counter_epoch && final.uptime_ticks>=initial.uptime_ticks){
        if(fprintf(f,"Elapsed_ticks=%lu Wakes=%lu Full_paints=%lu Tile_paints=%lu Updates=%lu\nReloads=%lu Reload_total_ticks=%lu Reload_last_ticks=%lu\nFrame_average_us=%lu Frame_peak_us=%lu Frame_samples=%lu Interval_ms=%u\nCache_bytes=%lu Cache_budget=%lu Cache_evictions=%lu\n",
            final.uptime_ticks-initial.uptime_ticks,final.wake_count-initial.wake_count,final.paint_count-initial.paint_count,
            final.tile_count-initial.tile_count,final.update_count-initial.update_count,final.reload_requests-initial.reload_requests,
            final.reload_total_ticks-initial.reload_total_ticks,final.reload_last_ticks,final.frame_average_us,final.frame_peak_us,
            final.frame_samples-initial.frame_samples,final.frame_interval_ms,final.hover_cache_bytes,final.hover_cache_budget,final.hover_cache_evictions)<0)ok=0;
    }else if(fputs("Dock restarted or was unavailable: counter deltas omitted; inspect trace sessions.\n",f)==EOF)ok=0;
    if(final.aminotis_version==1 && fprintf(f,
        "AmiNotis_enabled=%u Received=%lu Delivered=%lu Dropped=%lu Queued=%u Pending=%u\nAmiNotis_unavailable=%lu Queue_full=%lu Timeouts=%lu Rejected=%lu Cancelled=%lu Last_failure=%s Last_result=%u\n",
        final.aminotis_enabled,final.aminotis.received,final.aminotis.delivered,final.aminotis.dropped,final.aminotis.queued,final.aminotis.pending,
        final.aminotis.unavailable,final.aminotis.queue_full,final.aminotis.timeouts,final.aminotis.rejected,final.aminotis.cancelled,ND_AMINOTIS_FAILURE_NAME(final.aminotis.last_failure),final.aminotis.last_result)<0)ok=0;
    if(fclose(f))ok=0;return ok;
}
/* Keep the prior measurement for reports created after a crash or restart. */
static int retain_measurement(const char *name,const char *previous)
{
    char source[256],target[256];BPTR lock;
    sprintf(source,"%s/%s",ND_SUPPORT_DISK,name);sprintf(target,"%s/%s",ND_SUPPORT_DISK,previous);
    lock=Lock(source,ACCESS_READ);if(!lock)return 1;UnLock(lock);
    lock=Lock(target,ACCESS_READ);if(lock){UnLock(lock);if(!DeleteFile(target))return 0;}
    return Rename(source,target)!=0;
}
static int save_plugin_sample(ULONG elapsed)
{
    struct NDControlPluginResourcesMessage q;char path[256];FILE *f;unsigned int i,pages=0;int ok=1;
    sprintf(path,"%s/plugin-samples.txt",ND_SUPPORT_DISK);f=fopen(path,"a");if(!f)return 0;
    memset(&q,0,sizeof(q));
    do{
        if(nd_control_plugin_resources(&q)!=ND_CONTROL_OK){ok=0;break;}
        for(i=0;i<q.count;++i){const struct NDPluginResourceRow *row=&q.rows[i];const struct NDPluginSupportStats *v=&row->stats;
            if(fprintf(f,"elapsed_seconds=%lu slot=%u online=%u stale=%u stats_version=%u age_ticks=%lu heap=%lu peak=%lu partial=%u alloc=%lu free=%lu failures=%lu op=%u phase=%u events=%lu open=%lu read=%lu write=%lu dirs=%lu read_bytes=%lu write_bytes=%lu a=%ld b=%ld cpu_valid=%u cpu_hits=%lu cpu_samples=%lu sample_age=%lu\n",
                elapsed,row->reserved,row->online,row->stale,v->version,row->age_ticks,v->heap,v->peak,v->flags&1U,v->allocations,v->frees,v->failures,v->operation,v->phase,v->events,v->opens,v->reads,v->writes,v->directories,v->read_bytes,v->write_bytes,v->detail_a,v->detail_b,row->cpu_valid,row->cpu_hits,row->cpu_samples,q.sample_age)<0)ok=0;
        }q.offset+=q.count;
    }while(q.count && q.offset<q.total && ++pages<32);
    if(fclose(f))ok=0;return ok;
}
int nd_support_capture(unsigned int seconds,int guided)
{
    struct NDSupportSettings previous,capture;struct Window *win=NULL;struct Screen *screen=NULL;
    struct Gadget *gadgets=NULL,*last;struct NewGadget ng;struct IntuiMessage *msg;APTR visual=NULL;
    struct MsgPort *guard=NULL;struct EasyStruct request={sizeof(request),0,"NovaDock support recording","%s","OK"};
    const char *message;char text[160];FILE *f;BPTR lock;ULONG owner,start,now,chip,fast,min_chip,min_fast,ports,tasks;
    int done=0,complete=0,overridden=0,rc=20,last_second=-1,remaining,saved=0,sampling=0,sample_error=0;ULONG next_sample=0,last_probe=~0UL;
    struct NDControlPluginResourcesMessage sample;ULONG cls;UWORD id,code;struct DrawInfo *info;
    if(!seconds || seconds>60)return 20;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);
    if(!IntuitionBase||!GfxBase||!GadToolsBase)goto finish;
    guard=CreateMsgPort();if(!guard)goto finish;guard->mp_Node.ln_Name="NovaDock.SupportCapture";
    Forbid();if(FindPort(guard->mp_Node.ln_Name)){Permit();DeleteMsgPort(guard);guard=NULL;message="A support recording is already running.";goto notice;}
    AddPort(guard);Permit();screen=LockPubScreen("Workbench");if(!screen)goto finish;
    visual=GetVisualInfo(screen,TAG_DONE);if(!visual||!CreateContext(&gadgets))goto finish;
    memset(&ng,0,sizeof(ng));ng.ng_LeftEdge=12;ng.ng_TopEdge=screen->RastPort.TxHeight*4+20;ng.ng_Width=140;ng.ng_Height=screen->RastPort.TxHeight+8;
    ng.ng_GadgetText=guided?"Continue now":"Stop recording";ng.ng_GadgetID=1;ng.ng_VisualInfo=visual;ng.ng_TextAttr=screen->Font;ng.ng_Flags=PLACETEXT_IN;
    last=CreateGadget(BUTTON_KIND,gadgets,&ng,TAG_DONE);if(!last)goto finish;
    if(guided){struct NewGadget skip=ng;skip.ng_LeftEdge=160;skip.ng_Width=170;skip.ng_GadgetID=2;skip.ng_GadgetText="Use existing logs";
        last=CreateGadget(BUTTON_KIND,last,&skip,TAG_DONE);if(!last)goto finish;}
    win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,WA_Left,10,WA_Top,screen->BarHeight+12,
        WA_Width,screen->Width>600?580:screen->Width-20,WA_Height,ng.ng_TopEdge+ng.ng_Height+12,
        WA_Title,(ULONG)(guided?"NovaDock report - recording":"NovaDock support recording"),WA_Gadgets,(ULONG)gadgets,
        WA_DragBar,TRUE,WA_CloseGadget,TRUE,WA_DepthGadget,TRUE,WA_IDCMP,BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Flags,WFLG_SMART_REFRESH,TAG_DONE);if(!win)goto finish;
    nd_support_load(&previous,0);capture.level=2;capture.persist=1;
    lock=CreateDir("ENV:NovaDock");if(lock)UnLock(lock);lock=CreateDir(ND_SUPPORT_DISK);if(lock)UnLock(lock);
    if(!nd_support_write(ND_SUPPORT_ENV,&capture)){message=nd_support_error();goto notice;}
    owner=(ULONG)FindTask(NULL);start=(ULONG)time(NULL);
    f=fopen(ND_SUPPORT_CAPTURE_OWNER,"w");if(!f){nd_support_write(ND_SUPPORT_ENV,&previous);message="Could not start the automatic stop timer. Recording was not started.";goto notice;}
    saved=fprintf(f,"%lu %lu %d %d\n",owner,start+seconds+2,previous.level,previous.persist)>0;
    if(fclose(f))saved=0;
    if(!saved){nd_support_write(ND_SUPPORT_ENV,&previous);message="Could not save the recording timer.";goto notice;}
    if(!retain_measurement("capture.txt","capture.previous.txt") ||
       !retain_measurement("plugin-samples.txt","plugin-samples.previous.txt")){
        nd_support_write(ND_SUPPORT_ENV,&previous);message="Could not preserve the previous recording. Existing logs are still available.";goto notice;
    }
    (void)nd_control_send(ND_CONTROL_SUPPORT_FLUSH);Delay(25);
    memset(&initial,0,sizeof(initial));if(nd_control_diagnostics(&initial)!=ND_CONTROL_OK)memset(&initial,0,sizeof(initial));
    chip=min_chip=AvailMem(MEMF_CHIP);fast=min_fast=AvailMem(MEMF_FAST);resources(&ports,&tasks);
    memset(&sample,0,sizeof(sample));sample.operation=1;
    sampling=nd_control_plugin_resources(&sample)==ND_CONTROL_OK;
    next_sample=start+10;
    GT_RefreshWindow(win,NULL);
    while(!done){
        now=(ULONG)time(NULL);remaining=now>=start ? (int)seconds-(int)(now-start):(int)seconds;
        if(remaining<=0){complete=1;break;}
        /* Disk/metadata/minimum-RAM probes have one-second resolution.
         * Keep the buttons responsive without repeating I/O ten times/sec. */
        if(now!=last_probe){last_probe=now;
        if(sampling){memset(&sample,0,sizeof(sample));
            if(nd_control_plugin_resources(&sample)==ND_CONTROL_OK && !sample.sample_remaining && sample.sample_age!=ND_DIAGNOSTIC_AGE_UNKNOWN){
                if(!save_plugin_sample(now-start))sample_error=1;sampling=0;
            }
        }else if(now>=next_sample){memset(&sample,0,sizeof(sample));sample.operation=1;
            sampling=nd_control_plugin_resources(&sample)==ND_CONTROL_OK;next_sample=now+10;
        }
        if(!owner_is(owner)){overridden=1;done=1;break;} /* A later Use/Save supersedes us. */
        now=AvailMem(MEMF_CHIP);if(now<min_chip)min_chip=now;now=AvailMem(MEMF_FAST);if(now<min_fast)min_fast=now;
        }
        if(remaining!=last_second){
            struct RastPort *rp=win->RPort;last_second=remaining;info=GetScreenDrawInfo(screen);
            if(info){SetAPen(rp,info->dri_Pens[BACKGROUNDPEN]);RectFill(rp,10,win->BorderTop+2,win->Width-10,ng.ng_TopEdge-2);
                SetAPen(rp,info->dri_Pens[TEXTPEN]);SetDrMd(rp,JAM1);sprintf(text,"Recording: %d seconds remaining",remaining);
                Move(rp,12,win->BorderTop+rp->TxBaseline+7);Text(rp,text,strlen(text));
                strcpy(text,"Use NovaDock now to show the problem.");Move(rp,12,win->BorderTop+rp->TxHeight+rp->TxBaseline+12);Text(rp,text,strlen(text));
                FreeScreenDrawInfo(screen,info);}
        }
        while((msg=GT_GetIMsg(win->UserPort))!=NULL){cls=msg->Class;code=msg->Code;id=msg->IAddress?((struct Gadget *)msg->IAddress)->GadgetID:0;GT_ReplyIMsg(msg);
            if(cls==IDCMP_CLOSEWINDOW||(cls==IDCMP_RAWKEY&&code==0x45)||(cls==IDCMP_GADGETUP&&(id==1||id==2)))done=1;
            if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(win);GT_EndRefresh(win,TRUE);last_second=-1;}}
        Delay(5);
    }
    /* A sample can complete after the last periodic probe and before the
     * deadline. Collect it once without extending the recording or polling. */
    if(sampling){memset(&sample,0,sizeof(sample));
        if(nd_control_plugin_resources(&sample)==ND_CONTROL_OK &&
           !sample.sample_remaining && sample.sample_age!=ND_DIAGNOSTIC_AGE_UNKNOWN)
            if(!save_plugin_sample((ULONG)time(NULL)-start))sample_error=1;
    }
    (void)nd_control_send(ND_CONTROL_SUPPORT_FLUSH);memset(&final,0,sizeof(final));if(nd_control_diagnostics(&final)!=ND_CONTROL_OK)memset(&final,0,sizeof(final));
    if(initial.support_version!=3 || initial.support_level!=2 || !initial.support_persist)complete=0;
    saved=save_measurement("capture.txt",seconds,complete,chip,fast,ports,tasks,min_chip,min_fast,screen);
    if(sample_error)saved=0;
    if(owner_is(owner)){
        if(!nd_support_write(ND_SUPPORT_ENV,&previous))saved=0;
        (void)nd_control_send(ND_CONTROL_SUPPORT_FLUSH);
    }
    rc=saved?(complete?0:5):10;
    if(guided && saved)goto finish;
    message=overridden?"Recording stopped because logging settings changed. Your new settings were kept.":!saved?"Recording finished, but some data or previous capture settings could not be saved. Check Preferences > Support.":
        complete?"Recording finished. Previous logging settings have been restored.\nUse Create report to save your description and the recording.":"Recording stopped early. Previous logging settings have been restored.\nThe partial logs are still available.";
 notice:
    if(win){CloseWindow(win);win=NULL;}
    EasyRequestArgs(NULL,&request,NULL,(APTR)&message);
 finish:
    if(win)CloseWindow(win);if(gadgets)FreeGadgets(gadgets);if(visual)FreeVisualInfo(visual);if(screen)UnlockPubScreen(NULL,screen);
    if(guard){RemPort(guard);DeleteMsgPort(guard);}
    if(GadToolsBase)CloseLibrary(GadToolsBase);
    GadToolsBase=NULL;
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);
    GfxBase=NULL;
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase=NULL;
    return rc;
}
