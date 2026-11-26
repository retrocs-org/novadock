#ifndef ND_PLUGIN_RESOURCES_H
#define ND_PLUGIN_RESOURCES_H
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdlib.h>
#include "nd_plugin_support.h"
void nd_resources_open(void);
void nd_resources_close(void);
void nd_resources_forget(void *);
void nd_resources_snapshot(struct NDPluginSupportStats *);
void nd_resources_event(unsigned int,unsigned int,LONG,LONG);
void *nd_resources_allocvec(ULONG,ULONG);
void nd_resources_freevec(void *);
void *nd_resources_allocmem(ULONG,ULONG);
void nd_resources_freemem(void *,ULONG);
void *nd_resources_malloc(size_t);
void *nd_resources_calloc(size_t,size_t);
void *nd_resources_realloc(void *,size_t);
void nd_resources_free(void *);
BPTR nd_resources_openfile(const char *,LONG);
LONG nd_resources_read(BPTR,void *,LONG);
LONG nd_resources_write(BPTR,const void *,LONG);
LONG nd_resources_exnext(BPTR,struct FileInfoBlock *);
/* Keep native inline-library stubs before replacing their public macros. */
static void *nd_resource_raw_allocvec(ULONG n,ULONG f){return AllocVec(n,f);}
static void nd_resource_raw_freevec(void *p){FreeVec(p);}
static void *nd_resource_raw_allocmem(ULONG n,ULONG f){return AllocMem(n,f);}
static void nd_resource_raw_freemem(void *p,ULONG n){FreeMem(p,n);}
static BPTR nd_resource_raw_open(const char *p,LONG m){return Open((STRPTR)p,m);}
static LONG nd_resource_raw_read(BPTR f,void *p,LONG n){return Read(f,p,n);}
static LONG nd_resource_raw_write(BPTR f,const void *p,LONG n){return Write(f,(APTR)p,n);}
static LONG nd_resource_raw_exnext(BPTR l,struct FileInfoBlock *f){return ExNext(l,f);}
#if defined(ND_PLUGIN_RESOURCES) && !defined(ND_PLUGIN_RESOURCES_IMPLEMENTATION)
#undef AllocVec
#undef FreeVec
#undef AllocMem
#undef FreeMem
#undef Open
#undef Read
#undef Write
#undef ExNext
#define AllocVec(n,f) nd_resources_allocvec(n,f)
#define FreeVec(p) nd_resources_freevec(p)
#define AllocMem(n,f) nd_resources_allocmem(n,f)
#define FreeMem(p,n) nd_resources_freemem(p,n)
#define malloc(n) nd_resources_malloc(n)
#define calloc(n,s) nd_resources_calloc(n,s)
#define realloc(p,n) nd_resources_realloc(p,n)
#define free(p) nd_resources_free(p)
#define Open(p,m) nd_resources_openfile(p,m)
#define Read(f,p,n) nd_resources_read(f,p,n)
#define Write(f,p,n) nd_resources_write(f,p,n)
#define ExNext(l,f) nd_resources_exnext(l,f)
#endif
#endif
