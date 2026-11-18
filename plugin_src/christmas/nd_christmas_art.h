#ifndef ND_CHRISTMAS_ART_H
#define ND_CHRISTMAS_ART_H
/* 32-pixel tree, drawn as a small set of rectangles using mapped screen pens. */
typedef void (*NDChristmasRect)(void *,int,int,int,int,int);
static unsigned int nd_christmas_delay(int speed,int hidden)
{return hidden ? 500U:speed==0 ? 50U:speed==2 ? 15U:25U;}
static void nd_christmas_art(void *ctx,NDChristmasRect box,unsigned int frame,int pattern,int lights)
{
 static const unsigned char bulbs[12][2]={{15,8},{12,12},{18,12},{9,17},{15,16},{21,17},{6,22},{12,21},{18,22},{24,22},{10,26},{21,26}};
 int row,i,on,color;frame&=7U;
 box(ctx,0,14,25,4,5);box(ctx,6,12,28,8,3);box(ctx,2,13,28,6,1);
 for(row=0;row<5;++row){
  box(ctx,1,15-row,6+row,2+row*2,1);
  box(ctx,1,13-row,11+row,6+row*2,1);
  box(ctx,1,10-row,16+row,12+row*2,1);
  box(ctx,1,7-row,21+row,18+row*2,1);
 }
 box(ctx,2,15,7,1,2);box(ctx,2,13,12,1,2);box(ctx,2,10,17,1,2);box(ctx,2,7,22,1,2);
 box(ctx,3,15,1,2,5);box(ctx,3,13,3,6,1);box(ctx,3,14,2,4,3);
 if((frame&1U)==0)box(ctx,7,15,2,1,2);
 for(i=0;i<12 && lights;++i){
  on=pattern==2 ? 1:pattern==1 ? ((i+(int)frame)%4)<2:((i*3+(int)frame*5)%7)<4;
  color=on ? 3+i%3:0;box(ctx,color,bulbs[i][0],bulbs[i][1],2,2);
 }
}
#endif
