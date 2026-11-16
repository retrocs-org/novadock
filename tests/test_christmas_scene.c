#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_christmas_scene.h"
static unsigned char canvas[ND_XMAS_HEIGHT][ND_XMAS_WIDTH];
static unsigned char previous[ND_XMAS_HEIGHT][ND_XMAS_WIDTH];
static int calls,render;
static void box(void *context,int color,int x,int y,int w,int h)
{
 int a,b;(void)context;++calls;
 assert(color>=0 && color<ND_XMAS_COLORS && x>=0 && y>=0 && w>0 && h>0);
 assert(x+w<=ND_XMAS_WIDTH && y+h<=ND_XMAS_HEIGHT);
 if(render)for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)canvas[b][a]=(unsigned char)color;
}
int main(int argc,char **argv)
{
 unsigned long f,ticks=0;int x,y,c;FILE *file;char path[512];
 assert(nd_christmas_delay(0,0)==50 && nd_christmas_delay(1,0)==25 && nd_christmas_delay(2,0)==15);
 for(f=0;f<3520UL;++f){calls=0;nd_xmas_scene(NULL,box,f,(int)(f%3UL),(int)(f%2UL));assert(calls<500);}
 render=1;nd_xmas_scene(NULL,box,0,0,1);memcpy(previous,canvas,sizeof(canvas));
 nd_xmas_scene(NULL,box,1,0,1);assert(memcmp(previous,canvas,sizeof(canvas)));
 for(x=0;x<24;++x)assert(nd_xmas_advance(&ticks,1,25)==0);
 assert(nd_xmas_advance(&ticks,1,25)==1 && ticks==0);
 assert(nd_xmas_advance(&ticks,60,25)==2 && ticks==10);
 if(argc>1)for(f=0;f<110UL;++f){
  nd_xmas_scene(NULL,box,f,0,1);sprintf(path,"%s/frame-%03lu.ppm",argv[1],f);
  file=fopen(path,"wb");assert(file);fprintf(file,"P6\n320 160\n255\n");
  for(y=0;y<ND_XMAS_HEIGHT;++y)for(x=0;x<ND_XMAS_WIDTH;++x){
   c=canvas[y][x];fputc((int)(nd_xmas_rgb[c]>>16)&255,file);fputc((int)(nd_xmas_rgb[c]>>8)&255,file);fputc((int)nd_xmas_rgb[c]&255,file);
  }assert(!fclose(file));
 }
 puts("Christmas scene: complete sleigh cycle stays clipped, distinct snow/light frames, bounded drawing and elapsed-time pacing passed.");return 0;
}
