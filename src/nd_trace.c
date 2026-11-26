/* Runtime support capture. Bounded, rotating logs; no values or typed text. */
#include "nd_build_id.h"
#include "nd_trace.h"
#include "nd_config.h"
#include "nd_version.h"
#include "nd_support.h"
#include "nd_support_log.h"
#include <exec/memory.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
typedef struct IntuitionBase NDTraceIBase;
static NDTraceIBase *NDTraceIntuitionBase;
#define __NOLIBBASE__
#define IntuitionBase NDTraceIntuitionBase
#include <proto/intuition.h>
#undef __NOLIBBASE__
#include <devices/timer.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#define TimerBase NDTraceTimerBase
#include <proto/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct Device *NDTraceTimerBase;
static struct MsgPort *port;
static struct timerequest *timer;
const char nd_trace_version[]="$VER: NovaDock-Trace 0.6.3 (23.11.2026)";
static ULONG frequency,last,origin;
int nd_trace_level;
static int active,timer_pending,persistent,initialized,component_index,policy_loaded;
static ULONG policy_checked,ram_errors;
static char component_name[12];
static char *summary;
static char events[2048];
static unsigned int event_bytes;
static ULONG event_dropped;
static ULONG timer_target;

static struct {ULONG n,total,max;} metrics[NDT_COUNT];
static const char *names[NDT_COUNT]={"dispatch","timer","input","ipc","paint","hover","queue_age","prefs_event","dopus_probe","dopus_update","dopus_action","plugin_packet","desktop_capture","prefs_tick_late"};
static LONG context[4];
static char context_name[40];
static ULONG event_kind,event_gadget;
static LONG scene[8],options[6];
static int scene_known,options_known;
static struct {ULONG packets,graphs,badges,ticks,max;} tiles[ND_MAX_ITEMS];
static const char *tile_kinds[ND_MAX_ITEMS];
static ULONG millis(ULONG t){return frequency ? (t/frequency)*1000UL+((t%frequency)*1000UL)/frequency:0;}
ULONG nd_trace_now(void){struct EClockVal e;struct DateStamp d;if(timer){ReadEClock(&e);return e.ev_lo;}DateStamp(&d);return (ULONG)(d.ds_Days*4320000UL+d.ds_Minute*3000UL+d.ds_Tick);}
/* RAM is updated immediately; disk writes are queued to a separate process. */
static void record(const char *data,unsigned int bytes)
{
    if(!active||!bytes)return;
    if(!nd_support_append(ND_SUPPORT_RAM,(unsigned int)component_index,data,bytes))++ram_errors;
    if(persistent)nd_support_sink_write((unsigned int)component_index,data,bytes);
}
static void append_summary(unsigned int *length,const char *line)
{
    unsigned int n=(unsigned int)strlen(line);
    if(*length+n<=ND_SUPPORT_RECORD_MAX){memcpy(summary+*length,line,n);*length+=n;}
    else ++event_dropped;
}
void nd_trace_flush(void)
{
    int i;unsigned int length=0;char line[512];ULONG now,queued,lost,failed;struct DateStamp wall;
    if(!active||!summary)return;now=nd_trace_now();
    nd_support_sink_poll();nd_support_sink_status(&queued,&lost,&failed);DateStamp(&wall);
    sprintf(line,"t_ms=%lu wall_days=%ld minute=%ld tick=%ld task=%08lx chip=%lu fast=%lu largest=%lu pri=%ld mode=%d disk=%d queued=%lu dropped=%lu io_errors=%lu event_dropped=%lu\n",
            millis(now-origin),wall.ds_Days,wall.ds_Minute,wall.ds_Tick,(ULONG)FindTask(NULL),AvailMem(MEMF_CHIP),AvailMem(MEMF_FAST),AvailMem(MEMF_LARGEST),
            (LONG)FindTask(NULL)->tc_Node.ln_Pri,nd_trace_level,persistent,queued,lost,failed+ram_errors,event_dropped);
    append_summary(&length,line);
    if(event_bytes){memcpy(summary+length,events,event_bytes);length+=event_bytes;event_bytes=0;}
    for(i=0;i<NDT_COUNT;++i)if(metrics[i].n){
        sprintf(line,"metric=%s n=%lu total_ms=%lu max_ms=%lu\n",names[i],metrics[i].n,millis(metrics[i].total),millis(metrics[i].max));append_summary(&length,line);
    }
    if(context_name[0]){sprintf(line,"context=%s a=%ld b=%ld c=%ld d=%ld\n",context_name,context[0],context[1],context[2],context[3]);append_summary(&length,line);}
    if(metrics[NDT_QUEUE].n){sprintf(line,"slowest_event class=%lu gadget=%lu\n",event_kind,event_gadget);append_summary(&length,line);}
    if(scene_known){sprintf(line,"scene screens=%ld wb_front=%ld width=%ld height=%ld depth=%ld active_pri=%ld dopus_pri=%ld hotkeez_pri=%ld\n",scene[0],scene[1],scene[2],scene[3],scene[4],scene[5],scene[6],scene[7]);append_summary(&length,line);}
    if(options_known){sprintf(line,"options items=%ld graphs=%ld labels=%ld refresh=%ld transparency=%ld colourful=%ld\n",options[0],options[1],options[2],options[3],options[4],options[5]);append_summary(&length,line);}
    for(i=0;i<ND_MAX_ITEMS;++i)if(tiles[i].packets){sprintf(line,"tile=%d packets=%lu graphs=%lu badges=%lu total_ms=%lu max_ms=%lu schema=%s\n",i,tiles[i].packets,tiles[i].graphs,tiles[i].badges,millis(tiles[i].ticks),millis(tiles[i].max),tile_kinds[i]?tile_kinds[i]:"OTHER");append_summary(&length,line);}
    record(summary,length);memset(metrics,0,sizeof(metrics));memset(tiles,0,sizeof(tiles));last=now;
}
void nd_trace_scene(struct Screen*wb,void *base)
{NDTraceIBase *ib=(NDTraceIBase *)base;struct Screen*s;struct Task*t;struct MsgPort*p;ULONG key;int n;char name[16];if(!active||!ib||!nd_trace_due())return;
 scene_known=1;NDTraceIntuitionBase=ib;key=LockIBase(0);scene[0]=0;for(s=ib->FirstScreen;s;s=s->NextScreen)++scene[0];scene[1]=ib->FirstScreen==wb;scene[2]=wb?wb->Width:0;scene[3]=wb?wb->Height:0;scene[4]=wb?wb->RastPort.BitMap->Depth:0;scene[5]=-128;
 if(ib->ActiveWindow&&ib->ActiveWindow->UserPort&&ib->ActiveWindow->UserPort->mp_SigTask)scene[5]=((struct Task*)ib->ActiveWindow->UserPort->mp_SigTask)->tc_Node.ln_Pri;UnlockIBase(key);
 scene[6]=scene[7]=-128;Forbid();t=FindTask("dopus_hotkeez");if(t)scene[7]=t->tc_Node.ln_Pri;for(n=0;n<10;++n){if(!n)strcpy(name,"DOPUS");else sprintf(name,"DOPUS.%d",n);p=FindPort(name);if(p&&p->mp_SigTask){scene[6]=((struct Task*)p->mp_SigTask)->tc_Node.ln_Pri;break;}}Permit();}
