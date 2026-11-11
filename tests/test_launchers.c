#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"

int main(void)
{
    struct NDConfig a,b,capacity;
    int i;
    const char *file="tests/nd_launcher_roundtrip.cfg";
    char long_name[ND_LABEL_LEN + 10];
    nd_default_config(&a);
    a.shape=ND_SHAPE_CHAMFER;
    assert(a.count==11);
    assert(nd_add_launcher(&a,ND_ITEM_DRAWER,"Games","Games:",
                           "Icons/Games.iff"));
    assert(a.items[11].kind==ND_ITEM_DRAWER);
    assert(nd_item_is_volume(&a.items[11]));
    assert(nd_item_supports_space_meter(&a.items[11]));
    assert(a.items[11].show_volume_meter==1);
    assert(nd_add_launcher(&a,ND_ITEM_DRAWER,"Utilities","SYS:Utilities",
                           "Icons/Drawer.iff"));
    assert(!nd_item_is_volume(&a.items[12]));
    assert(nd_item_supports_space_meter(&a.items[12]));
    assert(a.items[12].show_volume_meter==0);
    assert(nd_remove_launcher(&a,12));
    assert(nd_used_percent(0,100)==0);
    assert(nd_used_percent(50,100)==50);
    assert(nd_used_percent(3,4)==75);
    assert(nd_used_percent(200,100)==100);
    assert(nd_used_percent(2500000000UL,4000000000UL)==62);
    assert(nd_edit_launcher(&a,11,ND_ITEM_DRAWER,"Games","SYS:Games",
                            "Icons/Drawer.iff"));
    assert(nd_set_launcher_action(&a,11,ND_LAUNCH_WORKBENCH,"","Games:",
                                  "control alt g",1));
    assert(nd_set_launcher_match(&a,11,"GameTask","Games*","GAMES.1"));
    assert(!nd_item_is_volume(&a.items[11]));
    a.items[11].kind=ND_ITEM_LAUNCH;
    strcpy(a.items[11].command,"SYS:");
    assert(!nd_item_is_volume(&a.items[11]));
    assert(!nd_item_supports_space_meter(&a.items[11]));
    a.items[11].kind=ND_ITEM_DRAWER;
    strcpy(a.items[11].command,"SYS:Games");
    assert(nd_item_supports_space_meter(&a.items[11]));
    a.items[11].show_volume_meter=0;
    assert(nd_add_spacer(&a,"Work"));
    a.items[12].spacer_width=64;
    a.items[12].spacer_line=0;
    a.items[12].spacer_line_width=5;
    a.items[12].spacer_line_color=ND_SPACER_COLOR_SECONDARY;
    assert(a.count==13);
    nd_default_config(&capacity);
    while (capacity.count<ND_MAX_ITEMS)
        assert(nd_add_launcher(&capacity,ND_ITEM_LAUNCH,"Spare","SYS:Tools/More",NULL));
    assert(!nd_add_launcher(&capacity,ND_ITEM_LAUNCH,"Overflow","SYS:Tools/More",NULL));
    memset(long_name,'x',sizeof(long_name));long_name[sizeof(long_name)-1]='\0';
    assert(!nd_edit_launcher(&a,11,ND_ITEM_DRAWER,long_name,"SYS:",NULL));
    assert(!nd_edit_launcher(&a,11,ND_ITEM_DRAWER,"Bad","SYS:|danger",NULL));
    assert(!nd_edit_launcher(&a,0,ND_ITEM_PLUGIN,"Bad","SYS:",NULL));
    assert(!nd_remove_launcher(&a,2)); /* plugin cannot be removed */
    assert(nd_write_config(&a,file));
    assert(nd_load_config(&b,file)==0);
    assert(b.count==13);
    assert(b.shape==ND_SHAPE_CHAMFER);
    assert(b.items[12].spacer_width==64);
    assert(b.items[12].spacer_line==0);
    assert(b.items[12].spacer_line_width==5);
    assert(b.items[12].spacer_line_color==ND_SPACER_COLOR_SECONDARY);
    assert(b.items[11].launch_mode==ND_LAUNCH_WORKBENCH);
    assert(!strcmp(nd_item_text(&b,&b.items[11],ND_TEXT_WORKDIR),"Games:"));
    assert(!strcmp(nd_item_text(&b,&b.items[11],ND_TEXT_HOTKEY),"control alt g"));
    assert(b.items[11].tools_menu==1);
    assert(b.items[11].show_volume_meter==0);
    assert(!strcmp(nd_item_text(&b,&b.items[11],ND_TEXT_MATCH_TASK),"GameTask"));
    assert(!strcmp(nd_item_text(&b,&b.items[11],ND_TEXT_MATCH_WINDOW),"Games*"));
    assert(!strcmp(nd_item_text(&b,&b.items[11],ND_TEXT_MATCH_PORT),"GAMES.1"));
    for(i=0;i<13;++i) {
        assert(b.items[i].kind==a.items[i].kind);
        assert(!strcmp(b.items[i].command,a.items[i].command));
        assert(!strcmp(nd_item_text(&b,&b.items[i],ND_TEXT_ICON),nd_item_text(&a,&a.items[i],ND_TEXT_ICON)));
    }
    assert(nd_remove_launcher(&b,11));
    assert(b.count==12);
    assert(b.items[11].kind==ND_ITEM_SPACER);
    assert(nd_write_config(&b,file));
    assert(nd_load_config(&a,file)==0);
    assert(a.count==12);
    remove(file);
    puts("App/drawer launchers, icons, removal, and persistence passed.");
    return 0;
}
