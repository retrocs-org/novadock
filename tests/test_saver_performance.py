#!/usr/bin/env python3
"""Execute native SAVER caching against a software graphics implementation."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'plugin_src/saver/nd_saver_window.h').read_text()
def function(signature):
 a=source.index(signature);b=source.index('{',a);i=b+1;n=1
 while n:n+=(source[i]=='{')-(source[i]=='}');i+=1
 return source[a:i]
struct=source[source.index("struct NDSaverBand"):source.index('static unsigned long saver_clock')]
h=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_saver_art.h"
#include "nd_saver_modes.h"
#include "nd_saver_stats.h"
typedef unsigned long ULONG;typedef unsigned short UWORD;typedef unsigned char UBYTE;
#define BMA_DEPTH 1
#define BMF_CLEAR 1
#define JAM1 1
struct BitMap {int width,height,depth;unsigned char *pixels;};
struct RastPort {struct BitMap *BitMap;int pen;};
struct Screen {int Width,Height;};struct Window {struct RastPort *RPort;};
static int fills,blits,masks,allocations,fail_at,freed,waits,pen_changes;
static struct BitMap *AllocBitMap(int w,int h,int depth,int flags,struct BitMap *friend){struct BitMap *b;(void)flags;(void)friend;++allocations;if(fail_at==allocations)return NULL;b=calloc(1,sizeof(*b));assert(b);b->width=w;b->height=h;b->depth=depth;b->pixels=calloc((size_t)w,h);assert(b->pixels);return b;}
static void FreeBitMap(struct BitMap *b){assert(b);free(b->pixels);free(b);++freed;}
static void WaitBlit(void){++waits;}
static unsigned long GetBitMapAttr(struct BitMap *b,int attr){assert(attr==BMA_DEPTH);return (unsigned long)b->depth;}
static UBYTE *AllocRaster(int w,int h){++allocations;if(fail_at==allocations)return NULL;return calloc((size_t)((w+15)/16)*2,h);}
static void FreeRaster(UBYTE *p,int w,int h){assert(p&&w>0&&h>0);free(p);++freed;}
static void InitRastPort(struct RastPort *r){memset(r,0,sizeof(*r));}
static void SetDrMd(struct RastPort *r,int mode){(void)r;assert(mode==JAM1);}
static void SetAPen(struct RastPort *r,UWORD pen){r->pen=pen;++pen_changes;}
static void RectFill(struct RastPort *r,int left,int top,int right,int bottom){int x,y;assert(left>=0&&top>=0&&right<r->BitMap->width&&bottom<r->BitMap->height&&right>=left&&bottom>=top);++fills;for(y=top;y<=bottom;++y)for(x=left;x<=right;++x)r->BitMap->pixels[y*r->BitMap->width+x]=(UBYTE)r->pen;}
static void BltBitMapRastPort(struct BitMap *from,int x,int y,struct RastPort *to,int xx,int yy,int w,int h,int mode){int row;assert(mode==0xc0&&x>=0&&y>=0&&xx>=0&&yy>=0&&x+w<=from->width&&y+h<=from->height&&xx+w<=to->BitMap->width&&yy+h<=to->BitMap->height);++blits;for(row=0;row<h;++row)memcpy(to->BitMap->pixels+(yy+row)*to->BitMap->width+xx,from->pixels+(y+row)*from->width+x,w);}
static void BltMaskBitMapRastPort(struct BitMap *from,int x,int y,struct RastPort *to,int xx,int yy,int w,int h,int mode,UBYTE *mask){int row,col,stride=((from->width+15)/16)*2;assert(mode==0xe0);++masks;for(row=0;row<h;++row)for(col=0;col<w;++col)if(mask[(y+row)*stride+((x+col)>>3)]&(0x80U>>((x+col)&7)))to->BitMap->pixels[(yy+row)*to->BitMap->width+xx+col]=from->pixels[(y+row)*from->width+x+col];}
'''+struct+chr(10)+function('static void saver_rect(')+chr(10)+'#include "nd_saver_fast.h"'+chr(10)+function('static void saver_present(')+chr(10)+r'''
static unsigned long saver_clock(void){return 0;}
static void saver_overlay(struct NDSaverWindow *s){(void)s;}
'''+function('static void saver_paint(')+r'''
static unsigned char expected[640*320],reference[320*160];static int ew,eh,count;
static void expected_box(void *ctx,int c,int x,int y,int w,int h){int xx,yy,x1=x*ew/320,y1=y*eh/160,x2=(x+w)*ew/320,y2=(y+h)*eh/160;(void)ctx;++count;for(yy=y1;yy<y2;++yy)for(xx=x1;xx<x2;++xx)expected[yy*ew+xx]=(UBYTE)c;}
static void old_copper(void *ctx,NDSaverRect box,unsigned long f){int y,x,i,col;box(ctx,0,0,0,320,160);for(i=0;i<40;++i){x=(int)(((unsigned long)i*7919UL+960UL-f%320UL)%320UL);y=(i*37)%160;box(ctx,i%3 ? 1:2,x,y,1,1);}for(i=0;i<4;++i){y=80+nd_saver_sin[(f+i*16)%64]/3;for(x=-8;x<=8;++x){col=13+i*3+(x<0 ? -x:x)/4;nd_saver_box(ctx,box,col,0,y+x,320,1);}}for(x=0;x<320;x+=4){y=80+nd_saver_sin[(x/4+f)%64]/3;nd_saver_box(ctx,box,2,x,y,4,2);}}
static void check_boing(int scale,int depth,int fail){struct NDSaverWindow s;struct Screen screen;struct Window window;struct RastPort output;int i,n=0,old_allocations;memset(&s,0,sizeof(s));ew=s.width=320*scale;eh=s.height=160*scale;s.last_pen=-1;screen.Width=s.width;screen.Height=s.height;s.screen=&screen;
 s.bitmap=AllocBitMap(s.width,s.height,depth,0,NULL);output.BitMap=AllocBitMap(s.width,s.height,depth,0,NULL);window.RPort=&output;s.window=&window;s.paint.BitMap=s.bitmap;for(i=0;i<29;++i)s.pens[i]=(UWORD)i;for(i=0;i<=320;++i)s.xmap[i]=i*scale;for(i=0;i<=160;++i)s.ymap[i]=i*scale;
 if(fail)fail_at=allocations+fail;
 for(s.frame=0;s.frame<240;++s.frame){fills=blits=masks=pen_changes=0;saver_paint(&s,3,0);nd_saver_scene(NULL,expected_box,s.frame,3,0);assert(!memcmp(expected,s.bitmap->pixels,(size_t)s.width*s.height));if(memcmp(expected,output.BitMap->pixels,(size_t)s.width*s.height)){int p;for(p=0;p<s.width*s.height;++p)if(expected[p]!=output.BitMap->pixels[p]){fprintf(stderr,"Output differs scale=%d depth=%d fail=%d frame=%lu at %d,%d expected=%d actual=%d dirty=%d,%d %dx%d\n",scale,depth,fail,s.frame,p%s.width,p/s.width,expected[p],output.BitMap->pixels[p],s.dirty_x,s.dirty_y,s.dirty_w,s.dirty_h);break;}abort();}
  if(s.frame>=16 && !fail && !(scale==2&&depth==32)){assert(fills==11&&blits==2+(int)s.band_count&&masks==0&&pen_changes<=1);assert(s.dirty_w*s.dirty_h<s.width*s.height/2);++n;}}
 if(!fail && !(scale==2&&depth==32))assert(n==224);
 if(fail){old_allocations=allocations;s.frame=300;saver_paint(&s,3,0);assert(allocations==old_allocations&&s.cache_failed);}
 /* A mode change/refresh must still leave a complete, current backing image. */
 s.frame=17;saver_paint(&s,4,0);nd_saver_scene(NULL,expected_box,17,4,0);assert(!memcmp(expected,s.bitmap->pixels,(size_t)s.width*s.height));saver_present(&s,1);s.frame=18;saver_paint(&s,3,0);nd_saver_scene(NULL,expected_box,18,3,0);if(memcmp(expected,output.BitMap->pixels,(size_t)s.width*s.height)){int p;for(p=0;p<s.width*s.height;++p)if(expected[p]!=output.BitMap->pixels[p]){fprintf(stderr,"Output differs scale=%d depth=%d fail=%d frame=%lu at %d,%d expected=%d actual=%d dirty=%d,%d %dx%d\n",scale,depth,fail,s.frame,p%s.width,p/s.width,expected[p],output.BitMap->pixels[p],s.dirty_x,s.dirty_y,s.dirty_w,s.dirty_h);break;}abort();}
 saver_free_cache(&s);assert(!s.background&&!s.sprites);FreeBitMap(s.bitmap);FreeBitMap(output.BitMap);fail_at=0;}
int main(void){unsigned long f;int old,new,w,h,sw,sh,k;static const char *sizes[]={"Auto","Full","320 x 160","640 x 320"};ew=320;eh=160;
 assert(!nd_saver_dimensions(159,80,8,"Auto",&w,&h));assert(!nd_saver_dimensions(320,79,8,"Full",&w,&h));
 for(sw=160;sw<=1920;sw+=37)for(sh=80;sh<=1080;sh+=53)for(k=0;k<4;++k){assert(nd_saver_dimensions(sw,sh,32,sizes[k],&w,&h));assert(w<=sw&&h<=sh&&w>=160&&h>=80&&w>=h*2&&w<=h*2+1);}
 assert(nd_saver_dimensions(1920,1080,32,"Auto",&w,&h)&&w==320&&h==160);
 assert(nd_saver_dimensions(1920,1080,8,"Auto",&w,&h)&&w==640&&h==320);
 assert(nd_saver_dimensions(640,256,8,"Full",&w,&h)&&w==512&&h==256);
 for(f=0;f<320;++f){count=0;old_copper(NULL,expected_box,f);old=count;memcpy(reference,expected,sizeof(reference));count=0;nd_saver_copper(NULL,expected_box,f);new=count;assert(!memcmp(reference,expected,sizeof(reference)));assert(new<old);}
 check_boing(1,5,0);check_boing(2,8,0);check_boing(1,32,0);check_boing(2,32,0);check_boing(1,8,1);check_boing(1,8,2);
 puts("SAVER cached Boing: 240-frame pixel equivalence at 1x/2x, 5/8/32-bit layouts, 11 fills + cached silhouette copies after warm-up, partial presentation, bounded memory and all allocation failures pass. Copper bars match all 320 original phases with fewer fills.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(h);command=[os.environ.get('CC','cc'),'-std=c99','-Wno-unused-function','-I'+str(ROOT/'include'),'-I'+str(ROOT/'plugin_src/saver'),str(p/'check.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':command+=['-fsanitize=address,undefined']
 subprocess.run(command,check=True);subprocess.run([str(p/'check')],check=True)
