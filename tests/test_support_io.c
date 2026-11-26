/* Fault-inject the real support writer: ENV-like rename failures and
 * partial writes must not destroy the last complete settings generation. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "nd_support.h"
static int no_rename,fail_stage,fail_backup,fail_writes;
static FILE *writing_primary;
static FILE *test_open(const char *path,const char *mode)
{
    FILE *f;
    if(mode[0]=='w'&&((fail_stage&&!strcmp(path,"support.cfg.new"))||(fail_backup&&!strcmp(path,"support.cfg.bak")))){errno=EACCES;return NULL;}
    f=fopen(path,mode);if(f&&mode[0]=='w'&&!strcmp(path,"support.cfg"))writing_primary=f;return f;
}
static size_t test_write(const void *bytes,size_t size,size_t count,FILE *f)
{
    if(f==writing_primary&&fail_writes){--fail_writes;fwrite(bytes,size,count/2,f);errno=ENOSPC;return count/2;}
    return fwrite(bytes,size,count,f);
}
static int test_close(FILE *f)
{if(f==writing_primary)writing_primary=NULL;return fclose(f);}
static int test_rename(const char *a,const char *b)
{if(no_rename){errno=EACCES;return -1;}return rename(a,b);}
#undef ND_SUPPORT_ENV
#undef ND_SUPPORT_ENVARC
#define ND_SUPPORT_ENV "support.cfg"
#define ND_SUPPORT_ENVARC "persistent.cfg"
#define fopen test_open
#define fwrite test_write
#define fclose test_close
#define rename test_rename
#include "../src/nd_support.c"
#undef fopen
#undef fwrite
#undef fclose
#undef rename
static void matches(int level,int persist)
{struct NDSupportSettings s;assert(nd_support_read("support.cfg",&s)&&s.level==level&&s.persist==persist);}
int main(void)
{
    struct NDSupportSettings a,b;FILE *f;
    a.level=0;a.persist=0;no_rename=1;
    assert(nd_support_write("support.cfg",&a));matches(0,0); /* first save */
    a.level=2;a.persist=1;assert(nd_support_write("support.cfg",&a));matches(2,1);
    assert(nd_support_read("support.cfg.bak",&b)&&b.level==0&&!b.persist);
    fail_stage=1;assert(!nd_support_write("support.cfg",&a));matches(2,1);
    assert(strstr(nd_support_error(),"support.cfg.new")&&strstr(nd_support_error(),"create staged"));fail_stage=0;
    a.level=1;a.persist=0;fail_backup=1;assert(!nd_support_write("support.cfg",&a));matches(2,1);fail_backup=0;
    fail_writes=1;assert(!nd_support_write("support.cfg",&a));matches(2,1);
    assert(strstr(nd_support_error(),"publish settings"));
    /* Even failed rollback leaves a complete backup for settings reload. */
    fail_writes=2;assert(!nd_support_write("support.cfg",&a));
    assert(nd_support_read("support.cfg.bak",&b)&&b.level==2&&b.persist);
    nd_support_load(&b,0);assert(b.level==2&&b.persist);
    assert(nd_support_write("support.cfg",&a));matches(1,0);
    /* A malformed primary never replaces a valid backup. */
    f=fopen("support.cfg","w");assert(f);fputs("MODE=BAD\n",f);fclose(f);
    a.level=0;assert(nd_support_write("support.cfg",&a));
    assert(nd_support_read("support.cfg.bak",&b)&&b.level==2&&b.persist);
    assert(!nd_support_write("support.cfg",NULL));
    remove("support.cfg");remove("support.cfg.bak");remove("support.cfg.new");
    puts("Support I/O: rename-less first/update saves, stage/backup failures, partial-write rollback, backup recovery and precise errors passed.");return 0;
}
