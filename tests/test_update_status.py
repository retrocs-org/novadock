#!/usr/bin/env python3
"""Exercise production update status persistence and manual/automatic authorization."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=re.sub(r'/\*.*?\*/','',(ROOT/'src/nd_update_native.c').read_text(),flags=re.S)
def block(name):
 start=source.index(name);start=source.rfind('\n',0,start)+1;at=source.index('{',start);depth=1;i=at+1;quote=None;escape=False
 while depth:
  c=source[i]
  if quote:
   if escape:escape=False
   elif c=='\\':escape=True
   elif c==quote:quote=None
  elif c in ('"',"'"):quote=c
  elif c=='{':depth+=1
  elif c=='}':depth-=1
  i+=1
 return source[start:i]
pre=r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "nd_update.h"
#include "nd_config.h"
#define MEMF_PUBLIC 0
#define MEMF_CLEAR 0
#define ACCESS_READ 0
#define MODE_NEWFILE 0
#define ND_PORT_NAME "Dock"
#define UPDATE_GUARD "Check"
typedef FILE *BPTR;typedef long LONG;
static struct NDConfig config;static int dock_running,check_running;
static void *AllocVec(size_t n,int flags){(void)flags;return calloc(1,n);}
static void FreeVec(void *p){free(p);}
int nd_load_effective(struct NDConfig *c){*c=config;return 0;}
static int guard_present(const char *p){return !strcmp(p,ND_PORT_NAME) ? dock_running:check_running;}
static unsigned long seconds(void){return 1000000UL;}
static BPTR Lock(const char *p,int mode){(void)p;(void)mode;return (BPTR)1;}
static BPTR CreateDir(const char *p){(void)p;return (BPTR)1;}
static void UnLock(BPTR p){(void)p;}
static BPTR Open(const char *p,int mode){(void)mode;return fopen(p,"wb");}
static LONG Write(BPTR f,const void *p,LONG n){return (LONG)fwrite(p,1,(size_t)n,f);}
static int Close(BPTR f){return fclose(f)==0;}
static int DeleteFile(const char *p){return unlink(p)==0;}
'''
functions='\n'.join(block(n) for n in ('enabled(', 'state_read(', 'state_write(', 'nd_update_status('))
main=r'''
int main(void){struct NDUpdateState state,loaded;struct NDUpdateStatus status;unsigned int hours=0;FILE *f;
 assert(enabled(3,&hours));assert(!enabled(1,NULL) && !enabled(2,NULL)); /* Check now does not opt in or require the dock */
 dock_running=1;config.check_updates_running=1;config.update_interval_hours=6;
 assert(!enabled(1,NULL) && enabled(2,&hours) && hours==6);
 memset(&state,0,sizeof(state));state.checked=999999;state.result=1;state.next_attempt=1007200;strcpy(state.latest,"0.4");strcpy(state.noticed,"0.5");state.last_notice=999000;
 state_write(&state);state_read(&loaded);assert(!memcmp(&state,&loaded,sizeof(state)));
 nd_update_status(&status);assert(status.result==1 && status.checked==999999 && !status.busy && !strcmp(status.version,"0.4"));
 check_running=1;state.result=5;state_write(&state);nd_update_status(&status);assert(status.busy && status.checking);
 state.result=2;strcpy(state.latest,"0.5");state_write(&state);nd_update_status(&status);assert(status.busy && !status.checking && status.result==2); /* available prompt is not still checking */
 check_running=0;state.result=3;state_write(&state);nd_update_status(&status);assert(!status.busy && status.result==3);
 f=fopen("ENVARC:NovaDock/UpdateState","w");assert(f);fputs("NDUPDATE1 1007200 999000 0.5\n",f);fclose(f);state_read(&loaded);assert(loaded.last_notice==999000 && !loaded.checked && !strcmp(loaded.noticed,"0.5"));
 f=fopen("ENVARC:NovaDock/UpdateState","w");assert(f);fputs("broken",f);fclose(f);state_read(&loaded);assert(!loaded.checked && !loaded.result);
 puts("Update status: manual check without opt-in, independent schedules, last-result persistence, checking/available state and legacy reads passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'ENVARC:NovaDock').mkdir();(p/'test.c').write_text(pre+functions+main)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'test.c'),str(ROOT/'src/nd_update.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],cwd=p,check=True)
