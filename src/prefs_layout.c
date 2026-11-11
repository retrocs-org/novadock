#include <string.h>
#include "nd_prefs_layout.h"

static int sane_font(int value,int fallback)
{
    if (value<4 || value>64) return fallback;
    return value<fallback ? fallback : value;
}

static int scale_value(int value,int numerator,int denominator)
{
    if (value<=0) return 0;
    return (value*numerator+denominator-1)/denominator;
}

void nd_prefs_layout_init(struct NDPrefsLayout *layout,
                          int font_x,int font_y,
                          int screen_width,int screen_height,
                          int screen_bar_height,
                          int base_width,int base_height)
{
    if (!layout) return;
    layout->font_x=sane_font(font_x,ND_PREFS_BASE_FONT_X);
    layout->font_y=sane_font(font_y,ND_PREFS_BASE_FONT_Y);
    layout->screen_width=screen_width;
    layout->screen_height=screen_height;
    layout->screen_bar_height=screen_bar_height;
    layout->base_width=base_width;
    layout->base_height=base_height;
    layout->window_width=scale_value(base_width,layout->font_x,
                                     ND_PREFS_BASE_FONT_X);
    layout->window_height=scale_value(base_height,layout->font_y,
                                      ND_PREFS_BASE_FONT_Y);
}

int nd_prefs_layout_x(const struct NDPrefsLayout *layout,int value)
{
    return scale_value(value,layout->font_x,ND_PREFS_BASE_FONT_X);
}

int nd_prefs_layout_y(const struct NDPrefsLayout *layout,int value)
{
    return scale_value(value,layout->font_y,ND_PREFS_BASE_FONT_Y);
}

int nd_prefs_layout_w(const struct NDPrefsLayout *layout,int value)
{
    return nd_prefs_layout_x(layout,value);
}

int nd_prefs_layout_h(const struct NDPrefsLayout *layout,int value)
{
    int height=nd_prefs_layout_y(layout,value);
    int minimum=layout->font_y+6;
    return height<minimum ? minimum : height;
}

int nd_prefs_layout_text_width(const struct NDPrefsLayout *layout,
                               const char *text)
{
    if (!layout || !text) return 0;
    return (int)strlen(text)*layout->font_x;
}

int nd_prefs_layout_button_width(const struct NDPrefsLayout *layout,
                                 int baseline_width,const char *text)
{
    int width=nd_prefs_layout_w(layout,baseline_width);
    int needed=nd_prefs_layout_text_width(layout,text)+layout->font_x*2;
    return width<needed ? needed : width;
}

int nd_prefs_layout_fits(const struct NDPrefsLayout *layout,
                         int horizontal_margin,int vertical_margin)
{
    int hmargin,vmargin,title_top;
    if (!layout) return 0;
    hmargin=nd_prefs_layout_x(layout,horizontal_margin);
    vmargin=nd_prefs_layout_y(layout,vertical_margin);
    title_top=layout->screen_bar_height+1;
    if (vmargin<title_top) vmargin=title_top;
    return layout->window_width+hmargin<=layout->screen_width &&
           layout->window_height+vmargin<=layout->screen_height;
}

int nd_prefs_layout_left(const struct NDPrefsLayout *layout)
{
    int left=(layout->screen_width-layout->window_width)/2;
    return left<0 ? 0 : left;
}

int nd_prefs_layout_top(const struct NDPrefsLayout *layout)
{
    int top=layout->screen_bar_height+1;
    if (top+layout->window_height>layout->screen_height)
        top=layout->screen_height-layout->window_height;
    return top<0 ? 0 : top;
}
