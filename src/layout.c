/* Portable dock layout rules; the host tests these without Amiga headers. */
#include <string.h>
#include "nd_layout.h"

/* Every rule below reads the configuration through a small view: the item
 * order (all items, or only the enabled ones) and the effective row counts.
 * This lets Preferences and the dock fit the visible dock directly on a full
 * configuration instead of fitting a compacted 92 KB copy. */
struct LView {
    const struct NDConfig *cfg;
    int count;
    int row_counts[ND_ROWS_MAX];
    unsigned char map[ND_MAX_ITEMS];
};
#define VITEM(v,i) (&(v)->cfg->items[(v)->map[(i)]])

static int wide_music(const struct LView *v,int index,int vertical)
{
    const struct NDItem *item;
    if (vertical || v->cfg->item_size==ND_ITEM_SIZE_TINY ||
        index<0 || index>=v->count) return 0;
    item=VITEM(v,index);
    return item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"MUSIC");
}

static int has_wide_music(const struct LView *v,int vertical)
{
    int i;
    if (vertical) return 0;
    for (i=0;i<v->count;++i)
        if (wide_music(v,i,vertical)) return 1;
    return 0;
}

static int item_extent(const struct LView *v,int index,int normal,
                       int gap,int vertical)
{
    int size;
    if (wide_music(v,index,vertical)) return normal*2+gap;
    if ((*VITEM(v,index)).kind!=ND_ITEM_SPACER) return normal;
    size=(*VITEM(v,index)).spacer_width;
    if (size && v->cfg->item_size==ND_ITEM_SIZE_TINY) size=(size+1)/2;
    return size ? size : (normal+1)/2;
}

static int content_extent(const struct LView *v,int normal,int gap,
                          int vertical)
{
    int i,total=0;
    for (i=0;i<v->count;++i)
        total+=item_extent(v,i,normal,gap,vertical);
    if (v->count>1) total+=(v->count-1)*gap;
    return total;
}

static int item_span(const struct LView *v,int index)
{
    return wide_music(v,index,0) ? 2 : 1;
}

static int manual_item_row(const struct LView *v,int index)
{
    int row,total=0;
    if (!v || !v->cfg->row_layout || index<0) return -1;
    for (row=0;row<v->cfg->rows && row<ND_ROWS_MAX;++row) {
        total+=v->row_counts[row];
        if (index<total) return row;
    }
    /* Runtime-only tiles and newly enabled catalogue entries join the last
     * configured row without invalidating the saved manual distribution. */
    return v->cfg->rows>0 ? v->cfg->rows-1 : 0;
}

static int manual_row_cells(const struct LView *v,int row)
{
    int i,cells=0;
    if (!v || !v->cfg->row_layout || row<0 || row>=v->cfg->rows) return 0;
    for (i=0;i<v->count;++i)
        if (manual_item_row(v,i)==row) cells+=item_span(v,i);
    return cells;
}

/* Row-major packing keeps one logical item intact.  A two-cell music tile is
 * moved to the next row instead of being split at the right edge. */
static int packed_rows(const struct LView *v,int columns)
{
    int i,slot=0,span,col;
    if (columns<1) return ND_MAX_ITEMS+1;
    if (v->cfg->row_layout) {
        for (i=0;i<v->cfg->rows;++i)
            if (manual_row_cells(v,i)>columns) return ND_MAX_ITEMS+1;
        return v->cfg->rows;
    }
    for (i=0;i<v->count;++i) {
        span=item_span(v,i);
        if (span>columns) return ND_MAX_ITEMS+1;
        col=slot%columns;
        if (col+span>columns) slot+=columns-col;
        slot+=span;
    }
    return (slot+columns-1)/columns;
}

static int grid_columns(const struct LView *v,int rows)
{
    int i,total=0,columns;
    if (v->cfg->row_layout) {
        columns=1;
        for (i=0;i<rows;++i) {
            int cells=manual_row_cells(v,i);
            if (cells>columns) columns=cells;
        }
        return columns;
    }
    for (i=0;i<v->count;++i) total+=item_span(v,i);
    columns=(total+rows-1)/rows;
    if (has_wide_music(v,0) && columns<2) columns=2;
    while (packed_rows(v,columns)>rows) ++columns;
    return columns;
}

