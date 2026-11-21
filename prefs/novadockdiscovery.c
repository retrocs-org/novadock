/* Discover self-describing plugins in NovaDock's Plugins drawer. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_artwork.h"
#include "nd_manifest.h"
#include "nd_manifest_file.h"
#include "nd_plugin_discovery.h"
#include "nd_catalog_cache.h"
static int catalog_force;
static int (*catalog_watch)(void *,int);
static void *catalog_watch_context;
void nd_plugin_catalog_watch(int (*watch)(void *,int),void *context){catalog_watch=watch;catalog_watch_context=context;}
static unsigned long cache_hits,cache_misses;
void nd_plugin_catalog_refresh(void){catalog_force=1;}
void nd_plugin_catalog_metrics(unsigned long *hits,unsigned long *misses){*hits=cache_hits;*misses=cache_misses;}

struct Library *IconBase=NULL;
struct NDDiscovery nd_prefs_discovery;

static int read_icon_manifest(const char *base,struct NDManifest *m);

static int has_suffix(const char *name,const char *suffix)
{
    size_t n=strlen(name),s=strlen(suffix),i;
    if (n<=s) return 0;
    for (i=0;i<s;++i)
        if (tolower((unsigned char)name[n-s+i])!=tolower((unsigned char)suffix[i]))
            return 0;
    return 1;
}
static int is_file(const char *path)
{
    struct FileInfoBlock *fib;
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    int file=0;
    if (!lock) return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib) {
        if (Examine(lock,fib)) file=fib->fib_DirEntryType<0;
        FreeDosObject(DOS_FIB,fib);
    }
    UnLock(lock);
    return file;
}
static int read_text_manifest(const char *path,struct NDManifest *m)
{ return nd_manifest_read_text(path,m); }

/* Catalogue walk.  One scratch entry is parsed at a time and handed to a
 * sink; only IDs are remembered (for duplicate suppression across the
 * install drawers). The walk state and option scratch space live on the heap. */
typedef int (*NDCatalogSink)(void *ctx,const struct NDPluginCatalogEntry *e);
struct NDCatalogFile {char name[108];struct NDCacheStamp stamp;unsigned long hash;};
struct NDCatalogPending {struct NDCatalogPending *next;char base[300];struct NDCacheRecord *record;};
struct CatalogWalk {
    struct NDPluginCatalogEntry scratch;
    char ids[ND_PLUGIN_CATALOG_MAX][20];
    char seen[6][256];   /* one per install drawer; they may coincide */
    int count,maximum,stop,diagnostics,negative_count,considered;
    struct NDCatalogFile *files;int file_count;int *file_index;
    struct NDCatalogPending *pending;
    const char *wanted; /* non-NULL for a focused ID lookup */
    NDCatalogSink sink;
    void *ctx;
};

static int walk_has(const struct CatalogWalk *w,const char *id)
{
    int i;
    for (i=0;i<w->count;++i) if (!strcmp(w->ids[i],id)) return 1;
    return 0;
}

static unsigned char catalog_fold(unsigned char c)
{return c>='A'&&c<='Z' ? c+('a'-'A'):c<128 ? c:(unsigned char)tolower(c);}
static int catalog_name_equal(const char *a,const char *b)
{while(*a&&*b){if(catalog_fold((unsigned char)*a++)!=catalog_fold((unsigned char)*b++))return 0;}return *a==*b;}
static unsigned long catalog_name_hash(const char *name)
{unsigned long h=5381UL;while(*name)h=(((h<<5)+h)^catalog_fold((unsigned char)*name++))&0xffffffffUL;return h;}
static void catalog_index(struct CatalogWalk *w)
{int i,slot;if(!w->file_index)return;memset(w->file_index,0,1024*sizeof(*w->file_index));for(i=0;i<w->file_count;++i){w->files[i].hash=catalog_name_hash(w->files[i].name);slot=(int)(w->files[i].hash&1023);while(w->file_index[slot])slot=(slot+1)&1023;w->file_index[slot]=i+1;}}
static const struct NDCacheStamp *catalog_file(const struct CatalogWalk *w,const char *name)
{int i,slot;unsigned long hash;if(w->file_index){hash=catalog_name_hash(name);slot=(int)(hash&1023);while(w->file_index[slot]){i=w->file_index[slot]-1;if(w->files[i].hash==hash&&catalog_name_equal(w->files[i].name,name))return &w->files[i].stamp;slot=(slot+1)&1023;}return NULL;}for(i=0;i<w->file_count;++i)if(catalog_name_equal(w->files[i].name,name))return &w->files[i].stamp;return NULL;}
static int catalog_sources(const struct CatalogWalk *w,const char *base,struct NDCacheStamp stamp[4])
{const char *name;const struct NDCacheStamp *found;char part[128],path[320];int i;static const char *suffix[]={"",".ndplugin",".info"};if(!w->files)return nd_cache_sources(base,stamp);name=strrchr(base,'/');name=name ? name+1:base;memset(stamp,0,4*sizeof(*stamp));for(i=0;i<3;++i){sprintf(part,"%s%s",name,suffix[i]);found=catalog_file(w,part);if(found)stamp[i]=*found;}sprintf(part,"%s.assets",name);found=catalog_file(w,part);if(!found)return 1;sprintf(path,"%s.assets/Artwork.ndart",base);return nd_cache_stamp(path,&stamp[3]);}

