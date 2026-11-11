#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_layout_profiles.h"

int main(void)
{
    struct NDConfig current,saved;
    char path[100];
    int option_count;
    nd_default_config(&current);
    saved=current;
    assert(nd_layout_name_valid("Work"));
    assert(nd_layout_name_valid("Games and Tools_2"));
    assert(!nd_layout_name_valid(""));
    assert(!nd_layout_name_valid(" leading"));
    assert(!nd_layout_name_valid("trailing "));
    assert(!nd_layout_name_valid("bad/name"));
    assert(!nd_layout_name_valid("bad:device"));
    assert(nd_layout_profile_path(path,sizeof(path),
                                  "ENVARC:NovaDock/Layouts","Work"));
    assert(strcmp(path,"ENVARC:NovaDock/Layouts/Work.cfg")==0);
    assert(!nd_layout_profile_path(path,8,"Layouts","Work"));

    current.theme=ND_THEME_OCEAN;
    current.side=ND_SIDE_LEFT;
    option_count=current.option_count;
    saved.theme=ND_THEME_COPPER;
    saved.side=ND_SIDE_TOP;
    saved.items[0].enabled=0;
    saved.items[1].graph=1;
    { struct NDItem swap=saved.items[0];
      saved.items[0]=saved.items[1];saved.items[1]=swap; }
    assert(nd_layout_apply(&current,&saved));
    assert(current.theme==ND_THEME_COPPER && current.side==ND_SIDE_TOP);
    assert(current.option_count==option_count);
    assert(strcmp(current.items[0].label,saved.items[0].label)==0);
    assert(current.items[0].graph==saved.items[0].graph);
    assert(current.items[1].enabled==saved.items[1].enabled);
    saved.count=0;
    assert(!nd_layout_apply(&current,&saved));
    puts("Named layout validation and application tests passed.");
    /* An imported theme survives a layout profile (by ID, not index). */
    nd_default_config(&saved);nd_default_config(&current);
    assert(nd_config_set_theme(&saved,"MyTheme"));
    assert(nd_config_set_theme(&current,"Ocean"));
    assert(nd_layout_apply(&current,&saved));
    assert(!strcmp(nd_config_theme_id(&current),"MyTheme"));
    assert(nd_config_set_theme(&saved,"Nord"));
    assert(nd_layout_apply(&current,&saved));
    assert(!strcmp(nd_config_theme_id(&current),"Nord"));
    return 0;
}
