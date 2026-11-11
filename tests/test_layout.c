#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "populated_dock.h"
#include "nd_layout.h"
int main(void)
{
    struct NDConfig c;
    struct NDLayout l;
    int width, height, row, column, span;
    populated_dock(&c);
    assert(c.count == 12);
    assert(nd_compact_enabled(&c)==10);
    assert(nd_layout_fit(&c, 640, 200, 11, &l));
    assert(l.tile_width == 55);
    assert(l.width == 609 && l.height == 90);
    assert(l.overflow && l.visible_count == 7 && l.slots == 10);
    assert(nd_layout_fit(&c, 1024, 768, 11, &l));
    assert(l.tile_width == 86 && l.width == 1010);
    /* Music spans two cells. The final tile must still be addressable
     * when an edit captures the automatic layout as a manual row. */
    assert(l.columns==c.count+1);
    c.row_layout=1;c.row_counts[0]=c.count;
    assert(nd_layout_item_cell(&c,c.count-1,l.columns,&row,&column,&span));
    assert(row==0 && column+span==l.columns);
    c.row_layout=0;

    assert(nd_layout_fit(&c, 320, 200, 11, &l));
    assert(l.overflow && l.visible_count == 2 && l.slots == 5);
    assert(l.tile_width == 55 && l.width == 309);
    assert(c.tile_width == 86); /* no mutations to actual config */
    c.item_size=ND_ITEM_SIZE_TINY;
    assert(nd_layout_fit(&c,640,200,11,&l));
    assert(!l.overflow && l.visible_count==10);
    assert(l.tile_width==43 && l.tile_height==38 && l.gap==3);
    assert(l.width<500); /* ten tiny items fit where normal items page */
    assert(nd_layout_fit(&c,320,200,11,&l));
    assert(l.visible_count>2); /* tiny paging exposes more items per page */
    c.item_size=ND_ITEM_SIZE_NORMAL;
    c.rows=2;
    assert(nd_layout_fit(&c,640,200,11,&l));
    assert(l.rows==2 && l.columns==6);
    assert(l.width<=630 && l.height<=179);
    assert(!l.overflow && l.visible_count==10);
    assert(nd_layout_item_cell(&c,5,l.columns,&row,&column,&span));
    assert(row==0 && column==5 && span==1);
    assert(nd_layout_item_cell(&c,7,l.columns,&row,&column,&span));
    assert(row==1 && column==1 && span==2); /* wide music tile stays intact */
    assert(nd_layout_row_used_cells(&c,0,l.columns)==6);
    assert(nd_layout_row_used_cells(&c,1,l.columns)==5);
    assert(!nd_layout_item_cell(&c,7,0,&row,&column,&span));
    c.rows=3;
    assert(nd_layout_fit(&c,640,200,11,&l));
    assert(l.rows==3 && l.columns==4);
    assert(l.tile_height<76); /* rows shrink vertically when required */
    c.rows=1;
    c.side = ND_SIDE_LEFT;
    assert(nd_layout_fit(&c, 640, 200, 11, &l));
    assert(l.overflow && l.visible_count == 1 && l.slots == 3);
    assert(nd_layout_fit(&c, 1024, 768, 11, &l));
    assert(l.tile_height == 68 && l.height == 739);
    c.count = 12;
    c.side = ND_SIDE_BOTTOM;
    assert(nd_layout_fit(&c, 640, 200, 11, &l));
    assert(l.overflow && l.visible_count == 7 && l.slots == 10);
    c.count = 13;
    assert(nd_layout_fit(&c, 1024, 768, 11, &l));
    c.count = ND_MAX_DOCK_ITEMS;
    assert(nd_layout_fit(&c, 1024, 768, 11, &l));
    assert(l.overflow);
    c.count = ND_MAX_DOCK_ITEMS + 1;
    assert(!nd_layout_fit(&c, 1024, 768, 11, &l));

    populated_dock(&c);
    c.count=1;
    assert(nd_add_spacer(&c,"Gap"));
    assert(nd_layout_fit(&c,1024,768,11,&l));
    assert(l.width==148); /* border + tile + gap + half-width spacer */
    c.items[1].spacer_width=30;
    assert(nd_layout_fit(&c,1024,768,11,&l));
    assert(l.width==135);

    populated_dock(&c);
    c.count=1;
    c.items[0].kind=ND_ITEM_PLUGIN;
    strcpy(c.items[0].id,"MUSIC");
    assert(nd_layout_fit(&c,320,200,11,&l));
    assert(!l.overflow && l.tile_width==86 && l.width==191);

    c.item_size=ND_ITEM_SIZE_TINY;
    assert(nd_layout_fit(&c,320,200,11,&l));
    assert(!l.overflow && l.tile_width==43 && l.width==57);
    /* Music drops the two-slot transport panel in compact mode. */
    assert(nd_layout_item_cell(&c,0,l.columns,&row,&column,&span));
    assert(span==1);

    c.item_size=ND_ITEM_SIZE_NORMAL;
    c.rows=2;
    assert(nd_layout_fit(&c,320,200,11,&l));
    assert(l.rows==2 && l.columns==2); /* configured empty row remains visible */

    /* A short second row reports only its occupied cells.  The Amiga
     * renderer converts the missing-cell width to a half-width row offset. */
    populated_dock(&c);
    c.count=8;
    c.rows=2;
    assert(nd_layout_fit(&c,1024,768,11,&l));
    assert(l.rows==2 && l.columns==5);
    assert(nd_layout_row_used_cells(&c,0,l.columns)==5);
    assert(nd_layout_row_used_cells(&c,1,l.columns)==4);
    assert(nd_layout_row_used_cells(&c,2,l.columns)==0);
    assert(nd_layout_row_used_cells(&c,-1,l.columns)==0);

    /* Manual row membership keeps uneven and empty rows instead of
     * rebalancing them. The frame follows the widest actual row. */
    c.row_layout=1;
    c.row_counts[0]=3;
    c.row_counts[1]=5;
    c.row_counts[2]=0;
    assert(nd_layout_fit(&c,1024,768,11,&l));
    assert(l.rows==2 && l.columns==6);
    assert(nd_layout_row_used_cells(&c,0,l.columns)==3);
    assert(nd_layout_row_used_cells(&c,1,l.columns)==6);
    assert(nd_layout_item_cell(&c,3,l.columns,&row,&column,&span));
    assert(row==1 && column==0);
    c.row_counts[0]=7;
    c.row_counts[1]=1;
    assert(nd_layout_fit(&c,1024,768,11,&l));
    assert(l.rows==2 && l.columns==7);
    assert(nd_layout_row_used_cells(&c,1,l.columns)==2); /* one wide item */
    c.row_counts[0]=8;
    c.row_counts[1]=0;
    assert(nd_layout_fit(&c,1024,768,11,&l));
    assert(l.rows==2 && nd_layout_row_used_cells(&c,1,l.columns)==0);

    nd_icon_fit(56, 64, 72, 41, &width, &height);
    assert(width == 35 && height == 41);
    nd_icon_fit(32, 32, 72, 41, &width, &height);
    assert(width == 32 && height == 32);
    nd_icon_fit(100, 50, 50, 50, &width, &height);
    assert(width == 50 && height == 25);
    nd_icon_fit(0, 50, 50, 50, &width, &height);
    assert(width == 0 && height == 0);
    puts("Responsive dock layout tests passed.");
    return 0;
}
