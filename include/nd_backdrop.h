#ifndef ND_BACKDROP_H
#define ND_BACKDROP_H

#include <exec/types.h>
#include "nd_ilbm.h"

struct BitMap;
struct RastPort;
struct Screen;
struct NDRect;

struct NDBackdropPart {
    struct BitMap *bitmap,*mask,*growth,*growth_mask;
    int length;
};

struct NDBackdropColor {
    ULONG rgb;
    LONG pen;
    int owned;
};

/* Set up from zeroed storage. Free before releasing the public screen. */
struct NDBackdrop {
    struct NDBackdropPart parts[3]; /* centre, start cap, end cap */
    struct NDBackdropColor palette[3 * ND_ICON_MAX_COLORS];
    int color_count;
    int thickness;
    int vertical;
};

void nd_backdrop_free(struct NDBackdrop *background,struct Screen *screen);
void nd_backdrop_load(struct NDBackdrop *background,struct Screen *screen,
                      const char *root,const char *theme_id,int thickness,
                      int vertical,ULONG matte,const struct NDRect *desktop);
void nd_tile_highlight(struct RastPort *rp,int x,int y,int w,int h,int shape,
                       ULONG fill,ULONG edge);
int nd_backdrop_paint_sized(const struct NDBackdrop *background,struct RastPort *rp,
                      int origin_x,int origin_y,int length,int thickness,
                      int x,int y,int w,int h);
int nd_backdrop_paint(const struct NDBackdrop *background,struct RastPort *rp,
                      int origin_x,int origin_y,int length,int x,int y,int w,int h);

#endif
