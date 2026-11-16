#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_saver_art.h"
#include "nd_saver_modes.h"
#include "nd_places_saver_art.h"
static unsigned char pixels[160][320],before[160][320];static int calls,width=320,height=160;
static void box(void *ctx,int c,int x,int y,int w,int h){int a,b;(void)ctx;assert(c>=0&&c<ND_SAVER_COLORS&&x>=0&&y>=0&&w>0&&h>0&&x+w<=width&&y+h<=height);++calls;for(b=y;b<y+h;++b)for(a=x;a<x+w;++a)pixels[b][a]=(unsigned char)c;}
int main(int argc,char **argv){unsigned long f;int mode,x,y,red,white;FILE *out;
 for(mode=0;mode<8;++mode)for(f=0;f<32760UL;f+=13){calls=0;nd_saver_scene(NULL,box,f,mode,0);assert(calls<=(mode==3 ? 600:mode>=4 ? 2500:65));if(mode==2)assert(calls==1);}
 assert(nd_saver_rgb[0]==0);nd_saver_scene(NULL,box,0,0,0);memcpy(before,pixels,sizeof(pixels));nd_saver_scene(NULL,box,9,0,0);assert(memcmp(before,pixels,sizeof(pixels)));
 nd_saver_scene(NULL,box,0,3,0);red=white=0;
 for(y=0;y<160;++y)for(x=0;x<320;++x){if(pixels[y][x]>=8&&pixels[y][x]<=10)++red;if(pixels[y][x]==2||pixels[y][x]>=11)++white;}
 assert(red>1500&&white>1500);
 if(argc>1){out=fopen(argv[1],"wb");assert(out);fprintf(out,"P6\n320 160\n255\n");for(y=0;y<160;++y)for(x=0;x<320;++x){unsigned long c=nd_saver_rgb[pixels[y][x]];fputc((int)(c>>16)&255,out);fputc((int)(c>>8)&255,out);fputc((int)c&255,out);}assert(!fclose(out));}
 assert(nd_saver_mode("Maze")==7&&nd_saver_mode("Demo shuffle")==8);
 assert(nd_saver_pick(8,(1U<<3)|(1U<<6),0,500)==3);
 assert(nd_saver_pick(8,(1U<<3)|(1U<<6),499,500)==3);
 assert(nd_saver_pick(8,(1U<<3)|(1U<<6),500,500)==6);
 assert(nd_saver_pick(8,(1U<<3)|(1U<<6),1000,500)==3);
 assert(nd_saver_pick(8,0,1000,500)==0&&nd_saver_pick(2,255,500,500)==2);
 width=height=32;memset(pixels,0,sizeof(pixels));nd_places_art(NULL,box);memcpy(before,pixels,sizeof(pixels));memset(pixels,0,sizeof(pixels));nd_saver_art(NULL,box);assert(memcmp(before,pixels,sizeof(pixels)));
 puts("SAVER: complete animation cycle, Boing Ball red/white checks, bounded scanline drawing, black idle frame, clipping and distinct native dock icons passed.");return 0;}
