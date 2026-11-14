#!/usr/bin/env python3
"""Exercise extraction ownership and optional icons in the real helper code."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class ArchiverWorkflow(unittest.TestCase):
 def test_new_and_existing_destinations(self):
  source=(ROOT/'plugin_src/archive/archive.c').read_text()
  start=source.index('static int extract_archive(')
  function=source[start:source.index('\nstatic int find_compressor',start)]
  harness=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_archive.h"
typedef long LONG;typedef long BPTR;typedef char *STRPTR;
#define ACCESS_READ 0
#define MODE_OLDFILE 0
#define EXTRACT_CONSOLE "test"
static char status_file[48],status[32];
static int make,icon,selection,exists,creates,deletes,icons,refreshes,runs,asks,rc,console_opens;
static void *FindTask(void *p){return p;}
static void write_status(const char *s){strcpy(status,s);}
static int nd_archive_extraction_request(const char *a,char *d,unsigned int n,int *m,int *i)
{(void)a;(void)n;strcpy(d,"RAM:chosen");*m=make;*i=icon;return selection;}
static BPTR Open(const char *s,int m){(void)s;(void)m;++console_opens;return 1;}
static void print_pair(BPTR c,const char *s,LONG a,LONG b){(void)c;(void)s;(void)a;(void)b;}
static int find_tool(int k,char *s,unsigned int n){(void)k;(void)n;strcpy(s,"C:UnZip");return 1;}
static void wait_and_close_console(BPTR c){(void)c;}
static void FPuts(BPTR c,STRPTR s){(void)c;(void)s;}
static void Flush(BPTR c){(void)c;}
static BPTR Lock(STRPTR s,int m){(void)s;(void)m;return exists;}
static void UnLock(BPTR l){(void)l;}
static BPTR CreateDir(STRPTR s){(void)s;++creates;return 1;}
static int DeleteFile(STRPTR s){(void)s;++deletes;return 0;}
static int ask_yes(BPTR c,const char *s){(void)c;(void)s;++asks;return 0;}
static LONG run_in_console(const char *s,BPTR c){(void)s;(void)c;++runs;return rc;}
static int ensure_workbench_icon(const char *s,int d){(void)s;assert(d==1);++icons;return 1;}
static void completed_at(const char *s,int d,BPTR c){(void)s;(void)c;assert(d==1);}
static int refresh_workbench_parent(const char *s){(void)s;++refreshes;return 1;}
'''+function+r'''
static void reset(void){make=icon=selection=1;exists=creates=deletes=icons=refreshes=runs=asks=rc=console_opens=0;}
int main(void){
 reset();assert(!extract_archive("RAM:a.zip",ND_ARCHIVE_ZIP));assert(creates==1&&icons==1&&runs==1&&!deletes);
 reset();icon=0;assert(!extract_archive("RAM:a.zip",ND_ARCHIVE_ZIP));assert(creates==1&&!icons&&refreshes==1);
 reset();make=0;exists=1;assert(!extract_archive("RAM:a.zip",ND_ARCHIVE_ZIP));assert(!creates&&!icons&&!deletes&&runs==1);
 reset();make=0;exists=1;rc=20;assert(extract_archive("RAM:a.zip",ND_ARCHIVE_ZIP)==20);assert(!creates&&!deletes&&!asks&&!icons);
 reset();rc=1;assert(extract_archive("RAM:a.zip",ND_ARCHIVE_ZIP)==5);assert(creates==1&&!deletes&&!asks&&icons==1);assert(!strcmp(status,"WARNINGS"));
 reset();rc=20;assert(extract_archive("RAM:a.zip",ND_ARCHIVE_ZIP)==20);assert(creates==1&&deletes==1&&asks==1&&!icons);
 reset();exists=1;assert(extract_archive("RAM:a.zip",ND_ARCHIVE_ZIP)==20);assert(!creates&&!deletes&&!runs);
 reset();selection=0;assert(!extract_archive("RAM:a.zip",ND_ARCHIVE_ZIP));assert(!console_opens&&!creates&&!deletes&&!runs);assert(!strcmp(status,"CANCELLED"));
 return 0;}
'''
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'workflow.c').write_text(harness)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'plugin_src/archive'),str(p/'workflow.c'),str(ROOT/'plugin_src/archive/nd_archive.c'),'-o',str(p/'workflow')],check=True)
   subprocess.run([str(p/'workflow')],check=True)
if __name__=='__main__':unittest.main()
