#!/usr/bin/env python3
"""Exercise actual December startup against disposable session/permanent profiles."""
from pathlib import Path
import subprocess, tempfile, os
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/dock_plugins.c').read_text();a=s.index('/* Seasonal history');b=s.index('static int launch_plugin',a)
source=r"""#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_options.h"
typedef long LONG;typedef long BPTR;
struct DateStamp {LONG ds_Days;};struct NDDock {struct NDConfig conf;};
static LONG days;static int available=1;
static void DateStamp(struct DateStamp *s){s->ds_Days=days;}
static BPTR CreateDir(const char *name){(void)name;return 1;}
static void UnLock(BPTR l){assert(l);}
static void *alloc_data(size_t n){return calloc(1,n);}
static void FreeVec(void *p){free(p);}
static int resolve_plugin_executable(struct NDDock *d,const char *name,char *out,unsigned int n){(void)d;(void)name;(void)n;strcpy(out,"Plugins/Christmas");return available;}
"""+s[a:b]+r"""
static struct NDDock dock;static struct NDConfig original,saved;
int main(void){int i;
 nd_default_config(&dock.conf);original=dock.conf;assert(nd_write_config(&original,"ENVARC:NovaDock/novadock.cfg"));
 dock.conf.theme=ND_THEME_LIGHT;strcpy(dock.conf.theme_id,"Light");

 days=calendar_days_before(2026,10);christmas_startup(&dock);assert(nd_christmas_index(&dock.conf)<0&&!nd_christmas_seen("ENVARC:NovaDock/Christmas.history"));
 /* Prior manual use does not exclude any user from the annual introduction. */
 assert(nd_christmas_remember("ENVARC:NovaDock/Christmas.history"));
 days=calendar_days_before(2026,11)+8;christmas_startup(&dock);assert(nd_christmas_index(&dock.conf)<0);
 days=calendar_days_before(2026,11)+9;available=0;christmas_startup(&dock);assert(nd_christmas_index(&dock.conf)<0);
 available=1;christmas_startup(&dock);i=nd_christmas_index(&dock.conf);assert(i>=0&&dock.conf.items[i].enabled&&nd_christmas_seen("ENVARC:NovaDock/Christmas-2026.history"));
 assert(nd_load_config(&saved,"ENVARC:NovaDock/novadock.cfg")==0);assert(saved.theme==original.theme&&!strcmp(saved.theme_id,original.theme_id));assert(saved.items[nd_christmas_index(&saved)].enabled);
 assert(nd_load_config(&saved,"ENV:NovaDock/novadock.cfg")==0);assert(saved.theme==ND_THEME_LIGHT&&saved.items[nd_christmas_index(&saved)].enabled);
 assert(nd_set_item_enabled(&dock.conf,i,0));days=calendar_days_before(2026,11)+24;christmas_startup(&dock);assert(!dock.conf.items[i].enabled);
 days=calendar_days_before(2027,11)+25;christmas_startup(&dock);assert(!dock.conf.items[i].enabled); /* after Christmas */
 days=calendar_days_before(2027,11)+9;christmas_startup(&dock);assert(dock.conf.items[i].enabled&&nd_christmas_seen("ENVARC:NovaDock/Christmas-2027.history"));
 assert(nd_set_item_enabled(&dock.conf,i,0));christmas_startup(&dock);assert(!dock.conf.items[i].enabled);
 puts("Seasonal startup: installed-file gating, persistent addition, unrelated session changes and previous use, December dates and once-per-season removal passed.");return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'ENVARC:NovaDock').mkdir();(p/'ENV:NovaDock').mkdir();(p/'check.c').write_text(source)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'check.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],cwd=p,check=True)
