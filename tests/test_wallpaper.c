/* Wallpaper-matched colours: shelf, tiles and hover step away from the
 * desktop average, text keeps contrast, accents keep the theme. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "nd_wallpaper.h"
static int luma(const unsigned char c[3]){return nd_wp_luma(c);}
int main(void)
{
    unsigned char theme[8][3],out[8][3],avg[3];
    int r,g,b,i;
    for(i=0;i<8;++i){theme[i][0]=(unsigned char)(i*30);theme[i][1]=(unsigned char)(200-i*20);theme[i][2]=(unsigned char)(i*7);}
    for(r=0;r<256;r+=17)for(g=0;g<256;g+=17)for(b=0;b<256;b+=17){
        int light;
        avg[0]=(unsigned char)r;avg[1]=(unsigned char)g;avg[2]=(unsigned char)b;
        light=luma(avg)>=128;
        nd_wallpaper_palette(avg,(const unsigned char (*)[3])theme,out);
        /* Shelf, tile and hover move steadily away from the desktop. */
        if(light) assert(luma(out[0])<=luma(avg)&&luma(out[1])<=luma(out[0])&&luma(out[2])<=luma(out[1]));
        else assert(luma(out[0])>=luma(avg)&&luma(out[1])>=luma(out[0])&&luma(out[2])>=luma(out[1]));
        /* Text contrasts with the shelf by at least 100 luma steps. */
        assert(abs(luma(out[3])-luma(out[0]))>=100);
        /* Accents are the theme's own. */
        for(i=0;i<3;++i)assert(out[5][i]==theme[5][i]&&out[7][i]==theme[7][i]);
    }
    puts("Wallpaper colours: shelf, tiles and hover step away from any desktop colour; text keeps contrast; accents stay the theme's.");
    return 0;
}
