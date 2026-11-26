#!/usr/bin/env python3
"""Exercise production component lookup against empty and stale install drawers."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/dock_launch.c').read_text()
a=s.index('static int program_directory_has_plugins(')
b=s.index('/* Preferences tools live',a)
production=s[a:b]
code=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef void *BPTR;typedef void *APTR;typedef char *STRPTR;typedef unsigned char UBYTE;
#define ACCESS_READ 0
#define MODE_OLDFILE 0
#define DOS_FIB 1
#define FIBF_SCRIPT 64
struct Process {APTR pr_WindowPtr;};
struct FileInfoBlock {int fib_DirEntryType,fib_Size,fib_Protection;char fib_FileName[128];};
struct NDDock {char directory[256];};
struct Node {const char *name;int type,size,script,readable;UBYTE head[4];};
static struct Node nodes[30];static int count,cursor,locks,files,allocs,fail_alloc,fail_name;
static const char *program="Bin",*assigned="Shared";static struct Process self;
static const char *resolve(const char *p,char *out){if(!strncmp(p,"NovaDock:",9)){strcpy(out,assigned);if(p[9]){strcat(out,"/");strcat(out,p+9);}return out;}return p;}
static BPTR Lock(STRPTR p,int mode){char tmp[600];int i;(void)mode;p=(STRPTR)resolve(p,tmp);for(i=0;i<count;++i)if(!strcmp(p,nodes[i].name)){++locks;return &nodes[i];}return NULL;}
static void UnLock(BPTR p){assert(p);--locks;}
static BPTR GetProgramDir(void){int i;for(i=0;i<count;++i)if(!strcmp(program,nodes[i].name))return &nodes[i];return NULL;}
static int NameFromLock(BPTR p,char *out,size_t n){const char *name=((struct Node *)p)->name;if(fail_name||strlen(name)>=n)return 0;strcpy(out,name);return 1;}
static int AddPart(char *p,const char *part,size_t n){size_t len=strlen(p);int slash=len&&p[len-1]!=':'&&p[len-1]!='/';if(len+slash+strlen(part)>=n)return 0;if(slash)strcat(p,"/");strcat(p,part);return 1;}
static void *AllocDosObject(int type,void *tags){(void)type;(void)tags;if(fail_alloc)return NULL;++allocs;return calloc(1,sizeof(struct FileInfoBlock));}
static void FreeDosObject(int type,void *p){(void)type;--allocs;free(p);}
static struct Process *FindTask(void *unused){(void)unused;return &self;}
static int Examine(BPTR lock,struct FileInfoBlock *f){cursor=0;f->fib_DirEntryType=((struct Node *)lock)->type;return 1;}
static int ExNext(BPTR lock,struct FileInfoBlock *f){const char *base=((struct Node *)lock)->name;size_t n=strlen(base);while(cursor<count){struct Node *v=&nodes[cursor++];if(!strncmp(v->name,base,n)&&v->name[n]=='/'&&!strchr(v->name+n+1,'/')){strcpy(f->fib_FileName,v->name+n+1);f->fib_DirEntryType=v->type;f->fib_Size=v->size;f->fib_Protection=v->script?FIBF_SCRIPT:0;return 1;}}return 0;}
static BPTR Open(const char *name,int mode){char tmp[600];int i;(void)mode;name=resolve(name,tmp);for(i=0;i<count;++i)if(!strcmp(name,nodes[i].name)&&nodes[i].readable){++files;return &nodes[i];}return NULL;}
static size_t Read(BPTR file,UBYTE *out,size_t n){struct Node *v=file;if(n>4)n=4;if(n>(size_t)v->size)n=v->size;memcpy(out,v->head,n);return n;}
static void Close(BPTR p){assert(p);--files;}
static void node(const char *name,int type,int kind){struct Node *v=&nodes[count++];memset(v,0,sizeof(*v));v->name=name;v->type=type;v->size=type<0?16:0;v->readable=1;if(kind==1){v->head[2]=3;v->head[3]=0xf3;}if(kind==2)v->script=1;}
static void reset(void){count=locks=files=allocs=fail_alloc=fail_name=0;program="Bin";assigned="Shared";self.pr_WindowPtr=(APTR)123;node("Bin",1,0);node("Shared",1,0);node("Shared/Plugins",1,0);node("Shared/Plugins/Custom",-1,1);}
''' + production + r'''
static void expect(const char *name){struct NDDock d;memset(&d,0xff,sizeof(d));get_program_dir(&d);assert(!strcmp(d.directory,name));assert(self.pr_WindowPtr==(APTR)123);assert(!locks&&!files&&!allocs);}
int main(void){char longpath[500];
 reset();expect("Shared");
 reset();node("Bin/Plugins",1,0);expect("Shared");
 reset();node("Bin/Plugins",1,0);node("Bin/Plugins/Plugin.ndplugin",-1,0);node("Bin/Plugins/Plugin.info",-1,0);node("Bin/Plugins/Plugin.assets",1,1);expect("Shared");
 reset();node("Bin/Plugins",1,0);node("Bin/Plugins/Custom",-1,1);expect("Bin");
 reset();node("Bin/Plugins",1,0);node("Bin/Plugins/Custom",-1,2);expect("Bin");
 reset();node("Bin/Plugins",-1,1);expect("Shared");
 reset();node("Bin/Plugins",1,0);node("Bin/Plugins/Unreadable",-1,1);nodes[count-1].readable=0;expect("Shared");
 reset();node("Bin/Plugins",1,0);node("Bin/Plugins/Short",-1,1);nodes[count-1].size=3;expect("Shared");
 reset();assigned="Absent";node("SYS:NovaDock",1,0);node("SYS:NovaDock/Plugins",1,0);node("SYS:NovaDock/Plugins/Custom",-1,1);expect("SYS:NovaDock");
 reset();assigned="Absent";expect("");
 reset();fail_alloc=1;expect("");
 reset();fail_name=1;expect("");
 reset();memset(longpath,'x',sizeof(longpath));longpath[sizeof(longpath)-1]=0;assert(!program_directory_has_plugins(longpath));assert(!program_directory_has_plugins(NULL));assert(!program_directory_has_plugins(""));assert(!locks&&!files&&!allocs);
 puts("Component directory: 13 production lookup cases passed, including empty/metadata-only siblings, portable precedence, custom Hunk/script plugins and cleanup.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
