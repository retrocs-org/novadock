/* Local support bundle collector. Never includes configuration or uploads data. */
#include "nd_build_id.h"
#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_support.h"
#include "nd_support_report.h"
#include "nd_support_save.h"
#include "nd_version.h"
#include "nd_cpu_info.h"
#include "nd_control_client.h"
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
long __stack=32768;
/* Use the OS Version command for the Kickstart release, not patched Exec. */
static void detect_system(struct NDSupportReport *r)
{
    struct Library *wb;BPTR out,in;LONG count;char path[96],version[256],rom[32];
    struct NDSupportMachine *m=&r->machine;struct Screen *screen;struct NameInfo mode;ULONG id,depth;
    char *start;unsigned int major,minor,name_length;unsigned int wb_major=0,wb_minor=0;
    strcpy(rom,"unavailable");
    sprintf(path,"T:NovaDock-Version-%08lx",(ULONG)FindTask(NULL));
    out=Open(path,MODE_NEWFILE);in=Open("NIL:",MODE_OLDFILE);
    if(out&&in){
        if(SystemTags("C:Version",SYS_Input,in,SYS_Output,out,TAG_DONE)==0){
            Close(out);out=0;out=Open(path,MODE_OLDFILE);
            if(out){count=Read(out,version,sizeof(version)-1);if(count>0){version[count]=0;
                start=strstr(version,"Kickstart ");if(start&&sscanf(start+10,"%u.%u",&major,&minor)==2&&major<1000&&minor<10000)sprintf(rom,"%u.%u",major,minor);}}
        }
    }
    if(in)Close(in);if(out)Close(out);DeleteFile(path);
    wb=OpenLibrary("workbench.library",0);if(wb){wb_major=wb->lib_Version;wb_minor=wb->lib_Revision;CloseLibrary(wb);}
    sprintf(m->cpu,"Motorola %s",nd_system_cpu((ULONG)SysBase->AttnFlags));
    if(wb_major==47&&wb_minor==42)strcpy(m->os,"3.2.3 (Workbench 47.42)");
    else if(wb_major)sprintf(m->os,"Workbench %u.%u",wb_major,wb_minor);
    else strcpy(m->os,"Unavailable");
    strcpy(m->kickstart,rom);sprintf(m->exec,"%u.%u",SysBase->LibNode.lib_Version,SysBase->LibNode.lib_Revision);
    sprintf(m->chip_ram,"%lu KB total / %lu KB free",AvailMem(MEMF_CHIP|MEMF_TOTAL)/1024UL,AvailMem(MEMF_CHIP)/1024UL);
    sprintf(m->fast_ram,"%lu KB total / %lu KB free",AvailMem(MEMF_FAST|MEMF_TOTAL)/1024UL,AvailMem(MEMF_FAST)/1024UL);
    strcpy(m->app,ND_BUILD_ID);strcpy(m->graphics,"Unavailable");strcpy(m->display_mode,"Unavailable");
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    if(IntuitionBase&&GfxBase){screen=LockPubScreen("Workbench");if(screen){
        depth=GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH);id=GetVPModeID(&screen->ViewPort);
        sprintf(m->graphics,"%u x %u / %lu-bit / ID %08lx",(unsigned int)screen->Width,(unsigned int)screen->Height,depth,id);
        memset(&mode,0,sizeof(mode));
        if(GetDisplayInfoData(NULL,(UBYTE *)&mode,sizeof(mode),DTAG_NAME,id)){
            name_length=sizeof(mode.Name);if(name_length>=sizeof(m->display_mode))name_length=sizeof(m->display_mode)-1;
            memcpy(m->display_mode,mode.Name,name_length);m->display_mode[name_length]=0;
        }else sprintf(m->display_mode,"Unnamed mode (%08lx)",id);
        UnlockPubScreen(NULL,screen);
    }}
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;
    nd_support_report_format_system(r);
}
static int copy_log(const char *base,const char *directory,const char *name,const char *saved_name)
{
    BPTR in,out;char path[512],buffer[4096];LONG count,written;ULONG total=0;int ok=1;
    sprintf(path,"%s/%s",directory,name);in=Open(path,MODE_OLDFILE);if(!in)return -1;
    strcpy(path,base);if(!AddPart(path,(STRPTR)saved_name,sizeof(path))){Close(in);return 0;}
    out=Open(path,MODE_NEWFILE);if(!out){Close(in);return 0;}
    while(total<ND_SUPPORT_LOG_LIMIT){
        ULONG left=ND_SUPPORT_LOG_LIMIT-total;
        count=Read(in,buffer,left<sizeof(buffer)?left:sizeof(buffer));
        if(count<=0){if(count<0)ok=0;break;}
        written=Write(out,buffer,count);if(written!=count){ok=0;break;}total+=(ULONG)count;
    }
    if(!Close(out))ok=0;Close(in);return ok;
}
static int report(const char *base,const struct NDSupportReport *details)
{
    struct NDDiagnosticsSnapshot s;struct DateStamp date;FILE *f;char path[512];LONG status;int ok=1;
    strcpy(path,base);if(!AddPart(path,"support.txt",sizeof(path)))return 0;
    status=nd_control_diagnostics(&s);DateStamp(&date);f=fopen(path,"w");if(!f)return 0;
    if(fprintf(f,"NovaDock " ND_VERSION " support bundle - trace format 3\nBuild ID: " ND_BUILD_ID "\nSource SHA256: " ND_BUILD_SOURCE_SHA256 "\nBuilt " __DATE__ " " __TIME__ "\nDays=%ld minute=%ld tick=%ld\nExec=%u.%u CPU_flags=%lu\nChip_free=%lu Fast_free=%lu Largest=%lu\nDock_status=%ld\n",
        date.ds_Days,date.ds_Minute,date.ds_Tick,SysBase->LibNode.lib_Version,SysBase->LibNode.lib_Revision,(ULONG)SysBase->AttnFlags,
        AvailMem(MEMF_CHIP),AvailMem(MEMF_FAST),AvailMem(MEMF_LARGEST),status)<0)ok=0;
    if(fprintf(f,"\nDetected at report start:\n%s\n",details->system)<0)ok=0;
    {static const char *const libraries[]={"workbench.library","intuition.library","graphics.library","layers.library","gadtools.library",NULL};int i;
     for(i=0;libraries[i];++i){struct Library *lib=OpenLibrary((STRPTR)libraries[i],0);if(lib){if(fprintf(f,"%s=%u.%u\n",libraries[i],lib->lib_Version,lib->lib_Revision)<0)ok=0;CloseLibrary(lib);}}}
    if(status==ND_CONTROL_OK&&fprintf(f,"Support_mode=%u Persistent=%u Queued=%lu Dropped=%lu Write_errors=%lu\nUptime_ticks=%lu Wakes=%lu Full_paints=%lu Tile_paints=%lu IPC_updates=%lu\nPlugins=%u Online=%u Stale=%u Watchdog_restarts=%lu\nReloads=%lu Applied=%lu Failed=%lu Last_ticks=%lu\nSaves=%lu Failed_saves=%lu Last_ticks=%lu\n",
        s.support_level,s.support_persist,s.support_queued,s.support_dropped,s.support_errors,s.uptime_ticks,s.wake_count,s.paint_count,s.tile_count,s.update_count,
        s.plugin_count,s.online_count,s.stale_count,s.watchdog_restarts,s.reload_requests,s.reload_applied,s.reload_failures,s.reload_last_ticks,
        s.saves_completed,s.saves_failed,s.save_last_ticks)<0)ok=0;
    if(status==ND_CONTROL_OK && s.aminotis_version==1 && fprintf(f,
        "AmiNotis_enabled=%u Received=%lu Delivered=%lu Dropped=%lu Queued=%u Pending=%u\nAmiNotis_unavailable=%lu Queue_full=%lu Timeouts=%lu Rejected=%lu Cancelled=%lu Last_failure=%s Last_result=%u\n",
        s.aminotis_enabled,s.aminotis.received,s.aminotis.delivered,s.aminotis.dropped,s.aminotis.queued,s.aminotis.pending,
        s.aminotis.unavailable,s.aminotis.queue_full,s.aminotis.timeouts,s.aminotis.rejected,s.aminotis.cancelled,ND_AMINOTIS_FAILURE_NAME(s.aminotis.last_failure),s.aminotis.last_result)<0)ok=0;
    if(status==ND_CONTROL_OK&&s.async_version==1&&fprintf(f,"Icon_jobs=%u Icons_ready=%u Launch_jobs=%u\n",s.icon_jobs,s.icons_ready,s.launch_jobs)<0)ok=0;
    if(status==ND_CONTROL_OK&&s.grip_version==1&&fprintf(f,"Dock_position=%u Grip=%d,%d %dx%d Counter_epoch=%lu\n",s.dock_position,(int)s.grip_left,(int)s.grip_top,(int)s.grip_width,(int)s.grip_height,s.counter_epoch)<0)ok=0;
    if(status==ND_CONTROL_OK&&s.render_version==1&&fprintf(f,"Frame_average_us=%lu Frame_peak_us=%lu Frame_samples=%lu Frame_interval_ms=%u\nHover_cache_bytes=%lu Hover_cache_budget=%lu Hover_cache_evictions=%lu\n",
        s.frame_average_us,s.frame_peak_us,s.frame_samples,s.frame_interval_ms,s.hover_cache_bytes,s.hover_cache_budget,s.hover_cache_evictions)<0)ok=0;
    if(status==ND_CONTROL_OK&&s.viewport_version==1&&fprintf(f,"Viewport_pending=%u Box=%d,%d %dx%d Layer_scroll=%ld,%ld\n",
        s.viewport_pending,(int)s.viewport_left,(int)s.viewport_top,(int)s.viewport_width,(int)s.viewport_height,s.viewport_scroll_x,s.viewport_scroll_y)<0)ok=0;
    {struct NDControlPluginResourcesMessage q;unsigned int i,pages=0;memset(&q,0,sizeof(q));
     if(fprintf(f,"\nPlugin resources: tracked allocations only; CPU is a statistical sample.\n")<0)ok=0;
     do{if(nd_control_plugin_resources(&q)!=ND_CONTROL_OK)break;
        for(i=0;i<q.count;++i){const struct NDPluginResourceRow *row=&q.rows[i];const struct NDPluginSupportStats *v=&row->stats;
         /* Numeric slot and operation; configured names/IDs are deliberately excluded. */
         if(fprintf(f,"plugin_slot=%u online=%u stale=%u stats_version=%u age_ticks=%lu heap=%lu peak=%lu partial=%u alloc=%lu free=%lu failures=%lu op=%u phase=%u events=%lu a=%ld b=%ld cpu_valid=%u cpu_hits=%lu cpu_samples=%lu sample_age=%lu\n",row->reserved,row->online,row->stale,v->version,row->age_ticks,v->heap,v->peak,v->flags&1U,v->allocations,v->frees,v->failures,v->operation,v->phase,v->events,v->detail_a,v->detail_b,row->cpu_valid,row->cpu_hits,row->cpu_samples,q.sample_age)<0)ok=0;
        }q.offset+=q.count;
     }while(q.count && q.offset<q.total && ++pages<32);
    }
    if(fprintf(f,"\nTrace logs exclude typed text, setting values, credentials and window titles.\nIf supplied, problem.txt contains the contact and problem details entered for this report. Configuration files are not included.\n")<0)ok=0;
    if(fclose(f))ok=0;return ok;
}
static int collection_error(int show,const char *text)
{
    struct EasyStruct request={sizeof(request),0,"NovaDock Support","%s","OK"};
    if(show){IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(IntuitionBase){EasyRequestArgs(NULL,&request,NULL,(APTR)&text);CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}}
    else puts(text);return 20;
}
/* Every interactive report gets a fresh bounded CPU/resource sample, including
 * reports that skip reproduction or were interrupted before the first sample. */
