#ifndef ND_WEATHER_ART_H
#define ND_WEATHER_ART_H
/* Static pixel art uses the dock palette. No bitmap loads or animation timers. */
typedef void (*NDWeatherRect)(void *,int,int,int,int,int);
#define ND_WX_UNKNOWN 0
#define ND_WX_SUN 1
#define ND_WX_MAINLY_CLEAR 2
#define ND_WX_PARTLY_CLOUDY 3
#define ND_WX_CLOUD 4
#define ND_WX_FOG 5
#define ND_WX_DRIZZLE 6
#define ND_WX_RAIN 7
#define ND_WX_SNOW 8
#define ND_WX_SLEET 9
#define ND_WX_STORM 10
static int nd_weather_art_state(int code)
{
    if(code==0)return ND_WX_SUN;
    if(code==1)return ND_WX_MAINLY_CLEAR;
    if(code==2)return ND_WX_PARTLY_CLOUDY;
    if(code==3)return ND_WX_CLOUD;
    if(code==45 || code==48)return ND_WX_FOG;
    if(code==51 || code==53 || code==55)return ND_WX_DRIZZLE;
    if(code==56 || code==57 || code==66 || code==67)return ND_WX_SLEET;
    if(code==61 || code==63 || code==65 || (code>=80 && code<=82))return ND_WX_RAIN;
    if(code==71 || code==73 || code==75 || code==77 || code==85 || code==86)return ND_WX_SNOW;
    if(code==95 || code==96 || code==99)return ND_WX_STORM;
    return ND_WX_UNKNOWN;
}
static void nd_weather_sun(void *ctx,NDWeatherRect box,int x,int y)
{
    int i;
    box(ctx,4,x-1,y-11,2,3);box(ctx,4,x-1,y+9,2,3);
    box(ctx,4,x-11,y-1,3,2);box(ctx,4,x+9,y-1,3,2);
    for(i=0;i<2;++i){box(ctx,4,x-8+i,y-8+i,2,2);box(ctx,4,x+6+i,y-8+i,2,2);
        box(ctx,4,x-8+i,y+6+i,2,2);box(ctx,4,x+6+i,y+6+i,2,2);}
    box(ctx,0,x-4,y-7,8,14);box(ctx,0,x-7,y-4,14,8);box(ctx,0,x-6,y-6,12,12);
    box(ctx,4,x-4,y-6,8,12);box(ctx,4,x-6,y-4,12,8);box(ctx,4,x-5,y-5,10,10);
    box(ctx,2,x-3,y-4,4,1);box(ctx,2,x-4,y-3,1,3);
}
static void nd_weather_cloud(void *ctx,NDWeatherRect box,int x,int y,int front)
{
    int fill=front ? 2:1;
    box(ctx,0,x+5,y,8,2);box(ctx,0,x+3,y+2,16,4);
    box(ctx,0,x+1,y+5,21,8);box(ctx,0,x+3,y+13,18,2);
    box(ctx,1,x+5,y+1,8,4);box(ctx,1,x+3,y+4,16,5);box(ctx,1,x+2,y+6,19,7);
    box(ctx,fill,x+6,y+2,6,5);box(ctx,fill,x+4,y+5,14,5);box(ctx,fill,x+3,y+7,16,5);
    box(ctx,3,x+3,y+12,17,1);box(ctx,1,x+4,y+13,15,1);
}
static void nd_weather_flake(void *ctx,NDWeatherRect box,int x,int y)
{
    box(ctx,2,x,y-2,1,5);box(ctx,2,x-2,y,5,1);
    box(ctx,2,x-1,y-1,3,3);box(ctx,3,x,y,1,1);
}
static void nd_weather_art(void *ctx,NDWeatherRect box,int state)
{
    int i;
    if(state==ND_WX_SUN){nd_weather_sun(ctx,box,16,15);return;}
    if(state==ND_WX_MAINLY_CLEAR || state==ND_WX_PARTLY_CLOUDY){
        nd_weather_sun(ctx,box,11,11);
        nd_weather_cloud(ctx,box,state==ND_WX_MAINLY_CLEAR ? 9:6,state==ND_WX_MAINLY_CLEAR ? 16:12,1);
        return;
    }
    if(state==ND_WX_CLOUD)nd_weather_cloud(ctx,box,1,3,0);
    nd_weather_cloud(ctx,box,state==ND_WX_CLOUD ? 7:4,state==ND_WX_CLOUD ? 10:7,1);
    if(state==ND_WX_DRIZZLE){for(i=0;i<3;++i){box(ctx,3,8+i*7,25,1,2);box(ctx,3,6+i*7,29,1,2);}}
    else if(state==ND_WX_RAIN){for(i=0;i<3;++i){box(ctx,3,9+i*7,24,2,3);box(ctx,3,8+i*7,27,2,3);box(ctx,2,9+i*7,25,1,1);}}
    else if(state==ND_WX_SNOW){nd_weather_flake(ctx,box,8,27);nd_weather_flake(ctx,box,16,29);nd_weather_flake(ctx,box,24,27);}
    else if(state==ND_WX_SLEET){nd_weather_flake(ctx,box,8,27);nd_weather_flake(ctx,box,24,27);box(ctx,3,16,25,2,5);}
    else if(state==ND_WX_FOG){box(ctx,3,3,24,25,2);box(ctx,1,6,28,24,2);box(ctx,2,3,29,3,1);}
    else if(state==ND_WX_STORM){box(ctx,4,17,22,6,3);box(ctx,4,14,25,7,3);box(ctx,4,17,28,3,2);box(ctx,4,16,30,3,2);box(ctx,3,7,25,2,4);}
    else if(state==ND_WX_UNKNOWN){box(ctx,0,13,10,8,11);box(ctx,3,15,11,4,1);box(ctx,3,18,12,2,3);box(ctx,3,16,15,3,1);box(ctx,3,16,16,1,1);box(ctx,3,16,19,1,1);}
}
#endif
