#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_weather_art.h"
static unsigned char pixels[11][32][32];
static void box(void *ctx,int color,int x,int y,int w,int h)
{
    unsigned char (*image)[32]=ctx;int a,b;
    assert(color>=0 && color<5 && x>=0 && y>=0 && w>0 && h>0 && x+w<=32 && y+h<=32);
    for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)image[b][a]=(unsigned char)color;
}
int main(void)
{
    int a,b;
    assert(nd_weather_art_state(0)==ND_WX_SUN);
    assert(nd_weather_art_state(2)==ND_WX_PARTLY_CLOUDY);
    assert(nd_weather_art_state(45)==ND_WX_FOG);
    assert(nd_weather_art_state(51)==ND_WX_DRIZZLE);
    assert(nd_weather_art_state(65)==ND_WX_RAIN);
    assert(nd_weather_art_state(67)==ND_WX_SLEET);
    assert(nd_weather_art_state(71)==ND_WX_SNOW);
    assert(nd_weather_art_state(81)==ND_WX_RAIN);
    assert(nd_weather_art_state(85)==ND_WX_SNOW);
    assert(nd_weather_art_state(95)==ND_WX_STORM);
    assert(nd_weather_art_state(-1)==ND_WX_UNKNOWN && nd_weather_art_state(999)==ND_WX_UNKNOWN);
    for(a=0;a<11;++a){nd_weather_art(pixels[a],box,a);
        for(b=0;b<a;++b)assert(memcmp(pixels[a],pixels[b],sizeof(pixels[a])));}
    puts("Weather artwork: clear, cloud, fog, rain, snow, sleet and storm states are distinct and bounded.");return 0;
}
