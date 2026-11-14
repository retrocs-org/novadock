#!/usr/bin/env python3
"""Exercise actual completed-job destination handling without a Shell process."""
from pathlib import Path
import os, subprocess, tempfile, unittest
ROOT=Path(__file__).resolve().parents[1]
class Destination(unittest.TestCase):
 def test_archive_parent_extracted_drawer_and_console_shortcut(self):
  source=(ROOT/'plugin_src/archive/archive.c').read_text()
  start=source.index('static char completed_destination[')
  code=source[start:source.index('\nstatic int ask_yes',start)]
  h=r"""#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_archive.h"
typedef long LONG;typedef long BPTR;typedef char *STRPTR;
#define ACCESS_READ 0
#define GVF_GLOBAL_ONLY 0
struct Library {int dummy;};static struct Library library,*WorkbenchBase;
static int libs=1,opens,closes,unlocks,reads,automatic=1,failed;
static char opened[420],printed[2048];
static struct Library *OpenLibrary(const char *name,int version){assert(!strcmp(name,"workbench.library")&&version==44);return libs ? &library:0;}
static int OpenWorkbenchObjectA(STRPTR path,void *tags){assert(!tags);strcpy(opened,path);++opens;return 1;}
static void CloseLibrary(struct Library *l){assert(l==&library);}
static BPTR Lock(STRPTR p,int mode){(void)p;(void)mode;return failed ? 0:1;}
static BPTR ParentDir(BPTR l){assert(l==1);return 2;}
static void UnLock(BPTR l){(void)l;++unlocks;}
static int NameFromLock(BPTR l,char *out,unsigned int n){(void)n;strcpy(out,l==1 ? "Work:unpacked":"Work:archives");return 1;}
static LONG GetVar(const char *name,char *out,unsigned long size,int flags){(void)flags;(void)size;assert(!strcmp(name,"NovaDock/ArchiveOpenDestination"));strcpy(out,automatic ? "YES":"NO");return automatic ? 3:2;}
static void FPuts(BPTR c,const char *s){(void)c;strcat(printed,s);}
static void Flush(BPTR c){(void)c;}
static int SetMode(BPTR c,int mode){(void)c;(void)mode;return 1;}
static LONG Read(BPTR c,unsigned char *b,unsigned long n){(void)c;(void)n;if(reads++)return 0;b[0]='o';b[1]='\r';return 2;}
static char *FGets(BPTR c,char *s,unsigned long n){(void)c;(void)s;(void)n;return 0;}
static void Close(BPTR c){(void)c;++closes;}
"""+code+r"""
static void reset(void){libs=1;opens=closes=unlocks=reads=0;automatic=1;failed=0;opened[0]=printed[0]=0;completed_destination[0]=0;}
int main(void){
 reset();completed_at("Work:archives/output.zip",0,1);assert(opens==1&&unlocks==2&&!strcmp(opened,"Work:archives"));
 reset();completed_at("Work:unpacked",1,1);assert(opens==1&&unlocks==1&&!strcmp(opened,"Work:unpacked"));
 reset();automatic=0;completed_at("Work:archives/output.lha",0,1);assert(!opens);
 wait_and_close_console(1);assert(opens==1&&closes==1&&strstr(printed,"Press O"));
 reset();libs=0;completed_at("Work:unpacked",1,1);assert(!opens&&strstr(printed,"could not be opened"));
 reset();failed=1;completed_at("Work:missing",1,1);assert(!opens&&!completed_destination[0]);
 wait_and_close_console(1);assert(!opens&&closes==1);
 return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'destination.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'plugin_src/archive'),str(p/'destination.c'),str(ROOT/'plugin_src/archive/nd_archive.c'),'-o',str(p/'check')],check=True)
   subprocess.run([str(p/'check')],check=True)
if __name__=='__main__':unittest.main()
