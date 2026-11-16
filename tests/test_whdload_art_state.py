#!/usr/bin/env python3
"""Run the production brush converter with real artwork and a bounded mock bitmap."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
def function(path,signature):
    text=path.read_text();start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
fit=function(root/'src/layout.c','void nd_icon_fit(')
convert=function(root/'src/dock_icons.c','static void build_one_brush(struct NDDock *dock,int i,int max_width,\n                            int max_height)\n{')
harness=r'''
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nd_ilbm.h"
#include "nd_collection_icons.h"
#include "nd_whdload_status.h"
typedef unsigned long ULONG;
#define BMF_CLEAR 1
struct BitMap {int Depth,w,h;unsigned char pixels[96*96];};
struct RastPort {struct BitMap *BitMap;int pen;};
struct Screen {struct RastPort RastPort;};
struct NDDock {struct NDILBM *brushes[1];struct BitMap *brush_bitmaps[1],*brush_masks[1];struct Screen *screen;long brush_pens[1][16];int scaled_icon_width[1],scaled_icon_height[1];};
static struct BitMap *AllocBitMap(int w,int h,int depth,int flags,struct BitMap *friend_map){struct BitMap *b=calloc(1,sizeof(*b));(void)flags;(void)friend_map;assert(w<=96 && h<=96);if(b){b->w=w;b->h=h;b->Depth=depth;}return b;}
static void FreeBitMap(struct BitMap *b){free(b);}
static void InitRastPort(struct RastPort *p){memset(p,0,sizeof(*p));}
static void SetAPen(struct RastPort *p,ULONG pen){p->pen=(int)pen;}
static void WritePixel(struct RastPort *p,int x,int y){assert(x>=0 && x<p->BitMap->w && y>=0 && y<p->BitMap->h);p->BitMap->pixels[y*96+x]=(unsigned char)p->pen;}
'''+fit+'\n'+convert+r'''
int main(void){struct NDDock dock;struct NDILBM image;struct Screen screen;struct BitMap reference;int file,limit,x,y,k,expected,painted,mode,state;const char *text;
assert(nd_collection_icon_index("Demos")>=0);
memset(&screen,0,sizeof(screen));screen.RastPort.BitMap=&reference;reference.Depth=8;
for(file=0;file<ND_COLLECTION_ICON_COUNT;++file)for(limit=9;limit<=32;limit+=23)for(mode=0;mode<2;++mode){
 memset(&dock,0,sizeof(dock));assert(nd_ilbm_load(nd_collection_icons[file].path,&image));assert(image.pixels[0]==image.transparent);
 dock.brushes[0]=&image;dock.screen=&screen;for(k=0;k<16;++k)dock.brush_pens[0][k]=k;
 if(mode)dock.brush_pens[0][2]=-1;
 build_one_brush(&dock,0,limit,limit);assert(dock.brush_bitmaps[0] && dock.brush_masks[0]);
 assert(dock.scaled_icon_width[0]>0 && dock.scaled_icon_height[0]>0);painted=0;
 for(y=0;y<dock.scaled_icon_height[0];++y)for(x=0;x<dock.scaled_icon_width[0];++x){
  k=image.pixels[(y*image.height/dock.scaled_icon_height[0])*ND_ICON_MAX_DIM+x*image.width/dock.scaled_icon_width[0]];
  expected=k!=image.transparent && dock.brush_pens[0][k]>=0;
  assert(dock.brush_masks[0]->pixels[y*96+x]==expected);
  if(expected){assert(dock.brush_bitmaps[0]->pixels[y*96+x]==k);++painted;}
 }
 if(limit==32)assert(painted>50);
 FreeBitMap(dock.brush_bitmaps[0]);FreeBitMap(dock.brush_masks[0]);
}
for(state=1;state<=6;++state){text=nd_whd_folder_value(state);assert(*text && strlen(text)<32 && nd_whd_folder_code(text)==state);}
assert(nd_whd_folder_code("CHECK FOLDER")==ND_WHD_FOLDER_UNREADABLE);
assert(nd_whd_folder_code("OPEN LIST")==0 && nd_whd_folder_code("INDEXING")==0 && nd_whd_folder_code("0 ITEMS")==0 && nd_whd_folder_code("INDEX INCOMPLETE")==0);
assert(strcmp(nd_whd_folder_value(ND_WHD_FOLDER_UNSET),nd_whd_folder_value(ND_WHD_FOLDER_MISSING)));
puts("Transparent and unmapped pixels preserve complete icon/mask rendering; WHDLoad drawer states stay distinct.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
    source=Path(tmp)/'art.c';source.write_text(harness);exe=Path(tmp)/'art'
    subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c89','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-I'+str(root/'include'),str(source),str(root/'src/ilbm.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],cwd=root,check=True)
