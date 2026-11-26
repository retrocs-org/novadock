#!/usr/bin/env python3
"""Run the production metadata receiver and renderer through on/off/theme states."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
for source in ('src/dock_draw.c','src/dock_icons.c'):
 text=(ROOT/source).read_text()
 assert 'nd_hass_' not in text and 'AMIHASS_PORT' not in text, 'Artwork renderer must remain independent of HASS logic'
def function(path,signature):
 s=path.read_text();a=s.index(signature);b=s.index('{',a)
 while ';' in s[a:b]:a=s.index(signature,a+len(signature));b=s.index('{',a)
 i=b+1;n=1
 while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]
h=r'''

#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0

#include <string.h>
#include <stdio.h>
#include "nd_config.h"
#include "nd_layout.h"
#include "nd_options.h"
#include "nd_artwork.h"
#include "nd_hass_art.h"
typedef unsigned long ULONG;typedef unsigned short UWORD;
#define ND_EXT_HAS_ARTWORK 4
#define ND_ICON_DARK 8
#define ND_ICON_CASE 9
#define ND_ICON_LIGHT 10
#define ND_ICON_ACCENT 11
#define ND_ICON_SECONDARY 12
#define ND_BG 0
#define ND_SUBTLE 1
#define ND_TEXT 2
#define ND_ACCENT 3
#define ND_ACCENT2 4
struct NDExtMessage {UWORD flags,reserved;ULONG reserved2[2];};
struct RastPort {int ink;};struct Window {struct RastPort *RPort;int LeftEdge;};
struct Screen {int MouseX;};
struct NDDock {struct NDConfig conf;struct NDHoverLayoutItem hover_layout[ND_MAX_ITEMS];int hover_plan_ready;int hover,is_drag,hover_headroom,tile_width,gap,width,painting_shelf,painting_wave,painting_mask;struct Screen *screen;struct NDArtwork *plugin_artwork[ND_MAX_ITEMS];ULONG plugin_art_key[ND_MAX_ITEMS];UWORD plugin_art_state[ND_MAX_ITEMS];struct Window *window;};
static int theme,loads;static unsigned char pixels[100][100];static char texts[3][32];static int text_count;
static void load_plugin_artwork(struct NDDock *d,int i){(void)d;(void)i;++loads;}
static void Forbid(void){}static void Permit(void){}static void *FindPort(const char *name){(void)name;return (void *)1;}
static ULONG pen(struct NDDock *d,int role){(void)d;return 16+role+theme;}
static int nd_icon_art_top(int y,int th,int size,int tiny){return y+(th-size-(tiny ? 0:20))/2;}
static void SetAPen(struct RastPort *p,ULONG ink){p->ink=(int)ink;}
static void RectFill(struct RastPort *p,int x,int y,int x2,int y2){int a,b;assert(x>=0&&y>=0&&x2<100&&y2<100&&x<=x2&&y<=y2);for(b=y;b<=y2;++b)for(a=x;a<=x2;++a)pixels[b][a]=(unsigned char)p->ink;}
static void centered_text_tight(struct NDDock *d,int color,const char *text,int x,int y,int width){(void)d;(void)color;(void)x;(void)y;(void)width;assert(text_count<3);strcpy(texts[text_count++],text);}
static void caption_text_tight(struct NDDock*d,int color,const char*text,int x,int y,int width){centered_text_tight(d,color,text,x,y,width);}
static void draw_plugin_running_indicator(struct NDDock *d,const struct NDItem *i,int x,int y,int w){(void)d;(void)i;(void)x;(void)y;(void)w;}
static int draw_bank_cached(struct NDDock *d,int i,const struct NDItem *item,struct NDArtwork *art,const unsigned char *runs,unsigned int count,int size,int x,int y,int tw,int th){(void)d;(void)i;(void)item;(void)art;(void)runs;(void)count;(void)size;(void)x;(void)y;(void)tw;(void)th;return 0;}
static void tile_dimensions(struct NDDock *d,int i,int *w,int *h){*w=80;*h=80;(void)d;(void)i;}
static void label_text(struct NDDock*d,int ink,const char*s,int x,int y,int w){caption_text_tight(d,ink,s,x,y,w);}
static void tile_coords(struct NDDock *d,int i,int *x,int *y){*x=i*80;*y=d->hover_headroom;}
'''+function(ROOT/'src/dock_plugins.c','static int receive_plugin_artwork(')+'\n'+function(ROOT/'src/dock_draw.c','static int icon_hovered(')+'\n'+function(ROOT/'src/dock_draw.c','static int wave_icon_size(')+'\n'+function(ROOT/'src/dock_draw.c','static int wave_icon_shift(')+'\n'+function(ROOT/'src/dock_draw.c','static int wave_icon_top(')+'\n'+function(ROOT/'src/dock_draw.c','static int draw_plugin_bank(')+r'''
int main(int argc,char **argv){static struct NDDock dock;struct NDArtwork a;struct NDItem *item=&dock.conf.items[0];struct NDExtMessage m;struct RastPort rp;struct Window window;unsigned char off[100][100];unsigned int i;
 assert(argc==3);memset(&a,0,sizeof(a));assert(nd_artwork_load(&a,argv[1]));for(i=0;i<16;++i)a.mapped[i]=i;
 dock.plugin_artwork[0]=&a;window.RPort=&rp;dock.window=&window;item->kind=ND_ITEM_PLUGIN;item->online=1;strcpy(item->command,"Plugins/HASS");strcpy(item->label,"Lamp");strcpy(item->caption,"Lamp");strcpy(item->value,"on");
 memset(&m,0,sizeof(m));m.flags=ND_EXT_HAS_ARTWORK;m.reserved2[0]=nd_artwork_key("ChristmasLights");assert(receive_plugin_artwork(&dock,0,&m));assert(!receive_plugin_artwork(&dock,0,&m));
 assert(draw_plugin_bank(&dock,0,item,0,0,80,80));memcpy(off,pixels,sizeof(off));text_count=0;memset(pixels,0,sizeof(pixels));
 m.reserved=1;assert(receive_plugin_artwork(&dock,0,&m));assert(draw_plugin_bank(&dock,0,item,0,0,80,80));assert(memcmp(off,pixels,sizeof(off)));assert(!strcmp(texts[0],"Lamp"));
 memcpy(off,pixels,sizeof(off));text_count=0;memset(pixels,0,sizeof(pixels));
 dock.conf.hover_magnification=100;assert(draw_plugin_bank(&dock,0,item,0,0,80,80));
 assert(memcmp(off,pixels,sizeof(off)) && text_count==1 && !strcmp(texts[0],"Lamp"));
 dock.conf.hover_magnification=0;
 assert(!receive_plugin_artwork(&dock,0,&m));assert(receive_plugin_artwork(&dock,0,NULL));text_count=0;assert(!draw_plugin_bank(&dock,0,item,0,0,80,80));nd_artwork_close(&a);
 assert(nd_artwork_load(&a,argv[2]));strcpy(item->command,"Plugins/Weather");strcpy(item->value,"5.8 C");strcpy(item->label,"Weather");m.reserved=7;m.reserved2[0]=nd_artwork_key("weather");assert(receive_plugin_artwork(&dock,0,&m));
 text_count=0;assert(draw_plugin_bank(&dock,0,item,0,0,80,80));assert(text_count==2&&!strcmp(texts[0],"5.8 C"));memcpy(off,pixels,sizeof(off));
 theme=32;text_count=0;memset(pixels,0,sizeof(pixels));assert(draw_plugin_bank(&dock,0,item,0,0,80,80));assert(memcmp(off,pixels,sizeof(off)));nd_artwork_close(&a);text_count=0;assert(!draw_plugin_bank(&dock,0,item,0,0,80,80));return 0;}
'''
with tempfile.TemporaryDirectory(prefix='novadock-art-render-') as tmp:
 p=Path(tmp);(p/'render.c').write_text(h);cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'render.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),str(ROOT/'src/layout.c'),'-o',str(p/'render')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'render'),str(ROOT/'Plugins/HASS.assets/Artwork.ndart'),str(ROOT/'Plugins/Weather.assets/Artwork.ndart')],check=True)
print('Production artwork receiver/renderer: state-only updates repaint on/off pixels; theme roles, labels, value rows and missing-bank fallback pass.')
