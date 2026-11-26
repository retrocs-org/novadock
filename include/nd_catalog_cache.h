#ifndef ND_CATALOG_CACHE_H
#define ND_CATALOG_CACHE_H
/* Disposable, checksummed native-layout cache. Source fingerprints remain the
 * authority; any missing/corrupt cache or uncertain stat falls back to parsing. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include "nd_plugin_discovery.h"
#include "nd_version.h"
#if defined(__VBCC__) || defined(AMIGA)
#include <proto/dos.h>
#include <proto/exec.h>
#else
#include <sys/stat.h>
#endif
struct NDCacheStamp {long kind,size,key,day,minute,tick,protection;};
struct NDCacheHeader {char magic[8];unsigned long bytes,packed,hash;struct NDCacheStamp stamp[4];};
#if defined(__VBCC__) || defined(AMIGA)
static void nd_cache_fib(const struct FileInfoBlock *fib,struct NDCacheStamp *s)
{memset(s,0,sizeof(*s));if(fib->fib_DirEntryType>0){s->kind=1;return;}s->kind=fib->fib_DirEntryType;s->protection=fib->fib_Protection;s->size=fib->fib_Size;s->day=fib->fib_Date.ds_Days;s->minute=fib->fib_Date.ds_Minute;s->tick=fib->fib_Date.ds_Tick;}
#endif
static int nd_cache_stamp(const char *path,struct NDCacheStamp *s)
{
 memset(s,0,sizeof(*s));
#if defined(__VBCC__) || defined(AMIGA)
 {BPTR lock=Lock((STRPTR)path,ACCESS_READ);struct FileInfoBlock *fib;LONG error;if(!lock){error=IoErr();return error==ERROR_OBJECT_NOT_FOUND||error==ERROR_DIR_NOT_FOUND;}fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return 0;}if(!Examine(lock,fib)){FreeDosObject(DOS_FIB,fib);UnLock(lock);return 0;}nd_cache_fib(fib,s);FreeDosObject(DOS_FIB,fib);UnLock(lock);}
#else
 {struct stat st;if(stat(path,&st)){return errno==ENOENT||errno==ENOTDIR;}s->kind=(long)st.st_mode;s->size=(long)st.st_size;s->key=(long)st.st_ino;s->day=(long)st.st_mtime;
#if defined(__APPLE__)
 s->tick=(long)st.st_mtimespec.tv_nsec;
#elif defined(__linux__) && defined(__USE_XOPEN2K8)
 s->tick=(long)st.st_mtim.tv_nsec;
#elif defined(__linux__)
 s->tick=(long)st.st_mtimensec;
#endif
 }
#endif
 return 1;
}
static unsigned long nd_cache_hash(const void *data,size_t n)
{const unsigned char *p=data;unsigned long h=5381UL;while(n--)h=(((h<<5)+h)^*p++)&0xffffffffUL;return h;}
static int nd_cache_sources(const char *base,struct NDCacheStamp stamp[4])
{char p[320];if(strlen(base)>280)return 0;if(!nd_cache_stamp(base,&stamp[0]))return 0;sprintf(p,"%s.ndplugin",base);if(!nd_cache_stamp(p,&stamp[1]))return 0;sprintf(p,"%s.info",base);if(!nd_cache_stamp(p,&stamp[2]))return 0;sprintf(p,"%s.assets/Artwork.ndart",base);return nd_cache_stamp(p,&stamp[3]);}
static void nd_cache_name(const char *base,char path[80])
{sprintf(path,"T:NDC4-%08lx",nd_cache_hash(base,strlen(base))^nd_cache_hash(ND_VERSION,strlen(ND_VERSION)));}
/* Zero runs remove unused schema slots and fixed-string padding. Hash the
 * compact bytes, then decode with exact input/output bounds before use. */
