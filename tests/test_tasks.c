#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_tasks.h"
#include "nd_media.h"

static void set(struct NDTaskEntry *t,unsigned long key,const char *label,
                const char *path)
{
    memset(t,0,sizeof(*t));
    t->key=key;strcpy(t->label,label);strcpy(t->path,path);t->windows=1;
}
int main(void)
{
    char label[ND_TASK_NAME_LEN];
    struct NDTaskEntry a[8],b[8];
    struct NDConfig cfg;
    struct NDMediaVolume disk;
    int n,base;
    /* Label choice. */
    assert(nd_task_label("Background CLI","Work:Apps/GoldED","GoldED 8",label,sizeof(label)) &&
           !strcmp(label,"GoldED"));
    assert(nd_task_label("MultiView",NULL,"MultiView - pic.iff",label,sizeof(label)) &&
           !strcmp(label,"MultiView"));
    assert(nd_task_label("Shell Process","","AmigaShell",label,sizeof(label)) &&
           !strcmp(label,"AmigaShell"));
    assert(nd_task_label("CON","",  "  My Window  ",label,sizeof(label)) &&
           !strcmp(label,"My Window"));
    assert(!nd_task_label("Background CLI","",NULL,label,sizeof(label)));
    assert(nd_task_label("Averylongtaskname",NULL,NULL,label,8) &&
           strlen(label)==7);
    assert(nd_task_label("Bad|Name",NULL,"Good",label,sizeof(label)) &&
           !strcmp(label,"Good"));
    assert(nd_task_ignored("IPrefs","Exchange, iprefs"));
    assert(!nd_task_ignored("IPrefs","Exchange"));
    assert(!nd_task_ignored("IPrefs",NULL));
    /* Normalisation: sorted, distinct keys, empty labels dropped. */
    set(&a[0],0x300,"Zed","");
    set(&a[1],0x100,"Alpha","Work:Alpha");
    set(&a[2],0x300,"Zed","");
    set(&a[3],0x200,"","");
    n=nd_tasks_normalize(a,4);
    assert(n==2 && a[0].key==0x100 && a[1].key==0x300);
    set(&b[0],0x100,"Alpha","Work:Alpha");
    set(&b[1],0x300,"Zed","");
    assert(nd_tasks_same(a,n,b,2));
    strcpy(b[1].label,"Zed2");
    assert(!nd_tasks_same(a,n,b,2));
    /* Automatic task tiles are retired. Old YES values are accepted only so
     * legacy configurations keep loading; they are normalised to off. */
    nd_default_config(&cfg);
    assert(cfg.running_section==0);
    nd_compact_enabled(&cfg);
    assert(nd_tasks_append(&cfg,a,n)==0);
    assert(nd_parse_line(&cfg,"RUNNING_SECTION=yes") && !cfg.running_section);
    base=cfg.count;
    assert(nd_tasks_append(&cfg,a,n)==0 && cfg.count==base);
    assert(nd_task_tile_key(&cfg.items[0])==0);
    memset(&disk,0,sizeof(disk));
    strcpy(disk.volume,"Empty");strcpy(disk.drive,"DF0:");
    assert(nd_media_append(&cfg,&disk,1,NULL)==1);
    assert(cfg.items[cfg.count-1].transient==ND_TRANSIENT_MEDIA);
    assert(nd_media_persistent_count(&cfg)==base);
    assert(nd_write_config(&cfg,"tests/nd_tasks_test.cfg"));
    {
        struct NDConfig back;
        int i;
        assert(nd_load_config(&back,"tests/nd_tasks_test.cfg")==0);
        assert(back.running_section==0);
        for (i=0;i<back.count;++i)
            assert(!back.items[i].transient && strcmp(back.items[i].command,"@task"));
    }
    remove("tests/nd_tasks_test.cfg");
    remove("tests/nd_tasks_test.cfg.bak");
    assert(!nd_parse_line(&cfg,"RUNNING_SECTION=sometimes"));
    /* Limits. */
    nd_default_config(&cfg);
    cfg.running_section=1;
    for (n=0;n<8;++n) { char l[8]; sprintf(l,"App%d",n); set(&a[n],0x1000+n,l,""); }
    base=cfg.count;
    assert(nd_tasks_append(&cfg,a,8)==ND_TASK_MAX && cfg.count==base+ND_TASK_MAX);
    puts("Task matching and retired automatic-task-tile policy passed.");
    return 0;
}
