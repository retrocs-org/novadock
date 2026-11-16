#ifndef ND_SAVER_ART_H
#define ND_SAVER_ART_H
#define ND_SAVER_WIDTH 320
#define ND_SAVER_HEIGHT 160
#define ND_SAVER_COLORS 29
#include "nd_boing_art.h"
typedef void (*NDSaverRect)(void *,int,int,int,int,int);
static const unsigned long nd_saver_rgb[ND_SAVER_COLORS]={0x000000UL,0x304661UL,0xe5eff7UL,0x85b7c7UL,0xb1c78bUL,0xaaaaaaUL,0x7d527fUL,0x44404bUL,0xf03535UL,0xcc2026UL,0x861a24UL,0xc7c8cbUL,0x888b91UL,0xff4655UL,0xf58b46UL,0xffdb72UL,0x9ed96bUL,0x48c8a6UL,0x56cce5UL,0x598be5UL,0x9069dfUL,0xe16fa8UL,0xe7a6c9UL,0x879ed9UL,0x6c9f94UL,0xffcb91UL,0xd8d9eeUL,0xc16190UL,0x586c9bUL};
static void nd_saver_box(void *ctx,NDSaverRect box,int c,int x,int y,int w,int h)
{if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>320)w=320-x;if(y+h>160)h=160-y;if(w>0&&h>0)box(ctx,c,x,y,w,h);}
/* Precomputed sphere runs keep the frame loop integer-only and bounded. */
static void nd_saver_boing(void *ctx,NDSaverRect box,unsigned long frame)
{
 unsigned int at,end;int i,phase=(int)(frame%40UL),t=(int)(frame%120UL),x,y,shadow;
 if(t>60)t=120-t;x=52+t*216/60;
 y=44+56*(phase-20)*(phase-20)/400;
 box(ctx,5,0,0,320,160);
 for(i=0;i<320;i+=20)box(ctx,6,i,0,1,160);
 for(i=8;i<160;i+=20)box(ctx,6,0,i,320,1);
 shadow=24+(y-44)/4;
 for(i=-5;i<=5;++i){t=shadow*(25-i*i)/25;nd_saver_box(ctx,box,7,x+12-t,144+i,t*2+1,1);}
 at=nd_boing_offsets[frame%ND_BOING_FRAMES];end=nd_boing_offsets[frame%ND_BOING_FRAMES+1];
 for(;at<end;at+=4)nd_saver_box(ctx,box,nd_boing_runs[at+3],x-40+nd_boing_runs[at]*2,y-40+nd_boing_runs[at+1]*2,nd_boing_runs[at+2]*2,2);
}
#include "nd_saver_demo.h"
static void nd_saver_scene(void *ctx,NDSaverRect box,unsigned long frame,int mode,int unused)
{
 unsigned long seed,z,f=frame%32760UL;int i,x,y,r;(void)unused;
 if(mode==4){nd_saver_copper(ctx,box,f);return;}
 if(mode==5){nd_saver_vectors(ctx,box,f);return;}
 if(mode==6){nd_saver_fireworks(ctx,box,f);return;}
 if(mode==7){nd_saver_maze(ctx,box,f);return;}
 if(mode==3){nd_saver_boing(ctx,box,f);return;}
 box(ctx,0,0,0,320,160);if(mode==2)return;
 if(mode==1){x=(int)((f*3UL)%560UL);if(x>280)x=560-x;y=(int)((f*2UL)%240UL);if(y>120)y=240-y;
  for(i=0;i<20;++i){r=i<10 ? 10-i:i-9;nd_saver_box(ctx,box,i%4==0 ? 2:3,x+r,y+i*2,40-r*2,2);}return;}
 for(i=0;i<64;++i){seed=(unsigned long)(i+1)*7919UL;z=16UL+(seed+256UL-f%256UL)%200UL;
  x=160+((int)(seed%321UL)-160)*160/(int)z;y=80+((int)((seed/7UL)%161UL)-80)*160/(int)z;r=z<50 ? 2:1;
  nd_saver_box(ctx,box,z<60 ? 2:z<120 ? 3:1,x,y,r,r);}
}
#endif
