#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_command.h"
#include "nd_config.h"
#include "nd_options.h"

int main(void)
{
    struct NDCommandConfig cfg;
    struct NDConfig dock;
    struct NDOptionDef defs[16];
    char out[64],done[64],script[300],value[ND_CMD_TEXT_LEN];
    unsigned int meter;
    int n;
    nd_cmd_defaults(&cfg);
    assert(!strcmp(cfg.command,"Version") && cfg.interval==60 && cfg.line==1);
    assert(nd_cmd_apply_option(&cfg,"COMMAND","Avail TOTAL"));
    assert(!nd_cmd_apply_option(&cfg,"COMMAND",""));
    assert(!nd_cmd_apply_option(&cfg,"COMMAND","Echo\nDelete"));
    assert(nd_cmd_apply_option(&cfg,"INTERVAL","5") && cfg.interval==5);
    assert(!nd_cmd_apply_option(&cfg,"INTERVAL","0"));
    assert(!nd_cmd_apply_option(&cfg,"LINE","21"));
    assert(nd_cmd_apply_option(&cfg,"METER","PERCENT") && cfg.meter==ND_CMD_METER_PERCENT);
    assert(!nd_cmd_apply_option(&cfg,"METER","BAR"));
    assert(nd_cmd_apply_option(&cfg,"CLICK","COMMAND") && cfg.click==ND_CMD_CLICK_COMMAND);
    assert(nd_cmd_apply_option(&cfg,"FUTURE","x"));
    assert(nd_cmd_file_names("CMD1",out,done,sizeof(out)));
    assert(!strcmp(out,"T:NovaDockCmd-CMD1.out") && !strcmp(done,"T:NovaDockCmd-CMD1.done"));
    assert(!nd_cmd_file_names("bad id",out,done,sizeof(out)));
    assert(nd_cmd_build_script("Avail TOTAL",done,script,sizeof(script)));
    assert(!strcmp(script,"FailAt 2147483647\nStack 16384\nAvail TOTAL\n"
                          "Echo >\"T:NovaDockCmd-CMD1.done\" \"$RC\""));
    assert(!nd_cmd_build_script("A\nB",done,script,sizeof(script)));
    /* Output extraction. */
    assert(nd_cmd_extract("\n\nKickstart 47.115, Workbench 47.3\n",1,0,
                          value,sizeof(value),&meter));
    assert(!strcmp(value,"Kickstart 47.115, Workbench 47.") && meter==65535U);
    assert(nd_cmd_extract("Type   Available  In-Use\n  chip  1,234  56\n",2,0,
                          value,sizeof(value),&meter) && !strcmp(value,"chip 1,234 56"));
    assert(!nd_cmd_extract("one\n\ntwo\n",3,0,value,sizeof(value),&meter) && !value[0]);
    assert(nd_cmd_extract("\x9b" "1mBold\x9b" "0m text\tend",1,0,value,sizeof(value),&meter) &&
           !strcmp(value,"Bold text end"));
    assert(nd_cmd_extract("\x1b[33mDisk 87% full\x1b[0m",1,ND_CMD_METER_PERCENT,
                          value,sizeof(value),&meter) &&
           !strcmp(value,"Disk 87% full") && meter==87);
    assert(nd_cmd_extract("Load 250",1,ND_CMD_METER_PERCENT,value,sizeof(value),&meter) &&
           meter==100);
    assert(nd_cmd_extract("no digits",1,ND_CMD_METER_PERCENT,value,sizeof(value),&meter) &&
           meter==65535U);
    assert(nd_cmd_parse_rc("0\n")==0 && nd_cmd_parse_rc("20")==20 &&
           nd_cmd_parse_rc("\"5\"")==5 && nd_cmd_parse_rc("")==-1);
    /* Every Command tile shares one schema but keeps its own values. */
    nd_default_config(&dock);
    assert(nd_parse_line(&dock,"PLUGIN=RAM free|Plugins/Command|CMD1|METER|ON"));
    assert(nd_parse_line(&dock,"PLUGIN=Kick|SYS:Utilities/NovaDock/Plugins/Command|CMD2|METER|ON"));
    assert(!strcmp(nd_option_schema_id(&dock,"CMD1"),"COMMAND"));
    assert(!strcmp(nd_option_schema_id(&dock,"CLOCK"),"CLOCK"));
    assert(nd_parse_line(&dock,"OPTION.CMD1.COMMAND=Avail TOTAL"));
    assert(nd_parse_line(&dock,"OPTION.CMD1.METER=PERCENT"));
    assert(!nd_parse_line(&dock,"OPTION.CMD1.METER=BAR"));
    assert(!strcmp(nd_option_get(&dock,"CMD1","COMMAND"),"Avail TOTAL"));
    assert(!strcmp(nd_option_get(&dock,"CMD2","COMMAND"),"Version"));
    assert(!strcmp(nd_option_get(&dock,"CMD2","INTERVAL"),"60"));
    n=nd_option_collect(&dock,"CMD1",defs,16);
    assert(n==8 && !strcmp(defs[0].id,"CMD1") && !strcmp(defs[0].key,"COMMAND"));
    assert(nd_option_collect(&dock,"CLOCK",defs,16)==4);
    {
        struct NDConfig next=dock;
        assert(!nd_options_changed(&dock,&next,"CMD1"));
        assert(nd_option_set(&next,"CMD1","INTERVAL","10"));
        assert(nd_options_changed(&dock,&next,"CMD1"));
        assert(!nd_options_changed(&dock,&next,"CMD2"));
    }
    assert(nd_write_config(&dock,"tests/nd_command_test.cfg"));
    {
        struct NDConfig back;
        assert(nd_load_config(&back,"tests/nd_command_test.cfg")==0);
        assert(!strcmp(nd_option_get(&back,"CMD1","METER"),"PERCENT"));
    }
    remove("tests/nd_command_test.cfg");
    remove("tests/nd_command_test.cfg.bak");
    puts("Command-output tile settings, script and output parsing passed.");
    return 0;
}
