#ifndef ND_SAVER_DEMO_H
#define ND_SAVER_DEMO_H
#include "nd_saver_maze.h"
static const short nd_saver_sin[64]={0,13,25,37,49,60,71,81,91,99,106,113,118,122,126,127,128,127,126,122,118,113,106,99,91,81,71,60,49,37,25,13,0,-13,-25,-37,-49,-60,-71,-81,-91,-99,-106,-113,-118,-122,-126,-127,-128,-127,-126,-122,-118,-113,-106,-99,-91,-81,-71,-60,-49,-37,-25,-13};
static void nd_saver_line(void *ctx,NDSaverRect box,int c,int x,int y,int xx,int yy)
{
 int dx=xx-x,dy=yy-y,n,i;
 if(!dx){if(dy<0){y=yy;dy=-dy;}nd_saver_box(ctx,box,c,x,y,1,dy+1);return;}
 if(!dy){if(dx<0){x=xx;dx=-dx;}nd_saver_box(ctx,box,c,x,y,dx+1,1);return;}
 n=(dx<0 ? -dx:dx)>(dy<0 ? -dy:dy) ? (dx<0 ? -dx:dx):(dy<0 ? -dy:dy);n=n/2+1;
 for(i=0;i<=n;++i)nd_saver_box(ctx,box,c,x+dx*i/n,y+dy*i/n,2,2);
}
static void nd_saver_copper(void *ctx,NDSaverRect box,unsigned long f)
{
 int y,x,i,col;
 box(ctx,0,0,0,320,160);
 for(i=0;i<40;++i){x=(int)(((unsigned long)i*7919UL+960UL-f%320UL)%320UL);y=(i*37)%160;box(ctx,i%3 ? 1:2,x,y,1,1);}
 for(i=0;i<4;++i){y=80+nd_saver_sin[(f+i*16)%64]/3;
  for(x=-8;x<=8;++x){col=13+i*3+(x<0 ? -x:x)/4;nd_saver_box(ctx,box,col,0,y+x,320,1);}}
 for(x=0;x<320;x+=4){y=80+nd_saver_sin[(x/4+f)%64]/3;nd_saver_box(ctx,box,2,x,y,4,2);}
}
static void nd_saver_vectors(void *ctx,NDSaverRect box,unsigned long f)
{
 static const signed char cube[8][3]={{-32,-32,-32},{32,-32,-32},{32,32,-32},{-32,32,-32},{-32,-32,32},{32,-32,32},{32,32,32},{-32,32,32}};
 static const signed char pyramid[5][3]={{-38,28,-38},{38,28,-38},{38,28,38},{-38,28,38},{0,-42,0}};
 static const signed char check[6][2]={{-38,0},{-12,28},{40,-30},{30,-40},{-12,12},{-28,-10}};
 static const unsigned char ce[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
 int x[12],y[12],i,a,b,n,kind=(int)((f/120UL)%3UL),sx,cx,sy,cy,px,py,pz,rx,rz,ry,zz;
 sx=nd_saver_sin[f%64];cx=nd_saver_sin[(f+16)%64];sy=nd_saver_sin[(f/2)%64];cy=nd_saver_sin[(f/2+16)%64];
 box(ctx,0,0,0,320,160);n=kind==0 ? 8:kind==1 ? 5:12;
 for(i=0;i<n;++i){if(kind==0){px=cube[i][0];py=cube[i][1];pz=cube[i][2];}else if(kind==1){px=pyramid[i][0];py=pyramid[i][1];pz=pyramid[i][2];}else{px=check[i%6][0];py=check[i%6][1];pz=i<6 ? -6:6;}
  rx=(px*cx+pz*sx)/128;rz=(pz*cx-px*sx)/128;ry=(py*cy-rz*sy)/128;zz=(rz*cy+py*sy)/128;
  x[i]=160+rx*220/(190+zz);y[i]=80+ry*220/(190+zz);}
 if(kind==0)for(i=0;i<12;++i)nd_saver_line(ctx,box,13+i%12,x[ce[i][0]],y[ce[i][0]],x[ce[i][1]],y[ce[i][1]]);
 else if(kind==1)for(i=0;i<4;++i){nd_saver_line(ctx,box,14+i*2,x[i],y[i],x[(i+1)%4],y[(i+1)%4]);nd_saver_line(ctx,box,2,x[i],y[i],x[4],y[4]);}
 else for(i=0;i<6;++i){a=(i+1)%6;b=i+6;nd_saver_line(ctx,box,8,x[i],y[i],x[a],y[a]);nd_saver_line(ctx,box,3,x[b],y[b],x[a+6],y[a+6]);nd_saver_line(ctx,box,2,x[i],y[i],x[b],y[b]);}
}
static void nd_saver_fireworks(void *ctx,NDSaverRect box,unsigned long f)
{
 int i,j,t,x,y,dx,dy,r,col;
 box(ctx,0,0,0,320,160);
 for(i=0;i<24;++i){x=(i*109)%320;y=(i*41)%86;box(ctx,1+(i%2),x,y,1,1);}
 for(i=0;i<20;++i){x=i*16;y=126+(i*7)%23;box(ctx,1,x,y,15,160-y);for(j=0;j<3;++j)if((i+j)%3)box(ctx,4,x+3+j*4,y+5,2,2);}
 for(i=0;i<5;++i){t=(int)((f+(unsigned long)i*19UL)%96UL);x=36+(i*57)%248;y=25+(i*13)%42;
  if(t<18){nd_saver_line(ctx,box,2,x,142-t*(142-y)/18,x,148-t*(142-y)/18);continue;}
  t-=18;if(t>60)continue;col=t>45 ? 1:13+i*3;
  for(j=0;j<18;++j){r=20+(j%3)*7;dx=nd_saver_sin[(j*64/18)%64]*t*r/(128*30);dy=nd_saver_sin[(j*64/18+16)%64]*t*r/(128*30)+t*t/110;
   nd_saver_box(ctx,box,col,x+dx,y+dy,2,2);if(t<40)nd_saver_box(ctx,box,2,x+dx*9/10,y+dy*9/10,1,1);}}
}
static void nd_saver_maze(void *ctx,NDSaverRect box,unsigned long frame)
{
 unsigned long period=34UL+ND_MAZE_PATH_LEN*2UL+20UL,t=frame%period;int i,n,a,b,ax,ay,bx,by,x,y,step;
 box(ctx,5,0,0,320,160);
 for(i=0;i<=19;++i)box(ctx,1,8+i*16,8,2,146);
 for(i=0;i<=9;++i)box(ctx,1,8,8+i*16,306,2);
 n=t<34UL ? (int)t*5:170;
 for(i=0;i<n;++i){a=nd_maze_edges[i*2];b=nd_maze_edges[i*2+1];ax=8+(a%19)*16;ay=8+(a/19)*16;bx=8+(b%19)*16;by=8+(b/19)*16;
  if(ax==bx)box(ctx,5,ax+2,(ay>by ? ay:by),14,2);else box(ctx,5,(ax>bx ? ax:bx),ay+2,2,14);}
 box(ctx,4,8,10,2,12);box(ctx,4,312,138,2,12);
 if(t<34UL)return;t-=34;step=(int)(t/2);if(step>=ND_MAZE_PATH_LEN)step=ND_MAZE_PATH_LEN-1;
 for(i=0;i<step;++i){a=nd_maze_path[i];box(ctx,3,15+(a%19)*16,15+(a/19)*16,2,2);}
 a=nd_maze_path[step];b=nd_maze_path[step<ND_MAZE_PATH_LEN-1 ? step+1:step];
 x=13+(a%19)*16+((b%19)-(a%19))*8*(int)(t%2);y=13+(a/19)*16+((b/19)-(a/19))*8*(int)(t%2);
 box(ctx,8,x,y,7,7);box(ctx,2,x+1,y+1,5,3);box(ctx,1,x+4,y+1,1,2);box(ctx,1,x+1,y+6,2,2);box(ctx,1,x+5,y+6,2,2);
}
#endif
