/* Regression tests for the 0.1 review fixes that have a portable core. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_option_runtime.h"

static int seen_format;
static int clock_option(void *ctx,const char *key,const char *value)
{
    (void)ctx;
    if (!strcmp(key,"FORMAT") && !strcmp(value,"ANALOG")) seen_format=1;
    return 1;
}
int main(void)
{
    struct NDConfig *cfg=(struct NDConfig *)malloc(sizeof(*cfg));
    struct NDConfig *back=(struct NDConfig *)malloc(sizeof(*back));
    const char *path="tests/nd_fixes_test.cfg";
    char target[230],icon[230],args[120];
    FILE *f;
    assert(cfg && back);

    /* 1. A launcher line far beyond 512 bytes must not hide plugin options. */
    nd_default_config(cfg);
    memset(target,'A',sizeof(target)-1);target[sizeof(target)-1]='\0';
    memcpy(target,"Work:",5);
    memset(icon,'B',sizeof(icon)-1);icon[sizeof(icon)-1]='\0';
    memcpy(icon,"Work:",5);
    memset(args,'C',sizeof(args)-1);args[sizeof(args)-1]='\0';
    assert(nd_add_launcher(cfg,ND_ITEM_LAUNCH,"Long",target,icon));
    assert(nd_set_launcher_action(cfg,cfg->count-1,ND_LAUNCH_CLI,args,
                                  "Work:",NULL,0));
    assert(nd_option_set(cfg,"CLOCK","FORMAT","ANALOG"));
    assert(nd_write_config(cfg,path));
    f=fopen(path,"r");assert(f);
    {
        char line[1400];int longest=0;
        while (fgets(line,sizeof(line),f))
            if ((int)strlen(line)>longest) longest=(int)strlen(line);
        assert(longest>520);
    }
    fclose(f);
    seen_format=0;
    assert(nd_plugin_options_read(path,"CLOCK",clock_option,NULL)==1);
    assert(seen_format);

    /* 2. Lines longer than even the runtime buffer are skipped, not fatal. */
    f=fopen(path,"a");assert(f);
    {
        int i;
        fputs("FUTURE_KEY=",f);
        for (i=0;i<1500;++i) fputc('x',f);
        fputs("\nOPTION.CLOCK.SECONDS=YES\n",f);
    }
    fclose(f);
    seen_format=0;
    assert(nd_plugin_options_read(path,"CLOCK",clock_option,NULL)==1);
    assert(seen_format);

    /* 3. Unknown keys survive a load/save round trip; case is tolerated. */
    nd_default_config(cfg);
    assert(nd_parse_line(cfg,"NEW_FEATURE=on"));
    assert(nd_parse_line(cfg,"Labels=hover"));
    assert(cfg->label_mode==ND_LABEL_HOVER);
    assert(nd_write_config(cfg,path));
    assert(nd_load_config(back,path)==0);
    assert(back->unknown_count==1 && !strcmp(back->unknown[0],"NEW_FEATURE=on"));
    assert(back->label_mode==ND_LABEL_HOVER);

    /* 4. Overflowing the preserved-line table drops extras, never fails. */
    nd_default_config(cfg);
    {
        char line[40];int i;
        for (i=0;i<ND_MAX_UNKNOWN+3;++i) {
            sprintf(line,"K%d=v",i);
            assert(nd_parse_line(cfg,line)==1);
        }
        assert(cfg->unknown_count==ND_MAX_UNKNOWN && cfg->unknown_dropped==3);
    }

    remove(path);
    {
        char backup[300];
        sprintf(backup,"%s.bak",path);remove(backup);
    }
    free(cfg);free(back);
    puts("Review regressions (option reader, forward compatibility) passed.");
    return 0;
}