static void sample_report_plugins(void)
{
    struct NDControlPluginResourcesMessage q;unsigned int i;
    memset(&q,0,sizeof(q));q.operation=1;
    if(nd_control_plugin_resources(&q)!=ND_CONTROL_OK)return;
    for(i=0;i<65;++i){Delay(5);memset(&q,0,sizeof(q));
        if(nd_control_plugin_resources(&q)!=ND_CONTROL_OK || !q.sample_remaining)break;
    }
}
int nd_plugin_activity(void);
int main(int argc,char **argv)
{
    char base[512],parent[512],destination[512],tool[260],message[900],leaf[64],name[48],saved_name[48];const char *argument=message;
    BPTR lock;struct DateStamp date;int i,g,location,result,ok=1,interactive,show_result,form,archive=0,packed=0;unsigned int page=0;
    static struct NDSupportReport details;char details_path[512];
    const char *names[]={"dock","prefs","dopus4"};
    struct EasyStruct request={sizeof(request),0,"NovaDock Support","%s","OK"};
    show_result=argc==0||(argc>1&&!strcmp(argv[1],"SHOW"));interactive=show_result||(argc>1&&!strcmp(argv[1],"FORM"));memset(&details,0,sizeof(details));
    if(argc>1 && !strcmp(argv[1],"ACTIVITY"))return nd_plugin_activity();
    if(argc>1 && !strcmp(argv[1],"RECORD"))
        return nd_support_capture(argc>2 ? (unsigned int)atoi(argv[2]):60U,0);
    if(argc>1 && !strcmp(argv[1],"BENCH"))return collection_error(0,"The temporary benchmark feature has been removed. Use Create report.");
    detect_system(&details);
    for(;;){
        ok=1;packed=0;
        if(interactive){form=nd_support_report_form_at(&details,page);if(!form)return 0;
            if(form<0)return collection_error(show_result,"Could not open report form. Use a larger Workbench screen or smaller font.");}
        DateStamp(&date);sprintf(leaf,"NovaDock-%ld-%ld",date.ds_Days,date.ds_Minute*3000L+date.ds_Tick);
        if(!NameFromLock(GetProgramDir(),parent,sizeof(parent))||!AddPart(parent,"SupportBundles",sizeof(parent)))return collection_error(show_result,"Could not locate the support destination.");
        if(interactive){
            archive=nd_support_find_lha(tool,sizeof(tool));
            lock=Lock(parent,ACCESS_READ);if(lock)UnLock(lock);
            result=nd_support_select_save(lock?parent:"RAM:",leaf,archive,destination,sizeof(destination));
            if(!result){page=4;continue;}
            if(result<0){
                collection_error(show_result,"Could not open the save chooser. Your answers are kept; choose Save report to try again.");
                page=4;continue;
            }
            strcpy(parent,destination);if(archive)*PathPart(parent)=0;
        }else{lock=CreateDir(parent);if(lock)UnLock(lock);}
        if(!nd_support_unique_drawer(parent,leaf,base,sizeof(base))){
            if(!interactive)return collection_error(show_result,"Could not create the report drawer. Check disk space and write access.");
            collection_error(show_result,"Could not create the report drawer. Your answers are kept. Check free space or choose another disk.");
            page=4;continue;
        }
        if(interactive){nd_support_save_progress("Checking plugin activity (5 seconds)...");sample_report_plugins();}
        if(show_result)nd_support_save_progress("Collecting logs...");
    (void)nd_control_send(ND_CONTROL_SUPPORT_FLUSH);
    for(i=0;i<3;++i)for(g=0;g<2;++g)for(location=0;location<2;++location){
        sprintf(name,"%s.log%s",names[i],g?".previous":"");
        sprintf(saved_name,"%s%s.log%s",names[i],location?"-disk":"",g?".previous":"");
        result=copy_log(base,location?ND_SUPPORT_DISK:ND_SUPPORT_RAM,name,saved_name);
        if(result==0)ok=0;
    }
    {
        static const char *const extras[]={"capture.txt","capture.previous.txt","plugin-samples.txt","plugin-samples.previous.txt",NULL};
        for(i=0;extras[i];++i)if(copy_log(base,ND_SUPPORT_DISK,extras[i],extras[i])==0)ok=0;
    }
    /* Keep RAM and disk sessions distinct. Every archived file stays bounded. */
    for(i=0;i<3;++i)for(location=0;location<2;++location){
        char directory[200];int session;
        sprintf(name,"%s.session",names[i]);sprintf(saved_name,"%s%s-session.txt",names[i],location?"-disk":"");
        if(copy_log(base,location?ND_SUPPORT_DISK:ND_SUPPORT_RAM,name,saved_name)==0)ok=0;
        for(session=1;session<=4;++session){
            sprintf(directory,"%s/Sessions/%s-%d",location?ND_SUPPORT_DISK:ND_SUPPORT_RAM,names[i],session);
            for(g=0;g<3;++g){
                if(g==2)strcpy(name,"session.txt");else sprintf(name,"%s.log%s",names[i],g?".previous":"");
                sprintf(saved_name,"%s-%s-%d.%s",names[i],location?"disk":"ram",session,g==2?"txt":g?"previous":"log");
                if(copy_log(base,directory,name,saved_name)==0)ok=0;
            }
        }
    }
    if(!report(base,&details))ok=0;
    if(interactive){strcpy(details_path,base);if(!AddPart(details_path,"problem.txt",sizeof(details_path))||!nd_support_report_write(details_path,&details))ok=0;}
    if(!ok && interactive){
        nd_support_save_progress(NULL);
        sprintf(message,"Some files could not be saved. Your answers are kept. Choose another disk or free some space, then try again.\n\nFiles saved so far:\n%s",base);
        collection_error(show_result,message);page=4;continue;
    }
    if(ok&&archive){
        if(show_result)nd_support_save_progress("Compressing report with LhA...");
        packed=nd_support_pack_report(tool,base,destination);
    }
    nd_support_save_progress(NULL);
    break;
    }
    if(packed){nd_support_discard_report(base);sprintf(message,"Report saved:\n%s\n\nSend this .lha file to support.",destination);}
    else if(!ok)sprintf(message,"Some report files could not be saved. Check disk space. Files saved so far:\n%s",base);
    else if(archive)sprintf(message,"Could not compress the report. Your files were saved here:\n%s\n\nSend all files in this drawer to support.",base);
    else sprintf(message,"Report saved in:\n%s\n\n%sSend all files in this drawer to support.",base,interactive?"LhA was not found. ":"");
    if(show_result){
        IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
        if(IntuitionBase){EasyRequestArgs(NULL,&request,NULL,(APTR)&argument);CloseLibrary((struct Library *)IntuitionBase);}
    }else puts(message);
    return ok?0:10;
}
