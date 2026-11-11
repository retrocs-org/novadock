#include <assert.h>
#include <stdio.h>
#include "nd_autohide.h"
static void same_rect(const struct NDRect *a, const struct NDRect *b)
{
    assert(a->left == b->left && a->top == b->top);
    assert(a->width == b->width && a->height == b->height);
}
static void in_screen(struct NDRect *r, int sw, int sh, int bar)
{
    assert(r->width > 0 && r->height > 0);
    assert(r->left >= 0 && r->left + r->width <= sw);
    assert(r->top >= bar && r->top + r->height <= sh);
}
int main(void)
{
    struct NDConfig cfg;
    struct NDLayout layout;
    struct NDRect full,tab,again;
    int side;
    nd_default_config(&cfg);
    assert(cfg.auto_hide == 0 && cfg.hide_delay == 3);
    assert(nd_parse_line(&cfg,"AUTO_HIDE=YES"));
    assert(cfg.auto_hide == 1);
    assert(nd_parse_line(&cfg,"HIDE_DELAY=5"));
    assert(cfg.hide_delay == 5);
    assert(!nd_parse_line(&cfg,"AUTO_HIDE=TRUE"));
    assert(!nd_parse_line(&cfg,"HIDE_DELAY=0"));
    assert(!nd_parse_line(&cfg,"HIDE_DELAY=61"));
    assert(cfg.hide_delay == 5);
    for (side=ND_SIDE_BOTTOM;side<=ND_SIDE_RIGHT;++side) {
        cfg.side=side;
        /* Nine large vertical tiles cannot fit 256 lines. Use 800 lines
         * for vertical docks while preserving 640px for horizontal docks. */
        {
            int sh = side == ND_SIDE_LEFT || side == ND_SIDE_RIGHT ? 800 : 256;
            assert(nd_layout_fit(&cfg,640,sh,11,&layout));
            assert(nd_dock_rect(&cfg,&layout,640,sh,11,&full));
            assert(nd_hidden_rect(&cfg,640,sh,11,&tab));
            in_screen(&full,640,sh,11);
            in_screen(&tab,640,sh,11);
            assert(nd_dock_rect(&cfg,&layout,640,sh,11,&again));
        }
        in_screen(&tab,640,800,11);
        if (side == ND_SIDE_BOTTOM || side == ND_SIDE_TOP) {
            assert(tab.width == 42 && tab.height == 12);
        } else { assert(tab.height == 42 && tab.width == 12); }
        same_rect(&again,&full);
    }
    cfg.side=ND_SIDE_BOTTOM;cfg.position=ND_POSITION_START;
    assert(nd_dock_rect(&cfg,&layout,640,800,11,&full));
    assert(full.left==ND_LAYOUT_MARGIN);
    assert(nd_hidden_rect(&cfg,640,800,11,&tab));
    assert(tab.left==ND_LAYOUT_MARGIN);
    cfg.position=ND_POSITION_END;
    assert(nd_dock_rect(&cfg,&layout,640,800,11,&full));
    assert(full.left==640-full.width-ND_LAYOUT_MARGIN);
    assert(nd_hidden_rect(&cfg,640,800,11,&tab));
    assert(tab.left==640-tab.width-ND_LAYOUT_MARGIN);
    cfg.side=ND_SIDE_LEFT;cfg.position=ND_POSITION_START;
    assert(nd_dock_rect(&cfg,&layout,640,800,11,&full));
    assert(full.top==11+ND_LAYOUT_MARGIN);
    cfg.position=ND_POSITION_END;
    assert(nd_hidden_rect(&cfg,640,800,11,&tab));
    assert(tab.top==800-tab.height-ND_LAYOUT_MARGIN);
    assert(nd_should_hide(1,0,0,0,25,5));
    assert(!nd_should_hide(1,0,0,0,24,5));
    assert(!nd_should_hide(0,0,0,0,250,5));
    assert(!nd_should_hide(1,1,0,0,250,5));
    assert(!nd_should_hide(1,0,1,0,250,5));
    assert(!nd_should_hide(1,0,0,1,250,5));
    cfg.side=ND_SIDE_TOP;cfg.position=ND_POSITION_CENTER;
    assert(nd_hidden_rect(&cfg, 40, 60, 11, &tab));
    in_screen(&tab,40,60,11);
    puts("Auto-hide config, geometry (all edges), and idle transition tests passed.");
    return 0;
}
