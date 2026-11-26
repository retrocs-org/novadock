#!/usr/bin/env python3
"""Run the production collection create/save/browse/delete path on a real filesystem."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
def function(source,name):
 start=source.index(name);start=source.rfind('static ',0,start);brace=source.index('{',start);depth=1;end=brace+1
 while depth:
  if source[end]=='{':depth+=1
  elif source[end]=='}':depth-=1
  end+=1
 return source[start:end]
class Collection(unittest.TestCase):
 def test_create_browse_restart_legacy_and_recoverable_delete(self):
  source=(ROOT/'plugin_src/notes/notes.c').read_text()
  h=r"""#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "nd_notes.h"
#include "nd_markdown.h"
typedef void *BPTR;typedef void *APTR;typedef char *STRPTR;
#define ACCESS_READ 0
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_NO_MORE_ENTRIES 232
#define DOS_FIB 1
#define MEMF_PUBLIC 0
struct Process {APTR pr_WindowPtr;};static struct Process process;
struct FileInfoBlock {int fib_DirEntryType;char fib_FileName[256];};
struct FileLock {DIR *dir;struct stat info;};
struct NoteEntry {unsigned int id;char title[80],preview[64],filename[40];};
static struct NoteEntry entries[ND_NOTE_MAX];static struct NDNote note;
static char root[128],path[220];static unsigned int count;static int last_error;
static void *FindTask(void *unused){(void)unused;return &process;}
static int IoErr(void){return last_error;}
static BPTR Lock(STRPTR name,int mode){struct FileLock *l;(void)mode;l=calloc(1,sizeof(*l));assert(l);
 if(stat(name,&l->info)){last_error=errno==ENOENT ? 205:212;free(l);return 0;}
 if(S_ISDIR(l->info.st_mode))l->dir=opendir(name);return l;}
static void UnLock(BPTR lock){struct FileLock *l=lock;if(l->dir)closedir(l->dir);free(l);}
static int Examine(BPTR lock,struct FileInfoBlock *f){struct FileLock *l=lock;f->fib_DirEntryType=S_ISDIR(l->info.st_mode) ? 1:-1;return 1;}
static int ExNext(BPTR lock,struct FileInfoBlock *f){struct FileLock *l=lock;struct dirent *d;
 while((d=readdir(l->dir))!=NULL){if(!strcmp(d->d_name,".")||!strcmp(d->d_name,".."))continue;
  strcpy(f->fib_FileName,d->d_name);f->fib_DirEntryType=d->d_type==DT_DIR ? 1:-1;return 1;}
 last_error=232;return 0;}
static BPTR CreateDir(STRPTR name){if(mkdir(name,0700)){last_error=errno==EEXIST ? 203:205;return 0;}return Lock(name,0);}
static int DeleteFile(STRPTR name){return rmdir(name)==0;}
static void *AllocDosObject(int type,void *tags){(void)type;(void)tags;return calloc(1,sizeof(struct FileInfoBlock));}
static void FreeDosObject(int type,void *p){(void)type;free(p);}
static void *AllocVec(unsigned long n,int flags){(void)flags;return malloc(n);}
static void FreeVec(void *p){free(p);}
static int AddPart(char *out,const char *name,unsigned long size){if(strlen(out)+strlen(name)+2>=size)return 0;if(out[strlen(out)-1]!=':')strcat(out,"/");strcat(out,name);return 1;}
static char *FilePart(char *name){char *p=strrchr(name,'/');return p ? p+1:name;}
"""
  for name in ('drawer_exists(','ensure_root(','note_path(','entry_compare(','scan_notes(','save_note(','new_note(','delete_note('):h+='\n'+function(source,name)
  h+=r"""
int main(void){FILE *f;struct NDNote reopened;char saved_path[220];
 strcpy(root,"Work:Notes");assert(ensure_root());assert(scan_notes()&&count==0);assert(new_note());
 assert(strstr(path,"note-0001.md"));strcpy(note.title,"A Markdown note");strcpy(note.text,"# Heading\nA **bold** note.");
 assert(save_note()==1);strcpy(saved_path,path);assert(scan_notes()&&count==1);
 assert(!strcmp(entries[0].title,"A Markdown note"));assert(!strcmp(entries[0].filename,"note-0001.md"));assert(!strcmp(entries[0].preview,"Heading\nA bold note."));
 assert(nd_note_load(saved_path,&reopened)==1&&!strcmp(reopened.text,note.text));
 f=fopen("Work:Notes/note-0002.ndnote","wb");assert(f);fputs("NOVADOCK-NOTE 1\nTitle=Older note\nFont=topaz.font\nSize=8\nStyle=0\n\nOlder saved text",f);fclose(f);
 f=fopen("Work:Notes/note-0001.md.uaem","wb");assert(f);fputs("ignored metadata",f);fclose(f);
 assert(scan_notes()&&count==2);assert(new_note());assert(strstr(path,"note-0003.md"));
 strcpy(path,saved_path);assert(nd_note_load(path,&note)==1);assert(delete_note()==1);assert(access(saved_path,F_OK));
 assert(scan_notes()&&count==1&&!strcmp(entries[0].title,"Older note"));
 assert(new_note());assert(strstr(path,"note-0001.md")); /* deleted item is recoverable, ID may be reused */
 puts("Notes collection: create .md, save, browse, restart reads, legacy coexistence and recoverable delete pass.");return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'check.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c99','-D_DARWIN_C_SOURCE','-D_DEFAULT_SOURCE','-Wno-deprecated-declarations','-I'+str(ROOT/'plugin_src/notes'),str(p/'check.c'),str(ROOT/'plugin_src/notes/nd_notes.c'),str(ROOT/'plugin_src/notes/nd_markdown.c'),'-o',str(p/'check')],check=True)
   subprocess.run([str(p/'check')],cwd=p,check=True)
if __name__=='__main__':unittest.main()
