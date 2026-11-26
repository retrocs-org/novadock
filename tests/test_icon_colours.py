#!/usr/bin/env python3
"""Shared palette remains legible across built-in themes; RGB override is stable."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_theme.h"
#include "nd_icon_colors.h"
int main(void){struct NDThemeSpec spec;unsigned char icons[5][3],stock[5][3],saved[5][3];int t,k;unsigned char metric[4][5][3];
 for(t=0;t<ND_THEME_COUNT;++t){nd_theme_defaults(t,&spec);nd_icon_palette(spec.rgb,0,icons);
  assert(nd_icon_luma(icons[2])-nd_icon_luma(icons[0])>600);
  assert(!memcmp(icons[3],spec.rgb[5],3)&&!memcmp(icons[4],spec.rgb[7],3));
  nd_icon_palette(spec.rgb,1,stock);if(!t)memcpy(saved,stock,sizeof(stock));else assert(!memcmp(stock,saved,sizeof(stock)));
 }
 for(k=1;k<=4;++k){int hi,lo;nd_metric_palette(spec.rgb,1,k,metric[k-1]);
  hi=lo=metric[k-1][1][0];{int c;for(c=1;c<3;++c){if(metric[k-1][1][c]>hi)hi=metric[k-1][1][c];if(metric[k-1][1][c]<lo)lo=metric[k-1][1][c];}}
  assert(hi-lo>=100);assert(nd_icon_luma(metric[k-1][2])-nd_icon_luma(metric[k-1][0])>600);
  if(k>1)assert(memcmp(metric[k-1],metric[0],sizeof(metric[0])));
 }
 puts("Icon colours: all 15 themes retain dark/light contrast; Colourful RGB ignores theme changes.");return 0;}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(s)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'test.c'),str(ROOT/'src/theme.c'),str(ROOT/'src/config.c'),str(ROOT/'src/nd_options.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

# The released bank's original RGB material must actually be distinctive.
import sys
sys.path.insert(0,str(ROOT/'tools'))
from plugin_art_colours import colourful_palette
materials=[]
for name in ('Preferences','MusicPlayer','Trash','Notes','Timer','Weather'):
 data=(ROOT/'Plugins'/(name+'.assets')/'Artwork.ndart').read_bytes()
 assert data[:4]==b'NDAR' and data[5]==2
 assert list(data[8+2*3:8+3*3])==list(colourful_palette(name)[2])
 material=tuple(data[14:17]);assert max(material)-min(material)>=100
 materials.append(material)
 assert list(data[57:60])==[1,2,3],name
assert len(set(materials))==len(materials)
print('Bundled Colourful materials are saturated, distinct and keep their theme roles.')
