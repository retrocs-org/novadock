#!/usr/bin/env python3
"""Count actual Notes layout/repaint work while extending a selection."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
def extract(s,name):
 a=s.rfind('static ',0,s.index(name));b=s.index('{',a);i=b+1;n=1
 while n:
  if s[i]=='{':n+=1
  elif s[i]=='}':n-=1
  i+=1
 return s[a:i]
class Render(unittest.TestCase):
 def test_cached_layout_and_changed_rows(self):
  s=(ROOT/'plugin_src/notes/nd_note_editor.c').read_text()
  h=r"""#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_markdown.h"
typedef char *STRPTR;
#define TAG_DONE 0
#define GTSC_Total 1
#define GTSC_Visible 2
#define GTSC_Top 3
#define JAM1 0
#define FILLPEN 0
#define FILLTEXTPEN 1
struct TextFont {unsigned int tf_YSize,tf_Baseline;};struct RastPort {int pen;};
struct Window {struct RastPort *RPort;int Width;};struct DrawInfo {int dri_Pens[2];};
struct NDNoteEditor {struct Window *window;struct DrawInfo *di;struct TextFont *fonts[3];struct NDNoteText edit;
 unsigned short start[2048],length[2048],heights[2048],baselines[2048];
 unsigned int lines,top,visible,body_y,body_h,painted_cursor,painted_anchor,painted_top;
 int layout_dirty,painted,paper,focus;int inks[6];void *scroll;};
static int widths,fills,text_calls;static const char text[]="abcdefghijklmnop";
static const char *document(struct NDNoteEditor *e){(void)e;return text;}
static struct NDMDStyle style_at(struct NDNoteEditor *e,unsigned int p){struct NDMDStyle f;(void)e;(void)p;memset(&f,0,sizeof(f));return f;}
static unsigned int face_for(struct NDMDStyle f){(void)f;return 0;}
static unsigned int glyph_width(struct NDNoteEditor *e,unsigned int p){(void)e;(void)p;++widths;return 8;}
static void sync_controls(struct NDNoteEditor *e){(void)e;}
static void set_type(struct NDNoteEditor *e,struct NDMDStyle f){(void)e;(void)f;}
static void GT_SetGadgetAttrs(void *g,struct Window *w,void *tags,...){(void)g;(void)w;(void)tags;}
static void SetDrMd(struct RastPort *rp,int mode){(void)rp;(void)mode;}
static void SetAPen(struct RastPort *rp,int pen){rp->pen=pen;}
static void RectFill(struct RastPort *rp,int a,int b,int c,int d){(void)rp;(void)a;(void)b;(void)c;(void)d;++fills;}
static void Move(struct RastPort *rp,int x,int y){(void)rp;(void)x;(void)y;}
static void Draw(struct RastPort *rp,int x,int y){(void)rp;(void)x;(void)y;}
static void Text(struct RastPort *rp,STRPTR text,unsigned int n){(void)rp;(void)text;(void)n;++text_calls;}
"""
  for name in ('cursor_line(','layout(','page_lines(','pixel_offset(','paint('):h+='\n'+extract(s,name)
  h+=r"""
int main(void){struct NDNoteEditor e;struct Window w;struct RastPort rp;struct DrawInfo di;struct TextFont font;int initial,i;
 memset(&e,0,sizeof(e));memset(&rp,0,sizeof(rp));memset(&di,0,sizeof(di));font.tf_YSize=8;font.tf_Baseline=7;
 w.Width=90;w.RPort=&rp;e.window=&w;e.di=&di;e.fonts[0]=&font;e.body_h=40;e.layout_dirty=1;e.focus=1;
 layout(&e);assert(e.lines==4);initial=widths;for(i=0;i<1000;++i)layout(&e);assert(widths==initial);
 paint(&e,1,0);assert(text_calls==4);fills=text_calls=0;
 e.edit.cursor=5;e.edit.anchor=0;paint(&e,1,1);
 /* Two changed rows plus their selected rectangles; untouched rows render nothing. */
 assert(text_calls==3&&fills==4);
 puts("Notes render: 1000 caret-only updates reuse layout; selection repaints only affected rows.");return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'check.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'plugin_src/notes'),str(p/'check.c'),'-o',str(p/'check')],check=True)
   subprocess.run([str(p/'check')],check=True)
 def test_real_elapsed_time_for_ui_heartbeat(self):
  s=(ROOT/'plugin_src/notes/notes.c').read_text()
  h='#include <assert.h>\ntypedef long LONG;struct DateStamp {long ds_Days,ds_Minute,ds_Tick;};\n'+extract(s,'note_loop_elapsed(')+r"""
int main(void){struct DateStamp a={1,1439,2990},b={2,0,10};assert(note_loop_elapsed(&a,&b)==20);b.ds_Days=0;assert(note_loop_elapsed(&a,&b)==500);b=a;b.ds_Tick+=20;assert(note_loop_elapsed(&a,&b)==20);return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'check.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89',str(p/'check.c'),'-o',str(p/'check')],check=True)
   subprocess.run([str(p/'check')],check=True)
if __name__=='__main__':unittest.main()
