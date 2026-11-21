/* Isolated Workbench checks: no network, real devices or user settings. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_task.h"
#include "nd_bench.h"
long __stack=65536;
static int command(const char *s)
{BPTR in,out;LONG rc;in=Open("NIL:",MODE_OLDFILE);out=Open("QATests:tool.log",MODE_NEWFILE);if(!in||!out)return 0;rc=SystemTags((STRPTR)s,SYS_Input,in,SYS_Output,out,NP_StackSize,65536UL,TAG_DONE);Close(in);Close(out);printf("%s: %ld\n",s,rc);return rc==0;}
static int guard(void){int live;Forbid();live=FindPort("NovaDock.QA.Bench")!=NULL;Permit();return live;}
static int files(void)
{BPTR lock;struct FileInfoBlock *fib;int n=0;lock=Lock("ENVARC:NovaDock/Bench",ACCESS_READ);if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return -1;}if(Examine(lock,fib))while(ExNext(lock,fib))if(fib->fib_DirEntryType<0)++n;FreeDosObject(DOS_FIB,fib);UnLock(lock);return n;}
int main(int argc,char **argv)
{struct NDTask task;struct NDTaskPacket *m;struct NDBenchJob *j;int i,baseline,result=10;memset(&task,0,sizeof(task));j=AllocVec(sizeof(*j),MEMF_PUBLIC|MEMF_CLEAR);if(!j)return 10;
 if(!command("QATests:Bench SELFTEST")||!command("QATests:QR SELFTEST")||!command("QATests:QR VIEW-SELFTEST")||!command("QATests:Moon SELFTEST")||!command("QATests:Colour SELFTEST")||!command("QATests:Colour SAMPLE-SELFTEST"))goto done;
 baseline=files();if(baseline<0)goto done;j->duration=1000;if(!nd_task_start(&task,"QATests:Bench","NovaDock.QA.Bench",1,j,sizeof(*j)))goto done;
 for(i=0;i<150&&!guard();++i){m=nd_task_poll(&task);if(m){FreeVec(m);goto done;}Delay(2);}if(!guard())goto done;
 nd_task_close(&task);for(i=0;i<150&&guard();++i)Delay(2);if(guard()||files()!=baseline)goto done;
 puts("PASS: running BENCH worker cancelled without writing a partial result.");
 j->duration=200;if(!nd_task_start(&task,"QATests:Bench","NovaDock.QA.Bench",1,j,sizeof(*j)))goto done;
 m=NULL;for(i=0;i<750&&!m;++i){m=nd_task_poll(&task);if(!m)Delay(2);}if(!m)goto done;
 if(m->result!=1||m->bytes!=sizeof(*j)){FreeVec(m);goto done;}*j=*(struct NDBenchJob *)m->data;FreeVec(m);if(strlen(strrchr(j->path,'/')+1)>30)goto done;
 if(!j->result.cpu||!j->result.memory||!j->result.graphics||files()!=baseline+1)goto done;
 puts("PASS: all three benchmark stages complete in a separate worker and save a dated report.");
 if(!nd_task_start(&task,"QATests:Bench","NovaDock.QA.Bench",2,j,sizeof(*j)))goto done;m=NULL;for(i=0;i<750&&!m;++i){m=nd_task_poll(&task);if(!m)Delay(2);}if(!m)goto done;i=m->result==1&&m->bytes==sizeof(*j)&&((struct NDBenchJob *)m->data)->count==1;FreeVec(m);if(!i)goto done;
 if(!DeleteFile(j->path)||files()!=baseline)goto done;puts("PASS: saved benchmark history reads back and disposable fixture cleans up.");if(argc>1&&!strcmp(argv[1],"ROOMS")&&!command("QATests:RoomsCheck"))goto done;result=0;
 done:nd_task_close(&task);FreeVec(j);if(result)puts("FAIL: 0.5 native tools did not finish.");return result;}
