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

int nd_hover_size(int distance,int radius,int strength,int max_width,int max_height,int *width,int *height)
{
    int weight,w,h;
    if (distance<0) distance=-distance;
    if (radius<1 || distance>=radius || strength<=0 || *width<1 || *height<1) return 0;
    if(strength>100)strength=100;
    /* Smoothstep has zero slope at both the peak and the wave edge.
     * Keep sub-percent precision so crossing an icon centre stays gentle. */
    weight=(radius-distance)*1024/radius;
    weight=weight*weight*(3072-2*weight)/1048576;
    weight=weight*strength/100;
    w=*width+*width*weight*7/4096;
    h=*height+*height*weight*7/4096;
    nd_icon_fit(w,h,max_width,max_height,&w,&h);
    if(w<*width || h<*height || (w==*width && h==*height))return 0;
    *width=w;*height=h;return 1;
}

int nd_icon_wave_size(int distance,int radius,int max_width,int max_height,int *width,int *height)
{return nd_hover_size(distance,radius,100,max_width,max_height,width,height);}
static int hover_extent(const struct NDHoverLayoutItem *item,int normal)
{
    int extent=item->vertical ? (normal ? item->base_height:item->height):
                                (normal ? item->base_width:item->width);
    if(item->kind==ND_HOVER_ART){
        if(item->vertical)extent+=item->label;
        else if(extent<item->label)extent=item->label;
    }
    return extent;
}
static void hover_segment(struct NDHoverLayoutItem *items,int first,int end,
                           int focus,int gap,int left,int right)
{
    int i,normal=0,expanded=0,budget,amount=100,width,previous,shift,span;
    if(first>=end)return;
    for(i=first;i<end;++i){normal+=hover_extent(&items[i],1);expanded+=hover_extent(&items[i],0);}
    normal+=(end-first-1)*gap;expanded+=(end-first-1)*gap;
    budget=right-left-normal;
    if(expanded>right-left && expanded>normal){
        amount=budget>0 ? budget*100/(expanded-normal):0;
        if(amount>100)amount=100;
    }
    for(i=first;i<end;++i){
        items[i].width=items[i].base_width+(items[i].width-items[i].base_width)*amount/100;
        items[i].height=items[i].base_height+(items[i].height-items[i].base_height)*amount/100;
        items[i].centre=items[i].anchor;
    }
    if(focus<first)focus=first;if(focus>=end)focus=end-1;
    {int extra=(hover_extent(&items[focus],0)-hover_extent(&items[focus],1))/2;
     for(i=focus-1;i>=first;--i){int own=hover_extent(&items[i],0)-hover_extent(&items[i],1);items[i].centre-=extra+own/2;extra+=own;}
     extra=(hover_extent(&items[focus],0)-hover_extent(&items[focus],1)+1)/2;
     for(i=focus+1;i<end;++i){int own=hover_extent(&items[i],0)-hover_extent(&items[i],1);items[i].centre+=extra+(own+1)/2;extra+=own;}}
    for(i=focus-1;i>=first;--i){
        width=(hover_extent(&items[i],0)+1)/2+hover_extent(&items[i+1],0)/2+gap;
        if(items[i].centre>items[i+1].centre-width)items[i].centre=items[i+1].centre-width;
    }
    for(i=focus+1;i<end;++i){
        width=(hover_extent(&items[i-1],0)+1)/2+hover_extent(&items[i],0)/2+gap;
        if(items[i].centre<items[i-1].centre+width)items[i].centre=items[i-1].centre+width;
    }
    span=items[end-1].centre+(hover_extent(&items[end-1],0)+1)/2-
         (items[first].centre-hover_extent(&items[first],0)/2);
    if(span>right-left){
        int minimum=0,free_space,room,slack=0,used=0,old_centre,next_centre,packed;
        for(i=first;i<end;++i)minimum+=hover_extent(&items[i],0);
        minimum+=(end-first-1)*gap;
        free_space=span-minimum;room=right-left-minimum;
        /* Remove only the excess spacing, proportionally across the row.
         * Switching straight to minimum gaps at this boundary makes the
         * entire dock jump even for a one-pixel change in the wave. Use
         * cumulative rounding so the last tile fits exactly as well. */
        if(free_space>0 && room>=0){
            old_centre=items[first].centre;
            for(i=first+1;i<end;++i){
                width=(hover_extent(&items[i-1],0)+1)/2+hover_extent(&items[i],0)/2+gap;
                next_centre=items[i].centre;
                slack+=next_centre-old_centre-width;
                packed=slack*room/free_space;
                items[i].centre=items[i-1].centre+width+packed-used;
                used=packed;old_centre=next_centre;
            }
        }
    }
    shift=0;
    previous=items[first].centre-hover_extent(&items[first],0)/2;
    width=items[end-1].centre+(hover_extent(&items[end-1],0)+1)/2;
    if(previous+shift<left)shift=left-previous;
    if(width+shift>right)shift=right-width;
    for(i=first;i<end;++i)items[i].centre+=shift;
}
void nd_hover_layout(struct NDHoverLayoutItem *items,int count,int focus,int gap,int left,int right)
{
    int i,first=0,bound=left;
    if(!items || count<1 || right<=left)return;
    for(i=0;i<count;++i){
        items[i].centre=items[i].anchor;
        if(items[i].kind==ND_HOVER_FIXED){
            hover_segment(items,first,i,focus,gap,bound,
                          items[i].anchor-hover_extent(&items[i],1)/2-gap);
            first=i+1;bound=items[i].anchor+(hover_extent(&items[i],1)+1)/2+gap;
        }
    }
    hover_segment(items,first,count,focus,gap,bound,right);
}