/* Keep only packed records until the second snapshot confirms their sources. */
static void catalog_remember(struct CatalogWalk *w,const char *base,const struct NDPluginCatalogEntry *e,const struct NDCacheStamp before[4])
{struct NDCatalogPending *p;struct NDCacheStamp after[4];if(!w->files){if(nd_cache_sources(base,after)&&!memcmp(before,after,sizeof(after)))nd_catalog_remember(base,e,after);return;}p=malloc(sizeof(*p));if(!p)return;p->record=nd_catalog_prepare(e,before);if(!p->record){free(p);return;}strcpy(p->base,base);p->next=w->pending;w->pending=p;}
static void catalog_commit_pending(struct CatalogWalk *w,int verified)
{struct NDCatalogPending *p,*next;struct NDCacheStamp after[4];for(p=w->pending;p;p=next){next=p->next;if(catalog_watch&&!catalog_watch(catalog_watch_context,w->count))verified=0;if(verified&&catalog_sources(w,p->base,after)&&!memcmp(after,p->record->header.stamp,sizeof(after)))nd_catalog_commit(p->base,p->record);free(p->record);free(p);}w->pending=NULL;}

static void catalog_consider(struct CatalogWalk *w,const char *dir,
                             const char *exe,int from_text)
{
    struct NDPluginCatalogEntry *e=&w->scratch;
    struct NDManifest *m=&e->manifest;
    char path[300],manifest_path[300];
    struct NDCacheStamp before[4];
    int r,cached,stable;
    if(catalog_watch&&!catalog_watch(catalog_watch_context,w->count)){w->stop=1;return;}
    if (w->stop || w->count>=w->maximum ||
        strlen(dir)+strlen(exe)+12>=sizeof(path)) return;
    ++w->considered;
    sprintf(path,"%s/%s",dir,exe);
    memset(e,0,sizeof(*e));
    nd_manifest_init(m,exe);
    manifest_path[0]='\0';
    stable=catalog_sources(w,path,before);
    cached=stable && !catalog_force && nd_catalog_cached(path,e,before);
    if(cached){++cache_hits;if(e->bundled==-1)return;r=1;strncpy(manifest_path,e->manifest_path,sizeof(manifest_path)-1);}
    else if (from_text) {
        ++cache_misses;
        sprintf(manifest_path,"%s%s",path,ND_MANIFEST_SUFFIX);
        r=read_text_manifest(manifest_path,m);
    } else {++cache_misses;r=read_icon_manifest(path,m);}
    /* A normal application icon is not a plugin manifest. */
    if (r<0) {if(stable && w->considered<=128 && w->negative_count++<ND_PLUGIN_CATALOG_MAX){strncpy(e->command,path,sizeof(e->command)-1);e->bundled=-1;catalog_remember(w,path,e,before);}return;}
    if (!r || (!cached && !nd_manifest_finish(m))) {
        if (!w->diagnostics) return;
        strncpy(e->problem,m->error[0] ? m->error : "invalid manifest",sizeof(e->problem)-1);
    } else if (w->wanted && strcmp(w->wanted,m->id)) {
        return;
    } else if (!cached && !is_file(path)) {
        if (!w->diagnostics) return;
        strcpy(e->problem,"plugin program is missing");
    } else if(!cached && nd_artwork_package_problem(path,m->features,e->problem,sizeof(e->problem))){
        if(!w->diagnostics)return;
    }
    if(e->problem[0] && !w->diagnostics)return;
    /* Invalid metadata is shown by file, even when an ID is absent or
     * duplicates another plugin. It cannot suppress a healthy plugin. */
    strncpy(e->command,path,sizeof(e->command)-1);
    strncpy(e->manifest_path,manifest_path,sizeof(e->manifest_path)-1);
    e->bundled=nd_manifest_bundled_id(m->id);
    if(!cached && r>0 && stable && w->considered<=128)catalog_remember(w,path,e,before);
    if (!e->problem[0] && walk_has(w,m->id)) return;
    if (!e->problem[0]) strcpy(w->ids[w->count],m->id);
    ++w->count;
    if (!w->sink(w->ctx,e)) w->stop=1;
}