int nd_trace_due(void){return active && (ULONG)(nd_trace_now()-last)>=frequency*(nd_trace_level==2?1UL:5UL);}
void nd_trace_options(LONG a,LONG b,LONG c,LONG d,LONG e,LONG f){options_known=1;options[0]=a;options[1]=b;options[2]=c;options[3]=d;options[4]=e;options[5]=f;}
void nd_trace_pulse(void)
{
    struct DateStamp d;ULONG now;
    nd_support_sink_poll();DateStamp(&d);now=((ULONG)d.ds_Days*1440UL+(ULONG)d.ds_Minute)*3000UL+(ULONG)d.ds_Tick;
    if(now<policy_checked||now-policy_checked>=100UL){policy_checked=now;nd_trace_reload();}
    if(nd_trace_due())nd_trace_flush();
}
void nd_trace_status(ULONG *queued,ULONG *dropped,ULONG *errors,int *persist)
{nd_support_sink_status(queued,dropped,errors);*errors+=ram_errors;*persist=persistent;}
void nd_trace_packet(unsigned int index,unsigned int badge,unsigned int graph){if(!active||index>=ND_MAX_ITEMS)return;++tiles[index].packets;if(badge)++tiles[index].badges;if(graph)++tiles[index].graphs;}
void nd_trace_packet_time(unsigned int index,ULONG start)
{
    ULONG elapsed;
    if(!active || index>=ND_MAX_ITEMS)return;
    elapsed=nd_trace_now()-start;tiles[index].ticks+=elapsed;
    if(elapsed>tiles[index].max)tiles[index].max=elapsed;
}
/* These names are fixed call-site labels, never commands or setting values.
 * Write BEGIN before a potentially blocking call so a hang leaves evidence. */
