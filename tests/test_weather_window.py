#!/usr/bin/env python3
"""Run real weather painting against small/wide screens with counted drawing."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/weather/nd_weather_window.c').read_text();a=s.index('struct NDWeatherView {');b=s.index('void nd_weather_view_close(',a)
h=r"""#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_weather.h"
#include "nd_weather_codes.h"
#include "nd_weather_art.h"
typedef unsigned long ULONG;typedef unsigned short UWORD;typedef char *STRPTR;typedef void *APTR;
#define JAM1 1
struct TextExtent {int te_Width;};struct RastPort {int unused;};struct Window {int Width,Height;struct RastPort *RPort;};
static int width,height,cursor_x,cursor_y,pen,rects,draw,seen_header,seen_snow;static struct RastPort rp;
static void SetFont(struct RastPort *r,void *font){(void)font;assert(r);}
static void SetDrMd(struct RastPort *r,int mode){assert(r&&mode==JAM1);}
static void SetAPen(struct RastPort *r,int p){assert(r);pen=p;}
static void RectFill(struct RastPort *r,int l,int t,int right,int bottom){assert(r&&l>=0&&t>=0&&right>=l&&bottom>=t&&right<width&&bottom<height);++rects;if(draw)printf("R|%d|%d|%d|%d|%d\n",pen,l,t,right,bottom);}
static void Move(struct RastPort *r,int x,int y){assert(r&&x>=0&&y>=0&&x<width&&y<height);cursor_x=x;cursor_y=y;}
static void Draw(struct RastPort *r,int x,int y){assert(r&&x>=0&&y>=0&&x<width&&y<height);if(draw)printf("L|%d|%d|%d|%d|%d\n",pen,cursor_x,cursor_y,x,y);cursor_x=x;cursor_y=y;}
static ULONG TextFit(struct RastPort *r,STRPTR text,ULONG n,struct TextExtent *fit,void *unused,int direction,int w,int h){(void)text;(void)unused;(void)direction;assert(r&&w>0&&h>=8);if(n>(ULONG)(w/8))n=w/8;fit->te_Width=(int)n*8;return n;}
static void Text(struct RastPort *r,STRPTR text,ULONG n){char copy[200];assert(r&&cursor_x+(int)n*8<=width&&cursor_y<height&&n<sizeof(copy));memcpy(copy,text,n);copy[n]=0;if(!strcmp(copy,"Rain / snow"))seen_header=1;if(strstr(copy,"1.8 cm")||strstr(copy,"1.8cm"))seen_snow=1;assert(!strstr(copy,"Rain Rain"));if(draw)printf("T|%d|%d|%d|%s\n",pen,cursor_x,cursor_y,copy);}
"""+s[a:b]+r"""
int main(int argc,char **argv){struct NDWeatherView p;struct Window win;int i,compact,empty,screen_height;char date[11];(void)argv;
 for(screen_height=256;screen_height<=480;screen_height+=224)for(compact=0;compact<2;++compact)for(empty=0;empty<2;++empty){
  memset(&p,0,sizeof(p));p.width=width=compact ? 296:616;p.top=18;p.row=screen_height==256 ? 22:26;p.compact=compact;height=p.top+44+5*p.row+60;assert(height+13<=screen_height);win.Width=width;win.Height=height;win.RPort=&rp;p.window=&win;p.text=2;p.bg=0;p.accent=3;p.line=1;p.have=!empty;p.weather.count=5;
  p.weather.temperature=58;p.weather.code=3;strcpy(p.weather.updated,"2026-11-16T01:30");strcpy(p.status,"Updated");
  for(i=0;i<5;++i){sprintf(date,"2026-11-%02d",16+i);strcpy(p.weather.days[i].date,date);p.weather.days[i].code=i==0 ? 71:i==1 ? 85:i==2 ? 61:3;p.weather.days[i].low=-30;p.weather.days[i].high=20;p.weather.days[i].rain=84;p.weather.days[i].snow=i==0 ? 18:0;p.weather.days[i].rain_amount=i==1 ? 24:0;}
  for(i=0;i<5;++i)p.icons[i]=i;
  seen_header=seen_snow=0;draw=argc>1&&!compact&&!empty;paint(&p);if(!empty)assert(seen_snow);if(!compact)assert(seen_header);if(draw)return 0;
 }
 assert(rects>100);puts("Weather window: narrow/wide and no-data painting stays in bounds; precipitation and snowfall labels are unambiguous.");return 0;}
"""
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-unused-variable','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),'-I'+str(ROOT/'plugin_src/weather'),str(p/'test.c'),str(ROOT/'plugin_src/weather/nd_weather.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
 if '--draw' in __import__('sys').argv:
  Path('/private/tmp/nd-weather-draw.txt').write_bytes(subprocess.check_output([str(p/'test'),'draw']))
