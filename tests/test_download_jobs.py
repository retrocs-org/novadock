#!/usr/bin/env python3
"""Exercise the actual download queue's native job lifecycle with fake I/O."""
from pathlib import Path
import subprocess, tempfile, os
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/download/download.c').read_text()
a=s.index('struct Item ');b=s.index('static int exists(',a)
c=s.index('static void close_job(');d=s.index('static unsigned int progress(',c)
h=r"""#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nd_download.h"
#include "nd_interaction.h"
typedef unsigned long ULONG;typedef long LONG;typedef long BPTR;
#define ACCESS_READ 0
#define MODE_OLDFILE 0
#define MODE_NEWFILE 1
#define SYS_Input 1
#define SYS_Output 2
#define SYS_Asynch 3
#define NP_StackSize 4
#define TAG_DONE 0
#define TRUE 1
#define FETCH_MAGIC 0x4e445746UL
#define FETCH_GUARD "NovaDock.Download.Fetch"
struct Node {char *ln_Name;};struct MsgPort {struct Node mp_Node;};struct Message {void *mn_ReplyPort;unsigned int mn_Length;};
struct FetchMessage {struct Message msg;ULONG magic;int kind;LONG result;char command[1800],directory[160];};
static ULONG ticks=100;static int busy,destination,renames,cleaned,freed,replies,command_error;
static struct FetchMessage *incoming;
static void Forbid(void){}static void Permit(void){}
static void *FindPort(const char *name){assert(!strcmp(name,FETCH_GUARD));return busy ? &busy:NULL;}
static void *FindTask(void *unused){(void)unused;return &busy;}
static int exists(const char *path){assert(strstr(path,"Work:")==path);return destination;}
static BPTR Lock(const char *path,int mode){(void)mode;assert(!strcmp(path,"C:curl"));return 1;}
static int NameFromLock(BPTR lock,char *out,size_t size){(void)size;assert(lock);strcpy(out,"System:C/curl");return 1;}
static void UnLock(BPTR lock){assert(lock);}
static char *FilePart(char *path){char *p,*last=path;for(p=path;*p;++p)if(*p==':'||*p=='/')last=p+1;return last;}
static int AddPart(char *path,const char *part,size_t size){assert(strlen(path)+strlen(part)+2<size);if(path[strlen(path)-1]!=':'&&path[strlen(path)-1]!='/')strcat(path,"/");strcat(path,part);return 1;}
static BPTR CreateDir(const char *path){assert(nd_download_staging_dir(path));return 1;}
static ULONG weather_ticks(void){return ticks;}
static struct MsgPort *CreateMsgPort(void){return calloc(1,sizeof(struct MsgPort));}
static void AddPort(struct MsgPort *port){assert(port);}
static void fetch_close_port(struct MsgPort *port){free(port);}
static void fetch_clean(const char *path){assert(nd_download_staging_dir(path));++cleaned;}
static BPTR Open(const char *path,int mode){(void)mode;assert(!strcmp(path,"NIL:"));return 1;}
static void Close(BPTR handle){assert(handle);}
static LONG SystemTags(const char *cmd,...){assert(strstr(cmd," WORK NovaDock.Download."));return command_error ? -1:0;}
static void *GetMsg(struct MsgPort *port){void *m=incoming;assert(port);incoming=NULL;return m;}
static void ReplyMsg(struct Message *m){assert(m);++replies;free(m);}
static void FreeVec(void *m){free(m);++freed;}
static int Rename(const char *from,const char *to){assert(strstr(from,"/response"));assert(!strcmp(to,"Work:Downloads/file.zip"));assert(!destination);++renames;return 1;}
"""+s[a:b]+s[c:d]+r"""
static void setup(void){assert(nd_download_url("https://example.org"));strcpy(tool,"C:curl");strcpy(queue[0].url,"https://example.org/a.zip?x=1&y=2");strcpy(queue[0].target,"Work:Downloads/file.zip");strcpy(queue[1].url,"https://example.org/second");strcpy(queue[1].target,"Work:Downloads/second.zip");count=2;destination=0;busy=0;}
static void message(int kind,int result){incoming=calloc(1,sizeof(*incoming));incoming->magic=FETCH_MAGIC;incoming->msg.mn_Length=sizeof(*incoming);incoming->kind=kind;incoming->result=result;strcpy(incoming->directory,job.directory);}
int main(void){int c;
 setup();busy=1;assert(start_job("Plugins/Download")==0&&!job.port&&count==2);busy=0;
 destination=1;assert(start_job("Plugins/Download")==-1&&!job.port&&count==2);destination=0;
 assert(start_job("Plugins/Download")==1&&job.port&&nd_download_staging_dir(job.directory));assert(strstr(job.directory,"Work:Downloads/")==job.directory);
 assert(strstr(job.command,"--disable --fail --location --proto =http,https --proto-redir =http,https"));assert(strstr(job.command,"--output \"Work:Downloads/NovaDockDownload-"));assert(strstr(job.command," -- \"https://example.org/a.zip?x=1&y=2\""));
 message(1,0);incoming->msg.mn_ReplyPort=&busy;job_events();assert(job.transferred&&replies==1); /* returned to worker ownership */
 /* The real ReplyMsg transfers the request. This stub retains it for inspection. */
 message(2,0);job_events();assert(!job.port&&count==1&&renames==1&&freed==1&&cleaned==1);assert(!strcmp(queue[0].target,"Work:Downloads/second.zip"));
 setup();assert(start_job("Plugins/Download")==1);job.transferred=1;destination=1;message(2,0);job_events();assert(renames==1&&strstr(status,"already exists")&&!job.port&&count==1);
 setup();assert(start_job("Plugins/Download")==1);job.transferred=1;message(2,22);job_events();assert(renames==1&&strstr(status,"curl 22")&&!job.port);
 setup();assert(start_job("Plugins/Download")==1);c=cleaned;ticks+=201;job_events();assert(!job.port&&count==1&&cleaned==c+1);
 setup();assert(start_job("Plugins/Download")==1);job.transferred=1;c=cleaned;ticks+=650UL*50UL+1;job_events();assert(!job.port&&count==1&&cleaned==c); /* worker owns staging until it exits */
 setup();command_error=1;assert(start_job("Plugins/Download")==-1&&!job.port);puts("DOWNLOAD jobs: single worker, full destination staging, fixed curl command, completion/failure, overwrite refusal and timeouts passed.");return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-unused-variable','-Wno-format','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'check.c'),str(ROOT/'src/interaction.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)
