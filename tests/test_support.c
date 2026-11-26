#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "nd_support.h"
int main(void)
{
    struct NDSupportSettings a,b;FILE *f;int level,persist;
    nd_support_load(&a,0);assert(a.level==0&&a.persist==0);
    nd_support_load(&a,1);assert(a.level==2&&a.persist==1);
    for(level=0;level<=2;++level)for(persist=0;persist<=1;++persist){
        a.level=level;a.persist=persist;assert(nd_support_write("support.cfg",&a));
        b.level=-1;b.persist=-1;assert(nd_support_read("support.cfg",&b));
        assert(a.level==b.level&&a.persist==b.persist);
    }
    a.level=9;assert(!nd_support_write("support.cfg",&a));
    assert(nd_support_read("support.cfg",&b)&&b.level==2&&b.persist==1);
    f=fopen("support.cfg","w");assert(f);fputs("MODE=TRACE\nSAVE_TO_DISK=MAYBE\n",f);fclose(f);
    b.level=7;b.persist=7;assert(!nd_support_read("support.cfg",&b));assert(b.level==7&&b.persist==7);
    f=fopen("support.cfg","w");assert(f);fputs("MODE=OFF\n",f);fclose(f);assert(!nd_support_read("support.cfg",&b));
    f=fopen("support.cfg","w");assert(f);fputs("# comment\n MODE = LOG \n SAVE_TO_DISK = YES \n",f);fclose(f);
    assert(nd_support_read("support.cfg",&b)&&b.level==1&&b.persist==1);
    a.level=1;a.persist=0;assert(!nd_support_write("missing/support.cfg",&a));
    assert(nd_support_read("support.cfg",&b)&&b.level==1&&b.persist==1);
    remove("support.cfg");remove("support.cfg.bak");remove("support.cfg.new");
    mkdir("ENV:NovaDock",0700);mkdir(ND_SUPPORT_DISK,0700);
    a.level=2;a.persist=1;assert(nd_support_write(ND_SUPPORT_ENV,&a));
    f=fopen(ND_SUPPORT_CAPTURE_OWNER,"w");assert(f);fprintf(f,"1 %lu 1 0\n",(unsigned long)time(NULL)-1UL);fclose(f);
    nd_support_expire_capture();assert(nd_support_read(ND_SUPPORT_ENV,&b)&&b.level==1&&!b.persist);
    assert(!fopen(ND_SUPPORT_CAPTURE_OWNER,"r"));
    f=fopen(ND_SUPPORT_CAPTURE_OWNER,"w");assert(f);fprintf(f,"1 %lu 0 0\n",(unsigned long)time(NULL)+60UL);fclose(f);
    a.level=1;a.persist=1;assert(nd_support_write(ND_SUPPORT_ENV,&a));
    nd_support_expire_capture();assert(nd_support_read(ND_SUPPORT_ENV,&b)&&b.level==1&&b.persist);
    f=fopen(ND_SUPPORT_DISK "/dock.log","w");assert(f);fputs("synthetic trace\n",f);fclose(f);
    f=fopen(ND_SUPPORT_CAPTURE_FILE,"w");assert(f);fprintf(f,"NovaDock capture 1 completed=1 seconds=60 end=%lu\n",(unsigned long)time(NULL));fclose(f);
    assert(nd_support_recorded());
    f=fopen(ND_SUPPORT_CAPTURE_FILE,"w");assert(f);fprintf(f,"NovaDock capture 1 completed=0 seconds=60 end=%lu\n",(unsigned long)time(NULL));fclose(f);assert(!nd_support_recorded());
    f=fopen(ND_SUPPORT_CAPTURE_FILE,"w");assert(f);fprintf(f,"NovaDock capture 1 completed=1 seconds=60 end=%lu\n",(unsigned long)time(NULL)-90000UL);fclose(f);assert(!nd_support_recorded());
    remove(ND_SUPPORT_ENV);remove(ND_SUPPORT_ENV ".bak");remove(ND_SUPPORT_CAPTURE_FILE);remove(ND_SUPPORT_DISK "/dock.log");
    puts("Support settings: modes, disk saving, atomic round-trips, malformed/missing fields and failed writes passed.");return 0;
}
