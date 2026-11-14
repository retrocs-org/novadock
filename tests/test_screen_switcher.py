#!/usr/bin/env python3
"""Exercise the production public-screen scan and activation with closing screens."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
harness=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef char *STRPTR;
struct Node {struct Node *ln_Succ;char *ln_Name;};
struct Screen {int Width,Height;};
struct PubScreenNode {struct Node psn_Node;struct Screen *psn_Screen;unsigned int psn_Flags;};
struct List {struct Node *lh_Head;};
#define PSNF_PRIVATE 1
static struct List list;static int list_locked,visitor_locked,available=1;
static struct Screen target={800,600},*front;
static struct List *LockPubScreenList(void){assert(!list_locked);list_locked=1;return &list;}
static void UnlockPubScreenList(void){assert(list_locked);list_locked=0;}
static struct Screen *LockPubScreen(STRPTR name){assert(!list_locked);if(available&&!strcmp(name,"Workbench")){visitor_locked=1;return &target;}return NULL;}
static void UnlockPubScreen(STRPTR name,struct Screen *s){(void)name;assert(visitor_locked&&s==&target);visitor_locked=0;}
static void ScreenToFront(struct Screen *s){assert(visitor_locked&&!list_locked);front=s;}
#include "nd_screens.h"
int main(void){struct NDScreenEntry entries[ND_SCREEN_MAX];struct PubScreenNode n[37];char names[36][160];int i,count;
memset(n,0,sizeof(n));for(i=0;i<36;++i){sprintf(names[i],"Screen%02d",35-i);n[i].psn_Node.ln_Name=names[i];n[i].psn_Node.ln_Succ=&n[i+1].psn_Node;n[i].psn_Screen=&target;}
list.lh_Head=&n[0].psn_Node;n[0].psn_Flags=PSNF_PRIVATE;n[1].psn_Node.ln_Name=NULL;n[2].psn_Screen=NULL;memset(names[3],'X',sizeof(names[3])-1);names[3][sizeof(names[3])-1]=0;
count=nd_screen_collect(entries);assert(count==32&&!list_locked);assert(!strcmp(entries[0].name,"Screen00"));assert(entries[0].width==800&&entries[0].height==600);
for(i=1;i<count;++i)assert(strcmp(entries[i-1].name,entries[i].name)<0);
n[0].psn_Flags=0;n[0].psn_Node.ln_Name="Workbench";n[0].psn_Node.ln_Succ=&n[36].psn_Node;
assert(nd_screen_collect(entries)==1);assert(nd_screen_raise(entries[0].name));assert(front==&target&&!visitor_locked);
available=0;front=NULL;assert(!nd_screen_raise(entries[0].name)&&!front&&!visitor_locked);
list.lh_Head=&n[36].psn_Node;assert(nd_screen_collect(entries)==0&&!list_locked);
puts("Public-screen bounds, private exclusion, sorting, locks and closed-screen activation passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(harness)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-pedantic','-I'+str(ROOT/'plugin_src/screenswitcher'),str(p/'check.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)
