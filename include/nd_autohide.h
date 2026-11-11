#ifndef ND_AUTOHIDE_H
#define ND_AUTOHIDE_H
#include "nd_layout.h"
/* All geometry is screen-relative, with no platform headers. */
struct NDRect { int left, top, width, height; };
#define ND_TAB_LENGTH 42
#define ND_TAB_THICKNESS 12
/* Return 0 if dimensions/side are invalid. Always leave a visible tab. */
int nd_dock_rect(const struct NDConfig *cfg, const struct NDLayout *layout,
                 int screen_width, int screen_height, int bar_height,
                 struct NDRect *rect);
int nd_hidden_rect(const struct NDConfig *cfg, int screen_width,
                   int screen_height, int bar_height, struct NDRect *rect);
/* A five-Hz timer; this is a minimum idle interval, not wall-clock precision. */
int nd_should_hide(int enabled, int hidden, int blocked,
                   int pointer_inside, int idle_ticks, int delay_seconds);
#endif
