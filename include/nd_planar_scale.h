#ifndef ND_PLANAR_SCALE_H
#define ND_PLANAR_SCALE_H
/* Nearest-neighbour planar scaling. No graphics-library calls or allocation;
 * used for bounded widget caches when the standard raster scaler is unreliable. */
static int nd_planar_scale(unsigned char *const *src,int src_stride,int src_rows,
                          unsigned char *const *dst,int dst_stride,int dst_rows,
                          int depth,int x,int y,int w,int h,int zw,int zh)
{
    unsigned short columns[256];unsigned char masks[256];
    int plane,row,col,offset;
    if(depth<1||depth>8||x<0||y<0||w<1||h<1||zw<1||zw>256||zh<1||zh>256||
       x+w>src_stride*8||y+h>src_rows||zw>dst_stride*8||zh>dst_rows)return 0;
    for(plane=0;plane<depth;++plane)if(!src[plane]||!dst[plane])return 0;
    for(col=0;col<zw;++col){offset=x+(long)col*w/zw;columns[col]=(unsigned short)(offset/8);masks[col]=(unsigned char)(128>>(offset&7));}
    for(plane=0;plane<depth;++plane)for(row=0;row<zh;++row){
        const unsigned char *s=src[plane]+(long)(y+(long)row*h/zh)*src_stride;
        unsigned char *d=dst[plane]+(long)row*dst_stride;
        for(col=0;col<zw;col+=8){
            unsigned char bits=0,keep;int n,end=zw-col<8 ? zw-col:8;
            for(n=0;n<end;++n)if(s[columns[col+n]]&masks[col+n])bits|=(unsigned char)(128>>n);
            keep=(unsigned char)(end==8 ? 0:255>>end);
            d[col/8]=(unsigned char)((d[col/8]&keep)|bits);
        }
    }
    return 1;
}
#endif
