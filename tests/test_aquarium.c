#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_aquarium_art.h"
static unsigned char canvas[160][320],before[160][320];static int width,height,calls;
static void box(void *ctx,int c,int x,int y,int w,int h){int a,b;(void)ctx;assert(c>=0&&c<16&&x>=0&&y>=0&&w>0&&h>0&&x+w<=width&&y+h<=height);++calls;for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)canvas[b][a]=(unsigned char)c;}
int main(void){unsigned long frame;int fish;width=320;height=160;
 for(frame=0;frame<65520UL;frame+=7){calls=0;nd_aquarium_scene(NULL,box,frame,(int)(frame%8)+1,(int)(frame&1UL));assert(calls<300);}
 for(fish=1;fish<=8;++fish){memset(canvas,0,sizeof(canvas));nd_aquarium_scene(NULL,box,0,fish,1);memcpy(before,canvas,sizeof(canvas));nd_aquarium_scene(NULL,box,5,fish,1);assert(memcmp(before,canvas,sizeof(canvas)));}
 width=height=32;for(frame=0;frame<35;++frame){calls=0;nd_aquarium_tile(NULL,box,frame);assert(calls<25);}assert(nd_aquarium_rgb[0]);puts("AQUARIUM: complete motion cycle, tile clipping and bounded rendering passed.");return 0;}
