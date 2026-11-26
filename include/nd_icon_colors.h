#ifndef ND_ICON_COLORS_H
#define ND_ICON_COLORS_H
/* Shared by the live dock and Appearance. Inks: dark, case, light, accents. */
#define ND_ICON_INKS 5
static int nd_icon_luma(const unsigned char *c)
{return c[0]*3+c[1]*6+c[2];}
static void nd_icon_palette(const unsigned char theme[8][3],int colourful,unsigned char out[5][3])
{
    static const unsigned char stock[5][3]={{36,44,57},{124,157,186},{232,239,245},{48,172,226},{70,204,146}};
    int i,c,dark=0,light=3;
    if(colourful){for(i=0;i<5;++i)for(c=0;c<3;++c)out[i][c]=stock[i][c];return;}
    /* TEXT can be black in light themes. Do not use it for the illuminated
     * face of an icon, or use a bright accent for its dark display. */
    if(nd_icon_luma(theme[6])<nd_icon_luma(theme[dark]))dark=6;
    if(nd_icon_luma(theme[3])<nd_icon_luma(theme[dark]))dark=3;
    if(nd_icon_luma(theme[1])>nd_icon_luma(theme[light]))light=1;
    if(nd_icon_luma(theme[2])>nd_icon_luma(theme[light]))light=2;
    for(c=0;c<3;++c){out[0][c]=theme[dark][c];out[1][c]=theme[4][c];out[2][c]=theme[light][c];out[3][c]=theme[5][c];out[4][c]=theme[7][c];}
}
/* Each live metric keeps a recognizable material in Colourful mode. */
static void nd_metric_palette(const unsigned char theme[8][3],int colourful,int kind,unsigned char out[5][3])
{
    static const unsigned char stock[4][5][3]={
        {{15,48,32},{42,174,91},{233,255,234},{250,198,60},{103,226,140}},
        {{16,53,35},{46,162,88},{224,255,236},{241,190,60},{56,220,143}},
        {{55,31,15},{236,145,42},{255,246,214},{251,205,70},{64,214,172}},
        {{15,36,65},{47,135,226},{229,249,255},{66,225,166},{255,170,61}}
    };
    int i,c;
    if(!colourful || kind<1 || kind>4){nd_icon_palette(theme,colourful,out);return;}
    for(i=0;i<5;++i)for(c=0;c<3;++c)out[i][c]=stock[kind-1][i][c];
}
#endif
