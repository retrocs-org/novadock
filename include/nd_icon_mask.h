#ifndef ND_ICON_MASK_H
#define ND_ICON_MASK_H

/* Remove only background-colour pixels connected to an image border.  This
 * preserves identical pens enclosed inside the artwork, which is important
 * for dithered classic Workbench icons.  `opaque` contains one byte per pixel
 * and is updated in place.  The caller supplies width*height queue entries. */
int nd_icon_remove_border_background(const unsigned char *pens,
                                     unsigned char *opaque,
                                     int width,int height,
                                     unsigned long *queue,
                                     unsigned long queue_count);

/* Shared artwork area for launcher and launcher-style plugin tiles. */
void nd_icon_art_box(int tile_width,int tile_height,int tiny,int *width,int *height);
int nd_icon_art_top(int tile_y,int tile_height,int image_height,int tiny);
#endif
