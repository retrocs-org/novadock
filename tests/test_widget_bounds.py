#!/usr/bin/env python3
"""widget_content_bounds() scans masks byte-wise; it must report exactly the
bounding box a per-pixel scan finds, including partial last bytes and bits
set beyond the widget width (padding in BytesPerRow)."""
from pathlib import Path
import os, subprocess, tempfile
ROOT = Path(__file__).resolve().parents[1]
src = (ROOT / 'src/dock_draw.c').read_text(encoding='latin-1')
a = src.index('static void widget_content_bounds(')
production = src[a:src.index('\n}', a) + 2]
program = r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct BitMap {int BytesPerRow;unsigned char *Planes[1];};
struct NDHoverWidget {struct BitMap *mask;int content_left,content_top,content_width,content_height;};
typedef unsigned long ULONG;
static void WaitBlit(void){}
''' + production + r'''
static void reference(const struct BitMap *m,int width,int height,int *l,int *t,int *r,int *b)
{int x,y;*l=width;*t=height;*r=-1;*b=-1;
 for(y=0;y<height;++y)for(x=0;x<width;++x)
  if(m->Planes[0][y*m->BytesPerRow+x/8]&(128>>(x&7))){if(x<*l)*l=x;if(x>*r)*r=x;if(y<*t)*t=y;*b=y;}}
int main(void)
{
 static unsigned char plane[64*40];
 struct BitMap mask;struct NDHoverWidget cache;int run,w,h,l,t,r,b,n,i;
 srand(7);mask.Planes[0]=plane;mask.BytesPerRow=8;cache.mask=&mask;
 for(run=0;run<20000;++run){
  w=1+rand()%64;h=1+rand()%40;memset(plane,0,sizeof(plane));
  n=rand()%6;if(run%5==0)n=0;
  for(i=0;i<n;++i)plane[rand()%(mask.BytesPerRow*h)]|=(unsigned char)(1<<(rand()%8));
  if(run%7==0)memset(plane,0xff,sizeof(plane));
  reference(&mask,w,h,&l,&t,&r,&b);
  widget_content_bounds(&cache,w,h);
  if(r<l){assert(cache.content_width==0&&cache.content_height==0);continue;}
  assert(cache.content_left==l&&cache.content_top==t);
  assert(cache.content_width==r-l+1&&cache.content_height==b-t+1);
 }
 puts("Widget bounds: byte-wise mask scan matches the per-pixel bounding box.");
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='novadock-widget-bounds-') as tmp:
    p = Path(tmp); (p / 'bounds.c').write_text(program)
    cmd = [os.environ.get('CC', 'cc'), '-std=c89', '-Wall', '-Wno-misleading-indentation', str(p / 'bounds.c'), '-o', str(p / 'bounds')]
    if os.environ.get('SANITIZE') == '1': cmd += ['-fsanitize=address,undefined']
    subprocess.run(cmd, check=True); subprocess.run([str(p / 'bounds')], check=True)
