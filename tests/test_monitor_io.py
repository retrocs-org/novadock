#!/usr/bin/env python3
"""Run the production DOS worker reads through a small POSIX adapter."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'plugin_src/monitor/monitor.c').read_text()
def function(name):
    a=source.index('static void '+name+'(');b=source.index('\n}',a)+2
    return source[a:b]
shim=r'''
#define _DEFAULT_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "nd_monitor.h"
#define ACCESS_READ 0
#define MODE_OLDFILE 0
#define DOS_FIB 0
#define ERROR_NO_MORE_ENTRIES 232
#define OFFSET_BEGINNING SEEK_SET
typedef long LONG;
struct DateStamp {long ds_Days,ds_Minute,ds_Tick;};
struct FileInfoBlock {long fib_DirEntryType,fib_Size,fib_DiskKey;char fib_FileName[108];struct DateStamp fib_Date;};
struct Handle {FILE *file;DIR *dir;char path[512];};typedef struct Handle *BPTR;
struct MonitorMessage {int result,reset,skip;struct NDWatchSnapshot snapshot;struct NDLogCursor cursor;unsigned int length;unsigned char data[ND_LOG_CHUNK];};
static long error;
static void *AllocDosObject(int type,void *tags){(void)type;(void)tags;return calloc(1,sizeof(struct FileInfoBlock));}
static void FreeDosObject(int type,void *p){(void)type;free(p);}
static BPTR Lock(const char *path,int access){BPTR h;(void)access;h=calloc(1,sizeof(*h));assert(h);h->dir=opendir(path);if(!h->dir){free(h);return NULL;}strcpy(h->path,path);return h;}
static void UnLock(BPTR h){closedir(h->dir);free(h);}
static void info(struct FileInfoBlock *f,const struct stat *s){f->fib_DirEntryType=S_ISDIR(s->st_mode)?1:-1;f->fib_Size=s->st_size;f->fib_DiskKey=s->st_ino;f->fib_Date.ds_Days=s->st_mtime/86400;f->fib_Date.ds_Minute=s->st_mtime%86400/60;f->fib_Date.ds_Tick=s->st_mtime%60*50;}
static int Examine(BPTR h,struct FileInfoBlock *f){struct stat s;if(stat(h->path,&s))return 0;info(f,&s);return 1;}
static int ExNext(BPTR h,struct FileInfoBlock *f){struct dirent *d;struct stat s;char path[700];while((d=readdir(h->dir))!=NULL){if(!strcmp(d->d_name,".") || !strcmp(d->d_name,".."))continue;sprintf(path,"%s/%s",h->path,d->d_name);if(stat(path,&s)){error=5;return 0;}info(f,&s);strncpy(f->fib_FileName,d->d_name,107);return 1;}error=232;return 0;}
static long IoErr(void){return error;}
static BPTR Open(const char *path,int mode){BPTR h=calloc(1,sizeof(*h));(void)mode;assert(h);h->file=fopen(path,"rb");if(!h->file){free(h);return NULL;}return h;}
static void Close(BPTR h){fclose(h->file);free(h);}
static int ExamineFH(BPTR h,struct FileInfoBlock *f){struct stat s;if(fstat(fileno(h->file),&s))return 0;info(f,&s);return 1;}
static long Read(BPTR h,void *buf,long n){size_t got=fread(buf,1,n,h->file);return ferror(h->file)?-1:(long)got;}
static long Seek(BPTR h,long pos,int mode){long old=ftell(h->file);return fseek(h->file,pos,mode)?-1:old;}
'''
main=r'''
static struct MonitorMessage m;
static struct NDLogCursor saved;
static void write_file(const char *path,const char *mode,const char *text){FILE *f=fopen(path,mode);assert(f);fputs(text,f);assert(!fclose(f));}
int main(void){
 char name[80];unsigned int i;FILE *f;
 assert(!mkdir("drawer",0700));strcpy(m.path,"drawer");scan_drawer(&m);assert(m.result==1 && m.snapshot.valid && !m.snapshot.count);
 write_file("drawer/first","w","one");memset(&m,0,sizeof(m));strcpy(m.path,"drawer");scan_drawer(&m);assert(m.result==1 && m.snapshot.count==1 && m.snapshot.entries[0].size==3);
 for(i=0;i<256;++i){sprintf(name,"drawer/%u",i);write_file(name,"w","");}
 memset(&m,0,sizeof(m));strcpy(m.path,"drawer");scan_drawer(&m);assert(m.result==-2 && !m.snapshot.valid && m.snapshot.count==256);
 memset(&m,0,sizeof(m));strcpy(m.path,"absent");scan_drawer(&m);assert(m.result==0);
 write_file("log","w","initial\n");memset(&m,0,sizeof(m));strcpy(m.path,"log");read_log(&m);assert(m.result==1 && m.length==8 && !m.reset && m.cursor.offset==8);saved=m.cursor;
 write_file("log","a","warning\n");memset(&m,0,sizeof(m));strcpy(m.path,"log");m.cursor=saved;read_log(&m);assert(m.result==1 && m.length==8 && !m.reset && !memcmp(m.data,"warning\n",8));saved=m.cursor;
 memset(&m,0,sizeof(m));strcpy(m.path,"log");m.cursor=saved;read_log(&m);assert(m.result==1 && !m.length && !m.reset);
 write_file("log","w","new\n");memset(&m,0,sizeof(m));strcpy(m.path,"log");m.cursor=saved;read_log(&m);assert(m.result==1 && m.reset && m.length==4);saved=m.cursor;
 assert(!rename("log","old"));write_file("log","w","same length, different inode\n");memset(&m,0,sizeof(m));strcpy(m.path,"log");m.cursor=saved;read_log(&m);assert(m.result==1 && m.reset && m.length==29);
 f=fopen("biglog","w");assert(f);for(i=0;i<20000;++i)fputc('a',f);fputs("\nERROR tail\n",f);fclose(f);memset(&m,0,sizeof(m));strcpy(m.path,"biglog");read_log(&m);assert(m.result==1 && m.length==8192 && m.skip && m.cursor.offset==20012);
 saved=m.cursor;memset(&m,0,sizeof(m));strcpy(m.path,"missing");m.cursor=saved;read_log(&m);assert(!m.result && m.cursor.offset==saved.offset);
 puts("Production WATCH/LOGS worker: append, rotation, truncation, tail bounds and unavailable paths passed.");return 0;
}
'''
# Match only the payload fields used by the real worker functions.
shim=shim.replace('struct MonitorMessage {int','struct MonitorMessage {char path[ND_MONITOR_PATH];int')
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp);(p/'check.c').write_text(shim+function('scan_drawer')+'\n'+function('read_log')+'\n'+main)
    flags=['-g','-O1','-fsanitize=address,undefined'] if os.environ.get('SANITIZE')=='1' else []
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Wno-deprecated-declarations',*flags,'-I'+str(ROOT/'plugin_src/monitor'),str(p/'check.c'),str(ROOT/'plugin_src/monitor/nd_monitor.c'),'-o',str(p/'check')],check=True)
    subprocess.run([str(p/'check')],cwd=p,check=True)
