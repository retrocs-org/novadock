#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_screen_art.h"
static unsigned char pixels[32][32];
static void box(void *ctx,int color,int x,int y,int w,int h)
{
    int a,b;(void)ctx;assert(color>=0 && color<=4 && x>=0 && y>=0 && w>=0 && h>0 && x+w<=32 && y+h<=32);
    for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)pixels[b][a]=(unsigned char)color;
}
int main(void)
{
    unsigned char previous[32][32];int i;
    assert(nd_shot_art_state("READY",1)==ND_SHOT_IDLE);
    assert(nd_shot_art_state("IN 2",1)==ND_SHOT_COUNTDOWN);
    assert(nd_shot_art_state("CAPTURING",1)==ND_SHOT_CAPTURE);
    assert(nd_shot_art_state("SAVING 40%",1)==ND_SHOT_CAPTURE);
    assert(nd_shot_art_state("SAVED",1)==ND_SHOT_SAVED);
    assert(nd_shot_art_state("WRITE FAILED",1)==ND_SHOT_ERROR);
    assert(nd_shot_art_state("CANCELLED",1)==ND_SHOT_CANCELLED);
    assert(nd_shot_art_state("OFFLINE",0)==ND_SHOT_IDLE);
    memset(previous,0,sizeof(previous));
    for(i=0;i<=ND_SHOT_ERROR;++i){memset(pixels,0,sizeof(pixels));nd_camera_art(NULL,box,i);
        assert(memcmp(previous,pixels,sizeof(pixels)));memcpy(previous,pixels,sizeof(pixels));}
    memset(pixels,0,sizeof(pixels));nd_modes_art(NULL,box);assert(memcmp(previous,pixels,sizeof(pixels)));
    puts("Camera state transitions and monitor artwork are distinct and bounded.");return 0;
}