static void catalog_drawer_legacy(struct CatalogWalk *w,const char *dir)
{
    struct FileInfoBlock *fib;
    char name[108],other[300];
    BPTR lock=Lock((STRPTR)dir,ACCESS_READ);
    if (!lock) return;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (fib && Examine(lock,fib) && fib->fib_DirEntryType>0) {
        while (!w->stop && w->count<w->maximum && ExNext(lock,fib)) {
            if (fib->fib_DirEntryType>=0) continue;
            strncpy(name,fib->fib_FileName,sizeof(name)-1);
            name[sizeof(name)-1]='\0';
            if (has_suffix(name,ND_MANIFEST_SUFFIX)) {
                name[strlen(name)-strlen(ND_MANIFEST_SUFFIX)]='\0';
                catalog_consider(w,dir,name,1);
            } else if (has_suffix(name,".info")) {
                name[strlen(name)-5]='\0';
                sprintf(other,"%s/%s%s",dir,name,ND_MANIFEST_SUFFIX);
                if (!is_file(other))
                    catalog_consider(w,dir,name,0);
            }
        }
    }
    if (fib) FreeDosObject(DOS_FIB,fib);
    UnLock(lock);
}

/* One bounded directory snapshot supplies program/manifest/icon fingerprints;
 * only an existing artwork drawer needs a separate bank stat. */
static struct NDCatalogFile *catalog_snapshot(const char *dir,int *count)
{struct NDCatalogFile *files;struct FileInfoBlock *fib;BPTR lock;int n=0,ok=0;files=calloc(512,sizeof(*files));if(!files)return NULL;lock=Lock((STRPTR)dir,ACCESS_READ);fib=AllocDosObject(DOS_FIB,NULL);
 if(lock&&fib&&Examine(lock,fib)&&fib->fib_DirEntryType>0){while(n<512&&ExNext(lock,fib)){strncpy(files[n].name,fib->fib_FileName,107);files[n].name[107]=0;nd_cache_fib(fib,&files[n].stamp);++n;}ok=n<512&&IoErr()==ERROR_NO_MORE_ENTRIES;}
 if(fib)FreeDosObject(DOS_FIB,fib);if(lock)UnLock(lock);if(!ok){free(files);return NULL;}*count=n;return files;}
static void catalog_drawer(struct CatalogWalk *w,const char *dir)
{struct NDCatalogFile *files,*after;int n=0,i,after_count=0;char name[108],other[128];files=catalog_snapshot(dir,&n);if(!files){catalog_drawer_legacy(w,dir);return;}
 w->files=files;w->file_count=n;w->file_index=calloc(1024,sizeof(*w->file_index));catalog_index(w);
 for(i=0;i<n&&!w->stop&&w->count<w->maximum;++i){if(files[i].stamp.kind>=0)continue;strcpy(name,files[i].name);if(has_suffix(name,ND_MANIFEST_SUFFIX)){name[strlen(name)-strlen(ND_MANIFEST_SUFFIX)]=0;catalog_consider(w,dir,name,1);}else if(has_suffix(name,".info")){const struct NDCacheStamp *stamp;name[strlen(name)-5]=0;sprintf(other,"%s%s",name,ND_MANIFEST_SUFFIX);stamp=catalog_file(w,other);if(!stamp||stamp->kind>=0)catalog_consider(w,dir,name,0);}}
 if(w->pending){after=catalog_snapshot(dir,&after_count);w->files=after;w->file_count=after_count;catalog_index(w);catalog_commit_pending(w,after!=NULL);free(after);}
 w->files=NULL;w->file_count=0;free(w->file_index);w->file_index=NULL;free(files);}

