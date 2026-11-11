#include "nd_autohide.h"
static int clamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
static int aligned(int start,int available,int size,int margin,int position)
{
    if (position==ND_POSITION_START) return start+margin;
    if (position==ND_POSITION_END) return start+available-size-margin;
    return start+(available-size)/2;
}
int nd_dock_rect(const struct NDConfig *cfg, const struct NDLayout *layout,
                 int sw, int sh, int bar, struct NDRect *r)
{
    if (!cfg || !layout || !r || sw <= 0 || sh <= bar ||
        layout->width <= 0 || layout->height <= 0 ||
        layout->width > sw || layout->height > sh - bar ||
        cfg->side < ND_SIDE_BOTTOM || cfg->side > ND_SIDE_RIGHT) return 0;
    r->width = layout->width; r->height = layout->height;
    if (cfg->position<0 || cfg->position>=ND_POSITION_COUNT) return 0;
    r->left = aligned(0,sw,r->width,ND_LAYOUT_MARGIN,cfg->position);
    r->top = aligned(bar,sh-bar,r->height,ND_LAYOUT_MARGIN,cfg->position);
    if (cfg->side == ND_SIDE_BOTTOM) r->top = sh - r->height - ND_LAYOUT_MARGIN;
    if (cfg->side == ND_SIDE_TOP) r->top = bar + ND_LAYOUT_MARGIN;
    if (cfg->side == ND_SIDE_LEFT) r->left = ND_LAYOUT_MARGIN;
    if (cfg->side == ND_SIDE_RIGHT) r->left = sw - r->width - ND_LAYOUT_MARGIN;
    r->left = clamp(r->left, 0, sw - r->width);
    r->top = clamp(r->top, bar, sh - r->height);
    return 1;
}
int nd_hidden_rect(const struct NDConfig *cfg, int sw, int sh,
                   int bar, struct NDRect *r)
{
    int along, depth;
    if (!cfg || !r || sw <= 0 || sh <= bar ||
        cfg->side < ND_SIDE_BOTTOM || cfg->side > ND_SIDE_RIGHT) return 0;
    if (cfg->side == ND_SIDE_LEFT || cfg->side == ND_SIDE_RIGHT) {
        along = sw < ND_TAB_THICKNESS ? sw : ND_TAB_THICKNESS;
        depth = sh - bar < ND_TAB_LENGTH ? sh - bar : ND_TAB_LENGTH;
        r->width = along; r->height = depth;
        r->left = cfg->side == ND_SIDE_LEFT ? ND_LAYOUT_MARGIN : sw - along - ND_LAYOUT_MARGIN;
        r->top = aligned(bar,sh-bar,depth,ND_LAYOUT_MARGIN,cfg->position);
    } else {
        along = sw < ND_TAB_LENGTH ? sw : ND_TAB_LENGTH;
        depth = sh - bar < ND_TAB_THICKNESS ? sh - bar : ND_TAB_THICKNESS;
        r->width = along; r->height = depth;
        r->left = aligned(0,sw,along,ND_LAYOUT_MARGIN,cfg->position);
        r->top = cfg->side == ND_SIDE_TOP ? bar + ND_LAYOUT_MARGIN : sh - depth - ND_LAYOUT_MARGIN;
    }
    r->left = clamp(r->left, 0, sw - r->width);
    r->top = clamp(r->top, bar, sh - r->height);
    return r->width > 0 && r->height > 0;
}
int nd_should_hide(int enabled, int hidden, int blocked,
                   int pointer_inside, int idle_ticks, int delay_seconds)
{
    if (!enabled || hidden || blocked || pointer_inside) return 0;
    if (delay_seconds < 1 || delay_seconds > 60) return 0;
    return idle_ticks >= delay_seconds * 5;
}
