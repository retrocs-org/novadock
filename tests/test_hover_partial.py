#!/usr/bin/env python3
"""Partial hover frames must produce exactly the pixels of a full repaint.

A pixel model draws each tile the way paint_hover_wave() does: masked
artwork in its box, then a label, running dot and meter across the full tile
width centred on the box (below the box on vertical docks), hovered tile
last.  Over thousands of random magnification moves, hover changes and
content repaints, a retained frame updated with the production
hover_tile_extent(), nd_hover_dirty_span() and nd_hover_draw_set() must equal
a frame repainted from scratch."""
from pathlib import Path
import os, subprocess, tempfile
ROOT = Path(__file__).resolve().parents[1]
draw = (ROOT / 'src/dock_draw.c').read_text(encoding='latin-1')
a = draw.index('static void hover_tile_extent(')
extent = draw[a:draw.index('\n}', a) + 2]
program = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_hover_dirty.h"
#define W 400
#define H 160
#define N 5
struct RastPort {int TxHeight;};
struct Window {struct RastPort *RPort;};
struct NDDock {struct NDRect hover_rects[N];struct Window *window;int vertical,tw,th,gap;};
static int vertical(struct NDDock *d){return d->vertical;}
static void tile_coords(struct NDDock *d,int i,int *x,int *y)
{if(d->vertical){*x=10;*y=10+i*(d->th+d->gap);}else{*x=10+i*(d->tw+d->gap);*y=60;}}
static void tile_dimensions(struct NDDock *d,int i,int *w,int *h){(void)i;*w=d->tw;*h=d->th;}
''' + extent + r'''
static unsigned char full[H][W],frame[H][W];
static int rp_ascent=6;
static unsigned char content[N];
static int hovered;
static unsigned char visible[N];
static long drawn_total,drawn_frames;
static void put(unsigned char (*c)[W],int x,int y,unsigned char v){if(x>=0&&x<W&&y>=0&&y<H)c[y][x]=v;}
static void background(unsigned char (*c)[W],int x0,int y0,int x1,int y1)
{int x,y;for(y=y0;y<y1;++y)for(x=x0;x<x1;++x)put(c,x,y,(unsigned char)((x*7+y*3)&15));}
/* One tile as paint_hover_wave() draws it. */
static void tile(struct NDDock *d,unsigned char (*c)[W],int i)
{
    const struct NDRect *b=&d->hover_rects[i];int x,y,tx,ty,w,h,centre=b->left+b->width/2,base;
    tile_coords(d,i,&tx,&ty);tile_dimensions(d,i,&w,&h);
    for(y=0;y<b->height;++y)for(x=0;x<b->width;++x)
        if((x+y+i)%5)put(c,b->left+x,b->top+y,(unsigned char)(32+i*16+content[i]));   /* masked art */
    base=d->vertical ? b->top+b->height+d->window->RPort->TxHeight+2 : ty+h-9;
    if(i!=hovered)for(x=centre-(w-6)/2;x<centre-(w-6)/2+w-6;++x)put(c,x,base,(unsigned char)(200+i));  /* label */
    for(x=centre-w/2;x<centre-w/2+w;x+=7){put(c,x,base+3,(unsigned char)(220+i));put(c,x,base+4,(unsigned char)(220+i));} /* meter */
    for(y=base-rp_ascent;y<=base+2;++y)put(c,centre-(w-6)/2,y,(unsigned char)(240+i));                 /* glyph height and descender */
    put(c,centre-1,b->top+b->height+2,250);                                                              /* running dot */
}
static void compose(struct NDDock *d,unsigned char (*c)[W],int clip,int lo,int hi,const struct NDRect *ext)
{
    int pass,i;unsigned char set[N];
    if(clip){drawn_total+=nd_hover_draw_set(ext,visible,N,d->vertical,lo,hi,hovered,set);++drawn_frames;}
    for(pass=0;pass<2;++pass)for(i=0;i<N;++i){
        if((i==hovered)!=(pass==1) || !visible[i])continue;
        if(clip && !set[i])continue;
        tile(d,c,i);
    }
}
int main(void)
{
    struct NDDock d;struct Window win;struct RastPort rp;
    struct NDRect pbox[N],pext[N],cext[N],shelf={0,0,W,H};
    unsigned char changed[N];
    int step,i,orient,partials=0,prev_hover,variant;
    static const int gaps[3]={0,2,5};
    srand(11);win.RPort=&rp;rp.TxHeight=8;d.window=&win;
    for(variant=0;variant<6;++variant){
        orient=variant&1;d.gap=gaps[variant/2];
        d.vertical=orient;d.tw=60;d.th=orient ? 24:60;hovered=-1;
        for(i=0;i<N;++i)visible[i]=(unsigned char)(variant<4 || i!=N-1);   /* last tile scrolled away */
        memset(content,0,sizeof(content));
        for(i=0;i<N;++i){int x,y;tile_coords(&d,i,&x,&y);d.hover_rects[i].left=x+6;d.hover_rects[i].top=y+4;
            d.hover_rects[i].width=d.tw-12;d.hover_rects[i].height=orient ? 16:40;}
        background(frame,0,0,W,H);compose(&d,frame,0,0,0,cext);
        for(i=0;i<N;++i){pbox[i]=d.hover_rects[i];hover_tile_extent(&d,i,&pext[i]);}
        prev_hover=hovered;
        for(step=0;step<4000;++step){
            int lo,hi,k=rand()%N;
            memset(changed,0,sizeof(changed));
            switch(rand()%4){
            case 0: d.hover_rects[k].left+=rand()%9-4;break;
            case 1: d.hover_rects[k].top+=rand()%5-2;d.hover_rects[k].height+=rand()%5-2;
                    if(d.hover_rects[k].height<4)d.hover_rects[k].height=4;break;
            case 2: d.hover_rects[k].width+=rand()%9-4;if(d.hover_rects[k].width<6)d.hover_rects[k].width=6;break;
            default: if(rand()%2){content[k]=(unsigned char)(content[k]+1);changed[k]=1;} else hovered=rand()%(N+1)-1;
            }
            if(hovered!=prev_hover){if(prev_hover>=0)changed[prev_hover]=1;if(hovered>=0)changed[hovered]=1;}
            /* Whether any tile is hovered is part of the frame key: full frame. */
            for(i=0;i<N;++i)hover_tile_extent(&d,i,&cext[i]);
            if((hovered<0)!=(prev_hover<0)){
                background(frame,0,0,W,H);compose(&d,frame,0,0,0,cext);
            }else if(nd_hover_dirty_span(pbox,d.hover_rects,pext,cext,changed,visible,N,d.vertical,&shelf,&shelf,
                                         d.vertical ? H:W,&lo,&hi)){
                if(d.vertical)background(frame,0,lo,W,hi);else background(frame,lo,0,hi,H);
                compose(&d,frame,1,lo,hi,cext);++partials;
            }
            background(full,0,0,W,H);compose(&d,full,0,0,0,cext);
            if(memcmp(full,frame,sizeof(full))){printf("mismatch: orientation %d gap %d step %d\n",orient,d.gap,step);return 1;}
            for(i=0;i<N;++i){pbox[i]=d.hover_rects[i];pext[i]=cext[i];}
            prev_hover=hovered;
        }
    }
    printf("Partial hover frames: %d recomposed spans (gaps 0, 2, 5; both orientations; a hidden tile) match full repaints pixel for pixel, redrawing %ld.%ld of %d tiles on average.\n",
           partials,drawn_total/drawn_frames,(drawn_total*10/drawn_frames)%10,N);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='novadock-hover-partial-') as tmp:
    p = Path(tmp); (p / 'partial.c').write_text(program)
    cmd = [os.environ.get('CC', 'cc'), '-std=c89', '-Wall', '-Wextra', '-Wno-unused-function', '-I' + str(ROOT / 'include'),
           str(p / 'partial.c'), '-o', str(p / 'partial')]
    if os.environ.get('SANITIZE') == '1': cmd += ['-fsanitize=address,undefined']
    subprocess.run(cmd, check=True); subprocess.run([str(p / 'partial')], check=True)