static int view_item_cell(const struct LView *v,int index,int columns,
                          int *row,int *column,int *span_out)
{
    int i,slot=0,span,col;
    if (!v || index<0 || index>=v->count || columns<1) return 0;
    if (v->cfg->row_layout) {
        int item_row=manual_item_row(v,index),item_column=0;
        span=item_span(v,index);
        for (i=0;i<index;++i)
            if (manual_item_row(v,i)==item_row)
                item_column+=item_span(v,i);
        if (item_column+span>columns) return 0;
        if (row) *row=item_row;
        if (column) *column=item_column;
        if (span_out) *span_out=span;
        return 1;
    }
    for (i=0;i<=index;++i) {
        span=item_span(v,i);
        if (span>columns) return 0;
        col=slot%columns;
        if (col+span>columns) slot+=columns-col;
        if (i==index) {
            if (row) *row=slot/columns;
            if (column) *column=slot%columns;
            if (span_out) *span_out=span;
            return 1;
        }
        slot+=span;
    }
    return 0;
}

static int view_row_used_cells(const struct LView *v,int row,int columns)
{
    int i,item_row,column,span,used=0;
    if (!v || row<0 || columns<1) return 0;
    for (i=0;i<v->count;++i) {
        if (!view_item_cell(v,i,columns,&item_row,&column,&span))
            return 0;
        if (item_row==row && column+span>used) used=column+span;
        else if (!v->cfg->row_layout && item_row>row) break;
    }
    return used;
}

/* All items in configuration order, row counts as stored. */
static int view_all(struct LView *v,const struct NDConfig *cfg)
{
    int i;
    if (!cfg || cfg->count<0 || cfg->count>ND_MAX_ITEMS) return 0;
    v->cfg=cfg;v->count=cfg->count;
    for (i=0;i<cfg->count;++i) v->map[i]=(unsigned char)i;
    for (i=0;i<ND_ROWS_MAX;++i) v->row_counts[i]=cfg->row_counts[i];
    return 1;
}

/* Exactly what nd_compact_enabled() would produce: enabled items only, with
 * row counts normalized the same way (clamped to the enabled total, unused
 * rows cleared, any remainder joining the last configured row). */
static int view_enabled(struct LView *v,const struct NDConfig *cfg)
{
    int i,rows,remaining;
    if (!cfg || cfg->count<0 || cfg->count>ND_MAX_ITEMS) return 0;
    v->cfg=cfg;v->count=0;
    for (i=0;i<cfg->count;++i)
        if (cfg->items[i].enabled) v->map[v->count++]=(unsigned char)i;
    for (i=0;i<ND_ROWS_MAX;++i) v->row_counts[i]=cfg->row_counts[i];
    if (!cfg->row_layout) return 1;
    rows=cfg->rows;
    if (rows<ND_ROWS_MIN) rows=ND_ROWS_MIN;
    if (rows>ND_ROWS_MAX) rows=ND_ROWS_MAX;
    remaining=v->count;
    for (i=0;i<rows;++i) {
        if (v->row_counts[i]<0) v->row_counts[i]=0;
        if (v->row_counts[i]>remaining) v->row_counts[i]=remaining;
        remaining-=v->row_counts[i];
    }
    for (i=rows;i<ND_ROWS_MAX;++i) v->row_counts[i]=0;
    if (remaining>0) v->row_counts[rows-1]+=remaining;
    return 1;
}

int nd_layout_item_cell(const struct NDConfig *cfg,int index,int columns,
                        int *row,int *column,int *span_out)
{
    struct LView v;
    if (!view_all(&v,cfg)) return 0;
    return view_item_cell(&v,index,columns,row,column,span_out);
}

int nd_layout_item_cell_enabled(const struct NDConfig *cfg,int visible,
                                int columns,int *row,int *column,int *span_out)
{
    struct LView v;
    if (!view_enabled(&v,cfg)) return 0;
    return view_item_cell(&v,visible,columns,row,column,span_out);
}

int nd_layout_row_used_cells(const struct NDConfig *cfg,int row,int columns)
{
    struct LView v;
    if (!view_all(&v,cfg)) return 0;
    return view_row_used_cells(&v,row,columns);
}

