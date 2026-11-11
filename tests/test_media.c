#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_media.h"

static void set(struct NDMediaVolume *m,const char *volume,const char *drive,
                const char *device)
{
    memset(m,0,sizeof(*m));
    strcpy(m->volume,volume);strcpy(m->drive,drive);strcpy(m->device,device);
}
int main(void)
{
    struct NDConfig cfg;
    struct NDMediaVolume a[6],b[6];
    int n,before;
    assert(nd_media_device_removable("trackdisk.device",NULL));
    assert(nd_media_device_removable("TrackDisk.Device",""));
    assert(nd_media_device_removable("usbscsi.device",NULL));
    assert(!nd_media_device_removable("scsi.device",NULL));
    assert(!nd_media_device_removable("2nd.scsi.device",NULL));
    assert(nd_media_device_removable("scsi.device","foo.device, scsi.device"));
    assert(!nd_media_device_removable("trackdisk.device","scsi.device"));
    assert(!nd_media_device_removable("",NULL));

    set(&a[0],"Work","DF1:","trackdisk.device");
    set(&a[1],"Empty","DF0:","trackdisk.device");
    set(&a[2],"Work","DF1:","trackdisk.device");
    n=nd_media_normalize(a,3);
    assert(n==2 && !strcmp(a[0].drive,"DF0:") && !strcmp(a[1].drive,"DF1:"));
    set(&b[0],"Empty","DF0:","trackdisk.device");
    set(&b[1],"Work","DF1:","trackdisk.device");
    assert(nd_media_same(a,n,b,2));
    strcpy(b[1].volume,"Games");
    assert(!nd_media_same(a,n,b,2));
    assert(!nd_media_same(a,n,b,1));

    nd_default_config(&cfg);
    assert(cfg.removable_media==1);
    nd_compact_enabled(&cfg);
    before=cfg.count;
    assert(nd_media_persistent_count(&cfg)==before);
    /* Pinned volume and system volume are not duplicated. */
    assert(nd_add_launcher(&cfg,ND_ITEM_DRAWER,"Games","GAMES:","GAMES:Disk"));
    ++before;
    set(&a[0],"Empty","DF0:","trackdisk.device");
    set(&a[1],"Games","DF1:","trackdisk.device");
    set(&a[2],"Workbench","CF0:","carddisk.device");
    n=nd_media_append(&cfg,a,3,"workbench");
    assert(n==1 && cfg.count==before+1);
    assert(cfg.items[before].transient && cfg.items[before].kind==ND_ITEM_DRAWER);
    assert(!strcmp(cfg.items[before].command,"Empty:"));
    assert(!strcmp(nd_item_text(&cfg,&cfg.items[before],ND_TEXT_ICON),"Empty:Disk"));
    assert(!strcmp(cfg.items[before].id,"DF0:"));
    assert(nd_item_is_volume(&cfg.items[before]));
    assert(nd_media_persistent_count(&cfg)==before);
    /* Transient tiles never reach the configuration file. */
    assert(nd_write_config(&cfg,"tests/nd_media_test.cfg"));
    {
        struct NDConfig back;
        int i;
        assert(nd_load_config(&back,"tests/nd_media_test.cfg")==0);
        assert(back.removable_media==1);
        for (i=0;i<back.count;++i)
            assert(!back.items[i].transient &&
                   strcmp(back.items[i].command,"Empty:"));
    }
    remove("tests/nd_media_test.cfg");
    remove("tests/nd_media_test.cfg.bak");
    /* Disabled feature appends nothing; parse/write round trip. */
    nd_default_config(&cfg);
    assert(nd_parse_line(&cfg,"REMOVABLE_MEDIA=no") && !cfg.removable_media);
    assert(nd_media_append(&cfg,a,1,NULL)==0);
    assert(!nd_parse_line(&cfg,"REMOVABLE_MEDIA=maybe"));
    /* Bad names and the tile limit are respected. */
    nd_default_config(&cfg);
    set(&a[0],"Bad:Name","DF0:","trackdisk.device");
    assert(nd_media_append(&cfg,a,1,NULL)==0);
    while (cfg.count<ND_MAX_ITEMS-1)
        assert(nd_add_spacer(&cfg,"S"));
    set(&a[0],"One","DF0:","trackdisk.device");
    set(&a[1],"Two","DF1:","trackdisk.device");
    assert(nd_media_append(&cfg,a,2,NULL)==1 && cfg.count==ND_MAX_ITEMS);
    puts("Removable media detection policy and transient tiles passed.");
    return 0;
}
