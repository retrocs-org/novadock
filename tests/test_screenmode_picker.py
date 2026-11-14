#!/usr/bin/env python3
"""Run the real display-mode scanner with native, fixed-depth and unavailable modes."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/screenmode/screenmode.c').read_text();a=s.index('static void scan_modes(');b=s.index('\nstatic void close_chooser',a)
h=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_scrmode.h"
typedef unsigned long ULONG;typedef long LONG;typedef unsigned char UBYTE;
#define INVALID_ID 0xffffffffUL
#define MAX_PRESETS 8
#define DIPF_IS_WB 0x100UL
#define DIPF_IS_FOREIGN 0x80000000UL
#define DTAG_DISP 1
#define DTAG_DIMS 2
struct Node {char *ln_Name;};struct List {struct Node *lh_Head,*lh_Tail,*lh_TailPred;};
struct DisplayInfo {unsigned int NotAvailable;ULONG PropertyFlags;};
struct DimensionInfo {unsigned int MaxDepth,MinRasterWidth,MinRasterHeight,MaxRasterWidth,MaxRasterHeight;struct {int MinX,MinY,MaxX,MaxY;}Nominal;};
struct WBSignature {unsigned int width,height,depth;ULONG mode;};
struct Preset {struct Node node;char label[96],path[256];struct NDScreenModePreset mode;int generated;};
struct Chooser {struct Preset presets[MAX_PRESETS];struct List rows;int count,selected;};
static int nodes;
static int workbench_signature(struct WBSignature *s){s->width=640;s->height=256;s->depth=2;s->mode=1;return 1;}
static ULONG NextDisplayInfo(ULONG id){return id==INVALID_ID ? 0:id<6 ? id+1:INVALID_ID;}
static int ModeNotAvailable(ULONG id){return id==2;}
static int GetDisplayInfoData(void *unused,UBYTE *out,unsigned int size,int tag,ULONG id){(void)unused;(void)size;
if(tag==DTAG_DISP){struct DisplayInfo *d=(struct DisplayInfo *)out;d->PropertyFlags=id==3 ? 0:DIPF_IS_WB;if(id==4)d->PropertyFlags|=DIPF_IS_FOREIGN;d->NotAvailable=id==5;}
else{struct DimensionInfo *d=(struct DimensionInfo *)out;d->MaxDepth=id==4 ? 16:id==1 ? 2:8;d->MinRasterWidth=320;d->MinRasterHeight=200;d->MaxRasterWidth=4096;d->MaxRasterHeight=4096;d->Nominal.MaxX=id==4 ? 799:639;d->Nominal.MaxY=id==4 ? 599:255;if(id==6)d->Nominal.MaxX=0;}
return 1;}
static void mode_name(ULONG id,char *out,unsigned int size){(void)size;sprintf(out,"Mode %lu",id);}
static void AddTail(struct List *list,struct Node *node){(void)list;assert(node->ln_Name);++nodes;}
'''+s[a:b]+r'''
int main(void){struct Chooser c;unsigned char buffer[62],short_buffer[61];struct NDScreenModePreset back;
memset(&c,0,sizeof(c));scan_modes(&c);assert(c.count==3&&nodes==3);
assert(c.presets[0].mode.width==640&&c.presets[0].mode.depth==2&&strstr(c.presets[0].label,"CURRENT"));
assert(c.presets[1].mode.depth==2);assert(c.selected==0);assert(c.presets[2].mode.width==800&&c.presets[2].mode.depth==16);
assert(nd_scrmode_encode(&c.presets[2].mode,buffer,sizeof(buffer))==62);assert(nd_scrmode_parse(buffer,sizeof(buffer),&back));
assert(back.display_id==4&&back.width==800&&back.height==600&&back.depth==16&&back.control==1);
memset(short_buffer,0xaa,sizeof(short_buffer));assert(!nd_scrmode_encode(&back,short_buffer,sizeof(short_buffer))&&short_buffer[0]==0xaa);
back.depth=0;assert(!nd_scrmode_encode(&back,buffer,sizeof(buffer)));back.depth=33;assert(!nd_scrmode_encode(&back,buffer,sizeof(buffer)));
puts("Direct mode list filters unavailable modes, preserves native depth, uses RTG depth and writes valid bounded IFF preferences.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-Wno-deprecated-declarations','-I'+str(ROOT/'plugin_src/screenmode'),str(p/'check.c'),str(ROOT/'plugin_src/screenmode/nd_scrmode.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)