void nd_trace_phase(const char *name,int begin,ULONG start)
{
    char line[192];ULONG now;int length;
    if(!active)return;now=nd_trace_now();
    length=sprintf(line,"phase=%.80s edge=%s t_ms=%lu duration_ms=%lu\n",name,begin?"begin":"end",millis(now-origin),begin?0UL:millis(now-start));
    record(line,(unsigned int)length);
}
static const char *trace_schema(const char *schema)
{
    /* Return a source-owned constant; never persist a configured instance ID. */
    static const char *known[]={"DOPUS4","CPU","RAM","CLOCK","NET","ROADSHOW","EYES","MUSIC","PREFERENCES","TRASH","RUNNINGAPPS","SCREENSWITCHER","SENSORS","HASS","GAMES","WHDLOAD","ARCHIVE","MUSICLIB","ACTIONS","SHELF","AGENDA","ALERTS","AMINET","AMIRC","AMIROCKET","AQUARIUM","BENCH","BITSKIM","CALC","CHRISTMAS","CLIPBOARD","COLOUR","COMMAND","DOWNLOAD","LOGS","MIXER","MONITOR","MOON","NOTES","PET","PICTURES","PLACES","QR","SAVER","SCREENMODE","SCREENSHOT","SCRIPT","TIMER","TOOLTYPES","WATCH","WEATHER",NULL};
    unsigned int i;
    if(!schema || !*schema)return "NONE";
    for(i=0;known[i];++i)if(!strcmp(known[i],schema))return known[i];
    return "OTHER";
}
void nd_trace_plugin_support(unsigned int index,const struct NDPluginSupportStats *s)
{
 char line[480];int n;if(!active || !nd_trace_level || index>=ND_MAX_ITEMS)return;
 n=sprintf(line,"plugin_support slot=%u schema=%s t_ms=%lu op=%u phase=%u heap=%lu peak=%lu partial=%u alloc=%lu free=%lu fail=%lu events=%lu open=%lu read=%lu write=%lu dirs=%lu read_bytes=%lu write_bytes=%lu a=%ld b=%ld\n",index,tile_kinds[index]?tile_kinds[index]:"OTHER",millis(nd_trace_now()-origin),s->operation,s->phase,s->heap,s->peak,s->flags&1U,s->allocations,s->frees,s->failures,s->events,s->opens,s->reads,s->writes,s->directories,s->read_bytes,s->write_bytes,s->detail_a,s->detail_b);
 record(line,(unsigned int)n);
}
void nd_trace_tile_kind(unsigned int index,const char *schema)
{if(active && index<ND_MAX_ITEMS)tile_kinds[index]=trace_schema(schema);}
void nd_trace_page(unsigned int page,const char *schema)
{
    char line[128];int n;if(!active)return;
    n=sprintf(line,"page=%u schema=%s t_ms=%lu\n",page,trace_schema(schema),millis(nd_trace_now()-origin));record(line,(unsigned int)n);
}
void nd_trace_end(int m,ULONG start)
{
    ULONG delta;char line[160];int n;
    if(!active||m<0||m>=NDT_COUNT)return;delta=nd_trace_now()-start;
    ++metrics[m].n;metrics[m].total+=delta;if(delta>metrics[m].max)metrics[m].max=delta;
    if(delta>=frequency/10UL){n=sprintf(line,"slow=%s t_ms=%lu duration_ms=%lu\n",names[m],millis(nd_trace_now()-origin),millis(delta));record(line,(unsigned int)n);}
}
void nd_trace_note(const char *name,LONG a,LONG b,LONG c,LONG d)
{
    char line[192];int n;if(!active)return;
    if(!strcmp(context_name,name)&&context[0]==a&&context[1]==b&&context[2]==c&&context[3]==d)return;
    strncpy(context_name,name,sizeof(context_name)-1);context_name[sizeof(context_name)-1]=0;context[0]=a;context[1]=b;context[2]=c;context[3]=d;
    if(nd_trace_level<2)return;
    n=sprintf(line,"event=%.39s t_ms=%lu a=%ld b=%ld c=%ld d=%ld\n",context_name,millis(nd_trace_now()-origin),a,b,c,d);
    if(event_bytes+(unsigned int)n<=sizeof(events)){memcpy(events+event_bytes,line,(unsigned int)n);event_bytes+=(unsigned int)n;}else ++event_dropped;
}
void nd_trace_queue(ULONG sec,ULONG micro,ULONG kind,unsigned int gadget)
{struct timeval now;ULONG age,ticks;if(!active||!timer||!sec)return;GetSysTime(&now);if(now.tv_secs<sec||now.tv_secs-sec>60)return;
 age=(now.tv_secs-sec)*1000UL;if(now.tv_micro>=micro)age+=(now.tv_micro-micro)/1000UL;else{if(age<1000)return;age-=1000UL;age+=(1000000UL+now.tv_micro-micro)/1000UL;}
 ticks=(age/1000UL)*frequency+((age%1000UL)*frequency)/1000UL;++metrics[NDT_QUEUE].n;metrics[NDT_QUEUE].total+=ticks;if(ticks>=metrics[NDT_QUEUE].max){metrics[NDT_QUEUE].max=ticks;event_kind=kind;event_gadget=gadget;}}
