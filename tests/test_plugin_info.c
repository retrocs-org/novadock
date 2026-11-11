/* Plugin Manager metadata: every bundled plugin has built-in information,
 * and capability summaries read naturally and never overflow. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_manifest.h"
#include "nd_config.h"

static void manifest(struct NDManifest *m,const char *const *lines)
{
    nd_manifest_init(m,"Hello");
    while (*lines) assert(nd_manifest_line(m,*lines++)!=0);
}
static void package_rules(void)
{
    static const char *complete[]={"NOVADOCK_ID=HELLO","NOVADOCK_NAME=Hello",
        "NOVADOCK_VERSION=1.2","NOVADOCK_AUTHOR=A. Author",
        "NOVADOCK_DESCRIPTION=Says hello.","NOVADOCK_LICENSE=MIT",
        "NOVADOCK_FEATURES=meter,click","NOVADOCK_PROGRAM=Hello",
        "NOVADOCK_OPTION=GREETING|Greeting|TEXT||Hello",NULL};
    static const char *no_author[]={"NOVADOCK_ID=HELLO","NOVADOCK_VERSION=1",
        "NOVADOCK_DESCRIPTION=x","NOVADOCK_LICENSE=MIT",
        "NOVADOCK_FEATURES=meter","NOVADOCK_PROGRAM=Hello",NULL};
    static const char *bundled[]={"NOVADOCK_ID=CLOCK","NOVADOCK_VERSION=9",
        "NOVADOCK_AUTHOR=x","NOVADOCK_DESCRIPTION=x","NOVADOCK_LICENSE=MIT",
        "NOVADOCK_FEATURES=meter","NOVADOCK_PROGRAM=Clock",NULL};
    static const char *path_program[]={"NOVADOCK_ID=HELLO","NOVADOCK_VERSION=1",
        "NOVADOCK_AUTHOR=x","NOVADOCK_DESCRIPTION=x","NOVADOCK_LICENSE=MIT",
        "NOVADOCK_FEATURES=meter","NOVADOCK_PROGRAM=C:Hello",NULL};
    struct NDManifest m;
    char why[80];
    manifest(&m,complete);
    assert(nd_package_check(&m,"Hello.ndplugin",why,sizeof(why)));
    assert(nd_package_check(&m,"HELLO.NDPLUGIN",why,sizeof(why)));
    manifest(&m,complete);
    assert(!nd_package_check(&m,"Other.ndplugin",why,sizeof(why)) && strstr(why,"NOVADOCK_PROGRAM"));
    manifest(&m,complete);
    assert(!nd_package_check(&m,"Hello.info",why,sizeof(why)));
    manifest(&m,no_author);
    assert(!nd_package_check(&m,"Hello.ndplugin",why,sizeof(why)) && strstr(why,"AUTHOR"));
    manifest(&m,bundled);
    assert(!nd_package_check(&m,"Clock.ndplugin",why,sizeof(why)) && strstr(why,"bundled"));
    manifest(&m,complete);strcpy(m.program,"cLoCk");
    assert(!nd_package_check(&m,"cLoCk.ndplugin",why,sizeof(why)) && strstr(why,"bundled"));
    manifest(&m,complete);strcpy(m.program,"Hello.bak");
    assert(!nd_package_check(&m,"Hello.bak.ndplugin",why,sizeof(why)));
    manifest(&m,path_program);
    assert(!nd_package_check(&m,"Hello.ndplugin",why,sizeof(why)) && strstr(why,"plain"));
}
/* The SDK's example manifest must always pass Import's rules. */
static void sdk_template(void)
{
    struct NDManifest m;
    char line[512],why[80];
    FILE *f=fopen("plugin_src/template/Hello.ndplugin","r");
    assert(f);
    nd_manifest_init(&m,"Hello");
    while (fgets(line,sizeof(line),f)) {
        line[strcspn(line,"\r\n")]='\0';
        assert(nd_manifest_line(&m,line)!=0);
    }
    fclose(f);
    assert(nd_package_check(&m,"Hello.ndplugin",why,sizeof(why)));
}
static struct NDConfig cfg;
static void removal(void)
{
    struct NDManifest m;
    static const char *lines[]={"NOVADOCK_ID=HELLO","NOVADOCK_MULTIPLE=YES",
        "NOVADOCK_OPTION=GREETING|Greeting|TEXT||Hello",NULL};
    int i,before,defs,opts;
    nd_default_config(&cfg);
    before=cfg.count;defs=cfg.def_count;opts=cfg.option_count;
    manifest(&m,lines);
    assert(nd_manifest_finish(&m));
    assert(nd_manifest_merge(&cfg,&m,"SYS:Utilities/NovaDock/Plugins/Hello")==ND_MANIFEST_ADDED);
    assert(nd_option_set(&cfg,"HELLO","GREETING","Hej"));
    /* Two tiles in the dock: removal is refused and changes nothing. */
    assert(nd_set_item_enabled(&cfg,cfg.count-1,1));
    assert(nd_add_plugin_instance(&cfg,cfg.count-1));
    assert(!nd_remove_plugin(&cfg,"HELLO") && cfg.count==before+2);
    /* With its tiles removed it goes completely, leaving everything else. */
    for (i=0;i<cfg.count;++i)
        if (!strcmp(cfg.items[i].id,"HELLO")) cfg.items[i].enabled=0;
    assert(nd_remove_plugin(&cfg,"HELLO"));
    assert(cfg.count==before && cfg.def_count==defs && cfg.option_count==opts);
    for (i=0;i<cfg.count;++i) assert(strcmp(cfg.items[i].id,"HELLO"));
    assert(!nd_remove_plugin(&cfg,""));
}
int main(void)
{
    static const char *bundled[]={"CLOCK","CPU","RAM","CHIPRAM","FASTRAM",
        "NET","MUSIC","EYES","DOPUS4","ARCHIVE","RUNNINGAPPS","BITSKIM",NULL};
    struct NDManifest m;
    char out[96];
    int i;
    package_rules();
    removal();
    sdk_template();
    for (i=0;bundled[i];++i) {
        assert(nd_manifest_bundled_id(bundled[i]));
        assert(nd_manifest_builtin(bundled[i],&m));
        assert(!strcmp(m.id,bundled[i]) && m.name[0] && m.version[0] &&
               m.author[0] && m.description[0]);
    }
    assert(nd_manifest_builtin("EYES",&m) && m.multiple_instances);
    assert(nd_manifest_builtin("CPU",&m) && !m.multiple_instances);
    /* COMMAND tiles are managed by Add Item's own type, not listed here. */
    assert(!nd_manifest_builtin("COMMAND",&m));
    assert(!nd_manifest_builtin("HELLO",&m));
    nd_manifest_describe(" meter , click,wobble",0,0,out,sizeof(out));
    assert(!strcmp(out,"Meter, click, wobble"));
    nd_manifest_describe("drop,media",0,1,out,sizeof(out));
    assert(!strcmp(out,"Drop target, media controls; settings"));
    nd_manifest_describe("",1,1,out,sizeof(out));
    assert(!strcmp(out,"Several tiles; settings"));
    nd_manifest_describe(NULL,0,0,out,sizeof(out));
    assert(!strcmp(out,"Tile only"));
    nd_manifest_describe("meter,graph,click,popup,drop,media",1,1,out,24);
    assert(strlen(out)<24);
    printf("Plugin Manager metadata, package rules and removal passed.\n");
    return 0;
}
