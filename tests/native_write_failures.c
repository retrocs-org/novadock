/* Real packaged workers against a write-protected disposable UAE filesystem. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_task.h"
#include "nd_text.h"
#include "nd_colour_job.h"
#include "nd_qr_job.h"
#include "nd_bench.h"
long __stack=65536;
static int exists(const char *p){BPTR l=Lock((STRPTR)p,ACCESS_READ);if(!l)return 0;UnLock(l);return 1;}
static int command(const char *s){LONG r=SystemTags((STRPTR)s,TAG_DONE);printf("COMMAND %ld: %s\n",r,s);return r==0;}
static int write_text(const char *p,const char *s){FILE *f=fopen(p,"wb");int r;if(!f)return 0;r=fputs(s,f)>=0;return fclose(f)==0&&r;}
static int text_is(const char *p,const char *s){FILE *f=fopen(p,"rb");char b[2048];size_t n;if(!f)return 0;n=fread(b,1,sizeof(b)-1,f);b[n]=0;fclose(f);return !strcmp(b,s);}
static int worker(const char *program,unsigned int action,void *data,unsigned int bytes,int expected)
{struct NDTask t;struct NDTaskPacket *m=NULL;int i,ok;char path[160];memset(&t,0,sizeof(t));sprintf(path,"QATests:NovaDock/Plugins/%s",program);if(!nd_task_start(&t,path,"NovaDock.QA.WriteFailure",action,data,bytes))return 0;for(i=0;i<1500&&!m;++i){m=nd_task_poll(&t);if(!m)Delay(2);}ok=m&&m->bytes==bytes&&m->result==expected;if(m){printf("WORKER %s action %u: result %d expected %d, bytes %u\n",program,action,m->result,expected,m->bytes);if(m->bytes==bytes)memcpy(data,m->data,bytes);FreeVec(m);}nd_task_close(&t);return ok;}
static int clean_drawer(const char *path,int allowed)
{BPTR l=Lock((STRPTR)path,ACCESS_READ);struct FileInfoBlock *fib;int n=0,ok=1;if(!l)return 0;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(l);return 0;}if(!Examine(l,fib))ok=0;while(ok&&ExNext(l,fib)){if(!strncmp(fib->fib_FileName,"ndt",3)||strstr(fib->fib_FileName,".new"))ok=0;if(fib->fib_DirEntryType<0)++n;}FreeDosObject(DOS_FIB,fib);UnLock(l);return ok&&n==allowed;}
#define CHECK(expr,msg) do{if(!(expr)){printf("FAIL: %s (IoErr %ld)\n",msg,IoErr());goto done;}puts("PASS: " msg);}while(0)
int main(void)
{struct ColourJob *c;struct QRJob *q;struct NDBenchJob *b;struct NDColourPalette palette;int locked=0,result=10;BPTR file;char qrpath[240],benchpath[240],report[2048];struct NDBenchResult measured;
 const char *old="NDCOLOUR1\n112233|Original\n",*backup="NDCOLOUR1\n445566|Backup\n";
 if(!exists("QATests:ISOLATED-QA-ONLY")||!exists("AutoRun:Startup"))return 20;((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 c=AllocVec(sizeof(*c),MEMF_PUBLIC|MEMF_CLEAR);q=AllocVec(sizeof(*q),MEMF_PUBLIC|MEMF_CLEAR);b=AllocVec(sizeof(*b),MEMF_PUBLIC|MEMF_CLEAR);if(!c||!q||!b)goto done;
 CHECK(command("QATests:NovaDock/NovaDock QUIT"),"stopped the disposable guest startup dock before window checks");Delay(10);
 CHECK(command("QATests:NovaDock/Plugins/Bench PANEL-SELFTEST"),"BENCH actual window supports retry controls and screen bounds");
 CHECK(command("MakeDir QATests:WriteFailure QATests:WriteFailure/NovaDock QATests:WriteFailure/NovaDock/Colours QATests:WriteFailure/NovaDock/Bench"),"prepared isolated drawers");
 CHECK(command("Assign ENVARC: QATests:WriteFailure"),"redirected palette and report saves to disposable volume");
 CHECK(write_text("ENVARC:NovaDock/Colours/Palette",old)&&write_text("ENVARC:NovaDock/Colours/Palette.bak",backup)&&write_text("ENVARC:NovaDock/Bench/bench-prior.txt","Existing report\n")&&write_text("QATests:WriteFailure/existing.iff","Existing image\n"),"prepared existing files");
 CHECK(nd_colour_add(&c->palette,"Edited colour",0xabcdefUL),"prepared unsaved colour");c->rgb=0xabcdefUL;strcpy(c->name,"Edited colour");
 strcpy(q->text,"https://novadock.retrocs.org/write-failure-qa");strcpy(q->path,"QATests:WriteFailure/12345678901234567890123456.iff");strcpy(qrpath,q->path);
 CHECK(worker("QR",1,q,sizeof(*q),1)&&q->qr.size>0,"QR generated before failed export");
 CHECK(command("C:Lock QATests: ON >AutoRun:write-lock.log"),"locked disposable volume");locked=1;
 file=Open("QATests:WriteFailure/denied",MODE_NEWFILE);if(file)Close(file);CHECK(!file&&IoErr()==ERROR_DISK_WRITE_PROTECTED,"native DOS confirms disk write protection");
 CHECK(worker("Colour",4,c,sizeof(*c),0)&&strstr(c->problem,"write-protected"),"COLOUR reports write protection");
 CHECK(c->rgb==0xabcdefUL&&!strcmp(c->name,"Edited colour")&&c->palette.count==1&&c->palette.colours[0].rgb==0xabcdefUL,"COLOUR retains edited colour/name/candidate palette");
 CHECK(text_is("ENVARC:NovaDock/Colours/Palette",old)&&text_is("ENVARC:NovaDock/Colours/Palette.bak",backup)&&clean_drawer("ENVARC:NovaDock/Colours",2),"COLOUR original and backup unchanged; no partial file");
 CHECK(worker("QR",3,q,sizeof(*q),ND_TEXT_PROTECTED),"QR reports write protection");
 CHECK(q->qr.size>0&&!strcmp(q->text,"https://novadock.retrocs.org/write-failure-qa")&&!strcmp(q->path,qrpath)&&!exists(q->path)&&clean_drawer("QATests:WriteFailure",1),"QR retains text/path/matrix and leaves no partial file");
 b->duration=200;CHECK(worker("Bench",1,b,sizeof(*b),2)&&b->save_result==ND_TEXT_PROTECTED,"BENCH completes with explicit unsaved/write-protected status");
 CHECK(b->result.cpu&&b->result.memory&&b->result.graphics&&b->report[0]&&!exists(b->path)&&text_is("ENVARC:NovaDock/Bench/bench-prior.txt","Existing report\n")&&clean_drawer("ENVARC:NovaDock/Bench",1),"BENCH retains all measurements/report; prior report unchanged and no partial file");
 measured=b->result;strcpy(benchpath,b->path);strcpy(report,b->report);
 CHECK(worker("Bench",3,b,sizeof(*b),2)&&b->save_result==ND_TEXT_PROTECTED&&!memcmp(&measured,&b->result,sizeof(measured))&&!strcmp(report,b->report)&&!strcmp(benchpath,b->path),"BENCH retry while protected retains identical report and measurements");
 CHECK(command("C:Lock QATests: OFF >AutoRun:write-unlock.log"),"unlocked disposable volume");locked=0;
 CHECK(worker("Colour",4,c,sizeof(*c),1)&&nd_colour_read("ENVARC:NovaDock/Colours/Palette",&palette)==1&&palette.count==1&&palette.colours[0].rgb==0xabcdefUL&&text_is("ENVARC:NovaDock/Colours/Palette.bak",old),"COLOUR retry saves edits and backs up original");
 CHECK(worker("QR",3,q,sizeof(*q),1)&&exists(q->path)&&strlen(FilePart(q->path))==30,"QR retry exports a 30-character filename");
 strcpy(q->path,"QATests:WriteFailure/existing.iff");CHECK(worker("QR",3,q,sizeof(*q),ND_TEXT_EXISTS)&&text_is(q->path,"Existing image\n"),"QR never overwrites existing image");
 strcpy(q->path,"QATests:WriteFailure/Missing/out.iff");CHECK(worker("QR",3,q,sizeof(*q),ND_TEXT_PATH),"QR reports missing destination drawer");
 CHECK(worker("Bench",3,b,sizeof(*b),1)&&b->save_result==1&&text_is(b->path,report)&&!memcmp(&measured,&b->result,sizeof(measured))&&!strcmp(benchpath,b->path)&&strlen(FilePart(b->path))<=30,"BENCH retry saves the original report without rerunning measurements");
 CHECK(worker("Bench",3,b,sizeof(*b),2)&&b->save_result==ND_TEXT_EXISTS&&text_is(b->path,report),"BENCH retry never overwrites an existing report");
 puts("PASS: packaged COLOUR, QR and BENCH native write-failure recovery.");result=0;
 done:if(locked)command("C:Lock QATests: OFF >AutoRun:write-unlock.log");if(c)FreeVec(c);if(q)FreeVec(q);if(b)FreeVec(b);return result;
}
