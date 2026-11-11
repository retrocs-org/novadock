/* CPU use measured as elapsed time not spent in a lowest-priority task. */
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/alib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_metrics.h"

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("Activity");
/* Honoured by the vbcc and libnix startup code; harmless elsewhere. */
long __stack=16384;

#define ND_IDLE_STACK_SIZE 2048U
#define ND_IDLE_SAMPLE_TICKS 5U /* 0.1 second at the normal 50 Hz tick rate */
#define ND_IDLE_WAKE_SIGNAL SIGBREAKF_CTRL_F

struct NDIdleTask {
    struct Task task;
    UBYTE stack[ND_IDLE_STACK_SIZE];
};

/* Referenced by cpu_idle_hooks.asm. */
struct Device *TimerBase = NULL;
volatile ULONG nd_cpu_idle_ticks = 0;
volatile ULONG nd_cpu_idle_started = 0;
volatile ULONG nd_cpu_idle_sample_active = 0;
struct EClockVal nd_cpu_hook_eclock;

extern void nd_cpu_idle_launch(void);
extern void nd_cpu_idle_switch(void);

static void nd_cpu_idle_loop(void)
{
    for (;;) {
        /* Run only during the brief sampling window.  Between samples the
           task blocks completely while no sampling work is pending. */
        while (nd_cpu_idle_sample_active) { }
        Wait(ND_IDLE_WAKE_SIGNAL);
    }
}

static struct NDIdleTask *nd_cpu_start_idle_task(void)
{
    /* Monitors otherwise-unused time at Exec's absolute lowest priority.  A
       task monitor will attribute spare cycles to it, but useful work always
       pre-empts it.  Make that distinction visible in process viewers. */
    static char task_name[] = "NovaDock CPU sample (free)";
    struct NDIdleTask *idle;

    idle = (struct NDIdleTask *)AllocVec(sizeof(*idle), MEMF_PUBLIC | MEMF_CLEAR);
    if (!idle) return NULL;

    idle->task.tc_Node.ln_Type = NT_TASK;
    idle->task.tc_Node.ln_Pri = -128;
    idle->task.tc_Node.ln_Name = task_name;
    idle->task.tc_Flags = TF_SWITCH | TF_LAUNCH;
    idle->task.tc_SPLower = (APTR)idle->stack;
    idle->task.tc_SPUpper = (APTR)(idle->stack + sizeof(idle->stack));
    idle->task.tc_SPReg = idle->task.tc_SPUpper;
    idle->task.tc_Switch = nd_cpu_idle_switch;
    idle->task.tc_Launch = nd_cpu_idle_launch;
    NewList(&idle->task.tc_MemEntry);

    if (!AddTask(&idle->task, (APTR)nd_cpu_idle_loop, NULL)) {
        FreeVec(idle);
        return NULL;
    }

    /* AddTask() clears tc_Flags.  Arm the callbacks only after Exec has
       finished constructing the task, before this lower-priority task can
       first be scheduled. */
    Forbid();
    idle->task.tc_Flags |= TF_SWITCH | TF_LAUNCH;
    Permit();
    return idle;
}

static void nd_cpu_stop_idle_task(struct NDIdleTask *idle)
{
    if (!idle) return;
    Forbid();
    RemTask(&idle->task);
    Permit();
    FreeVec(idle);
}

static void nd_cpu_begin_sample(struct NDIdleTask *idle)
{
    Forbid();
    nd_cpu_idle_ticks = 0;
    nd_cpu_idle_sample_active = 1;
    Permit();
    Signal(&idle->task, ND_IDLE_WAKE_SIGNAL);
}

static ULONG nd_cpu_end_sample(void)
{
    ULONG ticks;
    Forbid();
    nd_cpu_idle_sample_active = 0;
    ticks = nd_cpu_idle_ticks;
    nd_cpu_idle_ticks = 0;
    Permit();
    return ticks;
}

int main(int argc, char **argv)
{
    struct NDClient client;
    struct MsgPort *timer_port = NULL;
    struct IORequest *timer_io = NULL;
    struct NDIdleTask *idle_task = NULL;
    struct EClockVal clock_now;
    ULONG sample_started, elapsed, idle;
    unsigned int meter, event = 0;
    char value[32], option[16];
    unsigned int poll = 50;
    unsigned int elapsed_ticks = 50;
    unsigned int last_meter = 101;
    unsigned int stable_samples = 0;
    int reread = 1;
    int result = 20;

    if (argc < 2 || !nd_client_init(&client, argv[1])) return 20;

    timer_port = CreateMsgPort();
    if (!timer_port) goto cleanup;
    timer_io = CreateIORequest(timer_port, sizeof(struct timerequest));
    if (!timer_io) goto cleanup;
    if (OpenDevice(TIMERNAME, UNIT_ECLOCK, timer_io, 0) != 0) goto cleanup;
    TimerBase = timer_io->io_Device;

    nd_cpu_idle_ticks = 0;
    idle_task = nd_cpu_start_idle_task();
    if (!idle_task) goto cleanup;

    for (;;) {
        unsigned int sample_ticks,effective_poll;
        if (reread) {
            nd_plugin_option_string("CPU", "POLL_TICKS", "50",
                                    option, sizeof(option));
            poll = (unsigned int)atoi(option);
            if (poll < 10 || poll > 1000) poll = 50;
            reread = 0;
        }

        effective_poll=poll;
        if (stable_samples>=50 && effective_poll<=333)
            effective_poll*=3;
        else if (stable_samples>=20 && effective_poll<=500)
            effective_poll*=2;
        effective_poll=nd_client_delay_ticks(&client,effective_poll);
        sample_ticks = effective_poll < ND_IDLE_SAMPLE_TICKS ?
                       effective_poll : ND_IDLE_SAMPLE_TICKS;
        nd_cpu_begin_sample(idle_task);
        ReadEClock(&clock_now);
        sample_started = clock_now.ev_lo;
        Delay(sample_ticks);
        ReadEClock(&clock_now);
        elapsed = clock_now.ev_lo - sample_started; /* wraparound intentional */
        idle = nd_cpu_end_sample();
        meter = nd_cpu_used_percent(idle, elapsed);
        if (last_meter<=100 &&
            ((meter>last_meter ? meter-last_meter : last_meter-meter)<=1))
            stable_samples++;
        else stable_samples=0;
        last_meter=meter;
        sprintf(value, "%u%%", meter);

        event = nd_client_update_graph_quiet(&client, "CPU", value, meter,
                                             meter, ND_GRAPH_NO_SAMPLE,
                                             elapsed_ticks, 500U);
        if (event & ND_EVENT_QUIT) {
            result = 0;
            break;
        }
        if (event & ND_EVENT_SETTINGS) {
            reread = 1;
            stable_samples=0;
            last_meter=101;
        }
        elapsed_ticks=sample_ticks;
        if (effective_poll > sample_ticks)
            elapsed_ticks+=nd_client_wait(&client,effective_poll-sample_ticks);
    }

cleanup:
    nd_cpu_idle_sample_active = 0;
    nd_cpu_stop_idle_task(idle_task);
    if (timer_io && timer_io->io_Device) CloseDevice(timer_io);
    TimerBase = NULL;
    if (timer_io) DeleteIORequest(timer_io);
    if (timer_port) DeleteMsgPort(timer_port);
    nd_client_close(&client);
    return result;
}
