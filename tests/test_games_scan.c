#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_games_scan.h"
struct Entry {const char *name;int kind;};
struct Directory {const char *path;const struct Entry *entries;};
struct Cursor {const struct Entry *next;};
struct Context {int opened,closed,found,calls,stop;char last[300];};
static const struct Entry root[]={{"A",2},{"Linked",3},{"Root.SLAVE",1},{"Broken",2},{NULL,0}};
static const struct Entry a[]={{"Nested",2},{NULL,0}},nested[]={{"Deep",2},{NULL,0}};
static const struct Entry deep[]={{"Demo.slave",1},{"Demo.info",1},{NULL,0}};
static const struct Directory tree[]={{"Games:",root},{"Games:A",a},{"Games:A/Nested",nested},{"Games:A/Nested/Deep",deep},{NULL,NULL}};
static void *open_dir(void *context,const char *path){struct Context *c=context;struct Cursor *p;int i;for(i=0;tree[i].path;++i)if(!strcmp(path,tree[i].path)){p=malloc(sizeof(*p));assert(p);p->next=tree[i].entries;++c->opened;return p;}return NULL;}
static int next_entry(void *context,void *handle,char *name,unsigned int size){struct Context *c=context;struct Cursor *p=handle;int kind;++c->calls;if(!p->next->name)return 0;assert(strlen(p->next->name)<size);strcpy(name,p->next->name);kind=p->next->kind;++p->next;return kind;}
static void close_dir(void *context,void *handle){++((struct Context *)context)->closed;free(handle);}
static int found(void *context,const char *path,const char *name,const char *slave){struct Context *c=context;assert(name[0] && slave[0]);++c->found;strcpy(c->last,path);return !c->stop;}
static const struct NDGamesScanOps ops={open_dir,next_entry,close_dir,found};
static void *deep_open(void *context,const char *path){struct Cursor *p=malloc(sizeof(*p));static const struct Entry branch[]={{"x",2},{NULL,0}};static const struct Entry leaf[]={{"End.slave",1},{NULL,0}};assert(p);p->next=strlen(path)<120 ? branch:leaf;++((struct Context *)context)->opened;return p;}
int main(void){struct NDGamesScan s;struct Context c;int steps,previous;struct NDGamesScanOps deep_ops=ops;
 memset(&s,0,sizeof(s));memset(&c,0,sizeof(c));assert(nd_games_scan_begin(&s,&ops,&c,"Games:"));
 for(steps=0;s.head && steps<100;++steps){previous=c.calls;nd_games_scan_step(&s,1);assert(c.calls-previous<=1);}
 assert(!s.head && c.found==2 && c.opened==4 && c.closed==4 && s.errors==1);assert(!strcmp(c.last,"Games:"));
 memset(&c,0,sizeof(c));assert(nd_games_scan_begin(&s,&ops,&c,"Games:"));nd_games_scan_step(&s,1);assert(c.opened==2);nd_games_scan_cancel(&s);assert(c.opened==c.closed);
 memset(&c,0,sizeof(c));c.stop=1;assert(nd_games_scan_begin(&s,&ops,&c,"Games:"));while(s.head)nd_games_scan_step(&s,2);assert(c.found==1 && c.closed==c.opened);
 memset(&c,0,sizeof(c));deep_ops.open=deep_open;assert(nd_games_scan_begin(&s,&deep_ops,&c,"Root:"));while(s.head)nd_games_scan_step(&s,3);assert(c.opened>50 && c.closed==c.opened && c.found==1);
 memset(&c,0,sizeof(c));assert(!nd_games_scan_begin(&s,&ops,&c,"Missing:"));assert(s.errors==1 && !s.head);assert(!nd_games_scan_begin(&s,&ops,&c,""));
 puts("WHDLoad recursive, bounded, cancel and resource-release tests passed.");return 0;}
