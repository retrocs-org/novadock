#!/usr/bin/env python3
"""Compare planar cache resampling with a pixel reference, including crop offsets."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
code=r"""
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_planar_scale.h"
static unsigned char source[8][80*192],dest[8][32*128];
static int pixel(const unsigned char *p,int stride,int x,int y){return (p[y*stride+x/8]>>(7-(x&7)))&1;}
int main(void){unsigned char *s[8],*d[8];int i,j,k,depth,x,y,w,h,zw,zh,p,xx,yy,expected;unsigned long cases=0;
 int offsets[]={0,1,7,15,16,203},widths[]={17,26,34,48,79},sizes[]={16,26,34,48,65,127};
 for(p=0;p<8;++p){s[p]=source[p];d[p]=dest[p];for(i=0;i<80*192;++i)source[p][i]=(unsigned char)((i*73+i/80*19+p*37)^0x5a);}
 for(depth=1;depth<=8;++depth)for(i=0;i<6;++i)for(j=0;j<5;++j)for(k=0;k<6;++k){
  x=offsets[i];y=105;w=widths[j];h=34;zw=sizes[k];zh=sizes[5-k];memset(dest,0xa5,sizeof(dest));
  assert(nd_planar_scale(s,80,192,d,32,128,depth,x,y,w,h,zw,zh));
  for(p=0;p<8;++p)for(yy=0;yy<128;++yy)for(xx=0;xx<256;++xx){
   expected=p<depth&&xx<zw&&yy<zh ? pixel(source[p],80,x+xx*w/zw,y+yy*h/zh):((0xa5>>(7-(xx&7)))&1);
   assert(pixel(dest[p],32,xx,yy)==expected);
  }
  ++cases;
 }
 assert(!nd_planar_scale(s,80,192,d,32,128,8,620,105,34,34,34,34));
 assert(!nd_planar_scale(s,80,192,d,32,128,8,0,180,34,34,34,34));
 assert(!nd_planar_scale(s,80,192,d,32,128,8,0,0,34,34,257,34));
 printf("Planar scaling: %lu cropped up/downscale cases, all depths, padding preserved and invalid bounds rejected.\n",cases);return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-O1','-Wall','-Wextra','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
