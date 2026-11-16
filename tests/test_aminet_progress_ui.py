#!/usr/bin/env python3
"""Exercise actual footer drawing and worker reporting without native libraries."""
from pathlib import Path
import subprocess,tempfile,os
r=Path(__file__).resolve().parents[1]
s=(r/'plugin_src/aminet/nd_aminet_ui.h').read_text();footer=s[s.index('static void am_download_state'):s.index('static void am_detach')]
s=(r/'plugin_src/aminet/aminet.c').read_text();report=s[s.index('static char progress_path'):s.index('static void am_action')]
code=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_aminet.h"
typedef unsigned long ULONG;typedef unsigned short UWORD;typedef long LONG;typedef long BPTR;
#define BACKGROUNDPEN 0
#define TEXTPEN 1
#define FILLPEN 2
#define JAM1 0
#define MODE_NEWFILE 1
struct RastPort {int dummy;};struct TextExtent {int dummy;};
struct DrawInfo {UWORD dri_Pens[3];};struct Window {struct RastPort *RPort;};
struct UI {struct Window *window;void *font,*screen;int width,height,download_active;unsigned long received,total;char status[128];};
static struct UI state,*ui=&state;static struct DrawInfo di={{0,1,2}};static int rectangles,pen,filled;
static void SetFont(struct RastPort *r,void *f){(void)r;(void)f;}
static struct DrawInfo *GetScreenDrawInfo(void *s){(void)s;return &di;}
static void FreeScreenDrawInfo(void *s,struct DrawInfo *d){(void)s;(void)d;}
static void SetDrMd(struct RastPort *r,int m){(void)r;(void)m;}
static void SetAPen(struct RastPort *r,int p){(void)r;pen=p;}
static void RectFill(struct RastPort *r,int x,int y,int right,int bottom){(void)r;assert(x>=8&&right<ui->width-8&&right>=x&&y>=ui->height-37&&bottom<ui->height-6&&bottom>=y);++rectangles;if(pen==2)filled=right-x+1;}
static ULONG TextFit(struct RastPort *r,const char *s,ULONG n,struct TextExtent *e,void *a,int b,int width,int height){(void)r;(void)s;(void)e;(void)a;(void)b;(void)height;return n<(ULONG)(width/8)?n:(ULONG)(width/8);}
static void Move(struct RastPort *r,int x,int y){(void)r;assert(x==10&&y==ui->height-11);}
static void Text(struct RastPort *r,const char *s,ULONG n){(void)r;(void)s;assert(n<=(ULONG)((ui->width-20)/8));}
'''+footer+r'''
static ULONG now;static int writes;static char written[64];
static ULONG weather_ticks(void){return now;}
static BPTR Open(const char *p,int mode){assert(!strcmp(p,"RAM:test/progress")&&mode==MODE_NEWFILE);return 1;}
static LONG Write(BPTR f,const char *s,LONG n){assert(f==1&&n<(LONG)sizeof(written));memcpy(written,s,(size_t)n);written[n]=0;++writes;return n;}
static void Close(BPTR f){assert(f==1);}
'''+report+r'''
int main(void){struct RastPort rp;struct Window win;int widths[]={300,308,640,740},heights[]={208,232,460};unsigned int i,j;
 win.RPort=&rp;state.window=&win;
 for(i=0;i<4;++i)for(j=0;j<3;++j){state.width=widths[i];state.height=heights[j];
  rectangles=filled=0;am_download_state(1,0,0);am_status("Connecting...");assert(rectangles==3&&filled==0);
  rectangles=filled=0;am_download_state(1,50,100);am_status("50 percent");assert(rectangles==4&&filled==(state.width-24)/2);
  rectangles=filled=0;am_download_state(1,100,100);am_status(state.status);assert(rectangles==4&&filled==state.width-24);
  rectangles=filled=0;am_download_state(0,0,0);am_status("Saved");assert(rectangles==1&&filled==0&&!strcmp(state.status,"Saved"));
  am_download_state(1,0,200);filled=0;am_status("Next download");assert(filled==0);
 }
 ui=NULL;am_download_state(1,50,100);am_status("Closed");
 strcpy(progress_path,"RAM:test/progress");assert(am_report_progress(0,0));assert(writes==1);
 am_report_progress(0,100);assert(writes==2&&!strcmp(written,"0 100\n"));
 now=10;am_report_progress(20,100);assert(writes==2);
 now=25;am_report_progress(40,100);assert(writes==3&&!strcmp(written,"40 100\n"));
 now=26;am_report_progress(80,100);assert(writes==3);
 am_report_progress(100,100);assert(writes==4&&!strcmp(written,"100 100\n"));
 am_report_progress(100,100);assert(writes==4);
 puts("AMINET footer: narrow/wide clipping, progress, refresh, completion/reset and throttled worker snapshots passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations','-I'+str(r/'plugin_src/aminet'),str(p/'test.c'),str(r/'plugin_src/aminet/nd_aminet.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
