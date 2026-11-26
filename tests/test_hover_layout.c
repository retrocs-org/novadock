#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_layout.h"
static int extent(struct NDHoverLayoutItem *p)
{int w=p->vertical ? p->height:p->width;if(p->kind==ND_HOVER_ART){if(p->vertical)w+=p->label;else if(w<p->label)w=p->label;}return w;}
static void verify(struct NDHoverLayoutItem *p,int n,int left,int right,int gap)
{int i;for(i=0;i<n;++i){int w=extent(&p[i]);assert(p[i].centre-w/2>=left);assert(p[i].centre+(w+1)/2<=right);assert(p[i].width>=p[i].base_width&&p[i].height>=p[i].base_height);if(i)assert(p[i].centre-w/2>=p[i-1].centre+(extent(&p[i-1])+1)/2+gap);}}
int main(void)
{
 struct NDHoverLayoutItem p[16];int v,focus,level,i,axis;
 for(v=0;v<2;++v)for(focus=0;focus<7;++focus)for(level=0;level<=100;level+=25){
  int at=v ? 7:100;
  memset(p,0,sizeof(p));axis=v ? 590:1014;
  for(i=0;i<7;++i){int slot=(!v&&i==2) ? 177:v ? 76:86;
   p[i].vertical=v;p[i].kind=i<4 ? ND_HOVER_ART:ND_HOVER_WIDGET;
   p[i].anchor=at+slot/2;p[i].base_width=i<4 ? 32:86;p[i].base_height=i<4 ? 32:76;
   p[i].label=i<4 ? (v ? 11:48):0;p[i].width=p[i].base_width;p[i].height=p[i].base_height;
   at+=slot+5;
  }
  for(i=0;i<7;++i)nd_hover_size(p[i].anchor-p[focus].anchor,182,level,p[i].kind==ND_HOVER_ART ? 128:256,170,&p[i].width,&p[i].height);
  nd_hover_layout(p,7,focus,5,2,axis-2);verify(p,7,2,axis-2,5);
 }
 /* A fixed divider remains at its anchor while each side packs independently. */
 memset(p,0,sizeof(p));for(i=0;i<7;++i){p[i].anchor=30+i*70;p[i].kind=ND_HOVER_ART;p[i].width=p[i].height=80;p[i].base_width=p[i].base_height=32;p[i].label=40;}
 p[3].kind=ND_HOVER_FIXED;p[3].width=p[3].base_width=20;
 nd_hover_layout(p,7,2,5,2,480);verify(p,7,2,480,5);assert(p[3].centre==p[3].anchor);
 /* Strength is monotonic, and zero leaves the original dimensions intact. */
 {int previous=32;for(level=0;level<=100;++level){int w=32,h=32;nd_hover_size(0,180,level,128,128,&w,&h);assert(w>=previous&&w<=128);previous=w;if(!level)assert(w==32&&h==32);}}
 /* Dense pointer sweeps must not jump when the selected tile changes. */
 {struct NDHoverLayoutItem previous[7];int x,have=0;
  for(x=0;x<=1000;++x){
   memset(p,0,sizeof(p));
   for(i=0;i<7;++i){p[i].anchor=100+i*120;p[i].kind=ND_HOVER_ART;p[i].base_width=p[i].base_height=48;p[i].width=p[i].height=48;p[i].label=62;nd_hover_size(p[i].anchor-x,240,100,128,160,&p[i].width,&p[i].height);}
   nd_hover_layout_at(p,7,x,5,2,1014);verify(p,7,2,1014,5);
   if(have)for(i=0;i<7;++i){int d=p[i].centre-previous[i].centre;if(d<0)d=-d;assert(d<=6);d=p[i].width-previous[i].width;if(d<0)d=-d;assert(d<=3);}
   memcpy(previous,p,sizeof(previous));have=1;
  }
 }
 /* A nearly full nine-item dock must cross its packing limit gradually.
  * MUSIC spans two cells; the old overflow branch moved the first icon by
  * hundreds of pixels in one step near RAM/Clock and the right-hand tiles. */
 {struct NDHoverLayoutItem previous[9];int x,have,side;
  for(side=0;side<2;++side)for(level=25;level<=100;level+=25){
   have=0;
   for(x=40;x<1014;++x){int slot=0;
    memset(p,0,sizeof(p));
    for(i=0;i<9;++i){int cell=i==2 ? 177:86;
     p[i].anchor=54+slot*91+cell/2;p[i].vertical=side;p[i].kind=ND_HOVER_ART;
     p[i].base_width=p[i].base_height=p[i].width=p[i].height=48;
     p[i].label=side ? 11:i==2 ? 80:40;
     nd_hover_size(p[i].anchor-x,182,level,128,141,&p[i].width,&p[i].height);
     slot+=i==2 ? 2:1;
    }
    nd_hover_layout_at(p,9,x,5,2,1011);verify(p,9,2,1011,5);
    if(have)for(i=0;i<9;++i){int delta=p[i].centre-previous[i].centre;if(delta<0)delta=-delta;assert(delta<=6);}
    memcpy(previous,p,sizeof(previous));have=1;
   }
  }
 }
 /* Continuous packing keeps dividers fixed throughout a hand-off. */
 for(axis=0;axis<480;++axis){
  memset(p,0,sizeof(p));for(i=0;i<7;++i){p[i].anchor=30+i*70;p[i].kind=ND_HOVER_ART;p[i].width=p[i].height=80;p[i].base_width=p[i].base_height=32;p[i].label=40;}
  p[3].kind=ND_HOVER_FIXED;p[3].width=p[3].base_width=20;
  nd_hover_layout_at(p,7,axis,5,2,480);verify(p,7,2,480,5);assert(p[3].centre==p[3].anchor);
 }
 puts("Hover layout: non-overlapping icons, labels and widgets; horizontal/vertical bounds, edge focus, fixed dividers and 0-100 strength passed.");return 0;
}
