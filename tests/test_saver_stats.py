#!/usr/bin/env python3
"""Measure actual SAVER frames (not skipped animation steps), pauses and report."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'plugin_src/saver/nd_saver_window.h').read_text()
a=s.index('static void saver_report(');b=s.index('static void saver_free_cache',a)
h=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_saver_stats.h"
struct NDSaverWindow {int diagnostics,width,height,fast_previous;unsigned long depth;void *background;struct NDSaverStats stats;};
'''+s[a:b]+r'''
int main(void){struct NDSaverWindow w;unsigned long i;FILE *f;char text[800];memset(&w,0,sizeof(w));w.diagnostics=1;w.width=640;w.height=320;w.depth=8;w.background=&w;w.fast_previous=1;
 for(i=0;i<=100;i+=2)nd_saver_stats_frame(&w.stats,100+i,1);
 assert(w.stats.fps10==250&&w.stats.average_ms==20&&w.stats.peak_ms==20);
 assert(w.stats.total_frames==51&&w.stats.total_intervals==50&&w.stats.total_ticks==100);
 /* Next measurement interval must not lose the boundary between windows. */
 for(i=102;i<=202;i+=2)nd_saver_stats_frame(&w.stats,100+i,2);
 assert(w.stats.fps10==250&&w.stats.average_ms==40&&w.stats.peak_ms==40);
 assert(w.stats.total_frames==102&&w.stats.total_ticks==202);
 w.stats.last=0;w.stats.frames=0; /* Focus loss excludes time spent paused. */
 nd_saver_stats_frame(&w.stats,5000,1);assert(w.stats.total_ticks==202);
 nd_saver_stats_frame(&w.stats,5004,1);assert(w.stats.total_ticks==206);
 nd_saver_stats_frame(&w.stats,1,1);assert(w.stats.total_ticks==206); /* Clock moves backwards. */
 saver_report(&w);f=fopen("RAM:NovaDock-SAVER.txt","r");assert(f);i=fread(text,1,sizeof(text)-1,f);text[i]=0;fclose(f);
 assert(strstr(text,"640 x 320, depth 8")&&strstr(text,"Boing cache: active")&&strstr(text,"Timing resolution: 20 ms"));
 return 0;}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'check.c').write_text(h)
 cmd=[os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(root/'include'),str(p/'check.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],cwd=t,check=True)
print('SAVER diagnostics: real frame cadence, averaged/peak draw time, paused clock exclusion and report output passed.')
