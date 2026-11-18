#!/usr/bin/env python3
"""Render production icon pixels against a reordered screen palette."""
from pathlib import Path
import os
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
def function(file, signature, last=False):
    source=(ROOT/file).read_text()
    start=source.rindex(signature) if last else source.index(signature)
    end=source.index('{',start)+1; depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
code=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nd_icon_mask.h"
typedef unsigned long ULONG;
typedef long LONG;
typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef UBYTE *PLANEPTR;
#define RGBBYTE(x) ((ULONG)(x)*0x01010101UL)
#define BMF_CLEAR 1
#define TAG_DONE 0
enum {ICONCTRLA_IsPaletteMapped=1,ICONCTRLA_GetWidth,ICONCTRLA_GetHeight,ICONCTRLA_GetImageData1,ICONCTRLA_GetTransparentColor1,ICONCTRLA_GetImageMask1,ICONCTRLA_GetPalette1,ICONCTRLA_GetPaletteSize1};
struct TagItem {ULONG ti_Tag,ti_Data;};
struct ColorRegister {UBYTE red,green,blue;};
struct Image {int Width,Height,Depth;UWORD *ImageData;UBYTE PlanePick,PlaneOnOff;};
struct DiskObject {struct {void *GadgetRender;} do_Gadget;};
struct BitMap {int Depth,w,h;UBYTE pixels[64];};
struct RastPort {struct BitMap *BitMap;int pen;};
struct Screen {struct RastPort RastPort;struct {void *ColorMap;} ViewPort;};
struct NDDock {struct Screen *screen;struct DiskObject *icons[1];};
static struct {int lib_Version;} library={44},*IconBase=&library;
static int chunky;
static UBYTE data[49],image_mask[14];
static struct ColorRegister palette[2]={{12,34,56},{78,90,123}};
static struct BitMap *AllocBitMap(int w,int h,int d,int flags,struct BitMap *friend_map){struct BitMap *b=calloc(1,sizeof(*b));(void)flags;(void)friend_map;assert(w*h<=64);if(b){b->w=w;b->h=h;b->Depth=d;}return b;}
static void FreeBitMap(struct BitMap *b){free(b);}
static void InitRastPort(struct RastPort *p){memset(p,0,sizeof(*p));}
static void SetAPen(struct RastPort *p,ULONG pen){p->pen=(int)pen;}
static void WritePixel(struct RastPort *p,int x,int y){assert(x<p->BitMap->w && y<p->BitMap->h);p->BitMap->pixels[y*p->BitMap->w+x]=(UBYTE)p->pen;}
static void *alloc_data(ULONG bytes){return malloc(bytes);}
static void FreeVec(void *p){free(p);}
static LONG FindColor(void *map,ULONG r,ULONG g,ULONG b,int max_pen){int i;UBYTE rgb[3];(void)map;assert(max_pen==255);for(i=0;i<8;++i){nd_icon_magicwb_rgb(i,rgb);if(r==RGBBYTE(rgb[0]) && g==RGBBYTE(rgb[1]) && b==RGBBYTE(rgb[2]))return 40+i;}if(r==RGBBYTE(12) && g==RGBBYTE(34) && b==RGBBYTE(56))return 90;if(r==RGBBYTE(78) && g==RGBBYTE(90) && b==RGBBYTE(123))return 91;assert(0);return -1;}
static void IconControlA(struct DiskObject *icon,struct TagItem *t){(void)icon;assert(IconBase->lib_Version>=44);*(LONG *)t[0].ti_Data=chunky;*(LONG *)t[1].ti_Data=7;*(LONG *)t[2].ti_Data=7;*(UBYTE **)t[3].ti_Data=chunky ? data:NULL;*(LONG *)t[4].ti_Data=-1;*(PLANEPTR *)t[5].ti_Data=image_mask;*(struct ColorRegister **)t[6].ti_Data=chunky ? palette:NULL;*(LONG *)t[7].ti_Data=chunky ? 2:0;}
'''
code+=function('src/layout.c','void nd_icon_fit(')+'\n'
code+=function('src/dock_icons.c','static int icon_mask_pixel(',True)+'\n'
code+=function('src/dock_icons.c','int planar_icon_pixel(')+'\n'
code+=function('src/dock_icons.c','static int render_icon(',True)+'\n'
code+=r'''
int main(void){struct Image image;struct DiskObject icon;struct NDDock dock;struct Screen screen;struct BitMap reference,*bitmap,*mask;UWORD planes[21];int x,y,p,pen,w,h,mode;
memset(&image,0,sizeof(image));memset(&screen,0,sizeof(screen));memset(planes,0,sizeof(planes));memset(image_mask,0,sizeof(image_mask));for(y=1;y<6;++y)for(x=1;x<6;++x)image_mask[y*2]|=(UBYTE)(0x80U>>x);reference.Depth=8;screen.RastPort.BitMap=&reference;dock.screen=&screen;dock.icons[0]=&icon;icon.do_Gadget.GadgetRender=&image;
image.Width=image.Height=7;image.ImageData=planes;
/* An enclosed six-colour interior keeps transparency cropping independent
 * of screen pen numbers. Every row includes all non-background pens. */
for(mode=0;mode<4;++mode){chunky=mode==2;library.lib_Version=mode==3 ? 37:44;image.Depth=mode==1 ? 2:3;image.PlanePick=mode==1 ? 3:7;memset(planes,0,sizeof(planes));
for(y=0;y<7;++y)for(x=0;x<7;++x){pen=(x+1)&image.PlanePick;data[y*7+x]=(UBYTE)(x&1);for(p=0;p<image.Depth;++p)if(pen&(1<<p))planes[p*7+y]|=(UWORD)(0x8000U>>x);}
assert(render_icon(&dock,0,7,7,&bitmap,&mask,&w,&h) && w==7 && h==7);
for(y=1;y<6;++y)for(x=1;x<6;++x){pen=(x+1)&image.PlanePick;if(mask->pixels[y*7+x]){assert(bitmap->pixels[y*7+x]==(chunky ? 90+(x&1):((mode==0 || mode==3) ? 40+pen:pen)));}}
assert(mask->pixels[3*7+3]);
FreeBitMap(bitmap);FreeBitMap(mask);}
puts("Embedded palettes and classic MagicWB indices map to reordered screen pens; standard four-colour icons retain screen pens.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
    src=Path(tmp)/'palette.c';src.write_text(code);exe=Path(tmp)/'palette'
    subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c89','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(src),str(ROOT/'src/icon_mask.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
