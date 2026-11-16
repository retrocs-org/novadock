#!/usr/bin/env python3
"""Exercise the actual CPU tooltip branch for every supported processor."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/dock_timers.c').read_text();a=s.index('    if(it->kind==ND_ITEM_PLUGIN && !strcmp(it->id,"CPU")){');b=s.index('    } else if (!strcmp(it->id,"RAM"))',a)
branch=s[a:b]+'    }\n'
h='''#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_cpu_info.h"
#define ND_ITEM_PLUGIN 1
struct Item {int kind,online;unsigned int meter;char id[20];};
struct Exec {unsigned long AttnFlags;} exec,*SysBase=&exec;
static void check(unsigned long flags,unsigned int meter,int online,const char *model,const char *usage){
 struct Item item,*it=&item;const char *name=NULL,*second_line=NULL;char volume_name[48],plugin_detail[80];int h=0;
 memset(it,0,sizeof(*it));it->kind=ND_ITEM_PLUGIN;strcpy(it->id,"CPU");it->online=online;it->meter=meter;SysBase->AttnFlags=flags;
'''+branch+''' assert(h==35&&!strcmp(name,model)&&!strcmp(second_line,usage));}
int main(void){
 check(0,0,1,"CPU 68000","0% CPU used");check(1,14,1,"CPU 68010","14% CPU used");check(3,14,1,"CPU 68020","14% CPU used");check(7,14,1,"CPU 68030","14% CPU used");check(15,100,1,"CPU 68040","100% CPU used");check(143,14,1,"CPU 68060","14% CPU used");check(143,65535,0,"CPU 68060","CPU usage unavailable");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(h);subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(root/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('CPU tooltip: 68000-68060 detection, live percentages, 0/100% bounds and unavailable samples passed.')
