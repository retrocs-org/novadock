#!/usr/bin/env python3
"""Exercise production launch-error messages and translated percent formats."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/dock_launch.c').read_text();start=source.index('void process_launch_jobs(');end=source.index('\nunsigned long launch_job_signals',start)
locale=(root/'src/nd_locale.c').read_text();locale=locale[locale.index('static int format_argument('):]
harness=r"""
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ND_LAUNCH_JOBS 4
#define ND_TR_FORMAT_MAX 80
struct NDDock {int unused;};struct NDTask {void *port;};
struct NDIOJob {unsigned long error;};
struct NDTaskPacket {int result;unsigned int bytes;char data[64];};
static struct {struct NDTask task;char label[32];} launch_jobs[4];
static struct NDTaskPacket *result;static int expire,feedback_count;static char feedback[256];
static const char *translation;
static const char *nd_tr(const char *s){return translation ? translation:s;}
static void FreeVec(void *p){free(p);}
static struct NDTaskPacket *nd_task_poll(struct NDTask *task){struct NDTaskPacket *m=result;if(task!=&launch_jobs[0].task)return NULL;if(m || expire)task->port=NULL;result=NULL;return m;}
static void show_feedback(struct NDDock *d,const char *s,int error){(void)d;assert(error);++feedback_count;strcpy(feedback,s);}
"""+locale+source[start:end]+r"""
int main(void){struct NDDock d;char long_text[200];
translation="%s: %u%% belegt%s";assert(nd_trf("%s: %u%% used%s")==translation);
translation="%s: %lu%% belegt%s";assert(strcmp(nd_trf("%s: %u%% used%s"),translation));
translation="%s %n";assert(strcmp(nd_trf("%s %u"),translation));
translation="%s %u";assert(nd_trf("%s %u")==translation);
memset(long_text,'X',sizeof(long_text)-1);long_text[sizeof(long_text)-1]=0;translation=long_text;assert(strcmp(nd_trf("%s %u"),translation));translation=NULL;
strcpy(launch_jobs[0].label,"My application");launch_jobs[0].task.port=&d;result=calloc(1,sizeof(*result));result->bytes=sizeof(struct NDIOJob);((struct NDIOJob *)result->data)->error=205;
process_launch_jobs(&d);assert(feedback_count==1 && strstr(feedback,"My application") && strstr(feedback,"205"));
launch_jobs[0].task.port=&d;result=calloc(1,sizeof(*result));result->result=1;process_launch_jobs(&d);assert(feedback_count==1);
launch_jobs[0].task.port=&d;expire=1;process_launch_jobs(&d);assert(feedback_count==2 && strstr(feedback,"My application") && strstr(feedback,"did not reply"));
return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp)/'feedback.c';p.write_text(harness);exe=Path(tmp)/'feedback'
 subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c89','-Wno-deprecated-declarations','-fsanitize=address,undefined',str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Production launch errors identify the application and DOS error; translated %% and long argument types are validated.')
