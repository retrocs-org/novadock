"""Exercise native export transactions with controlled DOS write/close/rename failures."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'plugin_src/common/nd_text.c').read_text();source=source[source.index('static unsigned long export_serial;'):]
code=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include "nd_text.h"
typedef long LONG;typedef unsigned long ULONG;typedef char *STRPTR;typedef void *APTR;typedef void *BPTR;
#define ACCESS_READ 0
#define MODE_NEWFILE 1
#define ERROR_OBJECT_EXISTS 203
#define ERROR_DIR_NOT_FOUND 204
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_INVALID_COMPONENT_NAME 210
#define ERROR_OBJECT_WRONG_TYPE 212
#define ERROR_DISK_WRITE_PROTECTED 214
#define ERROR_DISK_FULL 221
#define ERROR_DELETE_PROTECTED 222
#define ERROR_WRITE_PROTECTED 223
#define ERROR_READ_PROTECTED 224
struct DateStamp {long ds_Tick;};
static LONG error;static int fault,collisions;static char last_temp[320];
static LONG IoErr(void){return error;}
static void *FindTask(void *unused){(void)unused;return (void *)0x12345678;}
static void DateStamp(struct DateStamp *d){d->ds_Tick=17;}
static char *FilePart(char *p){char *a=strrchr(p,'/'),*b=strrchr(p,':');return a&&(!b||a>b) ? a+1:b ? b+1:p;}
static const char *host(const char *p){return strchr(p,':')+1;}
static BPTR Lock(STRPTR p,int mode){struct stat st;(void)mode;if(strstr(p,"ndt")&&collisions){--collisions;return (BPTR)1;}if(!stat(host(p),&st))return (BPTR)1;error=ERROR_OBJECT_NOT_FOUND;return NULL;}
static void UnLock(BPTR lock){(void)lock;}
static BPTR Open(STRPTR p,int mode){FILE *f;(void)mode;assert(strlen(FilePart(p))<=30);strcpy(last_temp,p);if(fault==1){error=ERROR_DISK_WRITE_PROTECTED;return NULL;}f=fopen(host(p),"wb");error=f ? 0:ERROR_DIR_NOT_FOUND;return f;}
static LONG Write(BPTR file,APTR data,unsigned int n){if(fault==2){fwrite(data,1,n/2,file);error=ERROR_DISK_FULL;return n/2;}return (LONG)fwrite(data,1,n,file);}
static LONG Close(BPTR file){fclose(file);error=fault==3 ? ERROR_DISK_WRITE_PROTECTED:0;return fault!=3;}
static LONG Rename(STRPTR from,STRPTR to){FILE *f;if(fault==4){f=fopen(host(to),"wb");assert(f);fputs("other writer",f);fclose(f);error=ERROR_OBJECT_EXISTS;return 0;}if(fault==5){error=ERROR_WRITE_PROTECTED;return 0;}return rename(host(from),host(to))==0;}
static LONG DeleteFile(STRPTR path){error=0;return !remove(host(path));}
'''+source+r'''
static int exists(const char *p){struct stat st;return !stat(p,&st);}
static void clean(const char *target){DIR *d=opendir(".");struct dirent *e;FILE *f;char text[40];while((e=readdir(d))!=NULL)assert(strncmp(e->d_name,"ndt",3));closedir(d);if(target){f=fopen(target,"rb");assert(f);memset(text,0,sizeof(text));fread(text,1,39,f);fclose(f);assert(!strcmp(text,"other writer"));}}
int main(void){char p[80];int i;strcpy(p,"VOL:123456789012345678901234567890");assert(nd_text_export(p,"complete",8)==1);assert(strlen(FilePart(last_temp))<=30);assert(nd_text_export(p,"replace",7)==ND_TEXT_EXISTS);remove(host(p));
 for(i=1;i<=5;++i){fault=i;assert(nd_text_export("VOL:failed","important report",16)==(i==2 ? ND_TEXT_FULL:i==4 ? ND_TEXT_EXISTS:ND_TEXT_PROTECTED));assert(!exists(host(last_temp)));if(i==4){clean("failed");remove("failed");}else{assert(!exists("failed"));clean(NULL);}}
 fault=0;collisions=2;assert(nd_text_export("VOL:retry","ok",2)==1&&!collisions);remove("retry");collisions=8;assert(nd_text_export("VOL:retry","ok",2)==ND_TEXT_IO);assert(!exists("retry"));
 assert(nd_text_export("relative","x",1)==ND_TEXT_PATH&&nd_text_export("VOL:","x",1)==ND_TEXT_PATH&&nd_text_export("VOL:empty",NULL,1)==ND_TEXT_PATH);
 assert(strstr(nd_text_export_problem(ND_TEXT_PROTECTED),"write-protected")&&strstr(nd_text_export_problem(ND_TEXT_FULL),"full"));clean(NULL);
 puts("Export: FFS-safe staging names, no overwrite, collision retry, disk-full/close/rename rollback, cleanup and actionable errors passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code);cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'plugin_src/common'),str(p/'test.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],cwd=p,check=True)
