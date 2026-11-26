#!/usr/bin/env python3
"""Render every shipped bank frame through the real Tiny/Normal cache path."""
from pathlib import Path
import json,os,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'src/dock_draw.c').read_text()
a=source.index('static int draw_plugin_bank(');b=source.index('\nstatic void draw_tile_content(',a)
banks=sorted((ROOT/'Plugins').glob('*.assets/Artwork.ndart'))
# Every bundled plugin has bank artwork or a dedicated live/launcher renderer.
covered={'CLOCK','EYES','CPU','RAM','NET','DOPUS4','RUNNINGAPPS','ACTIONS','CLIPBOARD','BITSKIM','SCREENSWITCHER','AMINET'}
inventory=json.loads((ROOT/'tests/plugin_qa.json').read_text())['plugins']
assert len(inventory)==47
for plugin in inventory:
 assert (ROOT/(plugin['program']+'.assets/Artwork.ndart')).is_file() or plugin['id'] in covered,plugin['id']
# The EYES live renderer must precede the Tiny fallback too.
assert source.index('draw_eyes(dock,index,x,y,tw,th);',source.index('static void draw_tile_content('))<source.index('draw_tiny_plugin(dock,item,x,y,tw,th);',source.index('static void draw_tile_content('))
code=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_artwork.h"
#include "nd_config.h"
typedef unsigned long ULONG;
#define ND_ICON_DARK 1
#define ND_ICON_CASE 2
#define ND_ICON_LIGHT 3
#define ND_ICON_ACCENT 4
#define ND_ICON_SECONDARY 5
#define ND_TEXT 6
#define ND_SUBTLE 7
struct RastPort {int ink;};struct Window {struct RastPort *RPort;};
struct NDDock {struct NDConfig conf;struct Window *window;struct NDArtwork *plugin_artwork[1];unsigned long plugin_art_key[1];unsigned int plugin_art_state[1];int hover_headroom,painting_shelf,painting_mask,painting_wave;};
static int grid[100][110],mask[100][110],ink_count,bound_w,bound_h;
static ULONG pen(struct NDDock *d,int c){return d->painting_mask ? 1:(ULONG)c;}
static void SetAPen(struct RastPort *r,ULONG c){r->ink=(int)c;}
static void RectFill(struct RastPort *r,int x,int y,int right,int bottom)
{int xx,yy;assert(x>=5&&y>=5&&right<5+bound_w&&bottom<5+bound_h);for(yy=y;yy<=bottom;++yy)for(xx=x;xx<=right;++xx){grid[yy][xx]=r->ink;++ink_count;}}
static int draw_bank_cached(struct NDDock *d,int i,const struct NDItem *it,struct NDArtwork *a,const unsigned char *runs,unsigned int n,int size,int x,int y,int w,int h){(void)d;(void)i;(void)it;(void)a;(void)runs;(void)n;(void)size;(void)x;(void)y;(void)w;(void)h;return 0;}
static int icon_hovered(struct NDDock *d,int i){(void)d;(void)i;return 0;}
static int wave_icon_size(struct NDDock *d,int i,int w,int h,int *iw,int *ih){(void)d;(void)i;(void)w;(void)h;(void)iw;(void)ih;return 0;}
static int wave_icon_shift(struct NDDock *d,int i,int s){(void)d;(void)i;(void)s;return 0;}
static int wave_icon_top(struct NDDock *d,int y,int h,int size,int tiny){(void)d;return y+(h-(tiny ? 4:24)-size)/2;}
static void label_text(struct NDDock *d,int c,const char *s,int x,int y,int w){(void)d;(void)c;(void)s;(void)x;(void)y;(void)w;}
#define centered_text_tight label_text
#define caption_text_tight label_text
static void draw_plugin_running_indicator(struct NDDock *d,const struct NDItem *it,int x,int y,int w){(void)d;(void)it;(void)x;(void)y;(void)w;}
'''+source[a:b]+r'''
int main(int argc,char **argv)
{struct NDArtwork art;static struct NDDock d;struct NDItem it;struct RastPort rp;struct Window win;int file,size,shelf,color,mode,x,y;unsigned int f,n,flags;unsigned long cases=0,frames=0;const unsigned char *record;
 memset(&d,0,sizeof(d));memset(&it,0,sizeof(it));win.RPort=&rp;d.window=&win;it.kind=ND_ITEM_PLUGIN;it.online=1;strcpy(it.label,"WEATHER");strcpy(it.caption,"CPU temperature");strcpy(it.value,"33.8 C");
 for(file=1;file<argc;++file){memset(&art,0,sizeof(art));assert(nd_artwork_load(&art,argv[file]));d.plugin_artwork[0]=&art;for(f=1;f<16;++f)art.mapped[f]=f&~1U;
 n=nd_artwork_u16(art.data+6);frames+=n;
 for(f=0;f<n;++f){record=art.data+nd_artwork_header(art.data)+f*32;flags=art.data[5]==2 ? record[25]:0;d.plugin_art_key[0]=nd_artwork_key((const char *)record);d.plugin_art_state[0]=record[24];
 for(size=0;size<4;++size)for(shelf=0;shelf<2;++shelf)for(color=0;color<2;++color){
  bound_w=size==0 ? 86:size==1 ? 43:size==2 ? 34:55;bound_h=size==0 ? 76:size==1 ? 38:size==2 ? 30:46;d.conf.item_size=size==0||size==3 ? ND_ITEM_SIZE_NORMAL:ND_ITEM_SIZE_TINY;d.conf.colourful_icons=color;d.hover_headroom=60;d.painting_shelf=shelf;
  for(mode=0;mode<2;++mode){memset(grid,255,sizeof(grid));ink_count=0;d.painting_mask=mode;
   assert(draw_plugin_bank(&d,0,&it,5,5,bound_w,bound_h));
   if(!shelf || (flags&ND_ARTWORK_VALUE)){assert(ink_count>=8);if(mode){for(y=0;y<100;++y)for(x=0;x<110;++x)assert((mask[y][x]>=0)==(grid[y][x]==1));}else memcpy(mask,grid,sizeof(mask));}
   else assert(!ink_count);
   ++cases;
  }
 }
 }
 nd_artwork_close(&art);
 }
 printf("All-plugin artwork: %lu frames in %d banks, %lu Tiny/Normal/cache/mask/colour render cases passed. All 46 plugins have artwork or a dedicated live icon.\n",frames,argc-1,cases);return 0;}
'''
with tempfile.TemporaryDirectory(prefix='nd-plugin-icon-states-',dir='/private/tmp') as tmp:
 p=Path(tmp);(p/'check.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-Wno-unused-function','-fsanitize=address,undefined','-I'+str(ROOT/'include'),str(p/'check.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check'),*map(str,banks)],check=True)
