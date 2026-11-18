#!/usr/bin/env python3
"""Exercise the real dock's single-helper hand-off and failure cleanup."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class ArchiveBatch(unittest.TestCase):
 def test_drop_copies_every_path_and_launches_once(self):
  s=(ROOT/'src/dock_plugins.c').read_text();a=s.index('int launch_archive_drop(');f=s[a:s.index('\nvoid start_plugins(',a)]
  h=r'''#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_archive.h"
#define ND_COMMAND_LEN 520
#define MEMF_PUBLIC 0
#define MEMF_CLEAR 0
#define MODE_NEWFILE 1
typedef unsigned long ULONG;typedef long BPTR;
struct WBArg {const char *path;};
struct AppMessage {unsigned int am_NumArgs;struct WBArg *am_ArgList;};
struct NDDock {struct {int count;struct {char id[20],command[64];}items[1];}conf;};
static int nd_shelf_plugin_path(const char *p){return !strcmp(p,"Plugins/Shelf");}
static int launches,deletes,create_fail,write_fail,launch_ok=1,allocations;
static char written[4096],verb[24];
static void *AllocVec(size_t n,int f){(void)f;++allocations;return calloc(1,n);}
static void FreeVec(void *p){--allocations;free(p);}
static int build_wb_path(const struct WBArg *a,char *p,unsigned int n,int *kind)
{*kind=0;if(!a->path||strlen(a->path)>=n)return 0;strcpy(p,a->path);return 1;}
static int launch_archive_helper(struct NDDock *d,int i,const char *p)
{(void)d;(void)i;(void)p;++launches;strcpy(verb,"HANDLE");return launch_ok;}
static int launch_plugin_helper(struct NDDock *d,int i,const char *v,const char *p)
{(void)d;(void)i;assert(!strcmp(v,"PUT") || strstr(p,"T:NovaDockArchiveDrop-")==p || strstr(p,"T:NovaDockShelfDrop-")==p);++launches;strcpy(verb,v);return launch_ok;}
static BPTR CreateDir(const char *p){(void)p;return !create_fail;}
static void UnLock(BPTR p){(void)p;}
static void *FindTask(void *p){(void)p;return (void *)0x1234;}
static BPTR Open(const char *p,int m){(void)p;(void)m;written[0]=0;return 1;}
static long FPuts(BPTR f,const char *p){(void)f;if(write_fail)return -1;strcat(written,p);return 0;}
static int Close(BPTR f){(void)f;return 1;}
static int DeleteFile(const char *p){(void)p;++deletes;return 1;}
'''+f+r'''
static void reset(void){launches=deletes=create_fail=write_fail=allocations=0;launch_ok=1;written[0]=verb[0]=0;}
int main(void){struct NDDock dock;struct AppMessage m;struct WBArg args[3];
 memset(&dock,0,sizeof(dock));dock.conf.count=1;strcpy(dock.conf.items[0].id,"ARCHIVE");
 args[0].path="Work:First file";args[1].path="Other:Drawer";args[2].path="Work:Third.txt";m.am_NumArgs=3;m.am_ArgList=args;
 reset();assert(launch_archive_drop(&dock,0,&m));assert(launches==1&&!strcmp(verb,"HANDLELIST"));
 assert(!strcmp(written,"Work:First file\nOther:Drawer\nWork:Third.txt\n")&&!deletes&&!allocations);
 reset();launch_ok=0;assert(!launch_archive_drop(&dock,0,&m));assert(launches==1&&deletes==2&&!allocations);
 reset();write_fail=1;assert(!launch_archive_drop(&dock,0,&m));assert(!launches&&deletes==2&&!allocations);
 reset();args[1].path="Work:bad\npath";assert(!launch_archive_drop(&dock,0,&m));assert(!launches&&!deletes&&!allocations);args[1].path="Other:Drawer";
 reset();create_fail=1;assert(!launch_archive_drop(&dock,0,&m));assert(!launches&&!deletes&&!allocations);
 reset();m.am_NumArgs=1;assert(launch_archive_drop(&dock,0,&m));assert(launches==1&&!strcmp(verb,"HANDLE")&&!allocations);
 reset();strcpy(dock.conf.items[0].id,"SHELF_2");strcpy(dock.conf.items[0].command,"Plugins/Shelf");m.am_NumArgs=3;assert(launch_archive_drop(&dock,0,&m));assert(launches==1&&!strcmp(verb,"HANDLELIST"));
 reset();m.am_NumArgs=1;assert(launch_archive_drop(&dock,0,&m));assert(launches==1&&!strcmp(verb,"PUT"));
 m.am_NumArgs=33;assert(!launch_archive_drop(&dock,0,&m));
 m.am_NumArgs=ND_ARCHIVE_MAX_SOURCES+1;assert(!launch_archive_drop(&dock,0,&m));return 0;}
'''
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'batch.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'plugin_src/archive'),str(p/'batch.c'),str(ROOT/'plugin_src/archive/nd_archive.c'),'-o',str(p/'batch')],check=True)
   subprocess.run([str(p/'batch')],check=True)
if __name__=='__main__':unittest.main()
