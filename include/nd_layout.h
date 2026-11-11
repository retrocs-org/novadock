#ifndef ND_LAYOUT_H
#define ND_LAYOUT_H
#include "nd_config.h"
#define ND_LAYOUT_BORDER 7
#define ND_LAYOUT_MARGIN 5
struct NDLayout {
    int tile_width;
    int tile_height;
    int gap;
    int width;
    int height;
    int rows;          /* actual horizontal rows (one for vertical docks) */
    int columns;       /* slots in each horizontal row */
    int visible_count; /* real item tiles visible at once */
    int slots;         /* item tiles plus optional previous/next controls */
    int overflow;      /* two control slots page through the remaining items */
};
/* Fits without leaving the Workbench screen or title bar. If every item
 * cannot remain readable, two native scroll slots expose a visible slice;
 * items are never silently dropped. Returns 0 only when even one item plus
 * both overflow controls cannot fit at the minimum tile size. */
int nd_layout_fit(const struct NDConfig *cfg, int screen_width,
                  int screen_height, int bar_height,
                  struct NDLayout *layout);
/* Same result as fitting a copy after nd_compact_enabled(), computed on the
 * full configuration directly: no 92 KB working copy is needed. */
int nd_layout_fit_enabled(const struct NDConfig *cfg, int screen_width,
                          int screen_height, int bar_height,
                          struct NDLayout *layout);
/* Return the zero-based row, column and cell span of an item in the
 * horizontal row-major grid. The caller supplies layout.columns. */
int nd_layout_item_cell(const struct NDConfig *cfg,int index,int columns,
                        int *row,int *column,int *span);
/* Number of occupied cells in one packed horizontal row.  This lets the
 * renderer center a short final row without changing the stable row-major
 * item mapping used by hit testing and Preferences. */
/* nd_layout_item_cell() for the VISIBLE-index'th enabled item, as if the
 * configuration had been compacted first. */
int nd_layout_item_cell_enabled(const struct NDConfig *cfg,int visible,
                                int columns,int *row,int *column,int *span);
int nd_layout_row_used_cells(const struct NDConfig *cfg,int row,int columns);
/* Aspect-fit an image inside a positive bounding box. Invalid source sizes
 * produce 0 x 0; valid images are never enlarged. */
void nd_icon_fit(int width, int height, int max_width, int max_height,
                 int *fitted_width, int *fitted_height);
#endif
