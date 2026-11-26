#ifndef ND_WALLPAPER_H
#define ND_WALLPAPER_H
/* Wallpaper-matched dock colours (DOCK_BACKGROUND=WALLPAPER): the dock's
 * solid shelf, tiles, hover and text colours are derived from the average
 * colour of the desktop behind the dock, so the shelf blends with any
 * Workbench pattern or picture.  Accents keep the theme's colours.  Text is
 * near-black on light results and near-white on dark ones for contrast.
 * Portable: indices as in dock_internal.h (ND_BG .. ND_ACCENT2). */
#if defined(__GNUC__)
#define ND_WALLPAPER static __attribute__((unused))
#else
#define ND_WALLPAPER static
#endif
static int nd_wp_luma(const unsigned char c[3])
{
    return (299*c[0]+587*c[1]+114*c[2])/1000;
}
/* Move C toward black (AMOUNT>0, per cent) or white (AMOUNT<0). */
static void nd_wp_shade(const unsigned char c[3],int amount,unsigned char out[3])
{
    int i;
    for (i=0;i<3;++i) {
        int v=amount>=0 ? c[i]-c[i]*amount/100 : c[i]+(255-c[i])*(-amount)/100;
        out[i]=(unsigned char)(v<0 ? 0 : v>255 ? 255 : v);
    }
}
ND_WALLPAPER void nd_wallpaper_palette(const unsigned char average[3],
                                    const unsigned char theme[8][3],
                                    unsigned char out[8][3])
{
    int light=nd_wp_luma(average)>=128,i;
    int away=light ? 1 : -1;          /* shade direction away from the desktop */
    for (i=0;i<8;++i) {out[i][0]=theme[i][0];out[i][1]=theme[i][1];out[i][2]=theme[i][2];}
    nd_wp_shade(average,away*18,out[0]);            /* ND_BG: the shelf */
    nd_wp_shade(average,away*30,out[1]);            /* ND_TILE */
    nd_wp_shade(average,away*42,out[2]);            /* ND_HOVER */
    if (nd_wp_luma(out[0])>=128) {out[3][0]=out[3][1]=out[3][2]=16;}   /* ND_TEXT */
    else {out[3][0]=out[3][1]=out[3][2]=240;}
    for (i=0;i<3;++i) out[4][i]=(unsigned char)((out[3][i]+out[0][i])/2);   /* ND_SUBTLE */
    nd_wp_shade(average,-away*12,out[6]);           /* ND_BAR_BG: the other way */
}
#endif
