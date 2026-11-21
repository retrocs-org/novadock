#!/usr/bin/env python3
"""Run the production percentage controls with a small GadTools test adapter."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/common/nd_panel.c').read_text()
def function(signature):
 a=s.index(signature);b=s.index('{',a);i=b+1;depth=1
 while depth:
  if s[i]=='{':depth+=1
  elif s[i]=='}':depth-=1
  i+=1
 return s[a:i]
fragment=s[s.index('    top=p->screen->WBorTop'):s.index('    p->window=OpenWindowTags')].replace('goto fail','return 0')
funcs='\n'.join(function(sig) for sig in ('int nd_panel_percent_get(', 'void nd_panel_percent_set(', 'int nd_panel_event('))
tags=sorted(set(re.findall(r'\b(?:GTSL_\w+|GTST_\w+|GTTX_\w+|PGA_\w+|GA_\w+|IDCMP_\w+|[A-Z]+_KIND|PLACETEXT_\w+|LORIENT_HORIZ|GFLG_SELECTED)\b',fragment+funcs)))
code=r'''
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef unsigned long ULONG;typedef unsigned short UWORD;typedef char *STRPTR;
#define TRUE 1
#define TAG_DONE 0
#define ND_PANEL_PERCENT 65535U
#define ND_PANEL_SWATCH 65534U
'''+ '\n'.join(f'#define {t} {i+10}' for i,t in enumerate(tags))+r'''
struct Font{int ta_YSize;};struct Screen{int WBorTop,Height,BarHeight;struct Font *Font;};
struct Window{void *UserPort;};
struct NewGadget{int ng_LeftEdge,ng_TopEdge,ng_Width,ng_Height,ng_GadgetID,ng_Flags;char *ng_GadgetText;};
struct Gadget{int GadgetID,Flags,level,disabled;struct NewGadget position;};
struct NDPanelField{const char *label,*value;unsigned int limit;};
struct NDPanel{struct Screen *screen;struct Window *window;struct Gadget *fields[12];unsigned char percentage[12],percent_dragging[12];unsigned int count,limits[12],button_count;unsigned char swatch[12];struct Gadget *buttons[4];char button_text[4][32];char labels[12][32],text[12][160];};
static void draw_swatches(struct NDPanel *p){(void)p;}
static struct Gadget gadgets[20];static int created,level_sets,disabled_sets;static struct IntuiMessage{ULONG Class;UWORD Code;void *IAddress;} message;static int queued,replied;
static void *CreateGadget(int kind,void *last,struct NewGadget *ng,...){
 struct Gadget *g=&gadgets[created++];va_list ap;ULONG tag,v;g->GadgetID=ng->ng_GadgetID;g->position=*ng;(void)last;
 if(kind==SLIDER_KIND){va_start(ap,ng);while((tag=va_arg(ap,ULONG))){v=va_arg(ap,ULONG);if(tag==GTSL_Level)g->level=(int)v;}va_end(ap);}
 return g;}
/* Use fixed-width tag values in the captured constructor, like the 32-bit target. */
static void GT_GetGadgetAttrs(struct Gadget *g,struct Window *w,void *r,ULONG tag,ULONG dest,...){(void)w;(void)r;assert(tag==GTSL_Level);*(ULONG *)dest=g->level;}
static void GT_SetGadgetAttrs(struct Gadget *g,struct Window *w,void *r,...){va_list ap;ULONG tag,v;(void)w;(void)r;va_start(ap,r);while((tag=va_arg(ap,ULONG))){v=va_arg(ap,ULONG);if(tag==GTSL_Level){g->level=(int)v;++level_sets;}if(tag==GA_Disabled){g->disabled=(int)v;++disabled_sets;}}va_end(ap);}
static struct IntuiMessage *GT_GetIMsg(void *port){(void)port;if(!queued)return NULL;queued=0;return &message;}
static void GT_ReplyIMsg(struct IntuiMessage *m){(void)m;++replied;}
static void GT_BeginRefresh(struct Window *w){(void)w;}static void GT_EndRefresh(struct Window *w,int yes){(void)w;(void)yes;}
static int construct(struct NDPanel *p,int w,const struct NDPanelField *fields,unsigned int count,const char **buttons,unsigned int buttons_count){
 struct Gadget *last=NULL;struct NewGadget ng;unsigned int i;int top,bw,label_x=92;memset(&ng,0,sizeof(ng));p->count=count;
'''+fragment+r'''
 return 1;}
'''+funcs+r'''
int main(void){struct NDPanel p;struct Screen screen;struct Font font;struct Window win;int h,i,n;
 const char *buttons[]={"Turn on","Turn off","Toggle","Close"};struct NDPanelField fields[]={{"Device","Desk lamp",0},{"State","on",0},{"Updated","Sample",0},{"Status","Connected",0},{"Brightness","50",ND_PANEL_PERCENT}};
 font.ta_YSize=8;screen.WBorTop=2;screen.BarHeight=11;screen.Font=&font;
 for(h=256;h<=480;h+=224)for(n=4;n<=5;++n){memset(&p,0,sizeof(p));p.screen=&screen;p.window=&win;screen.Height=h;created=0;
  assert(construct(&p,560,fields,n,buttons,4));assert(created==n+4);
  for(i=0;i<created;++i){struct NewGadget *g=&gadgets[i].position;assert(g->ng_LeftEdge>=4&&g->ng_LeftEdge+g->ng_Width+(i==4&&n==5 ? 56:0)<=556);assert(g->ng_TopEdge+g->ng_Height+12<h-11);}
  if(n==5){struct Gadget *g=p.fields[4];assert(nd_panel_percent_get(&p,4)==50);
   level_sets=disabled_sets=0;g->Flags=GFLG_SELECTED;nd_panel_percent_set(&p,4,80,1);assert(!level_sets&&g->level==50&&!g->disabled);
   g->Flags=0;nd_panel_percent_set(&p,4,80,1);assert(g->level==80);nd_panel_percent_set(&p,4,-1,0);assert(g->level==80&&g->disabled);
   nd_panel_percent_set(&p,4,140,1);assert(g->level==100&&!g->disabled);assert(nd_panel_percent_get(&p,0)==-1);
   /* GadTools can keep the outer gadget's SELECTED bit clear during drag. */
   g->Flags=0;queued=1;message.Class=IDCMP_GADGETDOWN;message.IAddress=g;
   assert(!nd_panel_event(&p));g->level=37;level_sets=0;
   nd_panel_percent_set(&p,4,100,1);assert(g->level==37&&!level_sets);
   nd_panel_percent_set(&p,4,100,1);assert(g->level==37&&!level_sets);
   queued=1;message.Class=IDCMP_GADGETUP;message.IAddress=g;
   assert(nd_panel_event(&p)==5&&replied>0);assert(nd_panel_percent_get(&p,4)==37);
   assert(!nd_panel_event(&p));nd_panel_percent_set(&p,4,37,1);assert(g->level==37);
   queued=1;message.Class=IDCMP_GADGETDOWN;message.IAddress=g;assert(!nd_panel_event(&p));
   nd_panel_percent_set(&p,4,-1,0);assert(g->disabled);
   nd_panel_percent_set(&p,4,65,1);assert(g->level==65&&!g->disabled);
   queued=1;message.Class=IDCMP_RAWKEY;message.Code=0x45;assert(nd_panel_event(&p)==-1);
  }
 }
 puts("HASS controls: 640x256/480 bounds, confirmed percentage, drag retention, offline disabling and release events passed.");return 0;}
'''
# Host varargs need explicit ULONG, unlike the target's uniform 32-bit int/long.
# Cast only call arguments in the production fragments; never alter definitions or comparisons.
def cast_calls(source):
 for name in ('CreateGadget','GT_SetGadgetAttrs'):
  offset=0
  while True:
   a=source.find(name+'(',offset)
   if a<0:break
   b=source.find(');',a)
   if b<0:break
   call=source[a:b]
   # Only real call sites, not the mock declarations.
   if 'struct Gadget *' not in call and 'int kind' not in call:
    call=re.sub(r'(?<=,)(\s*)(GTSL_\w+|GTST_\w+|GTTX_\w+|PGA_\w+|GA_\w+|PLACETEXT_\w+|LORIENT_HORIZ|TRUE|TAG_DONE|\d+|level|value)\b',r'\1(ULONG)\2',call)
    source=source[:a]+call+source[b:];b=a+len(call)
   offset=b+2
 return source
code=cast_calls(code)
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'ui.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra',str(p/'ui.c'),'-o',str(p/'ui')],check=True)
 subprocess.run([str(p/'ui')],check=True)
