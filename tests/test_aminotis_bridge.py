#!/usr/bin/env python3
"""Exercise the real asynchronous bridge with a deterministic service transport."""
from pathlib import Path
import subprocess,tempfile,os
ROOT=Path(__file__).resolve().parents[1]
code=r'''#define AN_HOST
#include "src/aminotis_sdk/aminotis.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct MsgPort {int mp_SigBit;};
struct ANClient {struct MsgPort *reply;void *pending;};
static struct MsgPort port={4};
static int available=1,reply_ready,submissions,closes,reject,response_code,capability=1;
static struct ANRequest sent;
static void Forbid(void){}static void Permit(void){}
static void *FindPort(const char *name){assert(!strcmp(name,AN_PORT));return available?&port:NULL;}
static void an_request(struct ANRequest *r,enum ANOperation op){memset(r,0,sizeof(*r));r->operation=op;}
static int an_submit(struct ANClient *c,const struct ANRequest *r){assert(!c->pending);if(reject)return AN_BUSY;c->reply=&port;c->pending=&port;sent=*r;++submissions;return AN_OK;}
static int an_receive(struct ANClient *c,struct ANResponse *r){if(!reply_ready)return 0;reply_ready=0;c->pending=NULL;memset(r,0,sizeof(*r));r->result=response_code;r->id=1;r->status.capabilities=capability ? AN_CAP_HISTORY:0;return 1;}
static void an_close(struct ANClient *c){++closes;c->pending=NULL;c->reply=NULL;}
#define ND_AMINOTIS_TEST
#include "src/nd_aminotis.c"
int main(void){int i;char longtext[800];struct NDAmiNotisStats report;
 nd_aminotis_post(0,"TIMER","Timer finished","Done",0);assert(!count&&!submissions);
 available=0;nd_aminotis_post(1,"TIMER","Timer finished","Done",0);assert(!count&&!submissions);available=1;nd_aminotis_stats(&report);assert(report.received==1&&report.unavailable==1&&report.dropped==1);
 nd_aminotis_post(1,"TIMER","Timer finished","Done",0);assert(count==1&&checking&&sent.operation==AN_STATUS&&nd_aminotis_signals()==16);
 reply_ready=1;nd_aminotis_tick(1,0);assert(!checking&&ready&&sent.operation==AN_POST&&!strcmp(sent.event.app,"NovaDock")&&!strcmp(sent.event.source,"TIMER")&&!strcmp(sent.event.title,"Timer finished"));
 reply_ready=1;nd_aminotis_tick(1,0);assert(!count&&!client.pending);nd_aminotis_stats(&report);assert(report.delivered==1&&report.last_failure==ND_AMINOTIS_FAILURE_UNAVAILABLE);
 memset(longtext,'X',sizeof(longtext));longtext[sizeof(longtext)-1]=0;
 for(i=0;i<30;++i)nd_aminotis_post(1,"DOWNLOAD","Progress",longtext,1);
 assert(count==ND_NOTIS_QUEUE&&client.pending&&strlen(sent.event.body)==512);nd_aminotis_stats(&report);assert(report.queue_full==22&&report.dropped==23&&report.queued==8&&report.pending);
 nd_aminotis_tick(1,2999);assert(client.pending);nd_aminotis_tick(1,1);assert(!client.pending&&!count&&!nd_aminotis_signals());nd_aminotis_stats(&report);assert(report.timeouts==1&&report.dropped==31&&report.received==32&&report.last_result==AN_TIMEOUT);
 nd_aminotis_post(1,"WATCH","Changed","Test",0);assert(count);nd_aminotis_tick(0,0);assert(!count&&!client.reply);
 reject=1;nd_aminotis_post(1,"WATCH","Changed","Test",0);assert(!count&&!client.reply);
 nd_aminotis_stats(&report);assert(report.cancelled==1&&report.rejected==1&&report.dropped==33);
 reject=0;capability=0;nd_aminotis_post(1,"TEST","Capability","Test",0);reply_ready=1;nd_aminotis_tick(1,0);nd_aminotis_stats(&report);assert(!report.queued&&report.last_failure==ND_AMINOTIS_FAILURE_CAPABILITY&&report.rejected==2);
 capability=1;nd_aminotis_post(1,"TEST","Rejected","Test",0);reply_ready=1;nd_aminotis_tick(1,0);response_code=AN_INVALID;reply_ready=1;nd_aminotis_tick(1,0);nd_aminotis_stats(&report);assert(report.rejected==3&&report.last_result==AN_INVALID);
 nd_aminotis_reset_stats();nd_aminotis_stats(&report);assert(!report.received&&!report.dropped&&!report.last_failure);
 stats.dropped=0xffffffffUL;stats.unavailable=0xffffffffUL;available=0;nd_aminotis_post(1,"TEST","Saturate","Test",0);nd_aminotis_stats(&report);assert(report.dropped==0xffffffffUL&&report.unavailable==0xffffffffUL);
 puts("AmiNotis bridge: opt-in, unavailable service, capability negotiation, nonblocking delivery, bounded queue, text limits, timeout and disable cleanup passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(ROOT),'-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
