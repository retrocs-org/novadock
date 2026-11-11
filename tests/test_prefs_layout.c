#include <assert.h>
#include <stdio.h>
#include "nd_prefs_layout.h"

int main(void)
{
    struct NDPrefsLayout layout;
    nd_prefs_layout_init(&layout,8,8,640,256,11,620,184);
    assert(layout.window_width==620 && layout.window_height==184);
    assert(nd_prefs_layout_x(&layout,270)==270);
    assert(nd_prefs_layout_h(&layout,16)==16);
    assert(nd_prefs_layout_button_width(&layout,40,"Diagnostics")==104);
    assert(nd_prefs_layout_fits(&layout,10,12));
    assert(nd_prefs_layout_left(&layout)==10);
    assert(nd_prefs_layout_top(&layout)==12);

    nd_prefs_layout_init(&layout,10,12,1024,768,16,620,184);
    assert(layout.window_width==775 && layout.window_height==276);
    assert(nd_prefs_layout_x(&layout,10)==13);
    assert(nd_prefs_layout_y(&layout,21)==32);
    assert(nd_prefs_layout_h(&layout,16)>=18);
    assert(nd_prefs_layout_text_width(&layout,"Width:")==60);
    assert(nd_prefs_layout_fits(&layout,10,12));

    nd_prefs_layout_init(&layout,0,0,640,200,10,620,184);
    assert(layout.font_x==8 && layout.font_y==8);
    assert(nd_prefs_layout_fits(&layout,10,12));
    nd_prefs_layout_init(&layout,12,16,640,200,10,620,184);
    assert(!nd_prefs_layout_fits(&layout,10,12));
    puts("Font-aware Preferences layout tests passed.");
    return 0;
}
