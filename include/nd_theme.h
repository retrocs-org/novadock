#ifndef ND_THEME_H
#define ND_THEME_H

#include "nd_config.h"

#define ND_THEME_COLOR_COUNT 8

#define ND_THEME_BIND_SHAPE    (1U << 0)
#define ND_THEME_BIND_SIDE     (1U << 1)
#define ND_THEME_BIND_POSITION (1U << 2)
#define ND_THEME_BIND_FONT     (1U << 3)
#define ND_THEME_BIND_LABELS   (1U << 4)
#define ND_THEME_BIND_ALL      (ND_THEME_BIND_SHAPE | ND_THEME_BIND_SIDE | \
                                ND_THEME_BIND_POSITION | ND_THEME_BIND_FONT | \
                                ND_THEME_BIND_LABELS)

struct NDThemeSpec {
    char name[32];
    unsigned char rgb[ND_THEME_COLOR_COUNT][3];
    int shape;
    int side;
    int position;
    int font_mode;
    int label_mode;
    unsigned int binding_mask;
    int spacer_width;
    int spacer_line;
    int spacer_line_width;
    int spacer_line_color;
    int bevel;           /* BEVEL=YES: classic raised frame around the dock */
};

const char *nd_theme_directory(int theme);
int nd_theme_id_valid(const char *theme_id);
int nd_theme_index_from_id(const char *theme_id);
void nd_theme_defaults(int theme,struct NDThemeSpec *spec);
void nd_theme_defaults_id(const char *theme_id,struct NDThemeSpec *spec);
int nd_theme_load(const char *root,int theme,struct NDThemeSpec *spec);
int nd_theme_load_id(const char *root,const char *theme_id,struct NDThemeSpec *spec);
int nd_theme_load_config(const char *root,const struct NDConfig *cfg,
                         struct NDThemeSpec *spec);
void nd_theme_apply_spacer(struct NDItem *item,const struct NDThemeSpec *spec);
void nd_theme_apply_spacers(struct NDConfig *cfg,const struct NDThemeSpec *spec);
void nd_theme_apply_preferences(struct NDConfig *cfg,const struct NDThemeSpec *spec);

#endif