static size_t nd_cache_pack(const void *input,size_t bytes,unsigned char *out,size_t limit)
{static const unsigned char zeros[128]={0};const unsigned char *p=input;size_t i=0,n=0,start,k;while(i<bytes){if(!p[i]&&i+2<bytes&&!p[i+1]&&!p[i+2]){start=i;if(bytes-i>=128&&!memcmp(p+i,zeros,128))i+=128;else while(i<bytes&&!p[i]&&i-start<128)++i;if(n>=limit)return 0;out[n++]=(unsigned char)(128+i-start-1);}else{start=i++;while(i<bytes&&i-start<128){if(!p[i]&&i+2<bytes&&!p[i+1]&&!p[i+2])break;++i;}k=i-start;if(n+1+k>limit)return 0;out[n++]=(unsigned char)(k-1);memcpy(out+n,p+start,k);n+=k;}}return n;}
static int nd_cache_unpack(const unsigned char *input,size_t bytes,void *output,size_t limit)
{unsigned char *out=output;size_t i=0,n=0,k;unsigned int tag;while(i<bytes){tag=input[i++];k=(tag&127)+1;if(k>limit-n)return 0;if(tag&128)memset(out+n,0,k);else{if(k>bytes-i)return 0;memcpy(out+n,input+i,k);i+=k;}n+=k;}return n==limit;}
static int nd_catalog_cached(const char *base,struct NDPluginCatalogEntry *out,const struct NDCacheStamp stamp[4])
{struct NDCacheHeader h;struct NDPluginCatalogEntry *entry=NULL;unsigned char *packed=NULL;char path[80];FILE *f;int ok=0;size_t maximum=sizeof(*entry)+sizeof(*entry)/128+6;nd_cache_name(base,path);f=fopen(path,"rb");if(!f)return 0;
 if(fread(&h,1,sizeof(h),f)!=sizeof(h)||memcmp(h.magic,"NDCAT04",8)||h.bytes!=sizeof(*entry)||!h.packed||h.packed>maximum||memcmp(h.stamp,stamp,sizeof(h.stamp)))goto done;
 packed=malloc((size_t)h.packed);entry=calloc(1,sizeof(*entry));if(!packed||!entry)goto done;
 if(fread(packed,1,(size_t)h.packed,f)!=h.packed||fgetc(f)!=EOF||ferror(f)||h.hash!=nd_cache_hash(packed,(size_t)h.packed)||!nd_cache_unpack(packed,(size_t)h.packed,entry,sizeof(*entry)))goto done;
 if(!memchr(entry->command,0,sizeof(entry->command))||strcmp(entry->command,base))goto done;*out=*entry;ok=1;
 done:if(entry)free(entry);if(packed)free(packed);fclose(f);return ok;}
/* Unused schema slots have no meaning. Emit their zero runs directly instead
 * of examining every byte on a 68000; retain all active options and tail fields. */
static size_t nd_cache_pack_entry(const struct NDPluginCatalogEntry *entry,unsigned char *out,size_t limit)
{size_t prefix,tail,gap,n,k,wrote;int count=entry->manifest.option_count;if(count<0||count>ND_MANIFEST_MAX_OPTIONS)return 0;prefix=offsetof(struct NDManifest,options)+(size_t)count*sizeof(struct NDOptionDef);tail=offsetof(struct NDManifest,error);gap=tail-prefix;n=nd_cache_pack(entry,prefix,out,limit);if(!n)return 0;while(gap){k=gap>128 ? 128:gap;if(n==limit)return 0;out[n++]=(unsigned char)(128+k-1);gap-=k;}wrote=nd_cache_pack((const unsigned char *)entry+tail,sizeof(*entry)-tail,out+n,limit-n);return wrote ? n+wrote:0;}
/* Prepare compact records while parsing; callers may validate a second directory
 * snapshot before committing them, without retaining full manifest schemas. */
struct NDCacheRecord {struct NDCacheHeader header;unsigned char data[1];};
static struct NDCacheRecord *nd_catalog_prepare(const struct NDPluginCatalogEntry *entry,const struct NDCacheStamp stamp[4])
{struct NDCacheRecord *r;unsigned char *packed;size_t maximum=sizeof(*entry)+sizeof(*entry)/128+6,n;packed=malloc(maximum);if(!packed)return NULL;n=nd_cache_pack_entry(entry,packed,maximum);if(!n){free(packed);return NULL;}r=malloc(sizeof(*r)+n);if(r){memset(&r->header,0,sizeof(r->header));memcpy(r->header.magic,"NDCAT04",8);r->header.bytes=sizeof(*entry);r->header.packed=(unsigned long)n;r->header.hash=nd_cache_hash(packed,n);memcpy(r->header.stamp,stamp,sizeof(r->header.stamp));memcpy(r->data,packed,n);}free(packed);return r;}
static void nd_catalog_commit(const char *base,const struct NDCacheRecord *r)
{char path[80],temp[100];FILE *f;int ok;size_t n=(size_t)r->header.packed;nd_cache_name(base,path);
#if defined(__VBCC__) || defined(AMIGA)
 sprintf(temp,"%s.%08lx.new",path,(unsigned long)FindTask(NULL));
#else
 sprintf(temp,"%s.new",path);
#endif
 f=fopen(temp,"wb");if(f){ok=fwrite(&r->header,1,sizeof(r->header),f)==sizeof(r->header)&&fwrite(r->data,1,n,f)==n;if(fclose(f))ok=0;if(ok){remove(path);if(rename(temp,path))remove(temp);}else remove(temp);}}
static void nd_catalog_remember(const char *base,const struct NDPluginCatalogEntry *entry,const struct NDCacheStamp stamp[4])
{struct NDCacheRecord *r=nd_catalog_prepare(entry,stamp);if(r){nd_catalog_commit(base,r);free(r);}}

#endif
