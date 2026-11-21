"""Exercise asynchronous catalogue completion, selection changes and failures."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(source,name):
 start=source.rfind('static ',0,source.index(name));brace=source.index('{',start);end=brace+1;depth=1
 while depth:
  if source[end]=='{':depth+=1
  elif source[end]=='}':depth-=1
  end+=1
 return source[start:end]
s=(ROOT/'prefs/novadockmanagerprefs.c').read_text()
code=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct Message {unsigned short mn_Length;void *mn_ReplyPort;};struct MsgPort {int dummy;};
#include "nd_catalog_job.h"
#include "nd_options.h"
#define D_LINES 7
#define D_TEXT 100
#define GA_Disabled 1
#define GTLV_Labels 2
#define GTLV_Selected 3
#define GTLV_MakeVisible 4
#define TAG_DONE 0
typedef void *APTR;typedef unsigned long ULONG;struct Node {int dummy;};struct List {int dummy;};struct NDPrefsLayout {int dummy;};
#include "nd_package_install.h"
#define PACKAGE_FILES 4
'''
a=s.index('struct ManagerPage {');b=s.index('\n};',a)+3;code+=s[a:b]+r'''
static int starts,force_flag,fail_start,show_calls,action,dropped;static char selected_request[20];
static struct NDTaskPacket *answer;
static void FreeVec(void *p){free(p);}
static void NewList(struct List *p){(void)p;}
static void GT_SetGadgetAttrs(void *g,void *w,void *x,...){(void)g;(void)w;(void)x;}
static void say(struct ManagerPage *p,const char *text,int beep){(void)beep;strncpy(p->message,text,sizeof(p->message)-1);}
static void nd_task_close(struct NDTask *job){memset(job,0,sizeof(*job));}
static struct NDTaskPacket *nd_task_poll(struct NDTask *job){struct NDTaskPacket *r=answer;answer=NULL;if(dropped){dropped=0;nd_task_close(job);return NULL;}if(r)nd_task_close(job);return r;}
int nd_catalog_job_start(struct NDTask *job,unsigned int kind,const char *id,int force){++starts;force_flag=force;action=kind;strcpy(selected_request,id ? id:"");if(fail_start)return 0;job->port=(struct MsgPort *)1;return 1;}
static void build_rows(struct ManagerPage *p){int i;p->count=p->installed_count;for(i=0;i<p->count;++i)strcpy(p->ids[i],p->installed[i].id);if(p->count)p->selected=0;}
static int manager_manifest(struct ManagerPage *p,const char *id);
static void show_details(struct ManagerPage *p){++show_calls;if(!p->catalog_loading&&p->selected>=0&&p->selected<p->count)manager_manifest(p,p->ids[p->selected]);}
'''
for name in ['catalog_controls(','manager_manifest(','rescan(','catalog_result(']:
 # The forward declaration is not a function definition.
 text=s
 if name=='manager_manifest(':text=s[s.index('static void catalog_controls('):]
 code+='\n'+function(text,name)
code+=r'''
static struct NDCatalogWork *reply(unsigned int kind,int result,int count){struct NDCatalogWork *w;answer=calloc(1,sizeof(*answer));assert(answer);answer->action=kind;answer->result=result;answer->bytes=sizeof(*w);w=(struct NDCatalogWork *)answer->data;w->count=count;return w;}
int main(void){struct ManagerPage *p=calloc(1,sizeof(*p));struct NDConfig *cfg=calloc(1,sizeof(*cfg));struct NDCatalogWork *w;int n;assert(p&&cfg&&sizeof(*w)<=ND_TASK_DATA);nd_default_config(cfg);p->cfg=cfg;p->win=(struct Window *)1;
 rescan(p,"CUSTOM_B");assert(p->catalog_loading&&p->catalog_job.port&&force_flag&&action==ND_CATALOG_INVENTORY&&!p->count);
 w=reply(ND_CATALOG_INVENTORY,0,2);strcpy(w->value.rows[0].id,"CUSTOM_A");strcpy(w->value.rows[1].id,"CUSTOM_B");catalog_result(p);
 assert(!p->catalog_loading&&p->count==2&&p->selected==1&&action==ND_CATALOG_LOOKUP&&!strcmp(selected_request,"CUSTOM_B"));
 p->selected=0;n=starts;show_details(p);assert(starts==n);w=reply(ND_CATALOG_LOOKUP,0,1);strcpy(w->value.entry.manifest.name,"Beta");catalog_result(p);assert(starts==n+1&&!strcmp(selected_request,"CUSTOM_A"));
 w=reply(ND_CATALOG_LOOKUP,0,1);strcpy(w->value.entry.manifest.name,"Alpha");catalog_result(p);assert(!p->catalog_job.port&&!strcmp(p->entry.manifest.name,"Alpha"));
 assert(manager_manifest(p,"CLOCK")==1);assert(manager_manifest(p,"CUSTOM_A")==1&&!strcmp(p->entry.manifest.name,"Alpha"));
 rescan(p,NULL);reply(ND_CATALOG_INVENTORY,-1,0);catalog_result(p);assert(!p->catalog_loading&&!p->catalog_job.port&&strstr(p->message,"failed"));
 fail_start=1;rescan(p,NULL);assert(!p->catalog_loading&&!p->catalog_job.port&&strstr(p->message,"could not start"));fail_start=0;
 rescan(p,NULL);reply(ND_CATALOG_INVENTORY,0,ND_PLUGIN_CATALOG_MAX+1);catalog_result(p);assert(!p->catalog_loading&&!p->catalog_job.port&&strstr(p->message,"failed"));
 rescan(p,NULL);dropped=1;catalog_result(p);assert(!p->catalog_loading&&!p->catalog_job.port&&strstr(p->message,"did not reply"));
 rescan(p,NULL);reply(99,0,1);catalog_result(p);assert(!p->catalog_loading&&!p->catalog_job.port&&strstr(p->message,"failed"));
 nd_task_close(&p->catalog_job);free(cfg);free(p);puts("Async inventory: completion, retained selection, coalesced/stale details, builtin/cache switching and failures passed.");return 0;}
'''
# Real functions declared by nd_task.h have external linkage.
code=code.replace('static void nd_task_close(', 'void nd_task_close(').replace('static struct NDTaskPacket *nd_task_poll(', 'struct NDTaskPacket *nd_task_poll(')
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),'-I'+str(ROOT/'prefs'),'-I'+str(ROOT/'plugin_src/common'),str(p/'test.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),str(ROOT/'src/nd_manifest.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],check=True)
