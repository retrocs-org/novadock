#ifndef ND_PREFS_LAYOUT_H
#define ND_PREFS_LAYOUT_H

/* Preferences coordinates are authored against the classic 8x8 Workbench
 * font.  This helper converts that baseline to the active screen font while
 * keeping all page-specific geometry in the page that owns it. */
#define ND_PREFS_BASE_FONT_X 8
#define ND_PREFS_BASE_FONT_Y 8

struct NDPrefsLayout {
    int font_x;
    int font_y;
    int screen_width;
    int screen_height;
    int screen_bar_height;
    int base_width;
    int base_height;
    int window_width;
    int window_height;
};

void nd_prefs_layout_init(struct NDPrefsLayout *layout,
                          int font_x,int font_y,
                          int screen_width,int screen_height,
                          int screen_bar_height,
                          int base_width,int base_height);
int nd_prefs_layout_x(const struct NDPrefsLayout *layout,int value);
int nd_prefs_layout_y(const struct NDPrefsLayout *layout,int value);
int nd_prefs_layout_w(const struct NDPrefsLayout *layout,int value);
int nd_prefs_layout_h(const struct NDPrefsLayout *layout,int value);
int nd_prefs_layout_text_width(const struct NDPrefsLayout *layout,
                               const char *text);
int nd_prefs_layout_button_width(const struct NDPrefsLayout *layout,
                                 int baseline_width,const char *text);
int nd_prefs_layout_fits(const struct NDPrefsLayout *layout,
                         int horizontal_margin,int vertical_margin);
int nd_prefs_layout_left(const struct NDPrefsLayout *layout);
int nd_prefs_layout_top(const struct NDPrefsLayout *layout);

#endif
