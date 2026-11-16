#!/usr/bin/env python3
"""Run the production executable resolver with process-local PROGDIR and spaces."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_plugins.c').read_text();a=s.index('static int resolve_plugin_executable(');b=s.index('\n}',a)+2;resolver=s[a:b]
harness=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef long BPTR;typedef long LONG;typedef char *STRPTR;typedef void *APTR;
#define ACCESS_READ 0
struct NDDock {char directory[256];};
static char requested[520];static int quiet,locks,missing;
static APTR quiet_requesters(void){assert(!quiet);quiet=1;return (APTR)1;}
static void restore_requesters(APTR p){assert(quiet&&p==(APTR)1);quiet=0;}
static BPTR Lock(STRPTR p,int mode){(void)mode;assert(quiet);strcpy(requested,p);if(missing)return 0;++locks;return 1;}
static int NameFromLock(BPTR lock,char *p,LONG size){const char *full="Work:My Apps/NovaDock/Plugins/Screenshot";assert(quiet&&lock);if(strlen(full)>=(unsigned long)size)return 0;strcpy(p,full);return 1;}
static void UnLock(BPTR lock){assert(quiet&&lock&&locks);--locks;}
'''+resolver+r'''
int main(void){struct NDDock dock;char path[520];strcpy(dock.directory,"Work:My Apps/NovaDock");
assert(resolve_plugin_executable(&dock,"PROGDIR:Plugins/Screenshot",path,sizeof(path)));assert(!strcmp(requested,"PROGDIR:Plugins/Screenshot"));assert(!strcmp(path,"Work:My Apps/NovaDock/Plugins/Screenshot"));assert(!quiet&&!locks);
assert(resolve_plugin_executable(&dock,"Plugins/ScreenMode",path,sizeof(path)));assert(!strcmp(requested,"Work:My Apps/NovaDock/Plugins/ScreenMode"));
assert(resolve_plugin_executable(&dock,"NovaDock:Plugins/ScreenSwitcher",path,sizeof(path)));assert(!strcmp(requested,"NovaDock:Plugins/ScreenSwitcher"));
missing=1;assert(!resolve_plugin_executable(&dock,"Gone:Screenshot",path,sizeof(path)));assert(!quiet&&!locks);missing=0;
assert(!resolve_plugin_executable(&dock,"Plugins/Screenshot",path,4));assert(!quiet&&!locks);
puts("Process-local plugin paths resolve before Shell launch; missing volumes and buffers fail quietly.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(harness)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations',str(p/'check.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)

# Exercise the real discovery walk too: process-local aliases must never be
# stored as commands, and several aliases of one drawer are scanned once.
s=(ROOT/'prefs/novadockdiscovery.c').read_text()
a=s.index('static int catalog_walk_mode(');b=s.index('\nstruct ArraySink',a)
harness=r'''#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef long BPTR;typedef char *STRPTR;typedef void *APTR;
#define ND_PLUGIN_CATALOG_MAX 48
#define MEMF_PUBLIC 1
#define MEMF_CLEAR 2
#define ACCESS_READ 0
struct Process {APTR pr_WindowPtr;};static struct Process self;
static int scan_count,resolve_failure;static void *IconBase;
typedef int (*NDCatalogSink)(void *,void *);
struct CatalogWalk {int maximum,stop,count,diagnostics;NDCatalogSink sink;void *ctx;char seen[6][256];};
static void *FindTask(void *name){(void)name;return &self;}
static void *AllocVec(unsigned long size,int flags){(void)flags;return calloc(1,size);}
static void FreeVec(void *p){free(p);}
static void *OpenLibrary(const char *name,int version){(void)name;(void)version;return (void *)1;}
static void CloseLibrary(void *p){assert(p);}
static BPTR GetProgramDir(void){return 1;}
static BPTR Lock(STRPTR name,int mode){(void)mode;assert(self.pr_WindowPtr==(APTR)-1L);return !strcmp(name,"PROGDIR:Plugins") ? 1:2;}
static void UnLock(BPTR lock){assert(lock);}
static int NameFromLock(BPTR lock,char *out,int size){assert(lock&&size>=40);if(resolve_failure&&lock==1)return 0;strcpy(out,"Work:My Apps/NovaDock/Plugins");return 1;}
static void catalog_drawer(struct CatalogWalk *w,const char *dir){assert(!strcmp(dir,"Work:My Apps/NovaDock/Plugins"));++scan_count;++w->count;}
'''+s[a:b]+r'''
int main(void){assert(catalog_walk(48,NULL,NULL)==1);assert(scan_count==1&&self.pr_WindowPtr==NULL&&IconBase==NULL);
scan_count=0;resolve_failure=1;assert(catalog_walk(48,NULL,NULL)==1);assert(scan_count==1&&self.pr_WindowPtr==NULL);
puts("Discovery stores canonical drawer paths, skips unresolved paths and deduplicates aliases.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'discovery.c').write_text(harness)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations',str(p/'discovery.c'),'-o',str(p/'discovery')],check=True)
 subprocess.run([str(p/'discovery')],check=True)
