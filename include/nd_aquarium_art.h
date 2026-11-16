#ifndef ND_AQUARIUM_ART_H
#define ND_AQUARIUM_ART_H
#define ND_AQUARIUM_WIDTH 320
#define ND_AQUARIUM_HEIGHT 160
#define ND_AQUARIUM_COLORS 16
typedef void (*NDAquariumRect)(void *,int,int,int,int,int);
static const unsigned long nd_aquarium_rgb[16]={0x092b44UL,0x134a62UL,0x236c78UL,0x4299a1UL,0xd2b57bUL,0xf5d27cUL,0xe68d57UL,0xba5e55UL,0x63985cUL,0x397e57UL,0xb8e4ddUL,0xf1f4d9UL,0x111f31UL,0x5b6f87UL,0xa78badUL,0x719bc4UL};
struct NDAquariumDraw {void *ctx;NDAquariumRect box;int width,height;};
static void nd_aquarium_box(struct NDAquariumDraw *d,int c,int x,int y,int w,int h)
{if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>d->width)w=d->width-x;if(y+h>d->height)h=d->height-y;if(w>0&&h>0)d->box(d->ctx,c,x,y,w,h);}
struct NDAquariumFish {struct NDAquariumDraw *draw;int x,y,scale,reverse;};
static void nd_fish_box(struct NDAquariumFish *f,int c,int x,int y,int w,int h)
{if(f->reverse)x=20-x-w;nd_aquarium_box(f->draw,c,f->x+x*f->scale,f->y+y*f->scale,w*f->scale,h*f->scale);}
static void nd_aquarium_fish(struct NDAquariumDraw *d,int x,int y,int scale,int reverse,int color,unsigned long frame)
{
 struct NDAquariumFish f;f.draw=d;f.x=x;f.y=y;f.scale=scale;f.reverse=reverse;
 nd_fish_box(&f,color,0,2,2,8);nd_fish_box(&f,color,2,3,2,6);nd_fish_box(&f,color,4,4,2,4);
 nd_fish_box(&f,color,6,2,11,8);nd_fish_box(&f,color,9,1,6,10);nd_fish_box(&f,color,17,4,2,4);
 nd_fish_box(&f,5,10,1,4,1);nd_fish_box(&f,7,9,4,2,5);nd_fish_box(&f,7,13,4,1,5);
 nd_fish_box(&f,11,15,3,2,2);nd_fish_box(&f,12,16,3,1,1);
 nd_fish_box(&f,5,9+(int)(frame&1UL),8,4,2);
}
static void nd_aquarium_scene(void *ctx,NDAquariumRect box,unsigned long frame,int fish,int bubbles)
{
 struct NDAquariumDraw d;int i,j,x,y,scale,reverse,cycle;unsigned long f=frame%65520UL;
 d.ctx=ctx;d.box=box;d.width=320;d.height=160;
 nd_aquarium_box(&d,1,0,0,320,40);nd_aquarium_box(&d,0,0,40,320,120);
 nd_aquarium_box(&d,2,0,0,320,3);nd_aquarium_box(&d,4,0,144,320,16);
 for(i=0;i<18;++i){x=(i*53+7)%320;y=147+(i*7)%12;nd_aquarium_box(&d,i%2 ? 5:13,x,y,3+i%4,2);}
 for(i=0;i<8;++i){x=10+i*43;y=95+(i*11)%24;
  for(j=0;j<5;++j){nd_aquarium_box(&d,9,x+(j+(int)(f/3UL))%3,y+j*10,3,12);nd_aquarium_box(&d,8,x-4+(j+(int)(f/3UL))%3,y+j*10,6,3);}
 }
 if(fish<1)fish=1;if(fish>8)fish=8;
 for(i=0;i<fish;++i){scale=1+i%3;cycle=320+20*scale;reverse=i&1;
  x=(int)((f*(unsigned long)(1+i%3)+(unsigned long)i*83UL)%(unsigned long)cycle)-20*scale;
  if(reverse)x=320-x-20*scale;y=16+(i*29)%105;
  nd_aquarium_fish(&d,x,y,scale,reverse,i%3==0 ? 6:i%3==1 ? 15:14,f+i);
 }
 if(bubbles)for(i=0;i<12;++i){x=12+(i*67)%300+(int)(f%3UL);y=150-(int)((f*2UL+(unsigned long)i*29UL)%160UL);
  nd_aquarium_box(&d,10,x+1,y,2,1);nd_aquarium_box(&d,10,x,y+1,1,2);nd_aquarium_box(&d,3,x+3,y+1,1,2);nd_aquarium_box(&d,3,x+1,y+3,2,1);}
}
static void nd_aquarium_tile(void *ctx,NDAquariumRect box,unsigned long frame)
{
 struct NDAquariumDraw d;d.ctx=ctx;d.box=box;d.width=d.height=32;
 nd_aquarium_box(&d,12,1,3,30,26);nd_aquarium_box(&d,1,3,5,26,22);nd_aquarium_box(&d,4,3,25,26,2);
 nd_aquarium_box(&d,9,5,18,2,7);nd_aquarium_box(&d,8,4,19,4,2);
 nd_aquarium_fish(&d,5+(int)(frame%5UL),10,1,0,6,frame);
 nd_aquarium_box(&d,10,25,6+(int)(frame%7UL),1,2);
}
#endif
