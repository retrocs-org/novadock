#include <exec/types.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "nd_client.h"
#include "nd_reply.h"

static void clear_timeout_completion(struct NDClient *c)
{
    ULONG signal;
    if (!c || !c->timeout_port) return;
    while (GetMsg(c->timeout_port)!=NULL) {}
    signal=1UL<<c->timeout_port->mp_SigBit;
    SetSignal(0L,signal);
}

static void stop_timeout(struct NDClient *c)
{
    if (!c || !c->timeout_request) return;
    if (!CheckIO(c->timeout_request)) AbortIO(c->timeout_request);
    WaitIO(c->timeout_request);
    /* timer.device may leave the shared port signal asserted after an
     * aborted request.  The request is reused for every exchange, so an old
     * completion must not be mistaken for the next exchange timing out. */
    clear_timeout_completion(c);
}

static void close_timeout(struct NDClient *c)
{
    if (!c) return;
    if (c->timeout_open && c->timeout_request) {
        CloseDevice(c->timeout_request);c->timeout_open=0;
    }
    if (c->timeout_request) {
        DeleteIORequest(c->timeout_request);c->timeout_request=NULL;
    }
    if (c->timeout_port) {
        DeleteMsgPort(c->timeout_port);c->timeout_port=NULL;
    }
}

static void abandon_reply(struct NDClient *c)
{
    if (!c || !c->reply) return;
    /* PA_IGNORE keeps a late reply safe; updated hosts reclaim ownership. */
    nd_reply_abandon(c->reply);
    c->reply=NULL;
    c->wire_packet=NULL;
    c->timed_out=1;
}

static unsigned int send_badge(struct NDClient *c);

/* A signal-less public port that only names this process for the dock's
 * wake-up (ND_WAKE_PORT_PREFIX).  Built by hand so plugins need no
 * amiga.lib; PA_IGNORE means nothing is ever queued or signalled on it. */
static void open_wake_port(struct NDClient *c,const char *id)
{
    struct MsgPort *port;
    struct List *list;
    strcpy(c->wake_name,ND_WAKE_PORT_PREFIX);
    strcat(c->wake_name,id);
    port=(struct MsgPort *)AllocVec(sizeof(*port),MEMF_PUBLIC|MEMF_CLEAR);
    if (!port) return;
    port->mp_Node.ln_Type=NT_MSGPORT;
    port->mp_Node.ln_Name=c->wake_name;
    port->mp_Flags=PA_IGNORE;
    port->mp_SigTask=FindTask(NULL);
    list=&port->mp_MsgList;
    list->lh_Head=(struct Node *)&list->lh_Tail;
    list->lh_Tail=NULL;
    list->lh_TailPred=(struct Node *)&list->lh_Head;
    list->lh_Type=NT_MESSAGE;
    Forbid();
    /* A stalled predecessor may still hold the name; the dock then falls
     * back to the task from the latest exchange. */
    if (!FindPort(c->wake_name)) {AddPort(port);c->wake_owned=1;}
    Permit();
    if (c->wake_owned) c->wake_port=port;
    else FreeVec(port);
}
static void close_wake_port(struct NDClient *c)
{
    if (!c->wake_port) return;
    if (c->wake_owned) RemPort(c->wake_port);
    FreeVec(c->wake_port);
    c->wake_port=NULL;c->wake_owned=0;
}
unsigned long nd_client_events2(struct NDClient *c)
{
    unsigned long bits;
    if (!c) return 0;
    bits=c->event2;c->event2=0;
    return bits;
}

