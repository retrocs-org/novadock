#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_manifest.h"
#include "nd_options.h"

static int load(struct NDManifest *m,const char *file,const char **lines)
{
    nd_manifest_init(m,file);
    for (;*lines;++lines) if (nd_manifest_line(m,*lines)==0) return 0;
    return nd_manifest_finish(m);
}
int main(void)
{
    static const char *weather[]={
        "; comment lines and other tool types are ignored",
        "DONOTWAIT",
        "NOVADOCK_ID=WEATHER",
        "NOVADOCK_NAME=Weather",
        "NOVADOCK_VIEW=GRAPH",
        "NOVADOCK_ABI=2",
        "NOVADOCK_MULTIPLE=YES",
        "NOVADOCK_OPTION=CITY|City|TEXT||Stockholm",
        "NOVADOCK_OPTION=UNITS|Units|CHOICE|C,F|C",
        "NOVADOCK_OPTION=POLL_TICKS|Refresh ticks|UINT|250,30000|3000",
        "NOVADOCK_FUTURE=ignored",
        NULL};
    static const char *weather2[]={
        "NOVADOCK_ID=WEATHER",
        "NOVADOCK_MULTIPLE=YES",
        "NOVADOCK_OPTION=CITY|City|TEXT||Stockholm",
        "NOVADOCK_OPTION=WIND|Show wind|BOOL||YES",
        NULL};
    struct NDManifest m;
    struct NDConfig cfg;
    struct NDOptionDef defs[16];
    int i,n,before;

    assert(load(&m,"Work:NovaDock/Plugins/Weather",weather));
    assert(!strcmp(m.id,"WEATHER") && !strcmp(m.name,"Weather"));
    assert(m.graph==1 && m.abi==2 && m.multiple_instances &&
           m.option_count==3);
    assert(nd_manifest_bundled_id("RUNNINGAPPS"));
    assert(!strcmp(m.options[1].id,"WEATHER") && !strcmp(m.options[1].choices,"C,F"));

    { static const char *l[]={"NOVADOCK_ID=EXTENDED","NOVADOCK_ABI=5",NULL};
      assert(load(&m,"Plugins/Extended",l) && m.abi==5); }
    /* Name defaults to the executable's file name. */
    { static const char *l[]={"NOVADOCK_ID=X1",NULL};
      assert(load(&m,"Plugins/Ticker",l) && !strcmp(m.name,"Ticker") && m.abi==1); }
    /* Rejections carry a reason. */
    { static const char *l[]={"NOVADOCK_NAME=NoId",NULL};
      assert(!load(&m,"P",l) && strstr(m.error,"ID")); }
    { static const char *l[]={"NOVADOCK_ID=bad id",NULL};
      assert(!load(&m,"P",l)); }
    { static const char *l[]={"NOVADOCK_ID=A","NOVADOCK_ABI=9",NULL};
      assert(!load(&m,"P",l) && strstr(m.error,"newer")); }
    { static const char *l[]={"NOVADOCK_ID=A","NOVADOCK_OPTION=K|L|UINT|1,5|9",NULL};
      assert(!load(&m,"P",l) && strstr(m.error,"default")); }
    { static const char *l[]={"NOVADOCK_ID=A","NOVADOCK_OPTION=K|L|COLOR||RED",NULL};
      assert(!load(&m,"P",l)); }
    { static const char *l[]={"NOVADOCK_ID=A","NOVADOCK_OPTION=K|L|BOOL|",NULL};
      assert(!load(&m,"P",l)); }
    { static const char *l[]={"NOVADOCK_ID=A","NOVADOCK_OPTION=K|L|BOOL||YES",
                              "NOVADOCK_OPTION=K|L2|BOOL||NO",NULL};
      assert(!load(&m,"P",l) && strstr(m.error,"duplicate")); }
    { static const char *l[]={"NOVADOCK_ID=A","NOVADOCK_VIEW=PIE",NULL};
      assert(!load(&m,"P",l)); }

    /* Merge: new plugins arrive disabled with their schema. */
    nd_default_config(&cfg);
    before=cfg.count;
    assert(load(&m,"Plugins/Weather",weather));
    assert(nd_manifest_merge(&cfg,&m,"Plugins/Weather")==ND_MANIFEST_ADDED);
    assert(cfg.count==before+1);
    assert(cfg.items[before].kind==ND_ITEM_PLUGIN && !cfg.items[before].enabled);
    assert(cfg.items[before].multiple_instances);
    assert(cfg.items[before].graph && !strcmp(cfg.items[before].command,"Plugins/Weather"));
    n=nd_option_collect(&cfg,"WEATHER",defs,16);
    assert(n==3);
    assert(nd_manifest_merge(&cfg,&m,"Plugins/Weather")==ND_MANIFEST_UNCHANGED);
    assert(cfg.count==before+1);
    /* An upgrade refreshes the schema, keeps stored values and the tile. */
    assert(nd_option_set(&cfg,"WEATHER","CITY","Lund"));
    cfg.items[before].enabled=1;
    assert(load(&m,"Plugins/Weather",weather2));
    assert(nd_manifest_merge(&cfg,&m,"Plugins/Weather")==ND_MANIFEST_UPDATED);
    n=nd_option_collect(&cfg,"WEATHER",defs,16);
    assert(n==2 && !strcmp(nd_option_get(&cfg,"WEATHER","CITY"),"Lund"));
    assert(cfg.items[before].enabled);
    /* The result survives a save/load round trip. */
    assert(nd_write_config(&cfg,"tests/nd_manifest_test.cfg"));
    {
        struct NDConfig back;
        assert(nd_load_config(&back,"tests/nd_manifest_test.cfg")==0);
        assert(nd_option_collect(&back,"WEATHER",defs,16)==2);
        for (i=0;i<back.count;++i)
            if (!strcmp(back.items[i].id,"WEATHER")) break;
        assert(i<back.count && back.items[i].enabled);
    }
    remove("tests/nd_manifest_test.cfg");
    remove("tests/nd_manifest_test.cfg.bak");
    /* A manifest cannot take over a bundled plugin. */
    { static const char *l[]={"NOVADOCK_ID=CLOCK","NOVADOCK_OPTION=X|X|BOOL||NO",NULL};
      int count=cfg.count,defs_before=cfg.def_count;
      assert(load(&m,"Plugins/Evil",l));
      assert(nd_manifest_merge(&cfg,&m,"Plugins/Evil")==ND_MANIFEST_UNCHANGED);
      assert(cfg.count==count && cfg.def_count==defs_before); }
    /* Bad command paths and a full dock are rejected cleanly. */
    { static const char *l[]={"NOVADOCK_ID=PIPE",NULL};
      assert(load(&m,"P",l));
      assert(nd_manifest_merge(&cfg,&m,"Plugins/a|b")==ND_MANIFEST_REJECTED); }
    while (cfg.count<ND_MAX_ITEMS) assert(nd_add_spacer(&cfg,"S"));
    { static const char *l[]={"NOVADOCK_ID=LATE","NOVADOCK_OPTION=K|L|BOOL||NO",NULL};
      int defs_before=cfg.def_count;
      assert(load(&m,"P",l));
      assert(nd_manifest_merge(&cfg,&m,"Plugins/Late")==ND_MANIFEST_REJECTED);
      assert(cfg.def_count==defs_before); }
    puts("Self-describing plugin manifests: parse, validate, merge, upgrade passed.");
    return 0;
}
