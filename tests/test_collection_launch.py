#!/usr/bin/env python3
"""Run the real collection launcher with native process calls stubbed."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/games/games.c').read_text()
a=s.index('static int find_tool(');b=s.index('/* ---- the games window',a)
h=r'''#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned long BPTR,ULONG;typedef long LONG;typedef char BYTE;typedef char *STRPTR;typedef void *APTR;
#define GAMES_PATH 300
#define COLLECTION_MUSIC music_mode
#define MEMF_PUBLIC 1
#define MEMF_CLEAR 2
#define WBPROJECT 4
#define ACCESS_READ 0
#define NT_MESSAGE 5
#define NP_Seglist 1
#define NP_FreeSeglist 2
#define NP_StackSize 3
#define NP_Name 4
#define NP_HomeDir 5
#define NP_CurrentDir 6
#define TAG_DONE 0
#define FALSE 0
struct MsgPort {int value;};
struct Node {int ln_Type;};struct Message {struct Node mn_Node;struct MsgPort *mn_ReplyPort;int mn_Length;};
struct WBArg {BPTR wa_Lock;BYTE *wa_Name;};
struct WBStartup {struct Message sm_Message;struct MsgPort *sm_Process;BPTR sm_Segment;int sm_NumArgs;void *sm_ToolWindow;struct WBArg *sm_ArgList;};
struct Process {struct MsgPort pr_MsgPort;};
struct DiskObject {int do_Type;char *do_DefaultTool;LONG do_StackSize;};
static struct DiskObject icon={WBPROJECT,"Missing:WrongDefault",8192};
static struct Process process;static struct MsgPort reply,*wb_reply=&reply;
static int running,music_mode,missing_tool,missing_icon,sent;
static char whdload_command[300]="WHDLoad",music_player[300],loaded[300];
static const char *expected_program="WHDLoad";
static char *FilePart(char *p){char *base=p;for(;*p;++p)if(*p==':'||*p=='/')base=p+1;return base;}
static char *PathPart(char *p){char *end=p;for(;*p;++p)if(*p==':'||*p=='/')end=p;return end;}
static int exists(const char *p){return !missing_tool&&(!strcmp(p,"C:/WHDLoad")||!strcmp(p,"Work:Tools/WHDLoad Demo")||!strcmp(p,"C:Music Player"));}
static int join(char *p,int n,const char *d,const char *f){if(strlen(d)+strlen(f)+2>(unsigned long)n)return 0;sprintf(p,"%s/%s",d,f);return 1;}
static struct DiskObject *GetDiskObject(STRPTR p){assert(!strcmp(p,"Work:Games/Demo/Demo"));return missing_icon ? NULL:&icon;}
static void FreeDiskObject(struct DiskObject *p){assert(p==&icon);}
static BPTR LoadSeg(STRPTR p){strcpy(loaded,p);return 1;}
static void UnLoadSeg(BPTR b){assert(b);}
static void *AllocVec(unsigned long n,int flags){(void)flags;return calloc(1,n);}
static void FreeVec(void *p){free(p);}
static char *copy_name(const char *p){char *s=malloc(strlen(p)+1);assert(s);strcpy(s,p);return s;}
static APTR quiet(void){return NULL;}static void unquiet(APTR p){(void)p;}
static BPTR Lock(STRPTR p,int mode){(void)p;(void)mode;return 1;}
static void UnLock(BPTR p){assert(p);}static BPTR ParentDir(BPTR p){return p;}static BPTR DupLock(BPTR p){return p;}
static struct Process *CreateNewProcTags(int first,...){assert(first==NP_Seglist);return &process;}
static void PutMsg(struct MsgPort *port,struct Message *message){
 struct WBStartup *startup=(struct WBStartup *)message;int i;
 assert(port==&process.pr_MsgPort&&startup->sm_NumArgs==2);
 assert(!strcmp(startup->sm_ArgList[0].wa_Name,expected_program));
 assert(!strcmp(startup->sm_ArgList[1].wa_Name,"Demo"));
 for(i=0;i<2;++i){assert(startup->sm_ArgList[i].wa_Lock);free(startup->sm_ArgList[i].wa_Name);}
 free(startup);++sent;
}
'''+s[a:b]+r'''
int main(void){
 assert(!start_game("Work:Games/Demo/Demo")&&sent==1&&!strcmp(loaded,"C:/WHDLoad"));
 strcpy(whdload_command,"Work:Tools/WHDLoad Demo");expected_program="WHDLoad Demo";
 icon.do_DefaultTool=NULL;assert(!start_game("Work:Games/Demo/Demo")&&sent==2);
 assert(!strcmp(loaded,"Work:Tools/WHDLoad Demo"));
 missing_tool=1;assert(strstr(start_game("Work:Games/Demo/Demo"),"configured WHDLoad"));assert(sent==2);missing_tool=0;
 missing_icon=1;assert(strstr(start_game("Work:Games/Demo/Demo"),"no icon"));assert(sent==2);
 /* MUSIC still launches an explicitly configured player without an icon. */
 music_mode=1;strcpy(music_player,"C:Music Player");expected_program="Music Player";
 assert(!start_game("Work:Games/Demo/Demo")&&sent==3&&!strcmp(loaded,"C:Music Player"));
 puts("Collection launcher uses each configured executable, preserves original Workbench icon arguments, and handles missing tools safely.");return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp);(p/'check.c').write_text(h)
    subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',str(p/'check.c'),'-o',str(p/'check')],check=True)
    subprocess.run([str(p/'check')],check=True)
