/* Plugin package install: all-or-nothing in every failure position. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_package_install.h"

#define DIR "tests/nd_pkg_tmp"
static const char *names[3]={"Hello","Hello.info","Hello.ndplugin"};

static void put(const char *path,const char *text)
{
    FILE *f=fopen(path,"wb");
    assert(f);fputs(text,f);fclose(f);
}
static int has(const char *path,const char *text)
{
    char buffer[64];
    size_t n;
    FILE *f=fopen(path,"rb");
    if (!f) return text==NULL;
    n=fread(buffer,1,sizeof(buffer)-1,f);fclose(f);buffer[n]=0;
    return text && !strcmp(buffer,text);
}
static int exists(const char *path)
{
    FILE *f=fopen(path,"rb");
    if (f) fclose(f);
    return f!=NULL;
}
static void clean(void)
{
    char path[128];
    int i;
    static const char *suffix[]={"",".tmp",".bak",NULL};
    for (i=0;i<3;++i) {
        int k;
        for (k=0;suffix[k];++k) {
            sprintf(path,DIR "/src/%s%s",names[i],suffix[k]);remove(path);
            sprintf(path,DIR "/dst/%s%s",names[i],suffix[k]);remove(path);
        }
    }
}
static void setup(struct NDPackageFile *f,int with_old)
{
    char path[128];
    int i;
    clean();
    memset(f,0,sizeof(*f)*3);
    for (i=0;i<3;++i) {
        sprintf(path,DIR "/src/%s",names[i]);put(path,"new");
        sprintf(f[i].source,DIR "/src/%s",names[i]);
        sprintf(f[i].destination,DIR "/dst/%s",names[i]);
        if (with_old) {sprintf(path,DIR "/dst/%s",names[i]);put(path,"old");}
    }
}
static void expect(const char *content)
{
    char path[128];
    int i;
    for (i=0;i<3;++i) {
        sprintf(path,DIR "/dst/%s",names[i]);
        assert(has(path,content));
        sprintf(path,DIR "/dst/%s.tmp",names[i]);assert(!exists(path));
        sprintf(path,DIR "/dst/%s.bak",names[i]);assert(!exists(path));
    }
}
int main(void)
{
    struct NDPackageFile f[3];
    int old,at,failures=0;
    assert(system("mkdir -p " DIR "/src " DIR "/dst")==0);
    setup(f,0);assert(nd_package_install(f,3));expect("new");
    setup(f,1);assert(nd_package_install(f,3));expect("new");
    /* A missing source: nothing changes. */
    setup(f,1);remove(DIR "/src/Hello.info");
    assert(!nd_package_install(f,3));expect("old");
    setup(f,0);remove(DIR "/src/Hello.ndplugin");
    assert(!nd_package_install(f,3));expect(NULL);
    /* A rename failing at every possible point restores the old state. */
    for (old=0;old<2;++old)
        for (at=1;at<=6;++at) {
            setup(f,old);
            nd_package_fail_rename_at=at;
            if (!nd_package_install(f,3)) {++failures;expect(old ? "old" : NULL);}
            else expect("new"); /* no rename left to fail at this position */
            nd_package_fail_rename_at=0;
        }
    assert(failures>=9);
    setup(f,1);
    nd_package_fail_close=1;
    assert(nd_package_install(f,3)==ND_PACKAGE_FAILED);expect("old");
    nd_package_fail_close=0;
    setup(f,1);
    nd_package_fail_rename_at=2;nd_package_fail_restore_at=3;
    assert(nd_package_install(f,3)==ND_PACKAGE_RECOVERY);
    assert(f[0].recovery && has(DIR "/dst/Hello.bak","old"));
    assert(!exists(DIR "/dst/Hello"));
    nd_package_fail_rename_at=nd_package_fail_restore_at=0;
    /* A retry cannot destroy the only surviving original. */
    assert(nd_package_install(f,3)==ND_PACKAGE_RECOVERY);
    assert(has(DIR "/dst/Hello.bak","old"));
    setup(f,1);
    nd_package_fail_rename_at=4;nd_package_fail_restore_at=6;
    assert(nd_package_install(f,3)==ND_PACKAGE_RECOVERY);
    assert(has(DIR "/dst/Hello.bak","old"));
    nd_package_fail_rename_at=nd_package_fail_restore_at=0;

    clean();
    assert(system("rm -rf " DIR)==0);
    printf("Plugin package install: all-or-nothing in %d failure positions.\n",failures);
    return 0;
}