/* Returns the number of plugins handed to SINK, or -1 without memory. */
static int catalog_walk_filtered(int maximum,NDCatalogSink sink,void *ctx,int diagnostics,const char *wanted)
{
    static const char *drawers[]={"PROGDIR:Plugins",
                                  "NovaDock:Plugins",
                                  "SYS:NovaDock/Plugins",
                                  "SYS:Resources/NovaDock/Plugins",
                                  "SYS:Utilities/NovaDock/Plugins",
                                  "Bin:NovaDock/Plugins",NULL};
    struct Process *self=(struct Process *)FindTask(NULL);
    struct CatalogWalk *w;
    APTR requesters;
    BPTR lock;
    int i,j,count,seen_count=0,opened_icon=0;
    if (maximum<=0) return 0;
    if (maximum>ND_PLUGIN_CATALOG_MAX) maximum=ND_PLUGIN_CATALOG_MAX;
    w=(struct CatalogWalk *)AllocVec(sizeof(*w),MEMF_PUBLIC|MEMF_CLEAR);
    if (!w) return -1;
    w->maximum=maximum;w->sink=sink;w->ctx=ctx;w->diagnostics=diagnostics;w->wanted=wanted;
    opened_icon=IconBase==NULL;
    /* Missing install drawers (no Bin: assign, no PROGDIR:) are skipped
     * silently instead of asking for a volume. */
    requesters=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1L;
    for (i=0;drawers[i] && !w->stop && w->count<maximum;++i) {
        if (i==0 && !GetProgramDir()) continue;
        lock=Lock((STRPTR)drawers[i],ACCESS_READ);
        if (!lock) continue;
        if (!NameFromLock(lock,w->seen[seen_count],sizeof(w->seen[0])))
            w->seen[seen_count][0]='\0';
        UnLock(lock);
        for (j=0;j<seen_count;++j)
            if (!strcmp(w->seen[j],w->seen[seen_count])) break;
        if (j<seen_count) continue;
        /* Saved commands must survive execution in another process: PROGDIR
         * is local to Preferences and cannot be handed to a fresh Shell. */
        if (w->seen[seen_count][0]) catalog_drawer(w,w->seen[seen_count]);
        if (seen_count<5) ++seen_count;
    }
    self->pr_WindowPtr=requesters;
    if (opened_icon && IconBase) {CloseLibrary(IconBase);IconBase=NULL;}
    count=w->count;
    catalog_force=0;
    FreeVec(w);
    return count;
}

static int catalog_walk_mode(int maximum,NDCatalogSink sink,void *ctx,int diagnostics)
{ return catalog_walk_filtered(maximum,sink,ctx,diagnostics,NULL); }

static int catalog_walk(int maximum,NDCatalogSink sink,void *ctx)
{ return catalog_walk_mode(maximum,sink,ctx,0); }

struct ArraySink { struct NDPluginCatalogEntry *entries; int count; };
static int array_sink(void *ctx,const struct NDPluginCatalogEntry *e)
{
    struct ArraySink *a=(struct ArraySink *)ctx;
    a->entries[a->count++]=*e;
    return 1;
}
int nd_plugin_catalog_scan(struct NDPluginCatalogEntry *entries,int maximum)
{
    struct ArraySink a;
    if (!entries || maximum<=0) return 0;
    a.entries=entries;a.count=0;
    (void)catalog_walk(maximum,array_sink,&a);
    return a.count;
}

struct SummarySink { struct NDPluginSummary *out; int count; };
static int summary_sink(void *ctx,const struct NDPluginCatalogEntry *e)
{
    struct SummarySink *a=(struct SummarySink *)ctx;
    struct NDPluginSummary *s=&a->out[a->count++];
    memset(s,0,sizeof(*s));
    strncpy(s->id,e->manifest.id,sizeof(s->id)-1);
    strncpy(s->name,e->manifest.name,sizeof(s->name)-1);
    strncpy(s->command,e->command,sizeof(s->command)-1);
    strncpy(s->version,e->manifest.version,sizeof(s->version)-1);
    s->multiple_instances=e->manifest.multiple_instances;
    strcpy(s->problem,e->problem);
    return 1;
}
int nd_plugin_catalog_summaries(struct NDPluginSummary *out,int maximum)
{
    struct SummarySink a;
    if (!out || maximum<=0) return 0;
    a.out=out;a.count=0;
    (void)catalog_walk(maximum,summary_sink,&a);
    return a.count;
}

int nd_plugin_catalog_inventory(struct NDPluginSummary *out,int maximum)
{
    struct SummarySink a;
    if (!out || maximum<=0) return 0;
    a.out=out;a.count=0;
    (void)catalog_walk_mode(maximum,summary_sink,&a,1);
    return a.count;
}

