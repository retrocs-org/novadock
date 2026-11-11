#!/usr/bin/env python3
"""Model of the dock's graph drawing (src/dock_draw.c graph_series,
draw_graph and draw_graph_step): scrolling a graph by one sample and
redrawing its four rightmost columns must give exactly the pixels of a full
redraw, with gaps in either trace, full graphs, and full redraws mixed in.
It checks the drawing rules, not the C code itself; keep the two in step."""
import random
W,H=64,70; BG,GUIDE,A,B=0,1,2,3
gx,gy,gw,gh=6,34,52,30
def line(buf,x0,y0,x1,y1,col,pattern):
    # Deterministic Bresenham; the pattern restarts at every Move (segment).
    dx,dy=abs(x1-x0),-abs(y1-y0); sx=1 if x0<x1 else -1; sy=1 if y0<y1 else -1
    err=dx+dy; k=0
    while True:
        if pattern>>(15-k%16)&1: buf[y0][x0]=col
        k+=1
        if x0==x1 and y0==y1: break
        e2=2*err
        if e2>=dy: err+=dy; x0+=sx
        if e2<=dx: err+=dx; y0+=sy
def rectfill(buf,x0,y0,x1,y1,col):
    for y in range(y0,y1+1):
        for x in range(x0,x1+1): buf[y][x]=col
def series(buf,hist,first_col):
    # Interleaved: segment i of A, then segment i of B, then markers.  A full
    # draw (first_col==gx) also draws the segment entering the leftmost point
    # from the sample just outside the graph, then clears that outer column,
    # exactly what a scrolled graph holds.
    count=min(len(hist),gw)
    extra=1 if (first_col==gx and len(hist)>count) else 0
    have=[False,False]; old=[(0,0),(0,0)]
    for age in range(count-1+extra,-1,-1):
        a,b=hist[len(hist)-1-age]
        px=gx+gw-1-age
        for s in (0,1):
            col=B if s else A; pat=0xCCCC if s else 0xFFFF
            v=b if s else a
            if v>100: have[s]=False; continue
            if px<first_col-1: have[s]=False; continue
            py=gy+gh-1-v*(gh-1)//100
            # The point just left of the zone is already drawn correctly:
            # it only starts the first segment into the zone.
            if px<first_col:
                old[s]=(px,py); have[s]=True; continue
            ox,oy=old[s]
            line(buf,ox if have[s] else px,oy if have[s] else py,px,py,col,pat)
            old[s]=(px,py); have[s]=True
    if extra: rectfill(buf,gx-1,gy,gx-1,gy+gh-1,BG)
    for s in (0,1):
        # The marker shows the current value: only when the newest sample
        # exists, so it is always in the two rightmost columns.
        if have[s] and old[s][0]==gx+gw-1:
            ox,oy=old[s]; col=B if s else A
            rectfill(buf,ox-1 if ox>gx else ox,oy,ox,oy+1 if oy<gy+gh-1 else oy,col)
def full(hist,phase):
    buf=[[BG]*W for _ in range(H)]
    s=(4-(phase&3))&3; pat=((0x8888>>s)|(0x8888<<(16-s)))&0xFFFF
    line(buf,gx,gy+gh//2,gx+gw-1,gy+gh//2,GUIDE,pat)
    series(buf,hist,gx); return buf
def step(buf,hist,phase):
    for y in range(gy,gy+gh):                      # ScrollRaster(1,0)
        row=buf[y]; row[gx:gx+gw]=row[gx+1:gx+gw]+[BG]
    zone=gx+gw-4
    rectfill(buf,zone,gy,gx+gw-1,gy+gh-1,BG)
    for c in range(zone,gx+gw):
        if (c-gx+phase)&3==0: buf[gy+gh//2][c]=GUIDE
    series(buf,hist,zone)
def area(buf): return [r[gx:gx+gw] for r in buf[gy:gy+gh]]
random.seed(2026); bad=0; checked=0
for run in range(60):
    hist=[]; phase=random.randrange(4)
    buf=full(hist,phase)
    for n in range(260):
        a=random.choice([random.randrange(101),random.randrange(101),65535])
        b=random.choice([random.randrange(101),65535])
        hist.append((a,b))
        if random.random()<0.1:                      # hover, reload...: full redraw
            buf=full(hist,phase)
        else:
            phase+=1; step(buf,hist,phase)
        checked+=1
        if area(buf)!=area(full(hist,phase)): bad+=1
assert bad==0, f'{bad} of {checked} scrolled graph frames differ from a full redraw'
print(f'Graph scrolling model: {checked} samples, every frame matches a full redraw.')
