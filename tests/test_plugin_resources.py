#!/usr/bin/env python3
"""Exercise production allocation accounting, failure fallbacks and ownership."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'plugin_src/common/nd_plugin_resources.c').read_text()
code=r"""
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
typedef unsigned long ULONG;typedef long LONG;typedef long BPTR;typedef unsigned short UWORD;typedef char *STRPTR;typedef void *APTR;
#define MEMF_PUBLIC 0
#define MEMF_CLEAR 1
struct FileInfoBlock {int a;};
#define Forbid() ((void)0)
#define Permit() ((void)0)
#include "nd_plugin_support.h"
static int raw_live,fail_metadata,fail_realloc;
static void *nd_resource_raw_allocvec(ULONG n,ULONG f){void*p;if(fail_metadata)return NULL;p=f&1?calloc(1,n):malloc(n);if(p)++raw_live;return p;}
static void nd_resource_raw_freevec(void*p){if(p){--raw_live;free(p);}}
static void *nd_resource_raw_allocmem(ULONG n,ULONG f){return nd_resource_raw_allocvec(n,f);}
static void nd_resource_raw_freemem(void*p,ULONG n){(void)n;nd_resource_raw_freevec(p);}
static BPTR nd_resource_raw_open(const char*p,LONG m){(void)p;return m;}
static LONG nd_resource_raw_read(BPTR f,void*p,LONG n){(void)f;(void)p;return n;}
static LONG nd_resource_raw_write(BPTR f,const void*p,LONG n){(void)f;(void)p;return n;}
static LONG nd_resource_raw_exnext(BPTR f,struct FileInfoBlock*p){(void)p;return f;}
static void *test_realloc(void *p,size_t n){return fail_realloc?NULL:realloc(p,n);}
#define realloc test_realloc
void nd_resources_free(void *);
"""+source[source.index('#define ND_RESOURCE_BUCKETS'):]+r"""
int main(void){struct NDPluginSupportStats s;char *a,*b;void *ptrs[4200];int i;
 a=nd_resources_malloc(10);assert(a&&!raw_live);nd_resources_free(a);
 nd_resources_open();a=nd_resources_malloc(4096);b=nd_resources_calloc(64,16);assert(a&&b);memset(a,7,4096);
 nd_resources_snapshot(&s);assert(s.heap==5120&&s.peak==5120&&s.allocations==2&&raw_live==2);
 fail_realloc=1;assert(!nd_resources_realloc(a,8192));fail_realloc=0;nd_resources_snapshot(&s);assert(s.heap==5120&&s.failures==1&&a[0]==7);
 a=nd_resources_realloc(a,8192);assert(a&&a[0]==7);nd_resources_snapshot(&s);assert(s.heap==9216&&s.peak==9216&&s.allocations==3&&s.frees==1);
 nd_resources_free(a);nd_resources_free(b);nd_resources_snapshot(&s);assert(!s.heap&&!raw_live);
 fail_metadata=1;a=nd_resources_malloc(99);assert(a);nd_resources_snapshot(&s);assert(!s.heap&&(s.flags&1));nd_resources_free(a);fail_metadata=0;
 assert(!nd_resources_calloc((size_t)-1,2));a=malloc(32);nd_resources_free(a); /* foreign allocation is safe */
 a=nd_resources_allocvec(128,MEMF_PUBLIC|MEMF_CLEAR);assert(a&&raw_live==2);nd_resources_forget(a);assert(raw_live==1);nd_resource_raw_freevec(a);assert(!raw_live);
 for(i=0;i<4200;++i){ptrs[i]=nd_resources_malloc(1);assert(ptrs[i]);}nd_resources_snapshot(&s);assert(s.heap==4096&&(s.flags&1)&&resource_records==4096&&raw_live==4096);
 for(i=0;i<4200;++i)nd_resources_free(ptrs[i]);assert(!raw_live&&!resource_records);
 nd_resources_open();a=nd_resources_malloc(128);nd_resources_close();nd_resources_snapshot(&s);assert(s.heap==128);nd_resources_close();assert(!raw_live);a[0]=9;free(a); /* metadata cleanup never frees caller-owned memory */
 nd_resources_open();nd_resources_event(ND_PLUGIN_OP_SCAN,2,129,0);nd_resources_snapshot(&s);assert(s.operation==8&&s.detail_a==129);nd_resources_close();assert(!raw_live);
 puts("Plugin resources: allocation families, realloc rollback, partial metadata, bounds, ownership transfer and balanced cleanup passed.");return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'exec').mkdir();(p/'exec/types.h').write_text('/* types declared by harness */\n');(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(p),'-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
