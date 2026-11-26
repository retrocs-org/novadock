#!/usr/bin/env python3
"""Preview backups preserve exact settings; startup avoids unrelated discovery."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]

def function(source, signature):
    start=source.index(signature); brace=source.index('{', start); end=brace+1; depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}'); end+=1
    return source[start:end]

source=(ROOT/'prefs/novadockprefs.c').read_text()
program=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_prefs_snapshot.h"
#include "nd_trace.h"
#include "nd_plugin_discovery.h"
#include "nd_options.h"
#include "nd_prefs_nav.h"
#define MEMF_PUBLIC 1
#define MEMF_CLEAR 2
#define Printf printf
typedef void *APTR;typedef char *STRPTR;typedef long LONG;
struct Process {APTR pr_WindowPtr;};
static struct Process task;
static void *FindTask(void *p){(void)p;return &task;}
static void *AllocVec(size_t n,int flags){(void)flags;return calloc(1,n);}
static void FreeVec(void *p){free(p);}
static int DeleteFile(STRPTR p){return !remove(p);}
struct NDConfig *nd_prefs_shared_config,*nd_prefs_shared_original;
int nd_prefs_shared_dirty,nd_prefs_preview_active;
struct NDDiscovery nd_prefs_discovery;
static char nd_revert_path[80];
#define ND_REVERT_PATH nd_revert_path
static int discoveries,lookups,restores,preview,fail_snapshot;
static struct NDConfig restored,original;
int nd_discover_plugins(struct NDConfig *c,struct NDDiscovery *d){(void)c;memset(d,0,sizeof(*d));++discoveries;return 0;}
int nd_catalog_worker(const char *parent){(void)parent;return 17;}
int nd_plugin_catalog_lookup(const char *id,struct NDPluginCatalogEntry *entry){memset(entry,0,sizeof(*entry));assert(!strcmp(id,"EXAMPLE"));strcpy(entry->manifest.id,id);++lookups;return 1;}
static int nd_control_sync_settings(void){return 1;}
static int fake_load(struct NDConfig *c){nd_default_config(c);original=*c;return 0;}
static int nd_control_write_config(const struct NDConfig *c,const char *path){assert(!strcmp(path,"ENV:NovaDock/novadock.cfg"));restored=*c;++restores;return 1;}
static int nd_control_reload(void){return 1;}
static void create_settings_dir(const char *p){(void)p;}
static int snapshot_save(const struct NDConfig *c,const char *p){return fail_snapshot ? 0:nd_prefs_snapshot_save(c,p);}
'''
program+=function(source,'static int command_page(')+'\n'
program+='static int session_discovered;\n'+function(source,'static void prepare_plugin_page(')+'\n'
program+='#define nd_prefs_snapshot_save snapshot_save\n'+function(source,'static void revert_files(')+'\n#undef nd_prefs_snapshot_save\n'
program+=r'''
static int run_child_pages(int page,int argc,char **argv,char *plugin){(void)argc;(void)argv;prepare_plugin_page(page,nd_prefs_shared_config,plugin);prepare_plugin_page(page,nd_prefs_shared_config,plugin);return 0;}
static int run_general_page(int argc,char **argv,struct NDConfig *c,int *page,char *plugin,int size,int updates){(void)argc;(void)argv;(void)plugin;(void)size;(void)updates;assert(!discoveries&&!lookups);*page=0;if(preview){c->tile_width+=13;nd_prefs_preview_active=1;}return 0;}
#define nd_load_effective fake_load
'''
program+=function(source,'int main(int argc, char **argv)').replace('int main(', 'static int prefs_main(',1)+'\n#undef nd_load_effective\n'
program+=r'''
int main(void){static struct NDConfig c,copy,before;FILE *f;long size;int value;
 char *general[]={"Prefs"},*net[]={"Prefs","PAGE=PLUGIN","PLUGIN=NET"},*script[]={"Prefs","PAGE=PLUGIN","PLUGIN=SCRIPT"},*custom[]={"Prefs","PAGE=PLUGIN","PLUGIN=EXAMPLE"},*organize[]={"Prefs","PAGE=ITEMS"};
 nd_default_config(&c);assert(nd_item_set_text(&c,&c.items[0],ND_TEXT_ICON,"Work:Icons/Custom.info"));assert(nd_option_set(&c,"NET","DEVICE","prism2.device"));assert(nd_parse_line(&c,"FUTURE_SETTING=preserve"));assert(nd_prefs_snapshot_save(&c,"snapshot"));memset(&copy,0,sizeof(copy));assert(nd_prefs_snapshot_load(&copy,"snapshot")&&!memcmp(&c,&copy,sizeof(c)));
 before=copy;f=fopen("snapshot","r+b");assert(f);assert(!fseek(f,64,SEEK_SET));value=fgetc(f);assert(value!=EOF);assert(!fseek(f,64,SEEK_SET));fputc(value^1,f);fclose(f);assert(!nd_prefs_snapshot_load(&copy,"snapshot")&&!memcmp(&before,&copy,sizeof(copy)));
 assert(nd_prefs_snapshot_save(&c,"snapshot"));f=fopen("snapshot","ab");assert(f);fputc(0,f);fclose(f);assert(!nd_prefs_snapshot_load(&copy,"snapshot"));
 f=fopen("snapshot","wb");assert(f);fputs("NDPS",f);fclose(f);assert(!nd_prefs_snapshot_load(&copy,"snapshot"));
 copy.count=ND_MAX_ITEMS+1;assert(!nd_prefs_snapshot_save(&copy,"invalid"));
 copy=c;copy.items[0].text[0]=ND_TEXT_POOL_SIZE;assert(!nd_prefs_snapshot_save(&copy,"invalid"));
 assert(!prefs_main(1,general)&&!discoveries&&!lookups);
 assert(!prefs_main(3,net)&&!discoveries&&!lookups);
 assert(!prefs_main(3,script)&&!discoveries&&!lookups);
 assert(!prefs_main(3,custom)&&!discoveries&&lookups==2);lookups=0;
 assert(!prefs_main(2,organize)&&discoveries==1);discoveries=0;
 preview=1;assert(!prefs_main(1,general)&&restores==1&&!memcmp(&original,&restored,sizeof(original)));
 fail_snapshot=1;assert(!prefs_main(1,general)&&restores==2&&!memcmp(&original,&restored,sizeof(original)));
 assert(!nd_prefs_shared_config&&!nd_prefs_shared_original&&!nd_prefs_preview_active);
 puts("Preferences: no startup/bundled scans, on-demand discovery, focused lookup, exact preview restoration, corruption rejection and memory fallback passed.");return 0;}
'''
with tempfile.TemporaryDirectory(prefix='novadock-prefs-startup-') as tmp:
    p=Path(tmp);(p/'test.c').write_text(program)
    cmd=[os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'test.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),str(ROOT/'src/nd_manifest.c'),'-o',str(p/'test')]
    if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
    subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],cwd=p,check=True)
# Navigation must prepare only the selected page, rather than the entire session.
assert 'prepare_plugin_page(page,nd_prefs_shared_config,plugin);' in function(source,'static int run_child_pages(')
assert 'nd_discover_plugins(' not in function(source,'int main(int argc, char **argv)')
