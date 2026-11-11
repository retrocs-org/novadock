#include "nd_icon_mask.h"

int nd_icon_remove_border_background(const unsigned char *pens,
                                     unsigned char *opaque,
                                     int width,int height,
                                     unsigned long *queue,
                                     unsigned long queue_count)
{
    unsigned int counts[256];
    unsigned char candidate[256];
    unsigned long needed,head=0,tail=0,index,next;
    int x,y,dx,dy,threshold,border=0,removed=0;
    if (!pens || !opaque || !queue || width<1 || height<1) return 0;
    needed=(unsigned long)width*(unsigned long)height;
    if (needed>queue_count) return 0;
    for (x=0;x<256;++x) { counts[x]=0;candidate[x]=0; }
    for (y=0;y<height;++y) {
        for (x=0;x<width;++x) {
            if (x!=0 && y!=0 && x!=width-1 && y!=height-1) continue;
            index=(unsigned long)y*(unsigned long)width+(unsigned long)x;
            if (opaque[index]) { ++counts[pens[index]];++border; }
        }
    }
    if (!border) return 0;
    threshold=border/8;
    if (threshold<2) threshold=2;
    for (x=0;x<256;++x)
        if ((int)counts[x]>=threshold) candidate[x]=1;
    for (y=0;y<height;++y) {
        for (x=0;x<width;++x) {
            if (x!=0 && y!=0 && x!=width-1 && y!=height-1) continue;
            index=(unsigned long)y*(unsigned long)width+(unsigned long)x;
            if (opaque[index] && candidate[pens[index]]) {
                opaque[index]=0;queue[tail++]=index;++removed;
            }
        }
    }
    while (head<tail) {
        index=queue[head++];
        x=(int)(index%(unsigned long)width);
        y=(int)(index/(unsigned long)width);
        for (dy=-1;dy<=1;++dy) for (dx=-1;dx<=1;++dx) {
            if ((!dx && !dy) || x+dx<0 || x+dx>=width ||
                y+dy<0 || y+dy>=height) continue;
            next=(unsigned long)(y+dy)*(unsigned long)width+
                 (unsigned long)(x+dx);
            if (opaque[next] && candidate[pens[next]]) {
                opaque[next]=0;queue[tail++]=next;++removed;
            }
        }
    }
    return removed;
}

void nd_icon_art_box(int tile_width,int tile_height,int tiny,int *width,int *height)
{
    int w=tile_width-(tiny ? 2 : 14),h=tile_height-(tiny ? 2 : 24);
    if(!tiny){if(w>48)w=48;if(h>40)h=40;}
    *width=w>0 ? w : 1;*height=h>0 ? h : 1;
}
int nd_icon_art_top(int tile_y,int tile_height,int image_height,int tiny)
{
    int top;
    if(tiny)return tile_y+(tile_height-image_height)/2;
    top=tile_y+3+(tile_height-24-image_height)/2;
    return top<tile_y+2 ? tile_y+2 : top;
}
