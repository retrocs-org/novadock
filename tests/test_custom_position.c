#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "nd_autohide.h"
#include "nd_layout_profiles.h"
static void inside(const struct NDRect *r,int w,int h,int bar)
{assert(r->left>=0&&r->top>=bar&&r->left+r->width<=w&&r->top+r->height<=h);}
int main(void)
{
 static struct NDConfig cfg,loaded,target;struct NDLayout layout;struct NDRect r,base,tab;
 int widths[]={320,640,1024,1920},heights[]={200,256,600,1080},pads[]={0,1,20,192};
 int side,w,h,p,k,offset,length,visible,travel,axis,cases=0;
 nd_default_config(&cfg);cfg.position=ND_POSITION_CUSTOM;
 for(side=0;side<4;++side)for(w=0;w<4;++w)for(h=0;h<4;++h)for(p=0;p<4;++p){
  cfg.side=side;visible=side<2 ? 150:100;length=side<2?widths[w]:heights[h]-11;
  if(visible+2*pads[p]>length)continue;
  layout.width=side<2?visible+2*pads[p]:60;
  layout.height=side<2?60:visible+2*pads[p];travel=length-visible;
  for(k=0;k<6;++k){
   offset=k==0?0:k==1?1:k==2?13:k==3?137:k==4?travel:INT_MAX;
   cfg.position_offset=offset;
   assert(nd_dock_rect_padded(&cfg,&layout,widths[w],heights[h],11,pads[p],&r,&axis));inside(&r,widths[w],heights[h],11);
   assert(axis>=0&&axis<=2*pads[p]);
   assert((side<2?r.left:r.top-11)+axis==(offset>travel?travel:offset));
   assert(nd_hidden_rect(&cfg,widths[w],heights[h],11,&tab));inside(&tab,widths[w],heights[h],11);
   assert(nd_drag_offset(offset,INT_MIN,length,visible)==0);
   assert(nd_drag_offset(offset,INT_MAX,length,visible)==travel);
   if(offset>=0&&offset<travel)assert(nd_drag_offset(offset,1,length,visible)==offset+1);
   ++cases;
  }
 }
 /* Presets retain their original geometry even when a saved custom value exists. */
 layout.width=300;layout.height=76;cfg.side=ND_SIDE_BOTTOM;cfg.position_offset=137;
 for(p=0;p<3;++p){cfg.position=p;assert(nd_dock_rect(&cfg,&layout,1024,600,11,&base));assert(nd_dock_rect_padded(&cfg,&layout,1024,600,11,20,&r,&axis));assert(!memcmp(&r,&base,sizeof(r)));}
 assert(nd_parse_line(&cfg,"POSITION=CUSTOM"));assert(nd_parse_line(&cfg,"POSITION_OFFSET=137"));
 assert(!nd_parse_line(&cfg,"POSITION_OFFSET=-1"));assert(!nd_parse_line(&cfg,"POSITION_OFFSET=12px"));assert(!nd_parse_line(&cfg,"POSITION_OFFSET=32768"));assert(!nd_parse_line(&cfg,"POSITION_OFFSET=999999999999999999999"));
 cfg.side=ND_SIDE_LEFT;assert(nd_write_config(&cfg,"custom-position.cfg"));assert(nd_load_config(&loaded,"custom-position.cfg")==0);
 assert(loaded.position==ND_POSITION_CUSTOM&&loaded.position_offset==137&&loaded.side==ND_SIDE_LEFT);
 target=loaded;target.position_offset=138;assert(!nd_config_runtime_equal(&target,&loaded));
 assert(nd_layout_apply(&target,&loaded)&&target.position_offset==137&&target.position==ND_POSITION_CUSTOM);
 remove("custom-position.cfg");remove("custom-position.cfg.bak");remove("custom-position.cfg.good");
 printf("Custom dock position: %d bounded canvas/hidden-tab cases, exact pixel steps, extreme deltas, config round trip and profile restore passed.\n",cases);
 return 0;
}
