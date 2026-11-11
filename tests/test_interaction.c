#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_interaction.h"
int main(void)
{
    struct NDRexxCommand c;
    char buffer[500];
    assert(nd_parse_rexx(" ping ",&c) && c.action==ND_RX_PING);
    assert(nd_parse_rexx("VERSION",&c) && c.action==ND_RX_VERSION);
    assert(nd_parse_rexx("LIST",&c) && c.action==ND_RX_LIST);
    assert(nd_parse_rexx("SUSPEND",&c) && c.action==ND_RX_SUSPEND);
    assert(nd_parse_rexx("resume",&c) && c.action==ND_RX_RESUME);
    assert(!nd_parse_rexx("SUSPEND now",&c));
    /* LIST STEM: validated, upper-cased, always ending in '.' */
    assert(nd_parse_rexx("LIST",&c) && c.text[0]=='\0');
    assert(nd_parse_rexx("LIST STEM tiles",&c) && c.action==ND_RX_LIST &&
           !strcmp(c.text,"TILES."));
    assert(nd_parse_rexx("list stem Dock.",&c) && !strcmp(c.text,"DOCK."));
    assert(!nd_parse_rexx("LIST STEM",&c));
    assert(!nd_parse_rexx("LIST STEM 1abc",&c));
    assert(!nd_parse_rexx("LIST STEM a=b",&c));
    assert(!nd_parse_rexx("LIST STEM a b",&c));
    assert(!nd_parse_rexx("LIST FOO x",&c));
    assert(!nd_parse_rexx("LIST STEM abcdefghijklmnopqrstuvwxyzabcdef",&c));
    assert(nd_parse_rexx("OPEN 3",&c) && c.action==ND_RX_OPEN && c.first==3);
    assert(nd_parse_rexx("move 2 7",&c) && c.action==ND_RX_MOVE && c.first==2 && c.second==7);
    assert(nd_parse_rexx("  RELOAD  ",&c) && c.action==ND_RX_RELOAD);
    assert(nd_parse_rexx("PREFS",&c) && c.action==ND_RX_PREFS);
    assert(nd_parse_rexx("STATUS",&c) && c.action==ND_RX_STATUS);
    assert(nd_parse_rexx("QUIT",&c) && c.action==ND_RX_QUIT);
    assert(!nd_parse_rexx("OPEN 0",&c));
    assert(!nd_parse_rexx("OPEN -4",&c));
    assert(!nd_parse_rexx("OPEN 4 ;delete",&c));
    assert(!nd_parse_rexx("MOVE 2 0",&c));
    assert(!nd_parse_rexx("MOVE 2 3 4",&c));
    assert(!nd_parse_rexx("QUIT NOW",&c));
    assert(!nd_parse_rexx("OPEN 1000",&c));
    assert(nd_build_openwith(buffer,sizeof(buffer),"SYS:Utilities/My Player","Work:Mods/My File.mod"));
    assert(!strcmp(buffer,"\"SYS:Utilities/My Player\" \"Work:Mods/My File.mod\""));
    assert(nd_build_openwith_action(buffer,sizeof(buffer),"SYS:Tools/Player",
                                    "QUIET","Work:Mods/My File.mod"));
    assert(!strcmp(buffer,"\"SYS:Tools/Player\" QUIET \"Work:Mods/My File.mod\""));
    assert(!nd_build_openwith_action(buffer,sizeof(buffer),"SYS:Tools/Player",
                                     "BAD\nARG","Work:Mods/Test.mod"));
    assert(!nd_build_openwith(buffer,sizeof(buffer),"SYS:Tools/Player","Work:a\" & B"));
    assert(!nd_build_openwith(buffer,sizeof(buffer),"SYS:Tools/Player","Work:a*Q"));
    assert(!nd_build_openwith(buffer,sizeof(buffer),"@quit","Work:Mods/Test.mod"));
    assert(!nd_build_openwith(buffer,10,"SYS:Tools/Player","Work:Mods/Test.mod"));
    assert(!nd_drop_opens_item(-1,100));
    assert(!nd_drop_opens_item(19,100));
    assert(nd_drop_opens_item(20,100));
    assert(nd_drop_opens_item(79,100));
    assert(!nd_drop_opens_item(80,100));
    assert(!nd_drop_after_item(-1,100));
    assert(!nd_drop_after_item(0,100));
    assert(!nd_drop_after_item(49,100));
    assert(nd_drop_after_item(50,100));
    assert(nd_drop_after_item(99,100));
    assert(nd_drop_after_item(150,100));
    assert(nd_drop_destination(0,2,0)==1); /* before while moving right */
    assert(nd_drop_destination(0,2,1)==2); /* after while moving right */
    assert(nd_drop_destination(3,1,0)==1); /* before while moving left */
    assert(nd_drop_destination(3,1,1)==2); /* after while moving left */
    assert(nd_drop_destination(1,1,0)==1);
    assert(nd_drop_destination(-1,1,0)==-1);
    assert(nd_right_button_down(2,0x69));       /* raw right-button down */
    assert(!nd_right_button_down(2,0xff));      /* pointer motion */
    assert(!nd_right_button_down(2,0xe9));      /* right-button up */
    assert(!nd_right_button_down(2,0x68));      /* left-button down */
    assert(!nd_right_button_down(4,0x69));      /* pointer-position event */
    assert(nd_launcher_matches_shell_window("NewShell","AmigaShell"));
    assert(nd_launcher_matches_shell_window("C:NewShell","AmigaShell - 2"));
    assert(nd_launcher_matches_shell_window("System:C/Shell","Shell"));
    assert(!nd_launcher_matches_shell_window("ShellMate","AmigaShell"));
    assert(!nd_launcher_matches_shell_window("NewShell","AmigaShellPlus"));
    assert(!nd_launcher_matches_shell_window("NewShell","Workbench"));
    assert(nd_text_matches_pattern("EaglePlayer*","EaglePlayer V2.06"));
    assert(nd_text_matches_pattern("*Shell*","AmigaShell - 2"));
    assert(nd_text_matches_pattern("Directory Opus ?.??","Directory Opus 4.16"));
    assert(nd_text_matches_pattern("workbench","Workbench"));
    assert(!nd_text_matches_pattern("EaglePlayer","EaglePlayer V2.06"));
    assert(!nd_text_matches_pattern("","Workbench"));
    /* Extended ARexx vocabulary. */
    assert(nd_parse_rexx("show",&c) && c.action==ND_RX_SHOW);
    assert(nd_parse_rexx("HIDE",&c) && c.action==ND_RX_HIDE);
    assert(nd_parse_rexx("Toggle",&c) && c.action==ND_RX_TOGGLE);
    assert(nd_parse_rexx("LOCK",&c) && c.action==ND_RX_LOCK);
    assert(nd_parse_rexx("unlock",&c) && c.action==ND_RX_UNLOCK);
    assert(nd_parse_rexx("CLICK 4",&c) && c.action==ND_RX_CLICK && c.first==4);
    assert(nd_parse_rexx("REMOVE 2",&c) && c.action==ND_RX_REMOVE && c.first==2);
    assert(!nd_parse_rexx("CLICK",&c));
    assert(nd_parse_rexx("OPEN Shell",&c) && c.action==ND_RX_OPEN &&
           c.first==0 && !strcmp(c.text,"Shell"));
    assert(nd_parse_rexx("OPEN \"DOpus 4\"",&c) && c.first==0 &&
           !strcmp(c.text,"DOpus 4"));
    assert(nd_parse_rexx("OPEN My Editor ",&c) && !strcmp(c.text,"My Editor"));
    assert(!nd_parse_rexx("OPEN \"unterminated",&c));
    assert(nd_parse_rexx("ADD \"Work:My Apps/Editor\"",&c) &&
           c.action==ND_RX_ADD && !strcmp(c.text,"Work:My Apps/Editor"));
    assert(nd_parse_rexx("ADD SYS:Utilities/MultiView",&c) &&
           !strcmp(c.text,"SYS:Utilities/MultiView"));
    assert(!nd_parse_rexx("ADD relative/path",&c));
    assert(nd_parse_rexx("SETVALUE MAIL \"MAIL\" \"3 new\" 30",&c) &&
           c.action==ND_RX_SETVALUE && !strcmp(c.id,"MAIL") &&
           !strcmp(c.text,"MAIL") && !strcmp(c.text2,"3 new") && c.meter==30);
    assert(nd_parse_rexx("setvalue UPS BAT 98%",&c) && c.meter==-1 &&
           !strcmp(c.text2,"98%"));
    assert(!nd_parse_rexx("SETVALUE bad!id A B",&c));
    /* SETBADGE id ["text"]: up to seven characters; no text clears. */
    assert(nd_parse_rexx("SETBADGE MAIL 3",&c) && c.action==ND_RX_SETBADGE &&
           !strcmp(c.id,"MAIL") && !strcmp(c.text,"3"));
    assert(nd_parse_rexx("setbadge MAIL \"NEW\"",&c) && !strcmp(c.text,"NEW"));
    assert(nd_parse_rexx("SETBADGE MAIL \"1234567\"",&c) && !strcmp(c.text,"1234567"));
    assert(nd_parse_rexx("SETBADGE MAIL",&c) && c.action==ND_RX_SETBADGE && !c.text[0]);
    assert(nd_parse_rexx("SETBADGE MAIL \"*\"",&c) && !strcmp(c.text,"*"));
    assert(!nd_parse_rexx("SETBADGE MAIL 12345678",&c));
    assert(!nd_parse_rexx("SETBADGE bad!id 1",&c));
    assert(!nd_parse_rexx("SETBADGE MAIL 1 2",&c));
    assert(!nd_parse_rexx("SETBADGE",&c));
    assert(!nd_parse_rexx("SETVALUE ID A B 101",&c));
    assert(!nd_parse_rexx("SETVALUE ID A",&c));
    assert(!nd_parse_rexx("PINGX",&c));
    /* AmigaDOS quoting escapes '*' and '"'. */
    assert(nd_quote_path(buffer,sizeof(buffer),"Work:My Apps/Tool") &&
           !strcmp(buffer,"\"Work:My Apps/Tool\""));
    assert(nd_quote_path(buffer,sizeof(buffer),"Work:Fun*Stuff/a\"b") &&
           !strcmp(buffer,"\"Work:Fun**Stuff/a*\"b\""));
    assert(!nd_quote_path(buffer,sizeof(buffer),"Work:bad\nname"));
    assert(!nd_quote_path(buffer,5,"Work:abc"));
    assert(nd_join_path(buffer,sizeof(buffer),"RAM:","x") && !strcmp(buffer,"RAM:x"));
    assert(nd_join_path(buffer,sizeof(buffer),"Work:A","x") && !strcmp(buffer,"Work:A/x"));
    assert(nd_build_copy_command(buffer,sizeof(buffer),"Work:Docs/readme.txt",0,"RAM:"));
    assert(!strcmp(buffer,"C:Copy \"Work:Docs/readme.txt\" TO \"RAM:\" CLONE QUIET"));
    assert(nd_build_copy_command(buffer,sizeof(buffer),"Work:Projects",1,"DH1:Backup"));
    assert(!strcmp(buffer,"C:Copy \"Work:Projects\" TO \"DH1:Backup/Projects\" ALL CLONE QUIET"));
    assert(!nd_build_copy_command(buffer,sizeof(buffer),"Work:",1,"RAM:"));
    assert(!nd_build_copy_command(buffer,sizeof(buffer),"RAM:x",1,"RAM:"));
    assert(nd_command_name_matches("Work:Apps/Editor","editor"));
    assert(nd_command_name_matches("Editor","Editor"));
    assert(!nd_command_name_matches("Work:Apps/EditorPlus","Editor"));
    assert(!nd_command_name_matches("","Editor"));
    puts("ARexx and Open With input tests passed.");
    return 0;
}
