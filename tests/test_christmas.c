#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_christmas_art.h"
static unsigned char pixels[8][32][32];static int calls;
static void box(void *data,int color,int x,int y,int w,int h)
{unsigned char (*p)[32]=data;int a,b;++calls;assert(color>=0 && color<8 && x>=0 && y>=0 && w>0 && h>0 && x+w<=32 && y+h<=32);for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)p[b][a]=(unsigned char)(color+1);}
int main(void)
{int i,pattern;unsigned char steady[32][32],dark[32][32];
 assert(nd_christmas_delay(0,0)==50 && nd_christmas_delay(1,0)==25 && nd_christmas_delay(2,0)==15);
 assert(nd_christmas_delay(2,1)==500);
 for(pattern=0;pattern<2;++pattern){for(i=0;i<8;++i){memset(pixels[i],0,sizeof(pixels[i]));calls=0;nd_christmas_art(pixels[i],box,i,pattern,1);assert(calls<55);}
  assert(memcmp(pixels[0],pixels[1],sizeof(pixels[0])));}
 memset(steady,0,sizeof(steady));memset(dark,0,sizeof(dark));nd_christmas_art(steady,box,0,2,1);nd_christmas_art(dark,box,0,2,0);assert(memcmp(steady,dark,sizeof(steady)));
 puts("Christmas tree: bounded native drawing, distinct light frames, lights off and low-rate hidden pacing passed.");return 0;}
