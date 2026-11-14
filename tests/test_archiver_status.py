#!/usr/bin/env python3
"""Production status aggregation must preserve dialogs and concurrent jobs."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class ArchiverStatus(unittest.TestCase):
 def test_concurrent_modes_and_terminal_cleanup(self):
  s=(ROOT/'plugin_src/archive/archive.c').read_text();a=s.index('static int collect_status(');f=s[a:s.index('\n}',a)+2]
  h=r"""
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ND_MAX_STATUS_FILES 8
#define ND_TEXT_LEN 32
static char status_subject[32];
#define ND_ARCHIVE_STATUS_PATTERN "test"
#define MEMF_PUBLIC 1
#define MEMF_CLEAR 2
typedef char *STRPTR;
struct AnchorPath {int ap_Strlen;struct {char fib_FileName[48];} ap_Info;};
static const char *states[8];static int count,cursor,removed,dead=-1;
static unsigned long seen_module;
static int status_owner_alive(const char *p,unsigned long module){seen_module=module;return atoi(p+2)!=dead;}
static void *AllocVec(unsigned long n,int flags){(void)flags;return calloc(1,n);}
static void FreeVec(void *p){free(p);}
static int MatchFirst(STRPTR p,struct AnchorPath *a){(void)p;cursor=0;if(!count)return 1;strcpy(a->ap_Info.fib_FileName,"0");return 0;}
static int MatchNext(struct AnchorPath *a){if(++cursor>=count)return 1;sprintf(a->ap_Info.fib_FileName,"%d",cursor);return 0;}
static void MatchEnd(struct AnchorPath *a){(void)a;}
static int read_status_file(const char *p,char *t,unsigned int n){(void)n;strcpy(t,states[atoi(p+2)]);return 1;}
static int remove_status(const char *p){(void)p;++removed;return 0;}
#define remove remove_status
"""+f+r"""
int main(void){char value[32];int running;
 count=1;states[0]="OPTIONS";assert(collect_status(value,sizeof(value),&running)==2);
 assert(!strcmp(value,"OPTIONS")&&running==1&&!removed);
 states[0]="COMPRESSING\tPhotos";assert(collect_status(value,sizeof(value),&running)==2);
 assert(!strcmp(value,"COMPRESSING")&&!removed);
 assert(!strcmp(status_subject,"Photos"));
 count=2;states[1]="EXTRACTING\tdemo.lha";assert(collect_status(value,sizeof(value),&running)==2);
 assert(!strcmp(value,"WORKING 2")&&running==2&&!removed);
 assert(!strcmp(status_subject,"Multiple items"));
 count=3;states[2]="DONE";assert(collect_status(value,sizeof(value),&running)==2);
 assert(running==2&&removed==1);
 count=1;states[0]="CANCELLED";assert(collect_status(value,sizeof(value),&running)==1);
 assert(!strcmp(value,"CANCELLED")&&!running&&removed==2);
 count=0;assert(!collect_status(value,sizeof(value),&running));
 count=1;dead=0;states[0]="COMPRESSING\tShowConfig";
 assert(collect_status(value,sizeof(value),&running)==1);
 assert(!strcmp(value,"INTERRUPTED")&&!running&&removed==3);
 dead=-1;states[0]="COMPRESSING\tActive file\tabcd";
 assert(collect_status(value,sizeof(value),&running)==2);
 assert(running==1&&seen_module==0xabcd&&!strcmp(status_subject,"Active file"));
 count=2;dead=1;states[1]="OPTIONS\tAbandoned\t1234";
 assert(collect_status(value,sizeof(value),&running)==2);
 assert(running==1&&!strcmp(value,"COMPRESSING")&&removed==4);
 count=1;dead=0;states[0]="EXTRACTING\tLost file\tabcd";
 assert(collect_status(value,sizeof(value),&running)==1&&!running);
 assert(!strcmp(value,"INTERRUPTED")&&removed==5);
 states[0]="DONE\tFinished\tabcd";
 assert(collect_status(value,sizeof(value),&running)==1&&!running);
 assert(!strcmp(value,"DONE")&&removed==6);return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'status.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',str(p/'status.c'),'-o',str(p/'status')],check=True)
   subprocess.run([str(p/'status')],check=True)
 def test_owner_identity_and_task_address_reuse(self):
  source=(ROOT/'plugin_src/archive/archive.c').read_text()
  a=source.index('static int status_task_matches(')
  functions=source[a:source.index('static void write_status(',a)]
  harness=r"""
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NT_PROCESS 13
#define NT_TASK 1
#define BADDR(p) ((void *)(p))
struct Node {struct Node *ln_Succ;int ln_Type;};
struct Task {struct Node tc_Node;};
struct Process {struct Task pr_Task;unsigned long pr_CLI;};
struct CommandLineInterface {unsigned long cli_Module,cli_CommandName;};
struct ExecBase {struct {struct Node *lh_Head;}TaskReady,TaskWait;};
static struct ExecBase base,*SysBase=&base;
static struct Task *current;
static int forbidden;
static struct Task *FindTask(void *p){(void)p;return current;}
static void Forbid(void){assert(!forbidden);forbidden=1;}
static void Permit(void){assert(forbidden);forbidden=0;}
static char *FilePart(char *s){char *b=s;while(*s){if(*s==':'||*s=='/')b=s+1;++s;}return b;}
"""+functions+r"""
int main(void){
 struct Process helper,self;struct CommandLineInterface cli;
 struct Node tail;unsigned char command[80];char path[100];
 memset(&helper,0,sizeof(helper));memset(&self,0,sizeof(self));memset(&tail,0,sizeof(tail));
 memset(&cli,0,sizeof(cli));helper.pr_Task.tc_Node.ln_Type=NT_PROCESS;
 helper.pr_CLI=(unsigned long)&cli;cli.cli_Module=0xabc;
 strcpy((char *)command+1,"Bin:Plugins/ArchiveExtractor");command[0]=(unsigned char)strlen((char *)command+1);cli.cli_CommandName=(unsigned long)command;
 helper.pr_Task.tc_Node.ln_Succ=&tail;current=&self.pr_Task;
 base.TaskReady.lh_Head=&tail;base.TaskWait.lh_Head=&helper.pr_Task.tc_Node;
 sprintf(path,"T:NovaDockArchive-%lx.status",(unsigned long)&helper.pr_Task);
 assert(status_owner_alive(path,0xabc)&&!forbidden);
 assert(!status_owner_alive(path,0xdef)&&!forbidden);
 assert(status_owner_alive(path,0)&&!forbidden);
 strcpy((char *)command+1,"C:OtherProgram");command[0]=(unsigned char)strlen((char *)command+1);
 assert(!status_owner_alive(path,0)); /* same task address, another command */
 base.TaskWait.lh_Head=&tail;assert(!status_owner_alive(path,0xabc));
 current=&helper.pr_Task;assert(status_owner_alive(path,0xabc));
 helper.pr_Task.tc_Node.ln_Type=NT_TASK;assert(!status_owner_alive(path,0xabc));
 helper.pr_Task.tc_Node.ln_Type=NT_PROCESS;helper.pr_CLI=0;assert(!status_owner_alive(path,0xabc));
 assert(!status_owner_alive("T:NovaDockArchive-1.status",0xabc)); /* no stale pointer dereference */
 assert(!status_owner_alive("T:NovaDockArchive-0.status",0));
 assert(!status_owner_alive("T:unrelated.status",0));
 return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'owner.c').write_text(harness)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',str(p/'owner.c'),'-o',str(p/'owner')],check=True)
   subprocess.run([str(p/'owner')],check=True)
if __name__=='__main__':unittest.main()
