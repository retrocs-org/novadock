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
#include "nd_manifest.h"
#include "nd_plugin_discovery.h"

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
{
    static char line[512];
    FILE *file=fopen(path,"r");
    int ok=1,ch;
    if (!file) return 0;
    while (fgets(line,sizeof(line),file)) {
        if (!strchr(line,'\n') && !feof(file)) {
            while ((ch=fgetc(file))!='\n' && ch!=EOF) {}
            continue;
        }
        if (nd_manifest_line(m,line)==0) {ok=0;break;}
    }
    fclose(file);
    return ok;
}

/* Catalogue walk.  One scratch entry is parsed at a time and handed to a
 * sink; only IDs are remembered (for duplicate suppression across the
 * install drawers).  The whole walk state lives on the heap (~5 KB). */
typedef int (*NDCatalogSink)(void *ctx,const struct NDPluginCatalogEntry *e);
struct CatalogWalk {
    struct NDPluginCatalogEntry scratch;
    char ids[ND_PLUGIN_CATALOG_MAX][20];
    char seen[6][256];   /* one per install drawer; they may coincide */
    int count,maximum,stop;
    NDCatalogSink sink;
    void *ctx;
};

static int walk_has(const struct CatalogWalk *w,const char *id)
{
    int i;
    for (i=0;i<w->count;++i) if (!strcmp(w->ids[i],id)) return 1;
    return 0;
}

static void catalog_consider(struct CatalogWalk *w,const char *dir,
                             const char *exe,int from_text)
{
    struct NDPluginCatalogEntry *e=&w->scratch;
    struct NDManifest *m=&e->manifest;
    char path[300],manifest_path[300];
    int r;
    if (w->stop || w->count>=w->maximum ||
        strlen(dir)+strlen(exe)+12>=sizeof(path)) return;
    sprintf(path,"%s/%s",dir,exe);
    if (!is_file(path)) return;
    memset(e,0,sizeof(*e));
    nd_manifest_init(m,exe);
    manifest_path[0]='\0';
    if (from_text) {
        sprintf(manifest_path,"%s%s",path,ND_MANIFEST_SUFFIX);
        r=read_text_manifest(manifest_path,m);
    } else r=read_icon_manifest(path,m);
    if (r<=0 || !nd_manifest_finish(m) || walk_has(w,m->id)) return;
    strncpy(e->command,path,sizeof(e->command)-1);
    strncpy(e->manifest_path,manifest_path,sizeof(e->manifest_path)-1);
    e->bundled=nd_manifest_bundled_id(m->id);
    strcpy(w->ids[w->count++],m->id);
    if (!w->sink(w->ctx,e)) w->stop=1;
}

static void catalog_drawer(struct CatalogWalk *w,const char *dir)
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

/* Returns the number of plugins handed to SINK, or -1 without memory. */
static int catalog_walk(int maximum,NDCatalogSink sink,void *ctx)
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
    w->maximum=maximum;w->sink=sink;w->ctx=ctx;
    if (!IconBase) {
        IconBase=OpenLibrary("icon.library",36);
        opened_icon=IconBase!=NULL;
    }
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
        catalog_drawer(w,drawers[i]);
        if (seen_count<5) ++seen_count;
    }
    self->pr_WindowPtr=requesters;
    if (opened_icon) {CloseLibrary(IconBase);IconBase=NULL;}
    count=w->count;
    FreeVec(w);
    return count;
}

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
    (void)catalog_walk(ND_PLUGIN_CATALOG_MAX,lookup_sink,&a);
    return a.found;
}
/* Returns 1 manifest read, 0 invalid, -1 icon carries no NOVADOCK_ types. */
static int read_icon_manifest(const char *base,struct NDManifest *m)
{
    struct DiskObject *icon;
    STRPTR *type;
    int seen=0,ok=1,r;
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
    int r=nd_manifest_merge(a->cfg,&e->manifest,e->command);
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
    (void)catalog_walk(ND_PLUGIN_CATALOG_MAX,merge_sink,&a);
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
