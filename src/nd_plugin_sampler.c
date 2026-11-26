#include "dock_internal.h"
#include "nd_trace.h"
/* Opt-in five-second statistical CPU estimate. timer.device schedules a
 * jittered soft interrupt; no scheduler patches, CIA ownership or busy loop.
 * Only exact plugin task pointers are attributed, never external programs. */
#include <exec/interrupts.h>
#include <exec/tasks.h>
struct NDPluginSampler {
 struct MsgPort *port;struct timerequest *timer;struct Interrupt interrupt;
 struct Task *owner,*targets[ND_MAX_ITEMS];ULONG hits[ND_MAX_ITEMS],samples;
 ULONG started,random,signal;volatile UWORD ready;UWORD pending,opened;
};
static ULONG sampler_interrupt(__reg("a1") struct NDPluginSampler *s)
{
 struct Task *task=FindTask(NULL);unsigned int i;
 ++s->samples;
 if(task && task->tc_State==TS_RUN)for(i=0;i<ND_MAX_ITEMS;++i)if(s->targets[i] && s->targets[i]==task){++s->hits[i];break;}
 s->ready=1;Signal(s->owner,s->signal);return 0;
}
ULONG plugin_sampler_signal(struct NDDock *d){return d->plugin_sampler ? d->plugin_sampler->signal:0;}
ULONG plugin_sampler_remaining(struct NDDock *d)
{ULONG elapsed;if(!d->plugin_sampler)return 0;elapsed=diagnostic_ticks()-d->plugin_sampler->started;return elapsed>=250UL ? 0:250UL-elapsed;}
static void sampler_arm(struct NDDock *d)
{
 struct NDPluginSampler *s=d->plugin_sampler;unsigned int i;
 /* The callback only compares these pointers, never dereferences foreign tasks. */
 Disable();for(i=0;i<ND_MAX_ITEMS;++i)s->targets[i]=(i<(unsigned int)d->conf.count && d->conf.items[i].kind==ND_ITEM_PLUGIN && d->conf.items[i].enabled && d->conf.items[i].online) ? d->plugin_task[i]:NULL;Enable();
 s->random=s->random*1664525UL+1013904223UL;s->timer->tr_node.io_Command=TR_ADDREQUEST;s->timer->tr_time.tv_secs=0;
 s->timer->tr_time.tv_micro=31000UL+s->random%39000UL;s->ready=0;s->pending=1;SendIO((struct IORequest *)s->timer);
}
void plugin_sampler_close(struct NDDock *d)
{
 struct NDPluginSampler *s=d->plugin_sampler;if(!s)return;
 if(s->pending){if(!CheckIO((struct IORequest *)s->timer))AbortIO((struct IORequest *)s->timer);WaitIO((struct IORequest *)s->timer);while(!s->ready)Wait(s->signal);s->pending=0;}
 if(s->opened)CloseDevice((struct IORequest *)s->timer);
 if(s->timer)DeleteIORequest((struct IORequest *)s->timer);
 if(s->port){s->port->mp_Flags=PA_SIGNAL;s->port->mp_SigTask=s->owner;DeleteMsgPort(s->port);}
 FreeVec(s);d->plugin_sampler=NULL;
}
int plugin_sampler_start(struct NDDock *d)
{
 struct NDPluginSampler *s;int i;
 if(d->plugin_sampler)return 1;
 s=(struct NDPluginSampler *)AllocVec(sizeof(*s),MEMF_PUBLIC|MEMF_CLEAR);if(!s)return 0;d->plugin_sampler=s;s->owner=FindTask(NULL);s->port=CreateMsgPort();
 if(!s->port)goto fail;s->signal=1UL<<s->port->mp_SigBit;s->timer=(struct timerequest *)CreateIORequest(s->port,sizeof(*s->timer));if(!s->timer)goto fail;
 if(OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *)s->timer,0))goto fail;s->opened=1;
 s->interrupt.is_Node.ln_Type=NT_INTERRUPT;s->interrupt.is_Node.ln_Name="NovaDock plugin CPU sample";s->interrupt.is_Data=s;s->interrupt.is_Code=(VOID (*)())sampler_interrupt;
 s->port->mp_Flags=PA_SOFTINT;s->port->mp_SigTask=(struct Task *)&s->interrupt;
 for(i=0;i<ND_MAX_ITEMS;++i){d->plugin_cpu_id[i][0]=0;if(i<d->conf.count)strcpy(d->plugin_cpu_id[i],d->conf.items[i].id);}
 s->started=diagnostic_ticks();s->random=s->started|1UL;d->plugin_cpu_samples=0;d->plugin_cpu_stamp=0;memset(d->plugin_cpu_hits,0,sizeof(d->plugin_cpu_hits));
 for(i=0;i<d->conf.count;++i)if(d->conf.items[i].kind==ND_ITEM_PLUGIN && d->conf.items[i].enabled){d->plugin_event2[i]|=ND_EVENT2_SUPPORT_SAMPLE;}
 wake_pending_plugins(d);sampler_arm(d);return 1;
 fail:plugin_sampler_close(d);return 0;
}
void plugin_sampler_service(struct NDDock *d)
{
 struct NDPluginSampler *s=d->plugin_sampler;unsigned int i;if(!s || !s->ready)return;
 WaitIO((struct IORequest *)s->timer);s->pending=0;
 for(i=0;i<ND_MAX_ITEMS;++i)if(strcmp(d->plugin_cpu_id[i],i<(unsigned int)d->conf.count ? d->conf.items[i].id:"")){d->plugin_cpu_stamp=0;d->plugin_cpu_samples=0;plugin_sampler_close(d);return;}
 d->plugin_cpu_samples=s->samples;for(i=0;i<ND_MAX_ITEMS;++i)d->plugin_cpu_hits[i]=s->hits[i];
 if(diagnostic_ticks()-s->started>=250UL){d->plugin_cpu_stamp=diagnostic_ticks();ND_TRACE_NOTE("plugin.cpu.sample",d->plugin_cpu_samples,0,0,0);plugin_sampler_close(d);}
 else sampler_arm(d);
}
