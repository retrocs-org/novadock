/* Native export failure/success checks in the disposable QATests: guest. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_support_save.h"
long __stack=32768;
static int failures;
static void check(int ok,const char *label){printf("%s %s\n",ok?"PASS":"FAIL",label);if(!ok)++failures;}
static int exists(const char *path){BPTR l=Lock((STRPTR)path,ACCESS_READ);if(l)UnLock(l);return l!=0;}
static int write_text(const char *path,const char *text){FILE*f=fopen(path,"wb");int ok;if(!f)return 0;ok=fputs(text,f)>=0;if(fclose(f))ok=0;return ok;}
static int has(const char *path,const char *text){FILE*f=fopen(path,"rb");char b[160];size_t n;if(!f)return 0;n=fread(b,1,sizeof(b)-1,f);b[n]=0;fclose(f);return strstr(b,text)!=NULL;}
static int no_stage(void)
{BPTR l=Lock("QATests:",ACCESS_READ);struct FileInfoBlock*f;int ok=1;if(!l)return 0;f=AllocDosObject(DOS_FIB,NULL);if(!f){UnLock(l);return 0;}if(Examine(l,f))while(ExNext(l,f))if(!strncmp(f->fib_FileName,"ND-report-stage-",16))ok=0;FreeDosObject(DOS_FIB,f);UnLock(l);return ok;}
int main(void)
{
 BPTR guard;char tool[260],source[512],next[512];
 setvbuf(stdout,NULL,_IONBF,0);guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 20;UnLock(guard);
 check(nd_support_find_lha(tool,sizeof(tool)),"find installed LhA");
 if(!nd_support_unique_drawer("QATests:","Export files",source,sizeof(source)))return 10;
 if(!write_text("QATests:Export files/support.txt","Native support snapshot\n")||
    !write_text("QATests:Export files/problem.txt","Optional user details\n")||
    !write_text("QATests:Export files/dock.log","RAM trace after restart\n")||
    !write_text("QATests:Export files/dock-disk.log","Disk trace before crash\n"))return 10;
 check(nd_support_pack_report(tool,source,"QATests:export report.lha"),"compress real report with installed LhA");
 check(exists("QATests:export report.lha")&&has("QATests:Export files/dock-disk.log","before crash"),"archive published while source remains available until caller cleanup");
 check(write_text("QATests:existing.lha","existing user file")&&!nd_support_pack_report(tool,source,"QATests:existing.lha")&&has("QATests:existing.lha","existing user file"),"existing destination remains untouched");
 check(!nd_support_pack_report("QATests:FailLhA",source,"QATests:failed.lha")&&!exists("QATests:failed.lha")&&has("QATests:Export files/support.txt","snapshot")&&no_stage(),"failed compression keeps all uncompressed files and removes partial archives");
 check(!nd_support_pack_report("QATests:RaceLhA",source,"QATests:race.lha")&&has("QATests:race.lha","other writer")&&no_stage(),"destination created during compression is preserved");
 check(nd_support_unique_drawer("QATests:","Export files",next,sizeof(next))&&strcmp(source,next),"drawer names avoid collisions");DeleteFile(next);
 check(write_text("QATests:Export files/unrelated.txt","keep this"),"prepare cleanup ownership check");nd_support_discard_report(source);
 check(has("QATests:Export files/unrelated.txt","keep this")&&!exists("QATests:Export files/dock.log")&&!exists("QATests:Export files/dock-disk.log"),"cleanup removes only collected files, including disk copies");
 puts(failures?"FAIL report archive checks":"PASS report archive checks");return failures?10:0;
}