/* GadTools can consume IntuiTicks internally. Sample their queue age before
 * filtering, without inspecting any key code, field text or message address. */
void nd_trace_port_queue(struct MsgPort *p)
{
    struct IntuiMessage *m;
    ULONG sec=0,micro=0,kind=0;
    if(!active || !p)return;
    Forbid();
    m=(struct IntuiMessage *)p->mp_MsgList.lh_Head;
    if(m->ExecMessage.mn_Node.ln_Succ){sec=m->Seconds;micro=m->Micros;kind=m->Class;}
    Permit();
    if(sec)nd_trace_queue(sec,micro,kind,0);
}
/* A diagnostic-only wake measures scheduler delays while Intuition owns an
 * active string gadget and withholds its normal IDCMP events. */
ULONG nd_trace_wait(ULONG signals)
{
    ULONG received,bit,now,late;
    if(nd_trace_level<2 || !timer)return Wait(signals);
    bit=1UL<<port->mp_SigBit;
    if(!timer_pending){
        timer->tr_node.io_Command=TR_ADDREQUEST;
        timer->tr_time.tv_secs=0;timer->tr_time.tv_micro=250000;
        timer_target=nd_trace_now()+frequency/4UL;
        SendIO((struct IORequest*)timer);timer_pending=1;
    }
    received=Wait(signals|bit);
    if((received&bit) && CheckIO((struct IORequest*)timer)){
        WaitIO((struct IORequest*)timer);timer_pending=0;
        now=nd_trace_now();late=(LONG)(now-timer_target)>0 ? now-timer_target:0;
        ++metrics[NDT_PREF_TICK].n;metrics[NDT_PREF_TICK].total+=late;
        if(late>metrics[NDT_PREF_TICK].max)metrics[NDT_PREF_TICK].max=late;
    }
    return received;
}
static void close_timer(void)
{
    if(timer){if(timer_pending){if(!CheckIO((struct IORequest *)timer))AbortIO((struct IORequest *)timer);WaitIO((struct IORequest *)timer);timer_pending=0;}
        if(NDTraceTimerBase)CloseDevice((struct IORequest *)timer);DeleteIORequest((struct IORequest *)timer);}
    if(port)DeleteMsgPort(port);timer=NULL;port=NULL;NDTraceTimerBase=NULL;
}
static void stop_record(const char *reason)
{char line[120];int n;if(!active)return;n=sprintf(line,"capture=stop reason=%s t_ms=%lu\n",reason,millis(nd_trace_now()-origin));record(line,(unsigned int)n);}
void nd_trace_marker(void)
{struct DateStamp d;char line[160];int n;if(!active)return;nd_trace_flush();DateStamp(&d);n=sprintf(line,"marker=manual t_ms=%lu days=%ld minute=%ld tick=%ld\n",millis(nd_trace_now()-origin),d.ds_Days,d.ds_Minute,d.ds_Tick);record(line,(unsigned int)n);}
static void finish(void)
{nd_trace_flush();stop_record("exit");active=nd_trace_level=0;nd_support_sink_close();close_timer();if(summary)FreeVec(summary);summary=NULL;}
static void begin_capture(void)
{
    struct EClockVal e;struct DateStamp date;char header[512];int length;
    if(!summary)summary=AllocVec(ND_SUPPORT_RECORD_MAX,MEMF_PUBLIC);
    if(!summary){++ram_errors;nd_trace_level=0;return;}
    frequency=50;port=CreateMsgPort();
    if(port){timer=(struct timerequest *)CreateIORequest(port,sizeof(*timer));
        if(timer){if(!OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)timer,0)){NDTraceTimerBase=timer->tr_node.io_Device;frequency=ReadEClock(&e);if(!frequency)frequency=50;}
            else{DeleteIORequest((struct IORequest *)timer);timer=NULL;}}}
    active=1;DateStamp(&date);origin=last=nd_trace_now();
    memset(metrics,0,sizeof(metrics));memset(tiles,0,sizeof(tiles));event_bytes=0;context_name[0]=0;
    length=sprintf(header,"NovaDock trace 3 component=%s app=" ND_VERSION " build_id=" ND_BUILD_ID " build=" __DATE__ " " __TIME__ " frequency=%lu cpu_flags=%lu days=%ld minute=%ld tick=%ld exec=%u.%u chip_total=%lu fast_total=%lu\n",component_name,frequency,(ULONG)SysBase->AttnFlags,date.ds_Days,date.ds_Minute,date.ds_Tick,SysBase->LibNode.lib_Version,SysBase->LibNode.lib_Revision,AvailMem(MEMF_CHIP|MEMF_TOTAL),AvailMem(MEMF_FAST|MEMF_TOTAL));
    record(header,(unsigned int)length);
}
void nd_trace_reload(void)
{
    struct NDSupportSettings s;int previous=nd_trace_level,old_persist=persistent;
    if(!initialized)return;
    if(component_index==0 && nd_trace_level==2)nd_support_expire_capture();
if(!policy_loaded){
#ifdef ND_TRACE
        nd_support_load(&s,1);
#else
        nd_support_load(&s,0);
#endif
        policy_loaded=1;
    }else if(!nd_support_read(ND_SUPPORT_ENV,&s))return;
    if(previous==s.level&&old_persist==s.persist)return;
    if(active)nd_trace_flush();
    if(active&&!s.level){stop_record("disabled");active=0;close_timer();if(summary)FreeVec(summary);summary=NULL;}
    nd_trace_level=s.level;persistent=s.persist;
    if(nd_trace_level&&!active)begin_capture();
    else if(active)nd_trace_note("support settings changed",previous,nd_trace_level,old_persist,persistent);
}
void nd_trace_init(const char *component)
{
    if(initialized)return;initialized=1;component_index=!strcmp(component,"prefs")?1:!strcmp(component,"dopus4")?2:0;
    strcpy(component_name,component_index==1?"prefs":component_index==2?"dopus4":"dock");
    atexit(finish);nd_trace_reload();
}
