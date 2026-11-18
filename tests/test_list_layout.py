#!/usr/bin/env python3
"""Execute the production list/button geometry for compact Workbench screens."""
from pathlib import Path
import os,subprocess,tempfile
R=Path(__file__).resolve().parents[1];s=(R/'plugin_src/common/nd_list.c').read_text()
a=s[s.index(' width=p->screen->Width-12;'):s.index(' p->font.ta_Name=')].replace('goto fail','return 0')
b=s[s.index(' top=height-rows*(button_h+4)-5;'):s.index(' p->button_count=count;')].replace('goto fail','return 0')
code=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef short WORD;typedef unsigned short UWORD;typedef char *STRPTR;
#define PLACETEXT_IN 1
#define BUTTON_KIND 1
#define TAG_DONE 0
struct Font{int tf_YSize;};struct Screen{int Width,Height,BarHeight,WBorTop;struct{struct Font *Font;}RastPort;};
struct Gadget{int x,y,w,h;};struct NDList{struct Screen *screen;struct Gadget *last,*buttons[9];char button_text[9][24];};
struct NewGadget{int ng_LeftEdge,ng_TopEdge,ng_Width,ng_Height,ng_GadgetID,ng_Flags;char *ng_GadgetText;};
static struct Gadget widgets[9];static int used;
static struct Gadget *CreateGadget(int kind,struct Gadget *last,struct NewGadget *ng,...){struct Gadget *g=&widgets[used++];(void)kind;(void)last;g->x=ng->ng_LeftEdge;g->y=ng->ng_TopEdge;g->w=ng->ng_Width;g->h=ng->ng_Height;assert(strlen(ng->ng_GadgetText)*8+8<=(unsigned int)g->w);return g;}
static int construct(struct NDList *p,unsigned int count,const char **buttons){int width,height,top,font_h,button_h,rows,list_h,i,bw;struct NewGadget ng;memset(&ng,0,sizeof(ng));
'''+a+b+r'''
 for(i=0;i<used;++i){struct Gadget *g=&widgets[i];assert(g->x>=4&&g->x+g->w<=width-4);assert(g->y>=12&&g->y+g->h<=height-4);}
 assert(list_h>=font_h*2);return 1;}
int main(void){struct NDList p;struct Screen screen;struct Font font;int h,f,n;const char *buttons[]={"Copy line","Open file","Mark read","Clear history","Check now","Resume view","Warnings/errors","Export...","Close"};screen.Width=640;screen.BarHeight=11;screen.WBorTop=2;screen.RastPort.Font=&font;p.screen=&screen;
 for(h=256;h<=480;h+=224)for(f=8;f<=16;f+=8)for(n=3;n<=9;n+=3){screen.Height=h;font.tf_YSize=f;used=0;assert(construct(&p,n,buttons));assert(used==n);}
 screen.Height=256;font.tf_YSize=8;used=0;assert(construct(&p,7,buttons));puts("Native list geometry: 3/6/7/9 buttons remain reachable at 640x256 and 640x480.");return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'check.c').write_text(code);cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations',str(p/'check.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-g','-O1','-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)
