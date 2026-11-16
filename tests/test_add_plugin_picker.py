#!/usr/bin/env python3
"""The real Add Item builder must offer disabled bundled meters from a minimal dock."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
def fn(s,name):
 a=s.rfind('static ',0,s.index(name));b=s.index('{',a);n=1;i=b+1
 while n:
  if s[i]=='{':n+=1
  elif s[i]=='}':n-=1
  i+=1
 return s[a:i]
class Picker(unittest.TestCase):
 def test_cpu_ram_available_and_active_singletons_filtered(self):
  s=(ROOT/'prefs/novadockadditemprefs.c').read_text()
  h=r"""#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
#include "nd_manifest.h"
#define ND_PLUGIN_CATALOG_MAX ND_MAX_ITEMS
struct Node {char *ln_Name;};struct List {int count;};
struct NDPluginSummary {char id[20],name[ND_LABEL_LEN],command[ND_COMMAND_LEN],problem[96];};
struct AddItemPage {struct NDConfig *cfg;struct NDPluginSummary plugin_catalog[48];int catalog_count,catalog_scanned,plugin_count,plugin_selected;struct List plugin_rows;struct Node plugin_nodes[48];char plugin_names[49][96],preselect[20];int plugin_row_cfg[48],plugin_row_cat[48];};
static void NewList(struct List *p){p->count=0;}
static void AddTail(struct List *p,struct Node *n){assert(n->ln_Name);++p->count;}
static int nd_plugin_catalog_inventory(struct NDPluginSummary *p,int max){(void)p;(void)max;return 0;}
"""
  for name in ('catalog_find(','config_has_plugin(','add_plugin_row(','build_plugin_rows('):h+='\n'+fn(s,name)
  h+=r"""
static int row(struct AddItemPage *p,const char *id){int i;for(i=0;i<p->plugin_count;++i)if(p->plugin_row_cfg[i]>=0&&!strcmp(p->cfg->items[p->plugin_row_cfg[i]].id,id))return i;return -1;}
int main(void){static struct NDConfig cfg;static struct AddItemPage page;FILE *f;int cpu;
f=fopen("minimal.cfg","w");assert(f);fputs("PLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON\nPLUGIN=Notes|Plugins/Notes|NOTES|METER|ON\nPLUGIN=Extract|Plugins/ArchiveExtractor|ARCHIVE|METER|OFF\nPLUGIN=Screen Mode|Plugins/ScreenMode|SCREENMODE|METER|OFF\n",f);fclose(f);
assert(nd_load_config(&cfg,"minimal.cfg")==0);page.cfg=&cfg;page.plugin_selected=-1;build_plugin_rows(&page);
assert(row(&page,"CPU")>=0&&row(&page,"RAM")>=0&&row(&page,"NET")>=0);assert(!strcmp(page.plugin_names[row(&page,"CPU")],"CPU Activity"));assert(row(&page,"CLOCK")==-1&&row(&page,"NOTES")==-1);
assert(!strcmp(page.cfg->items[page.plugin_row_cfg[row(&page,"ARCHIVE")]].label,"ARCHIVE"));
assert(!strcmp(page.cfg->items[page.plugin_row_cfg[row(&page,"SCREENMODE")]].label,"SMODE"));
assert(strstr(page.plugin_names[row(&page,"CPU")],"CPU Activity")&&!strchr(page.plugin_names[row(&page,"CPU")],'['));
cpu=page.plugin_row_cfg[row(&page,"CPU")];assert(nd_set_item_enabled(&cfg,cpu,1));build_plugin_rows(&page);assert(row(&page,"CPU")==-1&&row(&page,"RAM")>=0);
page.catalog_count=2;strcpy(page.plugin_catalog[0].id,"RAM");strcpy(page.plugin_catalog[0].problem,"invalid manifest");strcpy(page.plugin_catalog[1].id,"BAD_NEW");strcpy(page.plugin_catalog[1].name,"Broken");strcpy(page.plugin_catalog[1].problem,"missing program");build_plugin_rows(&page);assert(row(&page,"RAM")==-1);{int i;for(i=0;i<page.plugin_count;++i)assert(page.plugin_row_cat[i]!=1);}
puts("Add plugin: missing CPU/RAM/Network offered, active singletons filtered, disabled meters restore correctly.");return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'check.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'check.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),str(ROOT/'src/nd_manifest.c'),'-o',str(p/'check')],check=True)
   subprocess.run([str(p/'check')],cwd=p,check=True)
if __name__=='__main__':unittest.main()
