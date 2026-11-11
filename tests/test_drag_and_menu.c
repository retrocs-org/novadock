/* Host-side regression tests for the config portion of popup and drag/drop.
 * Workbench AppWindow events need native 68k/Workbench validation separately. */
#include "nd_config.h"
#include "populated_dock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct NDConfig a, b, c, running;
    char name[ND_LABEL_LEN];
    char moved[ND_LABEL_LEN];
    const char *filename = "tests/.drag_menu_test.cfg";
    nd_default_config(&a);
    a.count = 0;
    assert(nd_parse_line(&a,"LEFT_CLICK=MENU"));
    assert(nd_parse_line(&a,"LAUNCH=Shell|NewShell|Icons/Workbench.iff"));
    assert(nd_parse_line(&a,"PLUGIN=Hidden|Plugins/Clock|HIDDEN|METER|OFF"));
    assert(nd_parse_line(&a,"DRAWER=Games|Games:|Icons/Games.iff"));
    assert(nd_parse_line(&a,"LAUNCH=Tools|SYS:Utilities|Icons/Tools.iff"));
    assert(a.count == 4 && a.left_click == ND_CLICK_MENU);
    assert(nd_visible_to_full(&a,0) == 0);
    assert(nd_visible_to_full(&a,1) == 2);
    assert(nd_visible_to_full(&a,2) == 3);
    assert(nd_visible_to_full(&a,3) == -1);
    assert(nd_move_visible(&a,0,2));
    assert(!strcmp(a.items[3].label,"Shell"));
    assert(!strcmp(a.items[0].id,"HIDDEN") && !a.items[0].enabled);
    assert(!strcmp(a.items[1].label,"Games"));
    assert(!strcmp(a.items[2].label,"Tools"));
    assert(nd_insert_launcher_visible(&a,1,ND_ITEM_LAUNCH,
                                      "Internet","SYS:Tools/Browser",
                                      "Icons/Internet.iff"));
    assert(a.count == 5);
    assert(!strcmp(a.items[2].label,"Internet"));
    assert(nd_insert_spacer_visible(&a,2,"Tools"));
    assert(a.count == 6 && a.items[3].kind == ND_ITEM_SPACER);
    assert(nd_move_visible(&a,2,0));
    assert(a.items[1].kind == ND_ITEM_SPACER);
    assert(nd_remove_visible_item(&a,0));
    assert(a.count == 6 && !a.items[1].enabled); /* definition retained */
    assert(nd_remove_visible_launcher(&a,2)); /* remove Tools, not hidden plugin */
    assert(a.count == 5 && !strcmp(a.items[0].id,"HIDDEN"));
    assert(!nd_remove_visible_launcher(&a,100));
    assert(!nd_move_visible(&a,-1,1));
    assert(!nd_insert_launcher_visible(&a,50,ND_ITEM_LAUNCH,"X","Y",""));
    assert(nd_remove_visible_item(&a,0)); /* drawer becomes an available definition */
    assert(a.count == 5);
    assert(nd_remove_visible_item(&a,0)); /* launcher definition is also retained */
    assert(a.count == 5);
    assert(nd_remove_visible_item(&a,0)); /* retain the Shell definition */
    /* Dropping that same removed application restores its existing catalogue
     * entry rather than adding a duplicate or exhausting ND_MAX_ITEMS. */
    assert(nd_insert_launcher_visible(&a,0,ND_ITEM_LAUNCH,
                                      "Shell duplicate","NewShell",""));
    assert(a.count==5 && a.items[4].enabled);
    assert(!strcmp(a.items[4].label,"Shell"));
    a.items[0].enabled=1;
    assert(nd_remove_visible_item(&a,0)); /* plugins are retained but disabled */
    assert(a.count == 5 && !a.items[0].enabled);
    assert(nd_write_config(&a, filename));
    assert(nd_load_config(&b, filename) == 0);
    assert(b.count == 14 && b.left_click == ND_CLICK_MENU);
    assert(!strcmp(b.items[10].id,"EYES") &&
           b.items[10].multiple_instances && !b.items[10].enabled);
    assert(!strcmp(b.items[11].id,"DOPUS4") && !b.items[11].enabled);
    assert(!strcmp(b.items[12].id,"ARCHIVE") && !b.items[12].enabled);
    assert(!strcmp(b.items[13].id,"RUNNINGAPPS") && !b.items[13].enabled);
    assert(!strcmp(b.items[0].id, "HIDDEN") && !b.items[0].enabled);
    running = b;
    assert(nd_compact_enabled(&running) == 1);
    assert(!strcmp(running.items[0].label,"Shell"));
    remove(filename);
    nd_label_from_path("Work:Apps/EaglePlayer",name,sizeof(name));
    assert(!strcmp(name,"EaglePlayer"));
    nd_label_from_path("Work:Drawer.info",name,sizeof(name));
    assert(!strcmp(name,"Drawer"));
    nd_label_from_path("Games:",name,sizeof(name));
    assert(!strcmp(name,"Games"));
    nd_label_from_path("/some/very-very-long-directory-name-that-exceeds-a-label",
                       name,sizeof(name));
    assert(strlen(name) < ND_LABEL_LEN);
    assert(nd_parse_line(&b,"LEFT_CLICK=LAUNCH"));
    assert(b.left_click == ND_CLICK_LAUNCH);
    assert(!nd_parse_line(&b,"LEFT_CLICK=INVALID"));
    populated_dock(&c);
    assert(c.items[2].kind == ND_ITEM_PLUGIN);
    assert(nd_move_visible(&c,2,0));
    assert(c.items[0].kind == ND_ITEM_PLUGIN);
    assert(!strcmp(c.items[0].id,"CPU"));
    /* The low-level final-position semantics used by dock before/after drops:
     * moving forward to the target index lands after it; moving backward to
     * the target index lands before it. Adjacent destinations can therefore
     * represent both insertion edges without replacing either item. */
    populated_dock(&c);
    strcpy(moved,c.items[0].label);
    assert(nd_move_visible(&c,0,2));
    assert(!strcmp(c.items[2].label,moved));
    strcpy(moved,c.items[3].label);
    assert(nd_move_visible(&c,3,1));
    assert(!strcmp(c.items[1].label,moved));
    /* A full retained catalogue may recycle a disabled application when a
     * genuinely new Workbench object is dropped. Plugin definitions remain. */
    populated_dock(&c);
    c.items[0].enabled=0;
    while (c.count<ND_MAX_ITEMS) assert(nd_add_spacer(&c,"Saved gap"));
    assert(nd_insert_launcher_visible(&c,0,ND_ITEM_LAUNCH,
                                      "Scout","SYS:Tools/Scout",""));
    assert(c.count==ND_MAX_ITEMS && c.items[0].enabled);
    assert(!strcmp(c.items[0].label,"Scout"));
    assert(!strcmp(c.items[0].command,"SYS:Tools/Scout"));

    /* A row move changes membership as well as order. It must also support
     * moving an item into a previously empty row without replacing one. */
    populated_dock(&c);
    assert(nd_compact_enabled(&c)==10);
    c.rows=2;c.row_layout=1;
    c.row_counts[0]=5;c.row_counts[1]=5;c.row_counts[2]=0;
    strcpy(moved,c.items[4].label);
    assert(nd_move_visible_to_row(&c,4,9,1));
    assert(c.row_counts[0]==4 && c.row_counts[1]==6);
    assert(!strcmp(c.items[9].label,moved));
    c.rows=3;
    c.row_counts[0]=4;c.row_counts[1]=0;c.row_counts[2]=6;
    strcpy(moved,c.items[3].label);
    assert(nd_move_visible_to_row(&c,3,3,1));
    assert(c.row_counts[0]==3 && c.row_counts[1]==1 &&
           c.row_counts[2]==6);
    assert(!strcmp(c.items[3].label,moved));
    assert(nd_write_config(&c,filename));
    assert(nd_load_config(&b,filename)==0);
    assert(b.row_layout && b.rows==3);
    assert(b.row_counts[0]==3 && b.row_counts[1]==1 &&
           b.row_counts[2]==6);
    remove(filename);
    /* A final Trash tile remains an ordinary reorder target, including
     * hidden plugin definitions between Clock and Trash. Save/reload must
     * retain Clock after Trash rather than restoring its original slot. */
    populated_dock(&c);
    c.count=0;c.rows=1;c.row_layout=1;
    assert(nd_parse_line(&c,"LAUNCH=Shell|NewShell|Icons/Workbench.iff|ON"));
    assert(nd_parse_line(&c,"PLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON"));
    assert(nd_parse_line(&c,"PLUGIN=Hidden|Plugins/Network|NET|METER|OFF"));
    assert(nd_parse_line(&c,"PLUGIN=Trash|Plugins/Trash|TRASH|METER|ON"));
    c.row_counts[0]=3;c.row_counts[1]=c.row_counts[2]=0;
    assert(nd_move_visible_to_row(&c,1,2,0));
    assert(nd_write_config(&c,filename));
    assert(nd_load_config(&b,filename)==0);
    assert(nd_compact_enabled(&b)==3);
    assert(!strcmp(b.items[1].id,"TRASH"));
    assert(!strcmp(b.items[2].id,"CLOCK"));
    remove(filename);
    printf("Launcher context menu and Workbench-drop config tests passed.\n");
    return 0;
}