struct LookupSink { const char *id; struct NDPluginCatalogEntry *out; int found; };
static int lookup_sink(void *ctx,const struct NDPluginCatalogEntry *e)
{
    struct LookupSink *a=(struct LookupSink *)ctx;
    if (strcmp(e->manifest.id,a->id)) return 1;
    *a->out=*e;a->found=1;
    return 0;
}
int nd_plugin_catalog_lookup(const char *id,struct NDPluginCatalogEntry *out)
{
    struct LookupSink a;
    if (!id || !*id || !out) return 0;
    a.id=id;a.out=out;a.found=0;
    (void)catalog_walk_filtered(ND_PLUGIN_CATALOG_MAX,lookup_sink,&a,0,id);
    return a.found;
}
/* Returns 1 manifest read, 0 invalid, -1 icon carries no NOVADOCK_ types. */
static int read_icon_manifest(const char *base,struct NDManifest *m)
{
    struct DiskObject *icon;
    STRPTR *type;
    int seen=0,ok=1,r;
    if (!IconBase) IconBase=OpenLibrary("icon.library",36);
    if (!IconBase) return -1;
    icon=GetDiskObject((STRPTR)base);
    if (!icon) return -1;
    if (icon->do_ToolTypes)
        for (type=(STRPTR *)icon->do_ToolTypes;*type;++type) {
            r=nd_manifest_line(m,(const char *)*type);
            if (r!=-1) seen=1;
            if (r==0) {ok=0;break;}
        }
    FreeDiskObject(icon);
    return seen ? ok : -1;
}
static void note_problem(struct NDDiscovery *out,const char *name,
                         const char *why)
{
    ++out->rejected;
    if (!out->problem[0])
        sprintf(out->problem,"Plugin %.24s ignored: %.48s",name,why);
}
struct MergeSink { struct NDConfig *cfg; struct NDDiscovery *out; };
static int merge_sink(void *ctx,const struct NDPluginCatalogEntry *e)
{
    struct MergeSink *a=(struct MergeSink *)ctx;
    struct NDDiscovery *out=a->out;
    int r;
    if (e->problem[0]) {note_problem(out,e->manifest.name,e->problem);return 1;}
    r=nd_manifest_merge(a->cfg,&e->manifest,e->command);
    if (r==ND_MANIFEST_ADDED) {
        if (out->added<ND_DISCOVERY_MAX) {
            strcpy(out->added_ids[out->added],e->manifest.id);
            strcpy(out->added_names[out->added],e->manifest.name);
            ++out->added;
        }
    } else if (r==ND_MANIFEST_UPDATED) ++out->updated;
    else if (r==ND_MANIFEST_REJECTED)
        note_problem(out,e->manifest.name,
                     "ID clash, too many settings or dock full");
    return 1;
}
/* Formerly a 48-entry (~167 KB) catalogue was built first; plugins are now
 * merged one at a time as the drawers are read. */
int nd_discover_plugins(struct NDConfig *cfg,struct NDDiscovery *out)
{
    struct MergeSink a;
    if (!cfg || !out) return 0;
    memset(out,0,sizeof(*out));
    a.cfg=cfg;a.out=out;
    (void)catalog_walk_mode(ND_PLUGIN_CATALOG_MAX,merge_sink,&a,1);
    return out->added;
}
int nd_discovery_is_new(const char *id)
{
    int i;
    for (i=0;i<nd_prefs_discovery.added;++i)
        if (!strcmp(nd_prefs_discovery.added_ids[i],id)) return 1;
    return 0;
}

int nd_manifest_read_file(const char *path,struct NDManifest *m)
{
    if (!path || !m) return 0;
    return read_text_manifest(path,m);
}
int nd_plugin_install_drawer(char *out,int size)
{
    static const char *drawers[]={"PROGDIR:Plugins",
                                  "NovaDock:Plugins",
                                  "SYS:NovaDock/Plugins",
                                  "SYS:Resources/NovaDock/Plugins",
                                  "SYS:Utilities/NovaDock/Plugins",
                                  "Bin:NovaDock/Plugins",NULL};
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR requesters=self->pr_WindowPtr;
    BPTR lock;
    int i,found=0;
    if (!out || size<=0) return 0;
    out[0]='\0';
    self->pr_WindowPtr=(APTR)-1L;
    for (i=0;drawers[i] && !found;++i) {
        if (i==0 && !GetProgramDir()) continue;
        lock=Lock((STRPTR)drawers[i],ACCESS_READ);
        if (!lock) continue;
        found=NameFromLock(lock,out,size)!=0;
        UnLock(lock);
        if (!found) out[0]='\0';
    }
    self->pr_WindowPtr=requesters;
    return found;
}