static unsigned int exchange(struct NDClient *c, unsigned short cmd, int extension)
{
    struct MsgPort *host;
    struct Message *response;
    struct NDMediaMessage *wire;
    struct timerequest *timer;
    ULONG signals,reply_signal,timer_signal;
    unsigned int event,message_size;
    int found = 0;
    int expired=0;
    struct NDMessage *msg;
    struct NDExtMessage *ext;
    int use_ext=0;
    if (!c || !c->reply || !c->wire_packet || !c->timeout_open ||
        c->timed_out)
        return ND_EVENT_QUIT;
    wire=(struct NDMediaMessage *)c->wire_packet;
    ext=(struct NDExtMessage *)c->wire_packet;
    memcpy(wire,&c->packet,sizeof(c->packet));
    msg=&wire->base;
    msg->command = cmd;
    msg->event = ND_EVENT_NONE;
    msg->exec_msg.mn_ReplyPort = c->reply;
    timer=(struct timerequest *)c->timeout_request;
    /* Be defensive when reusing the timer request.  A stale signal here
     * makes an otherwise healthy plugin abandon its next message at once. */
    clear_timeout_completion(c);
    timer->tr_node.io_Command=TR_ADDREQUEST;
    timer->tr_time.tv_secs=ND_PLUGIN_IPC_TIMEOUT_SECONDS;
    timer->tr_time.tv_micro=0;
    SendIO((struct IORequest *)timer);
    /* Host can disappear at any moment; look up AND enqueue under Forbid.
     * The packet version is chosen here, once the host's ABI is known: a
     * dock older than ABI 5 would answer a v5 packet with QUIT. */
    Forbid();
    host = FindPort(ND_PORT_NAME);
    if (host) {
        c->host_abi=(UWORD)(host->mp_Node.ln_Pri>0 ? host->mp_Node.ln_Pri : 0);
        use_ext=c->host_abi>=ND_EXT_ABI_VERSION;
        if (use_ext) {
            UWORD a=c->packet.graph.graph_a,b=c->packet.graph.graph_b;
            msg->version=ND_EXT_ABI_VERSION;
            ext->flags=(UWORD)(extension==1 ? ND_EXT_HAS_GRAPH :
                               extension==2 ? ND_EXT_HAS_MEDIA : 0);
            if (extension==1) {ext->graph_a=a;ext->graph_b=b;}
            ext->reserved=0;ext->event2=0;
            ext->reserved2[0]=ext->reserved2[1]=0;
            message_size=sizeof(struct NDExtMessage);
        } else {
            msg->version = (UWORD)(extension == 2 ? ND_MEDIA_ABI_VERSION :
                                   extension == 1 ? ND_GRAPH_ABI_VERSION :
                                   ND_ABI_VERSION);
            message_size=(unsigned int)(extension == 2 ?
                sizeof(struct NDMediaMessage) : extension == 1 ?
                sizeof(struct NDGraphMessage) : sizeof(*msg));
        }
        msg->exec_msg.mn_Length=(UWORD)message_size;
        PutMsg(host, (struct Message *)msg);
        found = 1;
    }
    Permit();
    if (!found) {
        stop_timeout(c);return ND_EVENT_QUIT;
    }
    reply_signal=1UL << c->reply->mp_SigBit;
    timer_signal=1UL << c->timeout_port->mp_SigBit;
    response=NULL;
    while (!response && !expired) {
        signals=Wait(reply_signal|timer_signal);
        Forbid();
        response=GetMsg(c->reply);
        if (!response && ((signals&timer_signal) || CheckIO(c->timeout_request))) {
            abandon_reply(c);expired=1;
        }
        Permit();
    }
    stop_timeout(c);
    if (expired || response!=(struct Message *)msg) {
        if (!expired) {
            Forbid();abandon_reply(c);Permit();
        }
        return ND_EVENT_QUIT;
    }
    if (use_ext) c->event2|=ext->event2;
    memcpy(&c->packet,wire,sizeof(c->packet));
    event=c->packet.graph.base.event;
    c->throttled=(UWORD)((event&ND_EVENT_THROTTLE)!=0);
    c->wake_pending=0;
    if (event&ND_EVENT_HOST_BADGES) c->host_badges=1;
    /* A badge set before the dock was known to support badges goes out now. */
    if (cmd!=ND_MSG_BADGE && c->badge_pending && c->host_badges &&
        !(event&ND_EVENT_QUIT))
        event|=send_badge(c);
    return event;
}

