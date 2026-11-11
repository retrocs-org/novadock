/* Script tile RESULT parsing and command lines; HOOK and HOVER settings. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_script.h"
#include "nd_config.h"
#include "nd_interaction.h"

static void test_script(void)
{
    struct NDScriptResult r;
    char cmd[64];
    assert(!nd_script_parse("CPU|value|%",&r));
    assert(!nd_script_parse("CPU|value|50%%",&r));
    assert(!nd_script_parse("CPU|value|100000000000000000000",&r));
    assert(!nd_script_parse("CPU|value|100     garbage",&r));
    assert(nd_script_parse("CPU|value| 50 % ",&r) && r.meter==50);
    assert(nd_script_parse("42 mails",&r) && !strcmp(r.value,"42 mails") &&
           !r.has_caption && r.meter==ND_SCRIPT_NO_METER && !r.has_badge);
    assert(nd_script_parse("MAIL|3 new",&r) && r.has_caption &&
           !strcmp(r.caption,"MAIL") && !strcmp(r.value,"3 new"));
    assert(nd_script_parse("|3 new|75",&r) && !r.has_caption && r.meter==75);
    assert(nd_script_parse("CPU|12%|12 %|",&r) && r.meter==12 && r.has_badge &&
           r.badge[0]=='\0');
    assert(nd_script_parse("A|B|-|9",&r) && !r.has_meter && !strcmp(r.badge,"9"));
    assert(nd_script_parse("A|B|  |123456789",&r) && !r.has_meter &&
           !strcmp(r.badge,"1234567"));
    assert(!nd_script_parse("A|B|101",&r));
    assert(!nd_script_parse("A|B|x",&r));
    assert(!nd_script_parse("A|B|1|2|3",&r));
    assert(nd_script_parse("tab\there|line\nbreak",&r) &&
           !strcmp(r.caption,"tab here") && !strcmp(r.value,"line break"));
    assert(nd_script_parse("0123456789012345678901234567890123456789",&r) &&
           strlen(r.value)==31);
    assert(nd_script_parse("",&r) && r.value[0]=='\0');
    assert(nd_script_command(cmd,sizeof(cmd),"Rexx:Mail.rexx","UPDATE","SCRIPT1") &&
           !strcmp(cmd,"Rexx:Mail.rexx UPDATE SCRIPT1"));
    assert(nd_script_command(cmd,sizeof(cmd),"Work:My Tiles/t.rexx","CLICK","S2") &&
           !strcmp(cmd,"\"Work:My Tiles/t.rexx\" CLICK S2"));
    assert(!nd_script_command(cmd,sizeof(cmd),"a\"b","UPDATE","X"));
    assert(!nd_script_command(cmd,12,"Rexx:Mail.rexx","UPDATE","SCRIPT1"));
}
static void test_hooks(void)
{
    struct NDConfig *cfg=(struct NDConfig *)calloc(1,sizeof(*cfg));
    struct NDConfig *back=(struct NDConfig *)calloc(1,sizeof(*back));
    char cmd[128];
    const char *path="tests/nd_hooks_test.cfg";
    assert(cfg && back);
    nd_default_config(cfg);
    assert(nd_parse_line(cfg,"HOOK=disk_in|Rexx:Disk.rexx"));
    assert(nd_parse_line(cfg,"HOOK=NET_ONLINE|Run >NIL: Sync"));
    assert(nd_parse_line(cfg,"HOVER=both")); /* old configs remain loadable */
    assert(!strcmp(cfg->hooks[0].event,"DISK_IN"));
    assert(!nd_parse_line(cfg,"HOOK=BOOM|x"));
    assert(!nd_parse_line(cfg,"HOOK=QUIT|"));
    assert(!nd_parse_line(cfg,"HOOK=|x"));
    assert(!nd_parse_line(cfg,"HOOK=QUIT"));
    assert(!nd_parse_line(cfg,"HOVER=SPIN"));
    assert(cfg->hook_count==2);
    assert(nd_write_config(cfg,path));
    nd_default_config(back);
    assert(nd_load_config(back,path)==0);
    assert(back->hook_count==2 &&
           !strcmp(back->hooks[1].command,"Run >NIL: Sync"));
    assert(nd_config_runtime_equal(cfg,back));
    assert(nd_parse_line(back,"HOVER=ZOOM") && nd_config_runtime_equal(cfg,back));
    {
        FILE *f=fopen(path,"r");char line[512];assert(f);
        while(fgets(line,sizeof(line),f))assert(strncmp(line,"HOVER=",6));
        fclose(f);
    }
    remove(path);
    {
        char bak[64];
        sprintf(bak,"%s.bak",path);remove(bak);
        sprintf(bak,"%s.old",path);remove(bak);
        sprintf(bak,"%s.good",path);remove(bak);
    }
    assert(nd_hook_command(cmd,sizeof(cmd),"Rexx:Disk.rexx","DISK_IN","Work Disk") &&
           !strcmp(cmd,"Rexx:Disk.rexx DISK_IN \"Work Disk\""));
    assert(nd_hook_command(cmd,sizeof(cmd),"Say","QUIT","") &&
           !strcmp(cmd,"Say QUIT \"\""));
    assert(!nd_hook_command(cmd,sizeof(cmd),"Say","QUIT","a\"b"));
    assert(!nd_hook_command(cmd,12,"Rexx:Disk.rexx","DISK_IN","x"));
    assert(nd_hook_event_valid("SAVE_FAILED") && !nd_hook_event_valid("save_failed"));
    free(cfg);free(back);
}
static void test_script_heartbeat(void)
{
    unsigned int intervals[3]={1500U,3000U,4320000U};
    unsigned int i,since,elapsed,heartbeat,runs;
    for(i=0;i<3;++i){
        since=heartbeat=runs=0;
        while(runs<2){
            elapsed=nd_script_wait_ticks(0,since,intervals[i],0);
            assert(elapsed<=250U);
            since+=elapsed;heartbeat+=elapsed;
            assert(heartbeat<=500U); /* below the 1000-tick stale deadline */
            if(heartbeat>=500U)heartbeat=0;
            if(since>=intervals[i]){assert(since==intervals[i]);since=0;++runs;}
        }
    }
}
int main(void)
{
    assert(nd_script_wait_ticks(0,0,1500U,0)==250U);
    assert(nd_script_wait_ticks(0,0,4320000U,0)==250U);
    assert(nd_script_wait_ticks(0,1490U,1500U,0)==10U);
    assert(nd_script_wait_ticks(1,0,1500U,0)==25U);
    assert(nd_script_wait_ticks(0,0,1500U,1)==1U);
    test_script_heartbeat();
    test_script();
    test_hooks();
    printf("Script tile results/commands, hooks and retired-hover compatibility passed.\n");
    return 0;
}
