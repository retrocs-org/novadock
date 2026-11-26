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
struct AddItemPage {struct NDConfig *cfg;struct NDPluginSummary plugin_catalog[ND_PLUGIN_CATALOG_MAX];int catalog_count,catalog_scanned,plugin_count,plugin_selected;struct List plugin_rows;struct Node plugin_nodes[ND_PLUGIN_CATALOG_MAX];char plugin_names[ND_PLUGIN_CATALOG_MAX+1][96],preselect[20];int plugin_row_cfg[ND_PLUGIN_CATALOG_MAX],plugin_row_cat[ND_PLUGIN_CATALOG_MAX];};
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
assert(row(&page,"CPU")>=0&&row(&page,"RAM")>=0&&row(&page,"NET")>=0);assert(!strcmp(page.plugin_names[row(&page,"CPU")],"CPU ACTIVITY"));assert(row(&page,"CLOCK")==-1&&row(&page,"NOTES")==-1);
assert(!strcmp(page.cfg->items[page.plugin_row_cfg[row(&page,"ARCHIVE")]].label,"ARCHIVE"));
assert(!strcmp(page.cfg->items[page.plugin_row_cfg[row(&page,"SCREENMODE")]].label,"SMODE"));
assert(strstr(page.plugin_names[row(&page,"CPU")],"CPU ACTIVITY")&&!strchr(page.plugin_names[row(&page,"CPU")],'['));
cpu=page.plugin_row_cfg[row(&page,"CPU")];assert(nd_set_item_enabled(&cfg,cpu,1));build_plugin_rows(&page);assert(row(&page,"CPU")==-1&&row(&page,"RAM")>=0);
page.catalog_count=2;strcpy(page.plugin_catalog[0].id,"RAM");strcpy(page.plugin_catalog[0].problem,"invalid manifest");strcpy(page.plugin_catalog[1].id,"BAD_NEW");strcpy(page.plugin_catalog[1].name,"Broken");strcpy(page.plugin_catalog[1].problem,"missing program");build_plugin_rows(&page);assert(row(&page,"RAM")==-1);{int i;for(i=0;i<page.plugin_count;++i)assert(page.plugin_row_cat[i]!=1);}
{struct NDManifest m;struct NDOptionDef defs[32];int n,j;
assert(nd_manifest_builtin("HASS",&m));n=nd_option_collect(&cfg,"HASS",defs,32);
{int device=0,name=0,entity=0;assert(n>0);for(j=0;j<n;++j){
 if(!strcmp(defs[j].key,"DEVICE"))device=1;
 if(!strcmp(defs[j].key,"DEVICE_NAME"))name=1;
 if(!strcmp(defs[j].key,"ENTITY"))entity=1;
 m.options[m.option_count++]=defs[j];}assert(device&&name&&entity);}
/* HASS's built-in options must not consume third-party schema capacity. */
for(j=cfg.def_count;j<ND_MAX_OPTION_DEFS;++j){strcpy(cfg.defs[j].id,"EXTERNAL");sprintf(cfg.defs[j].key,"K%d",j);}cfg.def_count=ND_MAX_OPTION_DEFS;
assert(nd_manifest_merge(&cfg,&m,"Plugins/HASS")==ND_MANIFEST_ADDED);
assert(cfg.def_count==ND_MAX_OPTION_DEFS);}

assert(nd_manifest_bundled_id("HASS"));{struct NDManifest manifest;int hass,new_index;
assert(nd_manifest_builtin("HASS",&manifest) && manifest.multiple_instances);
build_plugin_rows(&page);hass=row(&page,"HASS");assert(hass>=0);
new_index=page.plugin_row_cfg[hass];assert(nd_set_item_enabled(&cfg,new_index,1));
build_plugin_rows(&page);assert(row(&page,"HASS")>=0);
assert(nd_add_plugin_instance(&cfg,new_index));assert(!strcmp(cfg.items[cfg.count-1].id,"HASS_2"));
build_plugin_rows(&page);assert(row(&page,"HASS")>=0 && row(&page,"HASS_2")==-1);
}
puts("Add plugin: missing CPU/RAM/Network offered, active singletons filtered, disabled meters restore correctly.");return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'check.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'check.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),str(ROOT/'src/nd_manifest.c'),'-o',str(p/'check')],check=True)
   subprocess.run([str(p/'check')],cwd=p,check=True)
if __name__=='__main__':unittest.main()
