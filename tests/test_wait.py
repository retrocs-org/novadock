#!/usr/bin/env python3
"""include/nd_wait.h: a bounded wait sleeps on the port and a timer, returns
as soon as a message is waiting, times out after the requested ticks, never
removes the message, falls back to bounded polling without timer.device,
releases a temporary timer, and waits on several signals at once."""
from pathlib import Path
import os, re, subprocess, tempfile
ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / 'include/nd_wait.h').read_text()
header = re.sub(r'#include <[^>]+>\n', '', header)
program = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned long ULONG;typedef long LONG;typedef char *STRPTR;
struct Node {struct Node *ln_Succ;};
struct List {struct Node *lh_Head;};
struct MsgPort {struct List mp_MsgList;int mp_SigBit;};
struct IORequest {int io_Command;};
struct NDTestTimeval {ULONG tv_secs,tv_micro;};
struct timerequest {struct IORequest tr_node;struct NDTestTimeval tr_time;};
#define TIMERNAME "timer.device"
#define UNIT_VBLANK 1
#define TR_ADDREQUEST 9
static struct Node tail={NULL},message={&tail};
static int forbid,ports,ios,opens,closes,delays,waits,open_fails,sent,arrive_after=-1;
static ULONG now,deadline;
static struct MsgPort *watched;
static void Forbid(void){++forbid;}
static void Permit(void){--forbid;assert(forbid>=0);}
static struct MsgPort *CreateMsgPort(void){struct MsgPort *p=calloc(1,sizeof(*p));p->mp_MsgList.lh_Head=&tail;p->mp_SigBit=5;++ports;return p;}
static void DeleteMsgPort(struct MsgPort *p){--ports;free(p);}
static struct IORequest *CreateIORequest(struct MsgPort *p,int size){(void)p;++ios;return calloc(1,(size_t)size);}
static void DeleteIORequest(struct IORequest *io){--ios;free(io);}
static int OpenDevice(STRPTR n,int u,struct IORequest *io,int f){(void)n;(void)u;(void)io;(void)f;if(open_fails)return 1;++opens;return 0;}
static void CloseDevice(struct IORequest *io){(void)io;++closes;}
static ULONG pending_signals;
static ULONG SetSignal(ULONG value,ULONG mask){ULONG old=pending_signals;pending_signals=(pending_signals&~mask)|(value&mask);return old;}
static void SendIO(struct IORequest *io){struct timerequest *t=(struct timerequest *)io;assert(io->io_Command==TR_ADDREQUEST);
 deadline=now+t->tr_time.tv_secs*50UL+t->tr_time.tv_micro/20000UL;++sent;}
static int CheckIO(struct IORequest *io){(void)io;return now>=deadline;}
static void AbortIO(struct IORequest *io){(void)io;deadline=now;}
static void WaitIO(struct IORequest *io){(void)io;now=now>deadline ? now:deadline;}
static void tick(void){++now;if(arrive_after>=0 && (int)now>=arrive_after)watched->mp_MsgList.lh_Head=&message;}
static int signal_after=-1;
static ULONG Wait(ULONG mask){ULONG got;++waits;for(;;){tick();if(signal_after>=0&&(int)now>=signal_after)pending_signals|=0x100;
 got=pending_signals&mask;if(got){pending_signals&=~got;return got;}
 if(watched&&watched->mp_MsgList.lh_Head==&message)return 1UL<<5;if(now>=deadline)return 1UL<<5;}}
static void Delay(int t){++delays;while(t-->0)tick();}
''' + header + r'''
int main(void)
{
 struct MsgPort *port=CreateMsgPort();struct NDWaitTimer t;memset(&t,0,sizeof(t));watched=port;
 /* A message already waiting: no timer, no sleep. */
 port->mp_MsgList.lh_Head=&message;assert(nd_wait_port(&t,port,150));assert(!sent&&!waits);
 assert(port->mp_MsgList.lh_Head==&message);          /* never removed */
 port->mp_MsgList.lh_Head=&tail;
 /* A reply after 10 ticks ends the wait at once, the timer is aborted. */
 now=0;arrive_after=10;assert(nd_wait_port(&t,port,150));assert(now==10&&waits==1&&sent==1);   /* one sleep, woken by the reply */
 /* No reply: the wait ends at the deadline (3 s = 150 ticks). */
 port->mp_MsgList.lh_Head=&tail;now=0;arrive_after=-1;waits=0;
 assert(!nd_wait_port(&t,port,150));assert(now==150);
 /* Fractions of a second are honoured. */
 now=0;assert(!nd_wait_port(&t,port,7));assert(now==7);
 assert(opens==1);nd_wait_timer_close(&t);assert(closes==1&&ios==0&&!t.open);
 /* Zero ticks only checks. */
 assert(!nd_wait_port(&t,port,0));assert(opens==1);
 /* A temporary timer is opened and released for one wait. */
 now=0;assert(!nd_wait_port(NULL,port,20));assert(opens==2&&closes==2&&ios==0&&ports==1);
 /* Without timer.device the same bound is kept by polling. */
 open_fails=1;now=0;delays=0;assert(!nd_wait_port(NULL,port,25));assert(delays==25&&ports==1&&ios==0);
 now=0;arrive_after=4;delays=0;assert(nd_wait_port(NULL,port,25));assert(delays==4);
 /* Several ports at once: wake on a signal, time out, or return a pending one. */
 open_fails=0;memset(&t,0,sizeof(t));arrive_after=-1;port->mp_MsgList.lh_Head=&tail;
 now=0;signal_after=6;assert(nd_wait_signals(&t,0x100UL,50)==0x100UL&&now==6);
 now=0;signal_after=-1;assert(nd_wait_signals(&t,0x100UL,12)==0&&now==12);
 pending_signals=0x300;assert(nd_wait_signals(&t,0x100UL,50)==0x100UL&&pending_signals==0x200);
 pending_signals=0;nd_wait_timer_close(&t);assert(ios==0);
 open_fails=1;now=0;delays=0;assert(nd_wait_signals(&t,0x100UL,7)==0);assert(delays==1);
 open_fails=0;
 assert(forbid==0);DeleteMsgPort(port);assert(ports==0);
 puts("Bounded waits: immediate messages, early wake, timeouts, fractions, temporary timers, polling fallback and multi-signal waits pass.");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='novadock-wait-') as tmp:
    p = Path(tmp); (p / 'wait.c').write_text(program)
    cmd = [os.environ.get('CC', 'cc'), '-std=c89', '-Wall', '-Wextra', '-Wno-unused-function', str(p / 'wait.c'), '-o', str(p / 'wait')]
    if os.environ.get('SANITIZE') == '1': cmd += ['-fsanitize=address,undefined']
    subprocess.run(cmd, check=True); subprocess.run([str(p / 'wait')], check=True)
