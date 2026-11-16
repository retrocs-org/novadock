#!/usr/bin/env python3
"""Exercise installed manifest parsing, rejected discovery and manager rows."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(source, name):
    start=source.rfind('static ',0,source.index(name))
    brace=source.index('{',start); depth=1; end=brace+1
    while depth:
        if source[end]=='{': depth+=1
        elif source[end]=='}': depth-=1
        end+=1
    return source[start:end]
s=(ROOT/'prefs/novadockdiscovery.c').read_text()
m=(ROOT/'prefs/novadockmanagerprefs.c').read_text()
h=r'''#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "nd_manifest.h"
#include "nd_options.h"
#include "nd_plugin_discovery.h"
#define FALSE 0
#define TRUE 1
#define D_LINES 7
#define D_TEXT 100
#define D_VERSION 0
#define D_AUTHOR 1
#define D_PROVIDES 2
#define D_DOCK 3
#define D_PROGRAM 4
#define D_ABOUT 5
#define D_ABOUT2 6
#define GTTX_Text 1
#define GA_Disabled 2
#define TAG_DONE 0
typedef unsigned long ULONG;
struct Node {char *ln_Name;};struct List {int count;};
static void NewList(struct List *l){l->count=0;}
static void AddTail(struct List *l,struct Node *n){assert(n->ln_Name);++l->count;}
static void GT_SetGadgetAttrs(void *g,void *w,void *r,...){(void)g;(void)w;(void)r;}
static int is_file(const char *p){struct stat st;return !stat(p,&st)&&S_ISREG(st.st_mode);}
static int read_icon_manifest(const char *p,struct NDManifest *m){(void)p;(void)m;return -1;}
'''+function(s,'read_text_manifest(')
a=s.index('typedef int (*NDCatalogSink)');b=s.index('static void catalog_drawer(',a)
h+='\n'+s[a:b]+'\nstruct SummarySink {struct NDPluginSummary *out;int count;};\n'+function(s,'summary_sink(')
h+=r'''
struct ManagerPage {
 struct NDConfig *cfg;struct NDPluginSummary installed[ND_PLUGIN_CATALOG_MAX];
 int installed_count,row_installed[ND_MAX_ITEMS],count,selected,can_add,can_remove,has_settings;
 char ids[ND_MAX_ITEMS][20],names[ND_MAX_ITEMS][ND_LABEL_LEN],text[ND_MAX_ITEMS][64];
 struct Node nodes[ND_MAX_ITEMS];struct List rows;struct NDPluginCatalogEntry entry;
 char detail_text[D_LINES][D_TEXT];void *win,*detail[D_LINES],*remove;
};
int nd_plugin_catalog_lookup(const char *id,struct NDPluginCatalogEntry *out){(void)id;(void)out;return 0;}
'''
for name in ('installed_summary(','dock_tiles(','plugin_version(','tile_status(','add_row(','build_rows(','wrap_two(','show_details('): h+='\n'+function(m,name)
h+=r'''
static struct NDPluginSummary inventory[48];static int count;
static int collect(void *ctx,const struct NDPluginCatalogEntry *e){
 struct SummarySink sink;(void)ctx;sink.out=inventory;sink.count=count;
 summary_sink(&sink,e);count=sink.count;return 1;
}
static void scan(int diagnostics){
 struct CatalogWalk *w=(struct CatalogWalk *)calloc(1,sizeof(*w));assert(w);
 count=0;w->maximum=48;w->sink=collect;w->diagnostics=diagnostics;
 catalog_consider(w,".","Good",1);
 catalog_consider(w,".","BadOption",1);
 catalog_consider(w,".","NoID",1);
 catalog_consider(w,".","Future",1);
 catalog_consider(w,".","Missing",1);
 catalog_consider(w,".","LongLine",1);
 catalog_consider(w,".","SameIdBroken",1);
 catalog_consider(w,".","PlainIcon",0);
 free(w);
}
int main(int argc,char **argv){
 static struct NDManifest parsed;static struct NDConfig cfg;static struct ManagerPage page;
 int i,j,checked=0;char line[128];
 for(i=1;i<argc;++i){nd_manifest_init(&parsed,argv[i]);
  if(!read_text_manifest(argv[i],&parsed)||!nd_manifest_finish(&parsed)){
   fprintf(stderr,"%s: %s\n",argv[i],parsed.error);return 1;
  }
  if(!strcmp(parsed.id,"SENSORS"))assert(parsed.option_count==18);
  if(!strcmp(parsed.id,"MUSICLIB"))assert(parsed.option_count==16);
  ++checked;
 }
 assert(checked>=17);
 nd_manifest_init(&parsed,"Limits");assert(nd_manifest_line(&parsed,"NOVADOCK_ID=LIMITS"));
 for(i=0;i<ND_MANIFEST_MAX_OPTIONS;++i){sprintf(line,"NOVADOCK_OPTION=K%d|Label|BOOL||NO",i);assert(nd_manifest_line(&parsed,line));}
 assert(nd_manifest_finish(&parsed));assert(!nd_manifest_line(&parsed,"NOVADOCK_OPTION=OVER|Label|BOOL||NO"));assert(parsed.error[0]);
 scan(0);assert(count==1&&!strcmp(inventory[0].id,"GOOD")&&!inventory[0].problem[0]);
 scan(1);assert(count==7);assert(strstr(inventory[1].problem,"default"));
 assert(strstr(inventory[2].problem,"ID"));assert(strstr(inventory[3].problem,"newer"));
 assert(strstr(inventory[4].problem,"missing"));assert(strstr(inventory[5].problem,"long"));
 assert(strstr(inventory[6].problem,"default"));
 nd_default_config(&cfg);page.cfg=&cfg;page.installed_count=count;
 memcpy(page.installed,inventory,sizeof(inventory));build_rows(&page);
 for(i=1;i<count;++i){
  for(j=0;j<page.count;++j)if(page.row_installed[j]==i)break;
  assert(j<page.count&&strstr(page.text[j],"Needs repair"));page.selected=j;show_details(&page);
  assert(!page.can_add&&!page.can_remove&&!page.has_settings);
  assert(!strcmp(page.detail_text[D_VERSION],"Needs repair"));
  assert(page.detail_text[D_ABOUT][0]&&page.detail_text[D_PROGRAM][0]);
 }
 /* No-ID faults stay distinct and never hide a healthy plugin with the same ID. */
 for(j=0;j<page.count;++j)if(page.row_installed[j]==0)break;
 assert(j<page.count&&!strstr(page.text[j],"Needs repair"));
 printf("All %d bundled manifests parsed; invalid, incompatible, missing and duplicate-ID files remain visible only in repair inventory.\n",checked);
 return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp)
    fixtures={
      'Good':'NOVADOCK_ID=GOOD\nNOVADOCK_NAME=Good plugin\n',
      'BadOption':'NOVADOCK_ID=BAD\nNOVADOCK_OPTION=A|A|UINT|1,5|9\n',
      'NoID':'NOVADOCK_NAME=No ID\n',
      'Future':'NOVADOCK_ID=FUTURE\nNOVADOCK_ABI=9\n',
      'Missing':'NOVADOCK_ID=MISSING\n',
      'LongLine':'NOVADOCK_ID=LONG\nNOVADOCK_NAME='+('a'*600)+'\n',
      'SameIdBroken':'NOVADOCK_ID=GOOD\nNOVADOCK_OPTION=A|A|UINT|1,5|9\n',
    }
    for name, contents in fixtures.items():
        (p/(name+'.ndplugin')).write_text(contents)
        if name!='Missing': (p/name).touch()
    (p/'check.c').write_text(h)
    subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',
        '-I'+str(ROOT/'include'),str(p/'check.c'),str(ROOT/'src/nd_manifest.c'),
        str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),'-o',str(p/'check')],check=True)
    subprocess.run([str(p/'check'),*[str(f) for f in sorted((ROOT/'Plugins').glob('*.ndplugin'))]],cwd=p,check=True)
