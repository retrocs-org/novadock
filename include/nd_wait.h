#ifndef ND_WAIT_H
#define ND_WAIT_H
/* Bounded waits for a message port without polling.
 *
 * nd_wait_port() sleeps until PORT holds a message or TICKS (1/50 s) have
 * passed, whichever comes first.  It never removes the message: the caller
 * still owns GetMsg() and the reply protocol.  A caller that waits often
 * keeps an NDWaitTimer open; passing NULL opens timer.device for the one
 * wait.  Without timer.device the wait falls back to Delay(1) polling for
 * the same bounded time, so behaviour never depends on the timer. */
#include <exec/types.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>

struct NDWaitTimer {struct MsgPort *port;struct timerequest *io;int open;};

static void nd_wait_timer_close(struct NDWaitTimer *t)
{
    if (!t) return;
    if (t->open) CloseDevice((struct IORequest *)t->io);
    if (t->io) DeleteIORequest((struct IORequest *)t->io);
    if (t->port) DeleteMsgPort(t->port);
    t->open=0;t->io=NULL;t->port=NULL;
}
static int nd_wait_timer_open(struct NDWaitTimer *t)
{
    if (t->open) return 1;
    t->port=CreateMsgPort();
    if (t->port)
        t->io=(struct timerequest *)CreateIORequest(t->port,sizeof(*t->io));
    if (t->io && !OpenDevice((STRPTR)TIMERNAME,UNIT_VBLANK,(struct IORequest *)t->io,0))
        t->open=1;
    if (!t->open) nd_wait_timer_close(t);
    return t->open;
}
static int nd_port_has_message(struct MsgPort *port)
{
    int waiting;
    Forbid();
    waiting=port->mp_MsgList.lh_Head->ln_Succ!=NULL;
    Permit();
    return waiting;
}
/* 1 = a message is waiting on PORT, 0 = TICKS passed first. */
static int nd_wait_port(struct NDWaitTimer *timer,struct MsgPort *port,ULONG ticks)
{
    struct NDWaitTimer local;
    struct NDWaitTimer *t=timer;
    int ready;
    if (nd_port_has_message(port)) return 1;
    if (!ticks) return 0;
    if (!t) {local.port=NULL;local.io=NULL;local.open=0;t=&local;}
    if (nd_wait_timer_open(t)) {
        ULONG port_bit=1UL<<port->mp_SigBit,timer_bit=1UL<<t->port->mp_SigBit;
        t->io->tr_node.io_Command=TR_ADDREQUEST;
        t->io->tr_time.tv_secs=ticks/50UL;
        t->io->tr_time.tv_micro=(ticks%50UL)*20000UL;
        SetSignal(0,timer_bit);
        SendIO((struct IORequest *)t->io);
        while (!(ready=nd_port_has_message(port)) && !CheckIO((struct IORequest *)t->io))
            Wait(port_bit|timer_bit);
        if (!CheckIO((struct IORequest *)t->io)) AbortIO((struct IORequest *)t->io);
        WaitIO((struct IORequest *)t->io);
        /* The port signal may have arrived together with the timer. */
        if (!ready) ready=nd_port_has_message(port);
    } else {
        ULONG waited;
        for (waited=0;!(ready=nd_port_has_message(port)) && waited<ticks;++waited) Delay(1);
    }
    if (t==&local) nd_wait_timer_close(&local);
    return ready;
}
/* Sleep until one of MASK's signals arrives or TICKS pass; returns the
 * signals received (0 after a timeout).  The signals are consumed, so the
 * caller drains the matching ports afterwards. */
static ULONG nd_wait_signals(struct NDWaitTimer *t,ULONG mask,ULONG ticks)
{
    ULONG got,timer_bit;
    got=SetSignal(0,mask)&mask;
    if (got || !ticks) return got;
    if (!nd_wait_timer_open(t)) {Delay(ticks);return SetSignal(0,mask)&mask;}
    timer_bit=1UL<<t->port->mp_SigBit;
    t->io->tr_node.io_Command=TR_ADDREQUEST;
    t->io->tr_time.tv_secs=ticks/50UL;
    t->io->tr_time.tv_micro=(ticks%50UL)*20000UL;
    SetSignal(0,timer_bit);
    SendIO((struct IORequest *)t->io);
    got=Wait(mask|timer_bit)&mask;
    if (!CheckIO((struct IORequest *)t->io)) AbortIO((struct IORequest *)t->io);
    WaitIO((struct IORequest *)t->io);
    SetSignal(0,timer_bit);
    return got;
}
#endif
