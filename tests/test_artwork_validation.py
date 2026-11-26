#!/usr/bin/env python3
"""Compare optimized artwork validation with the previous independent algorithm."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from pack_plugin_art import build_bank
REFERENCE=r'''
static int reference_validate(const unsigned char *d,unsigned long size)
{
 unsigned int frames,i,j,count,header;unsigned long offset,table,keys[ND_ARTWORK_MAX_FRAMES];const unsigned char *r,*p;
 if(!d || size<56 || size>ND_ARTWORK_MAX_BYTES || (memcmp(d,"NDAR\000",5) || (d[5]!=1 && d[5]!=2)))return 0;
 header=nd_artwork_header(d);if(size<header)return 0;
 if(d[5]==2){for(i=0;i<16;++i)if(d[56+i]>5 || (!i && d[56+i]))return 0;}
 frames=nd_artwork_u16(d+6);if(!frames || frames>ND_ARTWORK_MAX_FRAMES)return 0;table=(unsigned long)header+(unsigned long)frames*ND_ARTWORK_RECORD;if(table>size)return 0;
 for(i=0;i<frames;++i){r=d+header+(unsigned long)i*ND_ARTWORK_RECORD;
  if(!memchr(r,0,ND_ARTWORK_NAME) || !nd_artwork_name((const char *)r) || (d[5]==1 ? r[25]!=0:(r[25]&~15U)!=0))return 0;
  if((r[25]&ND_ARTWORK_HEADING) && !(r[25]&ND_ARTWORK_VALUE))return 0;
  keys[i]=nd_artwork_key((const char *)r);
  for(j=0;j<i;++j){p=d+header+(unsigned long)j*ND_ARTWORK_RECORD;if(keys[i]==keys[j] && strcmp((const char *)r,(const char *)p))return 0;if(r[24]==p[24]&&!strcmp((const char *)r,(const char *)p))return 0;}
  count=nd_artwork_u16(r+26);offset=nd_artwork_u32(r+28);if(!count || count>1024 || offset<table || offset>size || (unsigned long)count*5UL>size-offset)return 0;
  for(j=0;j<count;++j){p=d+offset+(unsigned long)j*5;if(!p[2] || !p[3] || p[0]+p[2]>32 || p[1]+p[3]>32 || !p[4] || p[4]>15)return 0;}
 }
 return 1;
}
'''
CODE=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int opens,allocations;
static FILE *counted_open(const char *path,const char *mode){++opens;return fopen(path,mode);}
static void *counted_malloc(size_t size){++allocations;return malloc(size);}
#define fopen counted_open
#define malloc counted_malloc
#include "nd_artwork.h"
#undef fopen
#undef malloc
'''+REFERENCE+r'''
static void same(const unsigned char *d,unsigned long size){assert(nd_artwork_validate(d,size)==reference_validate(d,size));}
int main(int argc,char **argv){int arg,i,j;unsigned long seed=0xc0ffeeUL,at,n;unsigned int header;unsigned char *d;struct NDArtwork a;
 for(arg=1;arg<argc;++arg){memset(&a,0,sizeof(a));opens=allocations=0;assert(nd_artwork_load(&a,argv[arg]));assert(opens==1&&allocations==2);n=a.size;assert(reference_validate(a.data,n));d=malloc(n+1);assert(d);
  same(a.data,n);for(i=0;i<128;++i){memcpy(d,a.data,n);seed=(seed*1664525UL+1013904223UL)&0xffffffffUL;at=seed%n;d[at]^=(unsigned char)(1+(seed>>24));same(d,n);}
  for(i=0;i<80 && (unsigned long)i<n;++i)same(a.data,(unsigned long)i);
  header=nd_artwork_header(a.data);
  if(nd_artwork_u16(a.data+6)>=2){
   memcpy(d,a.data,n);memcpy(d+header+32,d+header,25);assert(!nd_artwork_validate(d,n));same(d,n);
   memcpy(d,a.data,n);memset(d+header,0,24);memset(d+header+32,0,24);
   strcpy((char *)d+header,"O7sReRXXibgl");strcpy((char *)d+header+32,"FnsM2mlaKd2Y");d[header+24]=0;d[header+32+24]=1;
   assert(nd_artwork_key("O7sReRXXibgl")==nd_artwork_key("FnsM2mlaKd2Y"));assert(!nd_artwork_validate(d,n));same(d,n);
  }
  if(strstr(argv[arg],".assets/Artwork.ndart")){char program[ND_ARTWORK_PATH],why[96];size_t length=strlen(argv[arg])-strlen(".assets/Artwork.ndart");assert(length<sizeof(program));memcpy(program,argv[arg],length);program[length]=0;
   opens=allocations=0;assert(!nd_artwork_package_problem(program,"artwork",why,sizeof(why)));assert(opens==1&&allocations==1);
  }
  free(d);nd_artwork_close(&a);
 }
 puts("Artwork validation matches the previous algorithm for all shipped banks, 256-frame limits, duplicates, real name-hash collisions and mutated/truncated data.");return 0;}
'''
with tempfile.TemporaryDirectory(prefix='novadock-art-validation-') as tmp:
 p=Path(tmp);fixtures=[];pixel=[[0]*32 for _ in range(32)];pixel[0][0]=1
 for name,frames in [('states',[{'name':'Animated','state':i,'pixels':pixel} for i in range(256)]),('names',[{'name':'Frame%03d'%i,'state':0,'pixels':pixel} for i in range(256)])]:
  bank=p/(name+'.ndart');bank.write_bytes(build_bank(frames,[[0,0,0]]*16));fixtures.append(bank)
 (p/'test.c').write_text(CODE)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test'),*[str(f) for f in sorted((ROOT/'Plugins').glob('*.assets/Artwork.ndart'))],*[str(f) for f in fixtures]],check=True)