/* One ND_MSG_BADGE exchange; the update packet is left as it was. */
static unsigned int send_badge(struct NDClient *c)
{
    struct NDMessage *p=&c->packet.graph.base;
    char caption[ND_TEXT_LEN],value[ND_TEXT_LEN];
    UWORD meter=p->meter;
    unsigned int event;
    memcpy(caption,p->caption,sizeof(caption));
    memcpy(value,p->value,sizeof(value));
    strncpy(p->value,c->badge,ND_TEXT_LEN-1);
    p->value[ND_TEXT_LEN-1]='\0';
    event=exchange(c,ND_MSG_BADGE,0);
    memcpy(p->caption,caption,sizeof(caption));
    memcpy(p->value,value,sizeof(value));
    p->meter=meter;
    if (!(event&ND_EVENT_QUIT)) c->badge_pending=0;
    return event;
}

unsigned int nd_client_set_badge(struct NDClient *c, const char *text)
{
    char badge[ND_BADGE_LEN];
    if (!c || !c->reply) return ND_EVENT_QUIT;
    strncpy(badge,text ? text : "",ND_BADGE_LEN-1);
    badge[ND_BADGE_LEN-1]='\0';
    if (!strcmp(badge,c->badge) && !c->badge_pending) return ND_EVENT_NONE;
    strcpy(c->badge,badge);
    c->badge_pending=1;
    return c->host_badges ? send_badge(c) : ND_EVENT_NONE;
}
int nd_client_init(struct NDClient *c, const char *id)
{
    size_t len;
    memset(c, 0, sizeof(*c));
    if (!id) return 0;
    len = strlen(id);
    if (len == 0 || len >= ND_ID_LEN) return 0;
    c->reply = nd_reply_port();
    if (!c->reply) return 0;
    c->timeout_port=CreateMsgPort();
    if (!c->timeout_port) {nd_reply_delete(c->reply);c->reply=NULL;return 0;}
    c->timeout_request=CreateIORequest(c->timeout_port,sizeof(struct timerequest));
    if (!c->timeout_request ||
        OpenDevice(TIMERNAME,UNIT_VBLANK,c->timeout_request,0)!=0) {
        if (c->timeout_request) {
            DeleteIORequest(c->timeout_request);c->timeout_request=NULL;
        }
        DeleteMsgPort(c->timeout_port);c->timeout_port=NULL;
        nd_reply_delete(c->reply);c->reply=NULL;return 0;
    }
    c->timeout_open=1;
    c->wire_packet=AllocVec(sizeof(c->packet),MEMF_PUBLIC|MEMF_CLEAR);
    if (!c->wire_packet) {
        close_timeout(c);
        nd_reply_delete(c->reply);c->reply=NULL;
        return 0;
    }
    c->packet.graph.base.magic = ND_MAGIC;
    c->packet.graph.base.version = ND_ABI_VERSION;
    strncpy(c->packet.graph.base.id, id, ND_ID_LEN - 1);
    c->packet.graph.base.meter = ND_NO_METER;
    c->packet.graph.graph_a = ND_GRAPH_NO_SAMPLE;
    c->packet.graph.graph_b = ND_GRAPH_NO_SAMPLE;
    open_wake_port(c,id);
    return 1;
}
unsigned int nd_client_update(struct NDClient *c, const char *caption,
                              const char *value, unsigned int meter)
{
    struct NDMessage *p = &c->packet.graph.base;
    if (!c->reply) return ND_EVENT_QUIT;
    if (!caption || !value) return ND_EVENT_QUIT;
    strncpy(p->caption, caption, ND_TEXT_LEN - 1);
    p->caption[ND_TEXT_LEN - 1] = '\0';
    strncpy(p->value, value, ND_TEXT_LEN - 1);
    p->value[ND_TEXT_LEN - 1] = '\0';
    p->meter = (UWORD)(meter <= 100U ? meter : ND_NO_METER);
    return exchange(c, ND_MSG_UPDATE, 0);
}
unsigned int nd_client_update_quiet(struct NDClient *c,const char *caption,
                                    const char *value,unsigned int meter,
                                    unsigned int elapsed_ticks,
                                    unsigned int heartbeat_ticks)
{
    unsigned int normalized,event;
    int changed;
    if (!c || !c->reply || !caption || !value) return ND_EVENT_QUIT;
    normalized=meter<=100U ? meter : ND_NO_METER;
    if (!heartbeat_ticks) heartbeat_ticks=1;
    if (c->quiet_elapsed_ticks+elapsed_ticks<c->quiet_elapsed_ticks)
        c->quiet_elapsed_ticks=heartbeat_ticks;
    else c->quiet_elapsed_ticks+=elapsed_ticks;
    changed=c->wake_pending || !c->quiet_valid ||
            strcmp(c->quiet_caption,caption) ||
            strcmp(c->quiet_value,value) || c->quiet_meter!=normalized;
    if (!changed && c->quiet_elapsed_ticks<heartbeat_ticks)
        return ND_EVENT_NONE;
    event=nd_client_update(c,caption,value,normalized);
    if (!(event&ND_EVENT_QUIT)) {
        strncpy(c->quiet_caption,caption,ND_TEXT_LEN-1);
        c->quiet_caption[ND_TEXT_LEN-1]='\0';
        strncpy(c->quiet_value,value,ND_TEXT_LEN-1);
        c->quiet_value[ND_TEXT_LEN-1]='\0';
        c->quiet_meter=(UWORD)normalized;
        c->quiet_valid=1;
        c->quiet_elapsed_ticks=0;
    }
    return event;
}
unsigned int nd_client_update_graph(struct NDClient *c, const char *caption,
                                   const char *value, unsigned int meter,
                                   unsigned int a, unsigned int b)
{
    struct NDMessage *p = &c->packet.graph.base;
    if (!c->reply || !caption || !value) return ND_EVENT_QUIT;
    strncpy(p->caption, caption, ND_TEXT_LEN - 1);
    p->caption[ND_TEXT_LEN - 1] = '\0';
    strncpy(p->value, value, ND_TEXT_LEN - 1);
    p->value[ND_TEXT_LEN - 1] = '\0';
    p->meter = (UWORD)(meter <= 100U ? meter : ND_NO_METER);
    c->packet.graph.graph_a = (UWORD)(a <= 100U ? a : ND_GRAPH_NO_SAMPLE);
    c->packet.graph.graph_b = (UWORD)(b <= 100U ? b : ND_GRAPH_NO_SAMPLE);
    return exchange(c, ND_MSG_UPDATE, 1);
}
unsigned int nd_client_update_graph_quiet(struct NDClient *c,
                                   const char *caption,const char *value,
                                   unsigned int meter,unsigned int a,
                                   unsigned int b,unsigned int elapsed_ticks,
                                   unsigned int heartbeat_ticks)
{
    unsigned int normalized,graph_a,graph_b,event;
    int changed;
    if (!c || !c->reply || !caption || !value) return ND_EVENT_QUIT;
    normalized=meter<=100U ? meter : ND_NO_METER;
    graph_a=a<=100U ? a : ND_GRAPH_NO_SAMPLE;
    graph_b=b<=100U ? b : ND_GRAPH_NO_SAMPLE;
    if (!heartbeat_ticks) heartbeat_ticks=1;
    if (c->quiet_elapsed_ticks+elapsed_ticks<c->quiet_elapsed_ticks)
        c->quiet_elapsed_ticks=heartbeat_ticks;
    else c->quiet_elapsed_ticks+=elapsed_ticks;
    changed=c->wake_pending || !c->quiet_graph_valid ||
            strcmp(c->quiet_caption,caption) ||
            strcmp(c->quiet_value,value) || c->quiet_meter!=normalized ||
            c->quiet_graph_a!=graph_a || c->quiet_graph_b!=graph_b;
    if (!changed && c->quiet_elapsed_ticks<heartbeat_ticks)
        return ND_EVENT_NONE;
    event=nd_client_update_graph(c,caption,value,normalized,graph_a,graph_b);
    if (!(event&ND_EVENT_QUIT)) {
        strncpy(c->quiet_caption,caption,ND_TEXT_LEN-1);
        c->quiet_caption[ND_TEXT_LEN-1]='\0';
        strncpy(c->quiet_value,value,ND_TEXT_LEN-1);
        c->quiet_value[ND_TEXT_LEN-1]='\0';
        c->quiet_meter=(UWORD)normalized;
        c->quiet_graph_a=(UWORD)graph_a;
        c->quiet_graph_b=(UWORD)graph_b;
        c->quiet_graph_valid=1;
        c->quiet_elapsed_ticks=0;
    }
    return event;
}
unsigned int nd_client_update_media(struct NDClient *c, const char *caption,
                                   const char *value,
                                   unsigned int capabilities,
                                   unsigned int state,
                                   unsigned int volume,
                                   unsigned int *requested_volume)
{
    struct NDMediaMessage *media = &c->packet.media;
    struct NDMessage *p = &media->base;
    unsigned int event;
    if (requested_volume) *requested_volume = ND_MEDIA_VOLUME_UNKNOWN;
    if (!c->reply || !caption || !value) return ND_EVENT_QUIT;
    strncpy(p->caption, caption, ND_TEXT_LEN - 1);
    p->caption[ND_TEXT_LEN - 1] = '\0';
    strncpy(p->value, value, ND_TEXT_LEN - 1);
    p->value[ND_TEXT_LEN - 1] = '\0';
    p->meter = ND_NO_METER;
    media->capabilities = (UWORD)capabilities;
    media->state = (UWORD)state;
    media->volume = (UWORD)(volume <= 100U ? volume : ND_MEDIA_VOLUME_UNKNOWN);
    media->requested_volume = ND_MEDIA_VOLUME_UNKNOWN;
    event = exchange(c, ND_MSG_UPDATE, 2);
    if (requested_volume && (event & ND_EVENT_MEDIA_VOLUME) &&
        media->requested_volume <= 100U)
        *requested_volume = media->requested_volume;
    return event;
}
unsigned int nd_client_update_media_quiet(struct NDClient *c,
                                   const char *caption,const char *value,
                                   unsigned int capabilities,
                                   unsigned int state,unsigned int volume,
                                   unsigned int *requested_volume,
                                   unsigned int elapsed_ticks,
                                   unsigned int heartbeat_ticks)
{
    unsigned int normalized,event;
    int changed;
    if (requested_volume) *requested_volume=ND_MEDIA_VOLUME_UNKNOWN;
    if (!c || !c->reply || !caption || !value) return ND_EVENT_QUIT;
    normalized=volume<=100U ? volume : ND_MEDIA_VOLUME_UNKNOWN;
    if (!heartbeat_ticks) heartbeat_ticks=1;
    if (c->quiet_elapsed_ticks+elapsed_ticks<c->quiet_elapsed_ticks)
        c->quiet_elapsed_ticks=heartbeat_ticks;
    else c->quiet_elapsed_ticks+=elapsed_ticks;
    changed=c->wake_pending || !c->quiet_media_valid ||
            strcmp(c->quiet_caption,caption) ||
            strcmp(c->quiet_value,value) ||
            c->quiet_media_capabilities!=(UWORD)capabilities ||
            c->quiet_media_state!=(UWORD)state ||
            c->quiet_media_volume!=(UWORD)normalized;
    if (!changed && c->quiet_elapsed_ticks<heartbeat_ticks)
        return ND_EVENT_NONE;
    event=nd_client_update_media(c,caption,value,capabilities,state,normalized,
                                 requested_volume);
    if (!(event&ND_EVENT_QUIT)) {
        strncpy(c->quiet_caption,caption,ND_TEXT_LEN-1);
        c->quiet_caption[ND_TEXT_LEN-1]='\0';
        strncpy(c->quiet_value,value,ND_TEXT_LEN-1);
        c->quiet_value[ND_TEXT_LEN-1]='\0';
        c->quiet_media_capabilities=(UWORD)capabilities;
        c->quiet_media_state=(UWORD)state;
        c->quiet_media_volume=(UWORD)normalized;
        c->quiet_media_valid=1;
        c->quiet_elapsed_ticks=0;
    }
    return event;
}
unsigned int nd_client_delay_ticks(const struct NDClient *c,
                                   unsigned int active_ticks)
{
    unsigned long hidden;
    if (!c || !c->throttled) return active_ticks;
    hidden=(unsigned long)active_ticks*4UL;
    if (hidden<100UL) hidden=100UL;
    if (hidden>500UL) hidden=500UL;
    return (unsigned int)hidden;
}
static int open_wait_timer(struct NDClient *c)
{
    if (c->wait_open) return 1;
    c->wait_port=CreateMsgPort();
    if (!c->wait_port) return 0;
    c->wait_request=CreateIORequest(c->wait_port,sizeof(struct timerequest));
    if (!c->wait_request ||
        OpenDevice(TIMERNAME,UNIT_VBLANK,c->wait_request,0)!=0) {
        if (c->wait_request) DeleteIORequest(c->wait_request);
        c->wait_request=NULL;
        DeleteMsgPort(c->wait_port);c->wait_port=NULL;
        return 0;
    }
    c->wait_open=1;
    return 1;
}
static void close_wait_timer(struct NDClient *c)
{
    if (c->wait_open) {CloseDevice(c->wait_request);c->wait_open=0;}
    if (c->wait_request) {DeleteIORequest(c->wait_request);c->wait_request=NULL;}
    if (c->wait_port) {DeleteMsgPort(c->wait_port);c->wait_port=NULL;}
}
unsigned int nd_client_wait(struct NDClient *c,unsigned int ticks)
{
    struct timerequest *timer;
    struct DateStamp before,after;
    ULONG signals,timer_signal;
    LONG elapsed;
    if (!ticks) return 0;
    if (!c || !open_wait_timer(c)) {Delay(ticks);return ticks;}
    /* A wake can arrive between the preceding exchange and this wait. Keep
     * it as a forced-exchange marker instead of discarding it and delaying a
     * click/settings action until the next heartbeat. */
    signals=SetSignal(0L,ND_WAKE_SIGNAL);
    if (signals&ND_WAKE_SIGNAL) {c->wake_pending=1;return 0;}
    DateStamp(&before);
    timer=(struct timerequest *)c->wait_request;
    timer->tr_node.io_Command=TR_ADDREQUEST;
    timer->tr_time.tv_secs=ticks/50U;
    timer->tr_time.tv_micro=(ticks%50U)*20000UL;
    SendIO(c->wait_request);
    timer_signal=1UL<<c->wait_port->mp_SigBit;
    signals=Wait(timer_signal|ND_WAKE_SIGNAL);
    if (signals&ND_WAKE_SIGNAL) c->wake_pending=1;
    if (!CheckIO(c->wait_request)) AbortIO(c->wait_request);
    WaitIO(c->wait_request);
    if (!(signals&timer_signal)) SetSignal(0L,timer_signal);
    DateStamp(&after);
    elapsed=(after.ds_Days-before.ds_Days)*24L*60L*60L*50L+
            (after.ds_Minute-before.ds_Minute)*60L*50L+
            (after.ds_Tick-before.ds_Tick);
    if (elapsed<0) elapsed=0;
    if (elapsed>(LONG)ticks) elapsed=(LONG)ticks;
    return (unsigned int)elapsed;
}
void nd_client_close(struct NDClient *c)
{
    close_wake_port(c);
    close_wait_timer(c);
    if (c->reply) {
        (void)exchange(c, ND_MSG_GOODBYE, 0);
        if (c->reply) {nd_reply_delete(c->reply);c->reply=NULL;}
    }
    close_timeout(c);
    if (c->wire_packet) {FreeVec(c->wire_packet);c->wire_packet=NULL;}
}