void nd_icon_fit(int width, int height, int max_width, int max_height,
                 int *fitted_width, int *fitted_height)
{
    int out_width = width;
    int out_height = height;
    if (width <= 0 || height <= 0 || max_width <= 0 || max_height <= 0) {
        out_width = 0;
        out_height = 0;
    } else {
        if (out_width > max_width) {
            out_height = (out_height * max_width) / out_width;
            out_width = max_width;
            if (out_height < 1) out_height = 1;
        }
        if (out_height > max_height) {
            out_width = (out_width * max_height) / out_height;
            out_height = max_height;
            if (out_width < 1) out_width = 1;
        }
    }
    if (fitted_width) *fitted_width = out_width;
    if (fitted_height) *fitted_height = out_height;
}

static int view_fit(const struct LView *v, int screen_width,
                    int screen_height, int bar_height,
                    struct NDLayout *layout)
{
    int count, vertical, available, fitted, slots,rows,columns;
    int minimum_width,minimum_height;
    if (!v || !layout || v->count < 1 || v->count > ND_MAX_DOCK_ITEMS ||
        screen_width <= 0 || screen_height <= 0 || bar_height < 0 ||
        v->cfg->gap < 0 || v->cfg->tile_width < 55 || v->cfg->tile_height < 46 ||
        v->cfg->item_size < 0 || v->cfg->item_size >= ND_ITEM_SIZE_COUNT ||
        v->cfg->rows < ND_ROWS_MIN || v->cfg->rows > ND_ROWS_MAX)
        return 0;
    count = v->count;
    vertical = v->cfg->side == ND_SIDE_LEFT || v->cfg->side == ND_SIDE_RIGHT;
    minimum_width=v->cfg->item_size==ND_ITEM_SIZE_TINY ? 34 : 55;
    minimum_height=v->cfg->item_size==ND_ITEM_SIZE_TINY ? 30 : 46;
    layout->tile_width = v->cfg->item_size==ND_ITEM_SIZE_TINY ?
                         (v->cfg->tile_width+1)/2 : v->cfg->tile_width;
    layout->tile_height = v->cfg->item_size==ND_ITEM_SIZE_TINY ?
                          (v->cfg->tile_height+1)/2 : v->cfg->tile_height;
    layout->gap = v->cfg->item_size==ND_ITEM_SIZE_TINY ?
                  (v->cfg->gap+1)/2 : v->cfg->gap;
    if (layout->tile_width<minimum_width) layout->tile_width=minimum_width;
    if (layout->tile_height<minimum_height) layout->tile_height=minimum_height;
    layout->visible_count = count;
    layout->slots = count;
    layout->overflow = 0;
    layout->rows = 1;
    layout->columns = count;
    if (vertical) {
        available = screen_height - bar_height - 2 * ND_LAYOUT_MARGIN;
        layout->height = 2 * ND_LAYOUT_BORDER +
                         content_extent(v,layout->tile_height,layout->gap,1);
        if (layout->height > available) {
            fitted=layout->tile_height;
            while (fitted>=minimum_height &&
                   2*ND_LAYOUT_BORDER+
                   content_extent(v,fitted,layout->gap,1)>available)
                --fitted;
            if (fitted >= minimum_height) layout->tile_height = fitted;
            else {
                layout->tile_height = minimum_height;
                slots = (available - 2 * ND_LAYOUT_BORDER + layout->gap) /
                        (layout->tile_height + layout->gap);
                if (slots < 3) return 0;
                layout->overflow = 1;
                layout->slots = slots;
                layout->visible_count = slots - 2;
            }
        }
        layout->width = 2 * ND_LAYOUT_BORDER + layout->tile_width;
        layout->height = layout->overflow ?
            2 * ND_LAYOUT_BORDER + layout->slots * layout->tile_height +
              (layout->slots - 1) * layout->gap :
            2 * ND_LAYOUT_BORDER +
              content_extent(v,layout->tile_height,layout->gap,1);
        layout->columns=1;
    } else {
        if (v->cfg->rows>1) {
            rows=v->cfg->rows;
            columns=grid_columns(v,rows);
            available=screen_width-2*ND_LAYOUT_MARGIN;
            fitted=layout->tile_width;
            while (fitted>=minimum_width &&
                   2*ND_LAYOUT_BORDER+columns*fitted+
                   (columns-1)*layout->gap>available) --fitted;
            if (fitted>=minimum_width) layout->tile_width=fitted;
            else {
                layout->tile_width=minimum_width;
                columns=(available-2*ND_LAYOUT_BORDER+layout->gap)/
                        (layout->tile_width+layout->gap);
                if (columns<(has_wide_music(v,0) ? 2 : 1)) return 0;
                if (packed_rows(v,columns)>rows) {
                    slots=columns*rows;
                    if (slots<(has_wide_music(v,0) ? 5 : 3)) return 0;
                    layout->overflow=1;
                    layout->visible_count=slots-
                        (has_wide_music(v,0) ? 4 : 2);
                    if (layout->visible_count<1) return 0;
                }
            }
            /* Configured rows are structural, not merely a packing limit.
             * Preserve empty rows in the dock so runtime layout and the
             * Preferences organizer always show the same row count. */
            layout->rows=rows;
            layout->columns=columns;
            layout->slots=columns*layout->rows;
            fitted=layout->tile_height;
            available=screen_height-bar_height-2*ND_LAYOUT_MARGIN;
            while (fitted>=minimum_height &&
                   2*ND_LAYOUT_BORDER+layout->rows*fitted+
                   (layout->rows-1)*layout->gap>available) --fitted;
            if (fitted<minimum_height) return 0;
            layout->tile_height=fitted;
            layout->width=2*ND_LAYOUT_BORDER+columns*layout->tile_width+
                          (columns-1)*layout->gap;
            layout->height=2*ND_LAYOUT_BORDER+layout->rows*layout->tile_height+
                           (layout->rows-1)*layout->gap;
            return layout->width<=screen_width-2*ND_LAYOUT_MARGIN &&
                   layout->height<=screen_height-bar_height-2*ND_LAYOUT_MARGIN;
        }
        available = screen_width - 2 * ND_LAYOUT_MARGIN;
        layout->width = 2 * ND_LAYOUT_BORDER +
                        content_extent(v,layout->tile_width,layout->gap,0);
        if (layout->width > available) {
            fitted=layout->tile_width;
            while (fitted>=minimum_width &&
                   2*ND_LAYOUT_BORDER+
                   content_extent(v,fitted,layout->gap,0)>available)
                --fitted;
            if (fitted >= minimum_width) layout->tile_width = fitted;
            else {
                layout->tile_width = minimum_width;
                slots = (available - 2 * ND_LAYOUT_BORDER + layout->gap) /
                        (layout->tile_width + layout->gap);
                if (slots < (has_wide_music(v,0) ? 4 : 3)) return 0;
                layout->overflow = 1;
                layout->slots = slots;
                /* MusicPlayer consumes two ordinary horizontal slots.  Keep
                 * one extra slot free on every page so it can never overlap
                 * the fixed PREV/NEXT controls. */
                layout->visible_count = slots -
                    (has_wide_music(v,0) ? 3 : 2);
            }
        }
        layout->width = layout->overflow ?
            2 * ND_LAYOUT_BORDER + layout->slots * layout->tile_width +
              (layout->slots - 1) * layout->gap :
            2 * ND_LAYOUT_BORDER +
              content_extent(v,layout->tile_width,layout->gap,0);
        layout->height = 2 * ND_LAYOUT_BORDER + layout->tile_height;
        /* A wide Music tile occupies two cells even in a single row.
         * Item count is not the grid width: the final item must remain
         * addressable by drag/drop and row-layout capture. */
        layout->columns=layout->overflow ? layout->slots : grid_columns(v,1);
    }
    return layout->width <= screen_width - 2 * ND_LAYOUT_MARGIN &&
           layout->height <= screen_height - bar_height - 2 * ND_LAYOUT_MARGIN;
}

int nd_layout_fit(const struct NDConfig *cfg, int screen_width,
                  int screen_height, int bar_height,
                  struct NDLayout *layout)
{
    struct LView v;
    if (!view_all(&v,cfg)) return 0;
    return view_fit(&v,screen_width,screen_height,bar_height,layout);
}

int nd_layout_fit_enabled(const struct NDConfig *cfg, int screen_width,
                          int screen_height, int bar_height,
                          struct NDLayout *layout)
{
    struct LView v;
    if (!view_enabled(&v,cfg)) return 0;
    return view_fit(&v,screen_width,screen_height,bar_height,layout);
}
