#!/usr/bin/env python3
"""Exercise the production transparency cache with RGB and planar raster mocks."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'src/nd_transparency.c').read_text();s=s[s.index('static unsigned char mix('):]
h=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nd_transparency.h"
typedef unsigned long ULONG;
#define BMF_CLEAR 0
#define MEMF_PUBLIC 0
struct Library {int unused;};
struct BitMap {int Depth,BytesPerRow,w,h;unsigned char *Planes[1],*rgb;};
struct RastPort {struct BitMap *BitMap;};
struct Screen {struct RastPort RastPort;};
static struct Library library,*NDGlassCyberGfxBase;
static int rtg=1,read_fail,fail_tint,reads,writes,allocations,frees;
static struct Library *OpenLibrary(const char*n,int v){(void)n;(void)v;return rtg?&library:NULL;}
static void CloseLibrary(struct Library*l){assert(l==&library);}
static void WaitBlit(void){}
static void *AllocVec(ULONG n,int flags){(void)flags;return malloc(n);}
static void FreeVec(void*p){free(p);}
static struct BitMap *AllocBitMap(int w,int h,int depth,int flags,struct BitMap*friend_bitmap)
{struct BitMap*b;(void)flags;(void)friend_bitmap;if(depth>1&&fail_tint)return NULL;b=calloc(1,sizeof(*b));assert(b);b->Depth=depth;b->w=w;b->h=h;b->BytesPerRow=((w+15)/16)*2;b->Planes[0]=calloc(h,b->BytesPerRow);b->rgb=calloc((size_t)w*h,3);assert(b->Planes[0]&&b->rgb);++allocations;return b;}
static void FreeBitMap(struct BitMap*b){assert(b);free(b->Planes[0]);free(b->rgb);free(b);++frees;}
static void InitRastPort(struct RastPort*r){memset(r,0,sizeof(*r));}
static int nd_glass_depth(struct BitMap*b){return b->Depth;}
static int nd_glass_bitmap(struct BitMap*b){(void)b;return rtg;}
static ULONG nd_glass_rgb_line(void*out,int w,struct RastPort*r,int y){++reads;if(read_fail)return 0;assert(w==r->BitMap->w&&y<r->BitMap->h);memcpy(out,r->BitMap->rgb+y*w*3,w*3);return w;}
static ULONG nd_glass_write_line(void*in,int w,struct RastPort*r,int y){++writes;assert(w==r->BitMap->w&&y<r->BitMap->h);memcpy(r->BitMap->rgb+y*w*3,in,w*3);return w;}
static ULONG nd_glass_rgb_span(void*out,int w,struct RastPort*r,int x,int y){++reads;if(read_fail)return 0;assert(x>=0&&x+w<=r->BitMap->w&&y<r->BitMap->h);memcpy(out,r->BitMap->rgb+(y*r->BitMap->w+x)*3,w*3);return w;}
static ULONG nd_glass_write_span(void*in,int w,struct RastPort*r,int x,int y){++writes;assert(x>=0&&x+w<=r->BitMap->w&&y<r->BitMap->h);memcpy(r->BitMap->rgb+(y*r->BitMap->w+x)*3,in,w*3);return w;}
static void copy(struct BitMap*b,int x,int y,struct RastPort*r,int dx,int dy,int w,int h,unsigned char*mask)
{int xx,yy;assert(x>=0&&y>=0&&x+w<=b->w&&y+h<=b->h&&dx>=0&&dy>=0&&dx+w<=r->BitMap->w&&dy+h<=r->BitMap->h);for(yy=0;yy<h;++yy)for(xx=0;xx<w;++xx)if(!mask || (mask[(y+yy)*b->BytesPerRow+(x+xx)/8]&(128>>((x+xx)%8))))memcpy(r->BitMap->rgb+((dy+yy)*r->BitMap->w+dx+xx)*3,b->rgb+((y+yy)*b->w+x+xx)*3,3);}
static void BltBitMapRastPort(struct BitMap*b,int x,int y,struct RastPort*r,int dx,int dy,int w,int h,int mode){(void)mode;copy(b,x,y,r,dx,dy,w,h,NULL);}
static void BltMaskBitMapRastPort(struct BitMap*b,int x,int y,struct RastPort*r,int dx,int dy,int w,int h,int mode,void*mask){(void)mode;copy(b,x,y,r,dx,dy,w,h,mask);}
'''+s+r'''
int main(void)
{
 struct NDTransparency c;struct Screen screen;struct RastPort rp;struct BitMap*desktop,*dest;unsigned char tint[3]={200,180,160};int x,y,n,before,amount;
 memset(&c,0,sizeof(c));desktop=AllocBitMap(16,16,24,0,NULL);dest=AllocBitMap(16,16,24,0,NULL);screen.RastPort.BitMap=desktop;rp.BitMap=dest;memset(desktop->rgb,100,16*16*3);
 for(amount=0;amount<=100;amount+=25){
  nd_transparency_prepare(&c,&screen,desktop,16,16,amount,tint,0,0,0,16);
  if(!amount){assert(!c.amount&&!c.tint&&!c.mask);continue;}
  memset(dest->rgb,9,16*16*3);before=reads;nd_transparency_paint(&c,&rp,desktop,2,3,10,8);assert(reads==before);
  assert(dest->rgb[0]==9);for(y=3;y<11;++y)for(x=2;x<12;++x)assert(dest->rgb[(y*16+x)*3]==mix(100,200,amount));
 }
 /* Fresh underlay rows update tint pixels while reusing the warm cache. */
 nd_transparency_prepare(&c,&screen,desktop,16,16,50,tint,0,0,0,16);
 before=allocations;for(y=4;y<6;++y)memset(desktop->rgb+(y*16+3)*3,20,5*3);
 assert(nd_transparency_refresh(&c,desktop,3,4,5,2)&&allocations==before);
 for(y=0;y<16;++y)for(x=0;x<16;++x)assert(c.tint->rgb[(y*16+x)*3]==mix(y>=4&&y<6&&x>=3&&x<8?20:100,200,50));
 memset(desktop->rgb,100,16*16*3);
 rtg=0;nd_transparency_prepare(&c,&screen,desktop,16,16,50,tint,0,0,0,16);assert(c.mask&&!c.tint);n=0;
 for(y=0;y<8;++y)for(x=0;x<8;++x)n+=(c.mask->Planes[0][y*c.mask->BytesPerRow+x/8]&(128>>(x%8)))!=0;assert(n==32);
 memset(dest->rgb,0,16*16*3);nd_transparency_paint(&c,&rp,desktop,0,0,16,16);n=0;for(x=0;x<256;++x)n+=dest->rgb[x*3]==100;assert(n==128);
 rtg=1;read_fail=1;nd_transparency_prepare(&c,&screen,desktop,16,16,50,tint,0,0,0,16);assert(c.mask&&!c.tint);read_fail=0;
 fail_tint=1;nd_transparency_prepare(&c,&screen,desktop,16,16,50,tint,0,0,0,16);assert(c.mask&&!c.tint);fail_tint=0;
 nd_transparency_prepare(&c,&screen,desktop,16,16,50,tint,1,0,0,16);assert(c.tint&&c.tint->rgb[0]>c.tint->rgb[8*16*3]);
 nd_transparency_free(&c);assert(!c.amount&&!c.tint&&!c.mask);FreeBitMap(desktop);FreeBitMap(dest);assert(allocations==frees);
 puts("Transparency: 0/100 endpoints, RGB blends, glass shading, clipped cached paints, 50% planar coverage and allocation/reader fallbacks passed.");return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
