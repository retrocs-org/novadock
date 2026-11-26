#ifndef ND_HOVER_DIRTY_H
#define ND_HOVER_DIRTY_H
/* Which part of the magnification frame must be recomposed after a pointer
 * move.  Tiles whose rectangle and content are unchanged composite to the
 * same pixels, so only the span (along the dock axis) covered by a moved,
 * resized or repainted tile - at its old and its new position - and by a
 * changed shelf outline is repainted and presented.  Across the dock the
 * span always covers the full depth, which takes in labels, running dots
 * and badges drawn beside the artwork.  No AmigaOS headers. */
#include "nd_autohide.h"
#if defined(__GNUC__)
#define ND_HOVER_DIRTY static __attribute__((unused))
#else
#define ND_HOVER_DIRTY static
#endif
ND_HOVER_DIRTY void nd_span_add(int *lo,int *hi,int start,int length)
{
    if (length<=0) return;
    if (start<*lo) *lo=start;
    if (start+length>*hi) *hi=start+length;
}
ND_HOVER_DIRTY int nd_rect_same(const struct NDRect *a,const struct NDRect *b)
{
    return a->left==b->left && a->top==b->top && a->width==b->width && a->height==b->height;
}
/* Extents carry this safety margin on every side for the redrawn span; the
 * overlap test for redrawing neighbours uses the exact drawn bounds. */
#define ND_HOVER_EXT_MARGIN 2
/* Returns 1 and the span [*lo,*hi) when something changed, else 0.  BOX is
 * each tile's artwork rectangle; EXT is everything drawn for the tile
 * (artwork, label, running dot, meter and badges), which can be wider than
 * the artwork.  A visible tile counts as changed when its box or extent
 * moved or CHANGED marks it (repainted content, hover order); the span then
 * covers its old and new extent so no decoration is left behind.  Tiles
 * scrolled out of view (VISIBLE 0) are ignored. */
ND_HOVER_DIRTY int nd_hover_dirty_span(const struct NDRect *prev_box,const struct NDRect *cur_box,
                                    const struct NDRect *prev_ext,const struct NDRect *cur_ext,
                                    const unsigned char *changed,const unsigned char *visible,
                                    int count,int vertical,
                                    const struct NDRect *prev_shelf,const struct NDRect *cur_shelf,
                                    int limit,int *lo,int *hi)
{
    int i;
    *lo=limit;*hi=0;
    for (i=0;i<count;++i) {
        if (!visible[i]) continue;
        if (!changed[i] && nd_rect_same(&prev_box[i],&cur_box[i]) &&
            nd_rect_same(&prev_ext[i],&cur_ext[i])) continue;
        if (vertical) {
            nd_span_add(lo,hi,prev_ext[i].top,prev_ext[i].height);
            nd_span_add(lo,hi,cur_ext[i].top,cur_ext[i].height);
        } else {
            nd_span_add(lo,hi,prev_ext[i].left,prev_ext[i].width);
            nd_span_add(lo,hi,cur_ext[i].left,cur_ext[i].width);
        }
    }
    if (!nd_rect_same(prev_shelf,cur_shelf)) {
        if (vertical) {
            nd_span_add(lo,hi,prev_shelf->top,prev_shelf->height);
            nd_span_add(lo,hi,cur_shelf->top,cur_shelf->height);
        } else {
            nd_span_add(lo,hi,prev_shelf->left,prev_shelf->width);
            nd_span_add(lo,hi,cur_shelf->left,cur_shelf->width);
        }
    }
    if (*lo<0) *lo=0;
    if (*hi>limit) *hi=limit;
    return *hi>*lo;
}
/* Does a tile's extent reach into the span (along the dock axis)? */
ND_HOVER_DIRTY int nd_span_touches(const struct NDRect *ext,int vertical,int lo,int hi)
{
    int start=vertical ? ext->top : ext->left,length=vertical ? ext->height : ext->width;
    return length>0 && start<hi && start+length>lo;
}
/* Do the exact drawn bounds of two tiles (extents without the margin) overlap? */
ND_HOVER_DIRTY int nd_ext_overlap(const struct NDRect *a,const struct NDRect *b)
{
    const int m=ND_HOVER_EXT_MARGIN;
    return a->left+m<b->left+b->width-m && b->left+m<a->left+a->width-m &&
           a->top+m<b->top+b->height-m && b->top+m<a->top+a->height-m;
}
/* The tiles to redraw for the span [lo,hi), in one pass in drawing order
 * (every tile by index, the hovered tile last): a visible tile is redrawn
 * when it reaches into the span, or when it overlaps a tile already chosen
 * earlier in that order - that tile's redraw also repaints its parts outside
 * the span, so anything drawn after it on top must follow again.  A tile
 * drawn earlier never needs it: the chosen tile was on top of it anyway.
 * Other tiles keep their pixels.  Returns the number chosen. */
ND_HOVER_DIRTY int nd_hover_draw_set(const struct NDRect *ext,const unsigned char *visible,int count,
                                  int vertical,int lo,int hi,int hovered,unsigned char *draw)
{
    int step,i,j,chosen=0,order[64],n=0;
    if (count>64) count=64;
    for (i=0;i<count;++i) if (i!=hovered) order[n++]=i;
    if (hovered>=0 && hovered<count) order[n++]=hovered;
    for (i=0;i<count;++i) draw[i]=0;
    for (step=0;step<n;++step) {
        i=order[step];
        if (!visible[i]) continue;
        if (nd_span_touches(&ext[i],vertical,lo,hi)) draw[i]=1;
        else for (j=0;j<step;++j)
            if (draw[order[j]] && nd_ext_overlap(&ext[i],&ext[order[j]])) {draw[i]=1;break;}
        chosen+=draw[i];
    }
    return chosen;
}
#endif
