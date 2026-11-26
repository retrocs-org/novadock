#ifndef ND_ICON_RECTS_H
#define ND_ICON_RECTS_H
/* Cover the opaque pixels of a 32x32 pen map with few solid rectangles.
 * Each horizontal run of one pen is extended downwards while the rows below
 * repeat it exactly, so flat areas cost one RectFill instead of one per row
 * (about 58 instead of 133 fills for the bundled icons).  Pure C89 with no
 * AmigaOS dependency, so the host tests can check exact coverage. */
#define ND_ICON_RECT_SIDE 32
#define ND_ICON_RECT_CLEAR (-1)
struct NDIconRect { unsigned char x0,y0,x1,y1; unsigned short pen; }; /* x1/y1 exclusive */
/* PX holds 32*32 pens, ND_ICON_RECT_CLEAR for transparent.  Writes at most
 * MAX rectangles to OUT (when OUT is NULL it only counts) and returns how
 * many the icon needs. */
static int nd_icon_rects(const long *px,struct NDIconRect *out,int max)
{
    static unsigned char used[ND_ICON_RECT_SIDE*ND_ICON_RECT_SIDE];  /* off the small task stack */
    int x,y,s,e,yy,q,n=0,match;
    long c;
    for (x=0;x<ND_ICON_RECT_SIDE*ND_ICON_RECT_SIDE;++x) used[x]=0;
    for (y=0;y<ND_ICON_RECT_SIDE;++y)
        for (x=0;x<ND_ICON_RECT_SIDE;) {
            c=px[y*ND_ICON_RECT_SIDE+x];
            if (used[y*ND_ICON_RECT_SIDE+x] || c==ND_ICON_RECT_CLEAR) {++x;continue;}
            s=x;
            while (x<ND_ICON_RECT_SIDE && px[y*ND_ICON_RECT_SIDE+x]==c &&
                   !used[y*ND_ICON_RECT_SIDE+x]) ++x;
            e=x;
            for (yy=y+1;yy<ND_ICON_RECT_SIDE;++yy) {
                match=1;
                for (q=s;q<e;++q)
                    if (px[yy*ND_ICON_RECT_SIDE+q]!=c || used[yy*ND_ICON_RECT_SIDE+q]) {match=0;break;}
                if (!match) break;
            }
            for (q=y;q<yy;++q) {int r;for (r=s;r<e;++r) used[q*ND_ICON_RECT_SIDE+r]=1;}
            if (out && n<max) {
                out[n].x0=(unsigned char)s;out[n].x1=(unsigned char)e;
                out[n].y0=(unsigned char)y;out[n].y1=(unsigned char)yy;
                out[n].pen=(unsigned short)c;
            }
            ++n;
        }
    return n;
}
#endif
