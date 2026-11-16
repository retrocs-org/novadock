#ifndef ND_CHRISTMAS_SCENE_H
#define ND_CHRISTMAS_SCENE_H
#include "nd_christmas_art.h"
#define ND_XMAS_WIDTH 320
#define ND_XMAS_HEIGHT 160
#define ND_XMAS_COLORS 16
static const unsigned long nd_xmas_rgb[ND_XMAS_COLORS]={
 0x101b38UL,0x205440UL,0x654532UL,0xf5d07aUL,
 0xd8595fUL,0x84b6ceUL,0x34445aUL,0xf0f4edUL,
 0x1e3150UL,0x354663UL,0xbed7d9UL,0x081222UL,
 0xa06f52UL,0xc3dce8UL,0x4c8a68UL,0xf1a568UL};
struct NDXmasDraw {void *context;NDChristmasRect box;};
/* Clip sprites before passing them to the native or preview renderer. */
static void nd_xmas_box(struct NDXmasDraw *d,int c,int x,int y,int w,int h)
{
 if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}
 if(x+w>ND_XMAS_WIDTH)w=ND_XMAS_WIDTH-x;
 if(y+h>ND_XMAS_HEIGHT)h=ND_XMAS_HEIGHT-y;
 if(w>0 && h>0)d->box(d->context,c,x,y,w,h);
}
static void nd_xmas_tree_box(void *ctx,int c,int x,int y,int w,int h)
{nd_xmas_box((struct NDXmasDraw *)ctx,c,18+x*2,78+y*2,w*2,h*2);}
static void nd_xmas_pine(struct NDXmasDraw *d,int x,int y,int size)
{
 int j;nd_xmas_box(d,2,x+size/2-1,y+size,2,8);
 for(j=0;j<size/2;++j)nd_xmas_box(d,1,x+size/2-j,y+j*2,2+j*2,3);
}
static void nd_xmas_deer(struct NDXmasDraw *d,int x,int y,int step)
{
 int k=step ? 2:0;
 nd_xmas_box(d,2,x+5,y+5,13,5);nd_xmas_box(d,2,x+2,y+1,5,7);
 nd_xmas_box(d,2,x,y+2,5,3);nd_xmas_box(d,3,x+3,y-3,1,5);
 nd_xmas_box(d,3,x+1,y-3,4,1);nd_xmas_box(d,3,x+5,y-5,1,4);
 nd_xmas_box(d,2,x+6-k,y+9,2,6);nd_xmas_box(d,2,x+15+k,y+9,2,6);
 nd_xmas_box(d,3,x+4,y+5,2,3);nd_xmas_box(d,4,x,y+3,2,2);
 nd_xmas_box(d,2,x+18,y+4,3,2);
}
static void nd_xmas_scene(void *ctx,NDChristmasRect box,unsigned long frame,int pattern,int lights)
{
 struct NDXmasDraw draw;int i,x,y,bob;unsigned long f=frame%3520UL;
 static const signed char wave[8]={0,-1,-2,-1,0,1,2,1};
 draw.context=ctx;draw.box=box;
 nd_xmas_box(&draw,0,0,0,320,64);nd_xmas_box(&draw,8,0,64,320,46);
 nd_xmas_box(&draw,9,0,110,320,50);
 for(i=0;i<24;++i){x=(i*47+13)%320;y=(i*19+7)%86;
  nd_xmas_box(&draw,(i+(int)f)%7==0 ? 3:13,x,y,1,1);}
 nd_xmas_box(&draw,5,141,13,16,24);nd_xmas_box(&draw,5,137,17,24,16);
 nd_xmas_box(&draw,7,141,15,14,20);nd_xmas_box(&draw,7,139,18,19,13);
 nd_xmas_box(&draw,10,143,18,3,4);nd_xmas_box(&draw,10,152,27,3,3);
 for(i=0;i<9;++i)nd_xmas_pine(&draw,92+i*27,106-(i%3)*5,18+(i%3)*6);
 nd_xmas_box(&draw,10,0,142,320,18);nd_xmas_box(&draw,7,0,146,120,14);
 nd_xmas_box(&draw,7,113,151,94,9);nd_xmas_box(&draw,7,198,144,122,16);
 /* A snowy cabin, warm windows, chimney and drifting smoke. */
 nd_xmas_box(&draw,12,182,107,91,37);nd_xmas_box(&draw,2,182,137,91,7);
 for(i=0;i<10;++i)nd_xmas_box(&draw,2,177+i*2,103-i*2,102-i*4,3);
 nd_xmas_box(&draw,7,177,101,102,3);nd_xmas_box(&draw,7,179,98,98,3);
 nd_xmas_box(&draw,2,247,79,10,15);nd_xmas_box(&draw,7,245,77,14,3);
 for(i=0;i<3;++i)nd_xmas_box(&draw,6,250+((int)f/3+i)%5,69-i*7,7+i*2,3);
 nd_xmas_box(&draw,11,220,118,15,26);nd_xmas_box(&draw,3,231,132,2,2);
 for(i=0;i<2;++i){x=192+i*49;nd_xmas_box(&draw,3,x,116,18,17);
  nd_xmas_box(&draw,15,x+2,119+(int)(f%3UL),14,10);
  nd_xmas_box(&draw,2,x+8,116,2,17);nd_xmas_box(&draw,2,x,123,18,2);
  nd_xmas_box(&draw,7,x-1,133,20,3);}
 nd_christmas_art(&draw,nd_xmas_tree_box,(unsigned int)(f&7UL),pattern,lights);
 /* Reindeer lead Santa and his red sleigh from right to left. */
 x=190-(int)((f*4UL)%440UL);bob=wave[(f/2UL)&7UL];y=42+bob;
 nd_xmas_box(&draw,3,x+9,y+9,66,1);
 for(i=0;i<3;++i)nd_xmas_deer(&draw,x+i*24,y,(int)((f+i)&1UL));
 nd_xmas_box(&draw,3,x+75,y+21,36,2);nd_xmas_box(&draw,3,x+72,y+17,3,5);
 nd_xmas_box(&draw,4,x+78,y+8,27,10);nd_xmas_box(&draw,4,x+75,y+5,4,9);
 nd_xmas_box(&draw,3,x+78,y+17,27,2);nd_xmas_box(&draw,14,x+98,y-3,12,12);
 nd_xmas_box(&draw,4,x+82,y,11,10);nd_xmas_box(&draw,7,x+81,y+7,14,2);
 nd_xmas_box(&draw,15,x+82,y-6,9,7);nd_xmas_box(&draw,7,x+80,y-4,5,9);
 nd_xmas_box(&draw,4,x+81,y-10,11,4);nd_xmas_box(&draw,7,x+79,y-7,14,2);
 nd_xmas_box(&draw,7,x+90,y-12,3,3);
 /* Deterministic snow, requiring no random-number or disk activity. */
 for(i=0;i<44;++i){x=(i*73+(int)f*(1+i%2))%324-2;y=(i*37+(int)f*(1+i%3))%166-4;
  nd_xmas_box(&draw,13,x,y,i%3 ? 1:2,2);}
}
/* UI messages must not advance the animation faster than elapsed time. */
static unsigned long nd_xmas_advance(unsigned long *ticks,unsigned int elapsed,unsigned int interval)
{unsigned long steps;*ticks+=elapsed;steps=*ticks/interval;*ticks%=interval;return steps;}
#endif