/* Blend valid packings around the two neighbouring anchors. Their sizes
 * and bounds are identical; interpolating their centres preserves gaps and
 * fixed dividers, without snapping when the highlighted tile changes. */
void nd_hover_layout_at(struct NDHoverLayoutItem *items,int count,int position,int gap,int left,int right)
{
    struct NDHoverLayoutItem original[ND_MAX_DOCK_ITEMS];
    int centres[ND_MAX_DOCK_ITEMS];
    int a=-1,b=-1,i,weight;
    if(!items || count<1 || count>ND_MAX_DOCK_ITEMS || right<=left)return;
    for(i=0;i<count;++i){
        if(items[i].anchor<=position && (a<0 || items[i].anchor>items[a].anchor))a=i;
        if(items[i].anchor>=position && (b<0 || items[i].anchor<items[b].anchor))b=i;
    }
    if(a<0)a=b;if(b<0)b=a;
    if(a==b){nd_hover_layout(items,count,a,gap,left,right);return;}
    weight=(position-items[a].anchor)*256/(items[b].anchor-items[a].anchor);
    memcpy(original,items,(unsigned int)count*sizeof(*items));
    nd_hover_layout(items,count,a,gap,left,right);
    for(i=0;i<count;++i)centres[i]=items[i].centre;
    memcpy(items,original,(unsigned int)count*sizeof(*items));
    nd_hover_layout(items,count,b,gap,left,right);
    for(i=0;i<count;++i)
        items[i].centre=(centres[i]*(256-weight)+items[i].centre*weight+128)/256;
}

int nd_icon_hover_size(int tile_width,int tile_height,int tiny,int *width,int *height)
{
    int max_width=tile_width-8,max_height=tile_height-(tiny ? 8 : 24);
    int w,h;
    if (max_width>ND_HOVER_ICON_LIMIT) max_width=ND_HOVER_ICON_LIMIT;
    if (max_height>ND_HOVER_ICON_LIMIT) max_height=ND_HOVER_ICON_LIMIT;
    if (*width<=0 || *height<=0) return 0;
    nd_icon_fit(*width+*width/4,*height+*height/4,max_width,max_height,&w,&h);
    /* Do not shrink an icon that already occupies the available space. */
    if (w<*width || h<*height || (w==*width && h==*height)) return 0;
    *width=w;*height=h;
    return 1;
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
    {
        int grip=cfg->drag_bar ? ND_DRAG_BAR_SIZE:0;
        int vertical=cfg->side==ND_SIDE_LEFT || cfg->side==ND_SIDE_RIGHT;
        int ok=view_fit(&v,screen_width-(vertical ? 0:grip),
                         screen_height-(vertical ? grip:0),bar_height,layout);
        if(ok){if(vertical)layout->height+=grip;else layout->width+=grip;}
        return ok;
    }
}

int nd_layout_fit_enabled(const struct NDConfig *cfg, int screen_width,
                          int screen_height, int bar_height,
                          struct NDLayout *layout)
{
    struct LView v;
    if (!view_enabled(&v,cfg)) return 0;
    {
        int grip=cfg->drag_bar ? ND_DRAG_BAR_SIZE:0;
        int vertical=cfg->side==ND_SIDE_LEFT || cfg->side==ND_SIDE_RIGHT;
        int ok=view_fit(&v,screen_width-(vertical ? 0:grip),
                         screen_height-(vertical ? grip:0),bar_height,layout);
        if(ok){if(vertical)layout->height+=grip;else layout->width+=grip;}
        return ok;
    }
}

/* Clamp before adding so even extreme mouse deltas cannot overflow. */
int nd_drag_offset(int origin,int delta,int length,int dock_length)
{
    int travel;
    if(length<=0 || dock_length<=0)return 0;
    travel=length>dock_length ? length-dock_length:0;
    if (origin<0) origin=0;if (origin>travel) origin=travel;
    if (delta < -origin) return 0;
    if (delta > travel-origin) return travel;
    return origin+delta;
}

/* Snap the proposed dock origin, not the mouse's absolute position: the
 * leading grip of a wide centered dock may already be near the left edge. */
int nd_drag_snap(int position,int delta,int length,int dock_length)
{
    int travel=length-dock_length-2*ND_LAYOUT_MARGIN,origin;
    if(travel<=0)return position;
    origin=position==ND_POSITION_START ? 0:position==ND_POSITION_END ? travel:travel/2;
    origin+=delta;
    if(origin<travel/4)return ND_POSITION_START;
    if(origin>travel-travel/4)return ND_POSITION_END;
    return ND_POSITION_CENTER;
}
