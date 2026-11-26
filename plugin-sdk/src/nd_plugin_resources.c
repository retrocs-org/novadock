/* Shared by one plugin process and its internal workers. No Exec patches,
 * private-file loggers or unbounded queues. Tracking excludes library/code/stack
 * and graphics allocations; metadata failure never fails the user's allocation. */
#define ND_PLUGIN_RESOURCES_IMPLEMENTATION
#undef AllocVec
#undef FreeVec
#undef AllocMem
#undef FreeMem
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef Open
#undef Read
#undef Write
#undef ExNext
#include "nd_plugin_resources.h"
#include <string.h>
#define ND_RESOURCE_BUCKETS 128
#define ND_RESOURCE_LIMIT 4096
struct NDResourceAllocation {struct NDResourceAllocation *next;ULONG address,size;};
static struct NDResourceAllocation *resource_buckets[ND_RESOURCE_BUCKETS];
static struct NDPluginSupportStats resource_stats;
static unsigned int resource_clients,resource_records;
static unsigned int resource_hash(void *p){return (unsigned int)(((ULONG)p>>3)&(ND_RESOURCE_BUCKETS-1));}
static void resource_add(void *p,ULONG size)
{
 struct NDResourceAllocation *n;unsigned int h;if(!p){Forbid();if(resource_clients)++resource_stats.failures;Permit();return;}
 if(!resource_clients)return;
 n=(struct NDResourceAllocation *)nd_resource_raw_allocvec(sizeof(*n),MEMF_PUBLIC);Forbid();
 if(!n || resource_records>=ND_RESOURCE_LIMIT || !resource_clients){if(resource_clients)resource_stats.flags|=ND_PLUGIN_SUPPORT_PARTIAL;Permit();if(n)nd_resource_raw_freevec(n);return;}
 h=resource_hash(p);n->address=(ULONG)p;n->size=size;n->next=resource_buckets[h];resource_buckets[h]=n;++resource_records;
 resource_stats.heap+=size;if(resource_stats.heap>resource_stats.peak)resource_stats.peak=resource_stats.heap;++resource_stats.allocations;Permit();
}
static void resource_forget_address(ULONG address)
{
 struct NDResourceAllocation **link,*n;unsigned int h;if(!address)return;h=(unsigned int)((address>>3)&(ND_RESOURCE_BUCKETS-1));Forbid();link=&resource_buckets[h];while(*link && (*link)->address!=address)link=&(*link)->next;n=*link;if(n){*link=n->next;resource_stats.heap-=n->size;++resource_stats.frees;--resource_records;}Permit();if(n)nd_resource_raw_freevec(n);
}
void nd_resources_forget(void *p){resource_forget_address((ULONG)p);}
void nd_resources_open(void){Forbid();if(!resource_clients++)memset(&resource_stats,0,sizeof(resource_stats));resource_stats.version=ND_PLUGIN_SUPPORT_VERSION;Permit();}
void nd_resources_close(void)
{
 struct NDResourceAllocation *list=NULL,*n,*next;unsigned int i;Forbid();if(resource_clients && !--resource_clients){for(i=0;i<ND_RESOURCE_BUCKETS;++i){for(n=resource_buckets[i];n;n=next){next=n->next;n->next=list;list=n;}resource_buckets[i]=NULL;}resource_records=0;}Permit();while(list){n=list;list=n->next;nd_resource_raw_freevec(n);}
}
void nd_resources_snapshot(struct NDPluginSupportStats *s){Forbid();*s=resource_stats;Permit();}
void nd_resources_event(unsigned int op,unsigned int phase,LONG a,LONG b){if(op>=ND_PLUGIN_OP_COUNT)return;Forbid();if(resource_clients){resource_stats.operation=(UWORD)op;resource_stats.phase=(UWORD)phase;resource_stats.detail_a=a;resource_stats.detail_b=b;++resource_stats.events;}Permit();}
void *nd_resources_allocvec(ULONG n,ULONG f){void *p=nd_resource_raw_allocvec(n,f);resource_add(p,n);return p;}
void nd_resources_freevec(void *p){nd_resources_forget(p);nd_resource_raw_freevec(p);}
void *nd_resources_allocmem(ULONG n,ULONG f){void *p=nd_resource_raw_allocmem(n,f);resource_add(p,n);return p;}
void nd_resources_freemem(void *p,ULONG n){nd_resources_forget(p);nd_resource_raw_freemem(p,n);}
void *nd_resources_malloc(size_t n){void *p=malloc(n);resource_add(p,(ULONG)n);return p;}
void *nd_resources_calloc(size_t n,size_t s){void *p;if(s && n>(size_t)-1/s){resource_add(NULL,0);return NULL;}p=calloc(n,s);resource_add(p,(ULONG)(n*s));return p;}
void *nd_resources_realloc(void *p,size_t n){void *out;ULONG address=(ULONG)p;if(!p)return nd_resources_malloc(n);if(!n){nd_resources_free(p);return NULL;}out=realloc(p,n);if(!out){resource_add(NULL,0);return NULL;}resource_forget_address(address);resource_add(out,(ULONG)n);return out;}
void nd_resources_free(void *p){nd_resources_forget(p);free(p);}
BPTR nd_resources_openfile(const char *p,LONG mode){BPTR f=nd_resource_raw_open((STRPTR)p,mode);Forbid();if(resource_clients)++resource_stats.opens;Permit();nd_resources_event(ND_PLUGIN_OP_OPEN,f?2:3,f?1:0,mode);return f;}
LONG nd_resources_read(BPTR f,void *p,LONG n){LONG rc=nd_resource_raw_read(f,p,n);Forbid();if(resource_clients){++resource_stats.reads;if(rc>0)resource_stats.read_bytes+=(ULONG)rc;}Permit();nd_resources_event(ND_PLUGIN_OP_READ,rc<0?3:2,rc,n);return rc;}
LONG nd_resources_write(BPTR f,const void *p,LONG n){LONG rc=nd_resource_raw_write(f,(APTR)p,n);Forbid();if(resource_clients){++resource_stats.writes;if(rc>0)resource_stats.write_bytes+=(ULONG)rc;}Permit();nd_resources_event(ND_PLUGIN_OP_WRITE,rc<0?3:2,rc,n);return rc;}
LONG nd_resources_exnext(BPTR lock,struct FileInfoBlock *f){LONG rc=nd_resource_raw_exnext(lock,f);Forbid();if(resource_clients)++resource_stats.directories;Permit();nd_resources_event(ND_PLUGIN_OP_DIRECTORY,2,rc?1:0,0);return rc;}
