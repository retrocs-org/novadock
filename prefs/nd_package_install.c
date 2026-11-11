/* Transactional multi-file install; see nd_package_install.h.  The same
 * logic runs on AmigaDOS and, for the host tests, on POSIX files. */
#include <string.h>
#include "nd_package_install.h"
#ifdef ND_PACKAGE_TEST
int nd_package_fail_close=0;
#endif

#if defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__)
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
static int exists(const char *path)
{
    /* A missing volume must not raise a requester. */
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR requesters=self->pr_WindowPtr;
    BPTR lock;
    self->pr_WindowPtr=(APTR)-1L;
    lock=Lock((STRPTR)path,ACCESS_READ);
    self->pr_WindowPtr=requesters;
    if (lock) {UnLock(lock);return 1;}
    return 0;
}
static int copy(const char *from,const char *to)
{
    BPTR in,out=0;
    UBYTE *buffer=(UBYTE *)AllocVec(4096,MEMF_PUBLIC);
    LONG got=0;
    int ok=buffer!=NULL;
    in=ok ? Open((STRPTR)from,MODE_OLDFILE) : 0;
    if (in) out=Open((STRPTR)to,MODE_NEWFILE);
    if (!in || !out) ok=0;
    while (ok && (got=Read(in,buffer,4096))>0)
        if (Write(out,buffer,got)!=got) ok=0;
    if (got<0) ok=0;
    if (out && !Close(out)) ok=0;
    if (in && !Close(in)) ok=0;
    if (buffer) FreeVec(buffer);
    if (!ok) DeleteFile((STRPTR)to);
    return ok;
}
static int rename_file(const char *from,const char *to)
{
    return Rename((STRPTR)from,(STRPTR)to)!=0;
}
static void delete_file(const char *path) { DeleteFile((STRPTR)path); }
int nd_package_join(char *out,int size,const char *drawer,const char *name)
{
    if ((int)strlen(drawer)>=size) return 0;
    strcpy(out,drawer);
    return AddPart((STRPTR)out,(STRPTR)name,size)!=0;
}
#else
#include <stdio.h>
static int exists(const char *path)
{
    FILE *f=fopen(path,"rb");
    if (f) fclose(f);
    return f!=NULL;
}
static int copy(const char *from,const char *to)
{
    char buffer[4096];
    size_t got;
    int ok=1;
    FILE *in=fopen(from,"rb"),*out=in ? fopen(to,"wb") : NULL;
    if (!in || !out) ok=0;
    while (ok && (got=fread(buffer,1,sizeof(buffer),in))>0)
        if (fwrite(buffer,1,got,out)!=got) ok=0;
    if (in && ferror(in)) ok=0;
    if (out && fclose(out)!=0) ok=0;
    if (in && fclose(in)!=0) ok=0;
#ifdef ND_PACKAGE_TEST
    if (nd_package_fail_close) ok=0;
#endif
    if (!ok) remove(to);
    return ok;
}
#ifdef ND_PACKAGE_TEST
int nd_package_fail_rename_at=0;
int nd_package_fail_restore_at=0;
static int renames=0;
#endif
static int rename_file(const char *from,const char *to)
{
#ifdef ND_PACKAGE_TEST
    ++renames;
    if ((nd_package_fail_rename_at && renames==nd_package_fail_rename_at) ||
        (nd_package_fail_restore_at && renames==nd_package_fail_restore_at)) return 0;
#endif
    /* AmigaDOS Rename() refuses an existing destination; so must this. */
    if (exists(to)) return 0;
    return rename(from,to)==0;
}
static void delete_file(const char *path) { remove(path); }
int nd_package_join(char *out,int size,const char *drawer,const char *name)
{
    size_t n=strlen(drawer);
    if (n+strlen(name)+2>(size_t)size) return 0;
    strcpy(out,drawer);
    if (n && out[n-1]!=':' && out[n-1]!='/') strcat(out,"/");
    strcat(out,name);
    return 1;
}
#endif

int nd_package_file_exists(const char *path) { return exists(path); }

int nd_package_install(struct NDPackageFile *files,int count)
{
    int i,j,recovery=0;
#if defined(ND_PACKAGE_TEST) && !(defined(__amigaos__) || defined(AMIGA) || defined(__VBCC__))
    renames=0;
#endif
    if (!files || count<1) return 0;
    for (i=0;i<count;++i) {files[i].had_old=0;files[i].committed=0;files[i].recovery=0;}
    for (i=0;i<count;++i) {
        struct NDPackageFile *f=&files[i];
        if (strlen(f->destination)+5>=sizeof(f->temporary)) {
            for (j=0;j<i;++j) delete_file(files[j].temporary);
            return 0;
        }
        strcpy(f->temporary,f->destination);strcat(f->temporary,".tmp");
        strcpy(f->backup,f->destination);strcat(f->backup,".bak");
        /* Never overwrite a backup left by an interrupted/failed install. */
        if (exists(f->backup)) {
            f->recovery=1;
            for (j=0;j<i;++j) delete_file(files[j].temporary);
            return ND_PACKAGE_RECOVERY;
        }
        delete_file(f->temporary);
        if (!copy(f->source,f->temporary)) {
            for (j=0;j<i;++j) delete_file(files[j].temporary);
            return 0;
        }
    }
    for (i=0;i<count;++i) {
        struct NDPackageFile *f=&files[i];
        f->had_old=exists(f->destination);
        if (f->had_old) {
            delete_file(f->backup);
            if (!rename_file(f->destination,f->backup)) {f->had_old=0;break;}
        }
        if (!rename_file(f->temporary,f->destination)) {
            if (f->had_old && !rename_file(f->backup,f->destination))
                {f->recovery=1;recovery=1;}
            break;
        }
        f->committed=1;
    }
    if (i<count) {
        /* Undo the files already swapped in, newest first. */
        for (j=i-1;j>=0;--j) {
            struct NDPackageFile *f=&files[j];
            if (!f->committed) continue;
            delete_file(f->destination);
            if (f->had_old && !rename_file(f->backup,f->destination))
                {f->recovery=1;recovery=1;}
            else if (!f->had_old && exists(f->destination))
                {f->recovery=2;recovery=1;}
        }
        for (j=0;j<count;++j) delete_file(files[j].temporary);
        return recovery ? ND_PACKAGE_RECOVERY : ND_PACKAGE_FAILED;
    }
    for (i=0;i<count;++i) if (files[i].had_old) delete_file(files[i].backup);
    return 1;
}
